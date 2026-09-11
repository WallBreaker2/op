#include "HookExport.h"
#include "../base/Environment.h"
#include "../base/Utils.h"
#include "DisplayHook.h"
#include "InputHook.h"

#include <atomic>

using op::hook::DisplayHook;
using op::hook::InputHook;

namespace {

// 这些计数由注入到目标进程的导出函数在任意线程读写，必须是原子量：
// 普通的 ++/-- 会丢失更新，让计数提前归零并在 hook 仍存活时卸载本模块。
std::atomic<int> g_ref_count{0};
std::atomic<int> g_display_ref_count{0};
std::atomic<int> g_input_ref_count{0};

void release_module_if_idle() {
    if (g_ref_count.load() == 0) {
        ::FreeLibraryAndExitThread(static_cast<HMODULE>(RuntimeEnvironment::getInstance()), 0);
    }
}

} // namespace

long __stdcall SetDisplayHook(HWND hwnd_, int render_type_) {
    int ret = 0;
    RuntimeEnvironment::m_showErrorMsg = 2;
    if (!DisplayHook::is_hooked) {
        ret = DisplayHook::setup(hwnd_, render_type_);
        DisplayHook::is_hooked = ret == 1;
        if (DisplayHook::is_hooked) {
            g_ref_count.fetch_add(1);
            g_display_ref_count.store(1);
        }
    } else {
        if (DisplayHook::render_hwnd == hwnd_ && DisplayHook::render_type == render_type_) {
            g_display_ref_count.fetch_add(1);
            ret = 1;
        } else {
            if (g_display_ref_count.load() > 0) {
                setlog("DisplayHook already bound hwnd=%p render_type=%d, reject hwnd=%p render_type=%d",
                       DisplayHook::render_hwnd, DisplayHook::render_type, hwnd_, render_type_);
                return 0;
            }
            DisplayHook::release();
            if (g_ref_count.load() > 0)
                g_ref_count.fetch_sub(1);
            ret = DisplayHook::setup(hwnd_, render_type_);
            DisplayHook::is_hooked = ret == 1;
            if (DisplayHook::is_hooked) {
                g_ref_count.fetch_add(1);
                g_display_ref_count.store(1);
            }
        }
    }
    return ret;
}

long __stdcall ReleaseDisplayHook() {
    int ret = 0;
    if (DisplayHook::is_hooked && g_display_ref_count.load() > 0 && g_display_ref_count.fetch_sub(1) == 1) {
        DisplayHook::is_hooked = false;
        ret = DisplayHook::release();
        if (g_ref_count.load() > 0)
            g_ref_count.fetch_sub(1);
    }

    release_module_if_idle();

    return ret;
}

long __stdcall SetInputHook(HWND hwnd_, int) {
    int ret = 0;
    if (!InputHook::is_hooked) {
        ret = InputHook::setup(hwnd_);
        InputHook::is_hooked = ret == 1;
        if (InputHook::is_hooked) {
            g_ref_count.fetch_add(1);
            g_input_ref_count.store(1);
        }
    } else if (InputHook::input_hwnd == hwnd_) {
        // 鼠标和键盘 dx 共用一个远端 Hook，等双方都释放后再卸载。
        g_input_ref_count.fetch_add(1);
        ret = 1;
    }
    return ret;
}

long __stdcall ReleaseInputHook() {
    if (InputHook::is_hooked && g_input_ref_count.load() > 0 && g_input_ref_count.fetch_sub(1) == 1) {
        InputHook::release();
        InputHook::is_hooked = false;
        if (g_ref_count.load() > 0)
            g_ref_count.fetch_sub(1);
        release_module_if_idle();
    }
    return 1;
}

long __stdcall SetInputLock(int lock) {
    if (!InputHook::is_hooked)
        return lock == 0 ? 1 : 0;
    return InputHook::lockInput(lock);
}

unsigned long long __stdcall GetInputCursorShapeHash() {
    return InputHook::cursorShapeHash();
}

unsigned long long __stdcall GetInputCursorShapeMeta() {
    return InputHook::cursorShapeMeta();
}

unsigned long __stdcall GetInputCursorShapeHashLow() {
    return static_cast<unsigned long>(InputHook::cursorShapeHash() & 0xFFFFFFFFull);
}

unsigned long __stdcall GetInputCursorShapeHashHigh() {
    return static_cast<unsigned long>((InputHook::cursorShapeHash() >> 32) & 0xFFFFFFFFull);
}

unsigned long __stdcall GetInputCursorShapeMetaLow() {
    return static_cast<unsigned long>(InputHook::cursorShapeMeta() & 0xFFFFFFFFull);
}

unsigned long __stdcall GetInputCursorShapeMetaHigh() {
    return static_cast<unsigned long>((InputHook::cursorShapeMeta() >> 32) & 0xFFFFFFFFull);
}
