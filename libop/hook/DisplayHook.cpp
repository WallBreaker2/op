#include "DisplayHook.h"

#include "D3D10Capture.h"
#include "D3D11Capture.h"
#include "D3D12Capture.h"
#include "D3D9Capture.h"
#include "MinHook.h"
#include "MinHookRuntime.h"
#include "OpenGLCapture.h"
#include "kiero.hpp"
#include "kiero_d3d9.hpp"
#include "kiero_d3d10.hpp"
#include "kiero_d3d11.hpp"
#include "kiero_d3d12.hpp"
#include "kiero_opengl.hpp"
#include "../hook/ApiResolver.h"
#include "../base/AutomationModes.h"
#include "../base/Utils.h"
#include <atomic>
#include <string>
#include <vector>

namespace op::hook {

HWND DisplayHook::render_hwnd = NULL;
int DisplayHook::render_type = 0;
std::wstring DisplayHook::shared_res_name;
std::wstring DisplayHook::mutex_name;
void *DisplayHook::old_address;
void *DisplayHook::hook_target;
bool DisplayHook::is_hooked = false;
bool DisplayHook::pending_release = false;
std::atomic<long> DisplayHook::detour_depth{0};
static int is_capture;


namespace {

constexpr int kPresentIndex = 8;
constexpr int kD3D9EndSceneIndex = 42;

template <typename T> void *method_at(const std::vector<T> &methods, size_t index) {
    return index < methods.size() ? reinterpret_cast<void *>(methods[index]) : nullptr;
}

// 拿这个渲染类型所依赖的模块基址，用来判断缓存的 vtable 地址还有效。
// locate 成功之后就一定加载了，所以缓存里的基址总能取到。
HMODULE render_module(int render_type) {
    if (render_type == RDT_DX_DEFAULT || render_type == RDT_DX_D3D9) {
        return ::GetModuleHandleW(L"d3d9.dll");
    }
    if (render_type == RDT_DX_D3D10 || render_type == RDT_DX_D3D11 || render_type == RDT_DX_D3D12) {
        return ::GetModuleHandleW(L"dxgi.dll");
    }
    if (render_type == RDT_GL_ES) {
        return ::GetModuleHandleW(L"libEGL.dll");
    }
    if (render_type == RDT_GL_DEFAULT || render_type == RDT_GL_STD || render_type == RDT_GL_NOX ||
        render_type == RDT_GL_FI) {
        return ::GetModuleHandleW(L"opengl32.dll");
    }
    return nullptr;
}

// kiero 定位一个方法要临时建一个设备（实测 140ms），但同一进程里同一个模块的 vtable
// 地址不会变。重绑时直接复用上一次的结果，就不用再建一次临时设备。
struct LocatedTarget {
    bool valid = false;
    int render_type = 0;
    HMODULE module = nullptr;
    void *target = nullptr;
    void *detour = nullptr;
};

LocatedTarget g_located;

bool reuse_located_target(int render_type, void **target, void **detour) {
    if (!target || !detour) {
        return false;
    }
    *target = nullptr;
    *detour = nullptr;
    if (!g_located.valid || g_located.render_type != render_type) {
        return false;
    }
    const HMODULE module = render_module(render_type);
    if (module == nullptr || module != g_located.module) {
        return false;
    }
    *target = g_located.target;
    *detour = g_located.detour;
    return true;
}

void remember_located_target(int render_type, void *target, void *detour) {
    g_located.valid = true;
    g_located.render_type = render_type;
    g_located.module = render_module(render_type);
    g_located.target = target;
    g_located.detour = detour;
}

int locate_render_method(int render_type, void **target, void **detour) {
    if (!target || !detour)
        return 0;

    *target = nullptr;
    *detour = nullptr;

    if (render_type == RDT_DX_DEFAULT || render_type == RDT_DX_D3D9) {
        kiero::D3D9Output output;
        const kiero::Error error = kiero::locate<kiero::Implementation_D3D9>(nullptr, &output);
        if (error != kiero::Error_Nil) {
            setlog("DisplayHook locate D3D9 failed render_type=%d error=%d", render_type, error);
            return 0;
        }
        *target = method_at(output.device_methods, kD3D9EndSceneIndex);
        *detour = reinterpret_cast<void *>(dx9_hkEndScene);
    } else if (render_type == RDT_DX_D3D10) {
        kiero::D3D10Output output;
        const kiero::Error error = kiero::locate<kiero::Implementation_D3D10>(nullptr, &output);
        if (error != kiero::Error_Nil) {
            setlog("DisplayHook locate D3D10 failed render_type=%d error=%d", render_type, error);
            return 0;
        }
        *target = method_at(output.swapchain_methods, kPresentIndex);
        *detour = reinterpret_cast<void *>(dx10_hkPresent);
    } else if (render_type == RDT_DX_D3D11) {
        kiero::D3D11Output output;
        const kiero::Error error = kiero::locate<kiero::Implementation_D3D11>(nullptr, &output);
        if (error != kiero::Error_Nil) {
            setlog("DisplayHook locate D3D11 failed render_type=%d error=%d", render_type, error);
            return 0;
        }
        *target = method_at(output.swapchain_methods, kPresentIndex);
        *detour = reinterpret_cast<void *>(dx11_hkPresent);
    } else if (render_type == RDT_DX_D3D12) {
        kiero::D3D12Output output;
        const kiero::Error error = kiero::locate<kiero::Implementation_D3D12>(nullptr, &output);
        if (error != kiero::Error_Nil) {
            setlog("DisplayHook locate D3D12 failed render_type=%d error=%d", render_type, error);
            return 0;
        }
        *target = method_at(output.swapchain_methods, kPresentIndex);
        *detour = reinterpret_cast<void *>(dx12_hkPresent);
    } else if (render_type == RDT_GL_DEFAULT || render_type == RDT_GL_NOX) {
        kiero::OpenGLOutput output;
        const kiero::Error error = kiero::locate<kiero::Implementation_OpenGL>(nullptr, &output);
        if (error != kiero::Error_Nil) {
            setlog("DisplayHook locate OpenGL failed render_type=%d error=%d", render_type, error);
            return 0;
        }
        *target = output.methods["wglSwapBuffers"];
        *detour = reinterpret_cast<void *>(gl_hkwglSwapBuffers);
    } else if (render_type == RDT_GL_STD) {
        kiero::OpenGLOutput output;
        const kiero::Error error = kiero::locate<kiero::Implementation_OpenGL>(nullptr, &output);
        if (error != kiero::Error_Nil) {
            setlog("DisplayHook locate OpenGL failed render_type=%d error=%d", render_type, error);
            return 0;
        }
        *target = output.methods["glBegin"];
        *detour = reinterpret_cast<void *>(gl_hkglBegin);
    } else if (render_type == RDT_GL_ES) {
        *target = ResolveApi("libEGL.dll", "eglSwapBuffers");
        *detour = reinterpret_cast<void *>(gl_hkeglSwapBuffers);
    } else if (render_type == RDT_GL_FI) {
        kiero::OpenGLOutput output;
        const kiero::Error error = kiero::locate<kiero::Implementation_OpenGL>(nullptr, &output);
        if (error != kiero::Error_Nil) {
            setlog("DisplayHook locate OpenGL failed render_type=%d error=%d", render_type, error);
            return 0;
        }
        *target = output.methods["glFinish"];
        *detour = reinterpret_cast<void *>(gl_hkglFinish);
    }

    return *target && *detour ? 1 : 0;
}

} // namespace

int DisplayHook::setup(HWND hwnd_, int render_type_) {
    // 上一次 release() 没等到 detour 静默时留下了旧钩子和旧 trampoline。同一入口上
    // 再 MH_CreateHook 会返回 MH_ERROR_ALREADY_CREATED，绑定直接失败；所以先重试
    // 清理。目标进程每秒几十次进 detour，通常几十毫秒内就会静默；真等不到就放弃
    // 本次绑定（调用方重试），绝不覆盖还活着的 trampoline 指针。
    if (!finish_pending_release(1000)) {
        setlog("DisplayHook setup rejected: previous hook still busy depth=%ld hwnd=%p", detour_depth.load(), hwnd_);
        return 0;
    }

    DisplayHook::render_hwnd = hwnd_;
    DisplayHook::shared_res_name = MakeOpSharedResourceName(hwnd_);
    DisplayHook::mutex_name = MakeOpMutexName(hwnd_);

    render_type = render_type_;
    old_address = nullptr;
    hook_target = nullptr;

    void *address = nullptr;
    const bool reused = reuse_located_target(render_type_, &hook_target, &address);
    if (!reused) {
        if (!locate_render_method(render_type_, &hook_target, &address)) {
            setlog("DisplayHook setup locate failed hwnd=%p render_type=%d target=%p detour=%p", hwnd_,
                   render_type_, hook_target, address);
            return 0;
        }
        remember_located_target(render_type_, hook_target, address);
    }

    if (!AcquireMinHook()) {
        setlog("DisplayHook setup AcquireMinHook failed hwnd=%p render_type=%d target=%p detour=%p", hwnd_, render_type_,
               hook_target, address);
        return 0;
    }

    const MH_STATUS create_status = MH_CreateHook(hook_target, address, &old_address);
    const MH_STATUS enable_status = create_status == MH_OK ? MH_EnableHook(hook_target) : MH_UNKNOWN;
    if (create_status != MH_OK || enable_status != MH_OK) {
        setlog("DisplayHook setup MinHook failed hwnd=%p render_type=%d target=%p detour=%p create=%d enable=%d",
               hwnd_, render_type_, hook_target, address, create_status, enable_status);
        if (create_status == MH_OK) {
            MH_DisableHook(hook_target);
            MH_RemoveHook(hook_target);
        }
        ReleaseMinHook();
        old_address = nullptr;
        hook_target = nullptr;
        return 0;
    }

    set_capture_enabled(true);
    return is_capture;
}

bool DisplayHook::wait_for_detours(int timeout_ms) {
    for (int waited = 0; waited < timeout_ms; waited += 2) {
        if (detour_depth.load() == 0) {
            return true;
        }
        ::Sleep(2);
    }
    return detour_depth.load() == 0;
}

bool DisplayHook::has_pending_release() {
    return pending_release;
}

bool DisplayHook::finish_pending_release(int timeout_ms) {
    if (!pending_release) {
        return true;
    }
    // 捕获开关在 release() 里就关了，这里只需要等已经在跑的 detour 走完；
    // 钩子还是 disable 状态，不会再有新调用进来。
    if (!wait_for_detours(timeout_ms)) {
        setlog("DisplayHook::finish_pending_release still busy (%ld) timeout=%d", detour_depth.load(), timeout_ms);
        return false;
    }
    if (hook_target) {
        MH_RemoveHook(hook_target);
        ReleaseMinHook();
    }
    old_address = nullptr;
    hook_target = nullptr;
    render_hwnd = NULL;
    render_type = 0;
    pending_release = false;
    setlog("DisplayHook::finish_pending_release done");
    return true;
}

int DisplayHook::release() {
    set_capture_enabled(false);
    // 先摘钩：此后不会再有新的调用进来（MinHook 会挂起其余线程再改指令，
    // 挂起期间进来的调用要么已走完、要么还没开始）。
    if (hook_target) {
        MH_DisableHook(hook_target);
    }
    // 再等已经在 detour 里的线程走完。典型耗时是单帧捕获的 10~20ms，
    // 给 500ms 足够；真等不到就宁可漏掉 trampoline 也不能把它释放掉 ——
    // 释放后仍在跑的 detour 回去会执行已回收的代码。
    const bool quiesced = wait_for_detours(500);
    if (hook_target && quiesced) {
        MH_RemoveHook(hook_target);
        ReleaseMinHook();
    }
    // 这两组状态只允许在没有 detour 在跑时清空：detour 里既要用 old_address 调原函数，
    // 也要用 render_hwnd 取客户区尺寸。
    if (quiesced) {
        old_address = nullptr;
        hook_target = nullptr;
        render_hwnd = NULL;
        render_type = 0;
    } else {
        // 记成"待清理"：trampoline、钩子、窗口信息全部留着不动（在跑的 detour 还在用），
        // 等下一次 setup()/ReleaseDisplayHook() 再收尾。见 finish_pending_release()。
        pending_release = true;
        setlog("DisplayHook::release detour still running (%ld), keep trampoline", detour_depth.load());
    }
    return 1;
}

bool DisplayHook::capture_enabled() {
    return is_capture != 0;
}

void DisplayHook::set_capture_enabled(bool enabled) {
    is_capture = enabled ? 1 : 0;
}

namespace {
// Present 可能来自多个交换链线程，计数用原子量。
std::atomic<int> g_capture_failures{0};
} // namespace

void DisplayHook::note_capture_failure() {
    const int failures = g_capture_failures.fetch_add(1, std::memory_order_relaxed) + 1;
    if (failures < kCaptureFailureLimit) {
        return;
    }
    // 只在达到阈值的那一次打日志，避免每帧刷屏。
    if (failures == kCaptureFailureLimit) {
        setlog("hook capture disabled after %d consecutive failures", failures);
    }
    set_capture_enabled(false);
}

void DisplayHook::note_capture_success() {
    g_capture_failures.store(0, std::memory_order_relaxed);
}

} // namespace op::hook
