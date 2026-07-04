#include "OpenGLCapture.h"

#include "DisplayHook.h"
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

long gl_capture() {
    using glPixelStorei_t = decltype(glPixelStorei) *;
    using glReadBuffer_t = decltype(glReadBuffer) *;
    using glGetIntegerv_t = decltype(glGetIntegerv) *;
    using glReadPixels_t = decltype(glReadPixels) *;

    auto pglPixelStorei = (glPixelStorei_t)ResolveApi("opengl32.dll", "glPixelStorei");
    auto pglReadBuffer = (glReadBuffer_t)ResolveApi("opengl32.dll", "glReadBuffer");
    auto pglGetIntegerv = (glGetIntegerv_t)ResolveApi("opengl32.dll", "glGetIntegerv");
    auto pglReadPixels = (glReadPixels_t)ResolveApi("opengl32.dll", "glReadPixels");
    if (!pglPixelStorei || !pglReadBuffer || !pglGetIntegerv || !pglReadPixels) {
        DisplayHook::set_capture_enabled(false);
#if DEBUG_HOOK
        setlog("error.!pglPixelStorei || !pglReadBuffer || !pglGetIntegerv || !pglReadPixels");
#endif // DEBUG_HOOK

        return 0;
    }
    RECT rc;
    ::GetClientRect(DisplayHook::render_hwnd, &rc);
    int width = rc.right - rc.left, height = rc.bottom - rc.top;

    pglPixelStorei(GL_PACK_ALIGNMENT, 1);
    pglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    pglReadBuffer(GL_FRONT);

    SharedMemory mem;
    ProcessMutex mutex;
    if (mem.open(DisplayHook::shared_res_name) && mutex.open(DisplayHook::mutex_name)) {
        mutex.lock();
        uchar *pshare = mem.data<byte>();
        reinterpret_cast<FrameInfo *>(pshare)->format(DisplayHook::render_hwnd, width, height);
        pglReadPixels(0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pshare + sizeof(FrameInfo));
        mutex.unlock();
    } else {
        DisplayHook::set_capture_enabled(false);
#if DEBUG_HOOK
        setlog(L"egl !mem.open(DisplayHook::%s)&&mutex.open(DisplayHook::%s)",
               DisplayHook::shared_res_name.c_str(), DisplayHook::mutex_name.c_str());
#endif // DEBUG_HOOK
    }
    return 0;
}

void __stdcall gl_hkglBegin(GLenum mode) {
    static DWORD t = 0;
    using glBegin_t = decltype(glBegin) *;

    if (DisplayHook::capture_enabled())
        gl_capture();
    ((glBegin_t)DisplayHook::old_address)(mode);
}

void __stdcall gl_hkwglSwapBuffers(HDC hdc) {
    using wglSwapBuffers_t = void(__stdcall *)(HDC hdc);
    if (DisplayHook::capture_enabled())
        gl_capture();
    ((wglSwapBuffers_t)DisplayHook::old_address)(hdc);
}

long egl_capture() {
    using glPixelStorei_t = decltype(glPixelStorei) *;
    using glReadBuffer_t = decltype(glReadBuffer) *;
    using glGetIntegerv_t = decltype(glGetIntegerv) *;
    using glReadPixels_t = decltype(glReadPixels) *;

    auto pglPixelStorei = (glPixelStorei_t)ResolveApi("libglesv2.dll", "glPixelStorei");
    auto pglReadBuffer = (glReadBuffer_t)ResolveApi("libglesv2.dll", "glReadBuffer");
    auto pglGetIntegerv = (glGetIntegerv_t)ResolveApi("libglesv2.dll", "glGetIntegerv");
    auto pglReadPixels = (glReadPixels_t)ResolveApi("libglesv2.dll", "glReadPixels");
    if (!pglPixelStorei || !pglReadBuffer || !pglGetIntegerv || !pglReadPixels) {
#if DEBUG_HOOK
        setlog(L"egl !mem.open(DisplayHook::%s)&&mutex.open(DisplayHook::%s)",
               DisplayHook::shared_res_name.c_str(), DisplayHook::mutex_name.c_str());
#endif // DEBUG_HOOK

        return 0;
    }
    RECT rc;
    ::GetClientRect(DisplayHook::render_hwnd, &rc);
    int width = rc.right - rc.left, height = rc.bottom - rc.top;

    pglPixelStorei(GL_PACK_ALIGNMENT, 1);
    pglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    pglReadBuffer(GL_FRONT);

    SharedMemory mem;
    ProcessMutex mutex;
    if (mem.open(DisplayHook::shared_res_name) && mutex.open(DisplayHook::mutex_name)) {
        mutex.lock();
        uchar *pshare = mem.data<byte>();
        reinterpret_cast<FrameInfo *>(pshare)->format(DisplayHook::render_hwnd, width, height);
        pglReadPixels(0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pshare + sizeof(FrameInfo));
        mutex.unlock();
    } else {
        DisplayHook::set_capture_enabled(false);
#if DEBUG_HOOK
        setlog(L"egl !mem.open(DisplayHook::%s)&&mutex.open(DisplayHook::%s)",
               DisplayHook::shared_res_name.c_str(), DisplayHook::mutex_name.c_str());
#endif // DEBUG_HOOK
    }
    return 0;
}

unsigned int __stdcall gl_hkeglSwapBuffers(void *dpy, void *surface) {
    using eglSwapBuffers_t = decltype(gl_hkeglSwapBuffers) *;
    if (DisplayHook::capture_enabled())
        egl_capture();
    return ((eglSwapBuffers_t)DisplayHook::old_address)(dpy, surface);
}

void __stdcall gl_hkglFinish(void) {
    using glFinish_t = decltype(glFinish) *;
    if (DisplayHook::capture_enabled())
        gl_capture();
    ((glFinish_t)DisplayHook::old_address)();
}

} // namespace op::hook
