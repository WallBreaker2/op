#include "OpenGLCapture.h"

#include "DisplayHook.h"
#include "SharedFrame.h"
#include "SharedFrameChannel.h"
#include "SharedFrameExchange.h"
#include "SharedFrameWriter.h"
#include "../capture/FrameInfo.h"
#include "../hook/ApiResolver.h"
#include "../ipc/ProcessMutex.h"
#include "../ipc/SharedMemory.h"
#include "../base/AutomationModes.h"
#include "../base/Utils.h"
#include <gl\glu.h>

#define DEBUG_HOOK 0

namespace op::hook {

using op::capture::FrameInfo;

namespace {

// opengl32 与 libglesv2 的取像素流程一致，只有解析符号的 DLL 名不同。
// 返回 -1 表示符号解析失败，由调用方决定是否关闭捕获。
int gl_capture_impl(const char *dll) {
    using glPixelStorei_t = decltype(glPixelStorei) *;
    using glReadBuffer_t = decltype(glReadBuffer) *;
    using glGetIntegerv_t = decltype(glGetIntegerv) *;
    using glReadPixels_t = decltype(glReadPixels) *;

    auto pglPixelStorei = (glPixelStorei_t)ResolveApi(dll, "glPixelStorei");
    auto pglReadBuffer = (glReadBuffer_t)ResolveApi(dll, "glReadBuffer");
    auto pglGetIntegerv = (glGetIntegerv_t)ResolveApi(dll, "glGetIntegerv");
    auto pglReadPixels = (glReadPixels_t)ResolveApi(dll, "glReadPixels");
    if (!pglPixelStorei || !pglReadBuffer || !pglGetIntegerv || !pglReadPixels) {
#if DEBUG_HOOK
        setlog("ResolveApi(%s) failed", dll);
#endif // DEBUG_HOOK

        return -1;
    }
    RECT rc;
    ::GetClientRect(DisplayHook::render_hwnd, &rc);
    int width = rc.right - rc.left, height = rc.bottom - rc.top;

    pglPixelStorei(GL_PACK_ALIGNMENT, 1);
    pglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    pglReadBuffer(GL_FRONT);

    // 映射与跨进程互斥体按名字缓存复用，避免每帧重开映射导致整帧搬运反复缺页
    // （1080p 上实测 2.66ms → 1.01ms，见 SharedFrameChannel.h）。
    // 宿主可能只要一块（§3.2）：只读那几行。GL 的整帧读取本来就是 CPU 回读，
    // 按矩形读掉的正是最贵的那部分。
    const FrameRequestSnapshot raw_request = ReadFrameRequestByName(DisplayHook::shared_res_name, width, height);
    const FrameRequestSnapshot request =
        NormalizeFrameRequest(raw_request, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
    // 响应描述**这一帧缓冲区里的实际布局**（矩形/整窗），无条件写：请求 TTL 过期时
    // 注入侧退回整窗拷贝，响应必须跟着变成整窗，否则宿主会按旧矩形解释整窗帧（§72.19）。
    // 协商区不存在时 WriteFrameResponse 自己 no-op，旧宿主不受影响。

    SharedFrameChannelLease channel(DisplayHook::shared_res_name, DisplayHook::mutex_name);
    if (channel.ok()) {
        if (!channel.mutex().lock_timeout(op::hook::kSharedFrameLockTimeoutMs)) {
            // 宿主没在超时内释放 → 跳过本帧，别让 Present 无限等。
            // 不记 note_capture_failure：慢的是宿主那边，记失败会误禁捕。
            // 返回 0 而不是负数：调用处把 <0 当“API 解析失败”，会累加失败计数。
            return 0;
        }
        SharedMemory &mem = channel.mem();
        uchar *pshare = mem.data<byte>();
        if (SharedFrameHasCapacity(mem, width, height)) {
            // 只统一帧头：像素由 glReadPixels 直接写进共享段，没有中间缓冲可拷。
            WriteSharedFrameHeader(mem, DisplayHook::render_hwnd, width, height);
            uchar *pixels = pshare + sizeof(FrameInfo);
            if (request.valid) {
                // 行序与整窗一致（第 0 行 = 窗口最下面一行），翻转由宿主按区域做
                // （HookCapture.cpp 的 source_row = rh-1-row_offset-i），这里不能再翻。
                // Y 必须换算：glReadPixels 的原点在**左下角**，请求里的 y 是窗口坐标
                // （左上原点）。直接传 request.y 读到的是上下镜像的一条带 —— 目标画面
                // 上下对称时看不出来，真实游戏（血条、HUD、准星）会立刻错位。
                const int gl_y = height - (request.y + request.h);
                pglReadPixels(request.x, gl_y, request.w, request.h, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);
                WriteFrameResponse(mem, width, height, kFrameModeRegion, request.seq, request.x, request.y,
                                   request.w, request.h);
            } else {
                pglReadPixels(0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);
                WriteFrameResponse(mem, width, height, kFrameModeFull, 0, 0, 0, width, height);
            }
        } else {
            WriteSharedFrameHeader(mem, DisplayHook::render_hwnd, width, height);
        }
        channel.mutex().unlock();
    } else {
        DisplayHook::note_capture_failure();
#if DEBUG_HOOK
        setlog(L"!mem.open(DisplayHook::%s)&&mutex.open(DisplayHook::%s)", DisplayHook::shared_res_name.c_str(),
               DisplayHook::mutex_name.c_str());
#endif // DEBUG_HOOK
    }
    return 0;
}

// 一个进程里可能有多个 GL 窗口（多视口、工具窗、加载窗）。共享段是按**被绑定窗口**的 hwnd
// 命名的，别的窗口的帧写进来会让"像素区"和"响应区"来自不同窗口，宿主按整窗解释一块区域，
// 画面就冻在第一次读到的颜色上（和 §72.22 的 d3d11 多链缺陷同源）。
// hdc 为空时按当前线程的 DC 判断（glBegin/glFinish 那条路拿不到 hdc）。
// wglGetCurrentDC 不直接链 opengl32.lib（注入模块一直靠 ResolveApi 动态取符号），
// 这里同样按 GetProcAddress 拿一次。
HDC current_wgl_dc() {
    typedef HDC(WINAPI * wglGetCurrentDC_t)(void);
    static wglGetCurrentDC_t fn = [] {
        HMODULE mod = ::GetModuleHandleW(L"opengl32.dll");
        return mod ? reinterpret_cast<wglGetCurrentDC_t>(::GetProcAddress(mod, "wglGetCurrentDC")) : nullptr;
    }();
    return fn ? fn() : nullptr;
}

bool gl_is_bound_window(HDC hdc) {
    if (DisplayHook::render_hwnd == 0) {
        return true;
    }
    if (hdc == nullptr) {
        hdc = current_wgl_dc();
    }
    if (hdc == nullptr) {
        return true;
    }
    HWND wnd = ::WindowFromDC(hdc);
    return wnd == nullptr || wnd == DisplayHook::render_hwnd;
}

} // namespace

long gl_capture() {
    if (gl_capture_impl("opengl32.dll") < 0) {
        DisplayHook::note_capture_failure();
    } else {
        DisplayHook::note_capture_success();
    }
    return 0;
}

void __stdcall gl_hkglBegin(GLenum mode) {
    static DWORD t = 0;
    using glBegin_t = decltype(glBegin) *;
    // 先登记「我在 detour 里」，并留一份原函数地址。release() 要等这个计数归零
    // 才清 old_address、释放 trampoline；否则正在跑的这一次会调到空指针或已回收的代码。
    void *const original = DisplayHook::old_address;
    DisplayHook::DetourScope detour_scope;
    if (DisplayHook::capture_enabled() && gl_is_bound_window(nullptr))
        gl_capture();
    if (original)
        ((glBegin_t)original)(mode);
}

void __stdcall gl_hkwglSwapBuffers(HDC hdc) {
    using wglSwapBuffers_t = void(__stdcall *)(HDC hdc);
    // 先登记「我在 detour 里」，并留一份原函数地址。release() 要等这个计数归零
    // 才清 old_address、释放 trampoline；否则正在跑的这一次会调到空指针或已回收的代码。
    void *const original = DisplayHook::old_address;
    DisplayHook::DetourScope detour_scope;
    if (DisplayHook::capture_enabled() && gl_is_bound_window(hdc))
        gl_capture();
    if (original)
        ((wglSwapBuffers_t)original)(hdc);
}

long egl_capture() {
    if (gl_capture_impl("libglesv2.dll") < 0) {
        DisplayHook::note_capture_failure();
    } else {
        DisplayHook::note_capture_success();
    }
    return 0;
}

unsigned int __stdcall gl_hkeglSwapBuffers(void *dpy, void *surface) {
    using eglSwapBuffers_t = decltype(gl_hkeglSwapBuffers) *;
    // 先登记「我在 detour 里」，并留一份原函数地址。release() 要等这个计数归零
    // 才清 old_address、释放 trampoline；否则正在跑的这一次会调到空指针或已回收的代码。
    void *const original = DisplayHook::old_address;
    DisplayHook::DetourScope detour_scope;
    if (!original) {
        return 0;
    }
    // 注意：这里没有按窗口过滤。EGL 的 eglSwapBuffers 只给 display/surface，没有 HDC，
    // 而 ANGLE 的 eglQuerySurface(EGL_NATIVE_WINDOW) 返回 EGL_BAD_ATTRIBUTE(0x3004)，
    // 拿不到 surface 对应的窗口句柄 → 一个进程里有两个 EGL 窗口时无法只抓被绑定的那个。
    // 详见 doc/defect_fixes.md §72.26。
    if (DisplayHook::capture_enabled())
        egl_capture();
    return ((eglSwapBuffers_t)original)(dpy, surface);
}

void __stdcall gl_hkglFinish(void) {
    using glFinish_t = decltype(glFinish) *;
    // 先登记「我在 detour 里」，并留一份原函数地址。release() 要等这个计数归零
    // 才清 old_address、释放 trampoline；否则正在跑的这一次会调到空指针或已回收的代码。
    void *const original = DisplayHook::old_address;
    DisplayHook::DetourScope detour_scope;
    if (DisplayHook::capture_enabled())
        gl_capture();
    if (original)
        ((glFinish_t)original)();
}

} // namespace op::hook
