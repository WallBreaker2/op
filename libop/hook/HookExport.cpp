#include "HookExport.h"
#include "../base/Environment.h"
#include "DisplayHook.h"
#include "InputHook.h"

using op::hook::DisplayHook;
using op::hook::InputHook;

namespace {

int g_ref_count = 0;
int g_input_ref_count = 0;

void release_module_if_idle() {
    if (g_ref_count == 0) {
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
        g_ref_count += DisplayHook::is_hooked;
    } else {
        if (DisplayHook::render_hwnd == hwnd_ && DisplayHook::render_type == render_type_) {
            ret = 1;
        } else {
            DisplayHook::release();
            ret = DisplayHook::setup(hwnd_, render_type_);
            DisplayHook::is_hooked = ret == 1;
            if (!DisplayHook::is_hooked && g_ref_count > 0) {
                g_ref_count--;
            }
        }
    }
    return ret;
}

long __stdcall ReleaseDisplayHook() {
    int ret = 0;
    if (DisplayHook::is_hooked) {
        DisplayHook::is_hooked = false;
        ret = DisplayHook::release();
        if (g_ref_count > 0)
            g_ref_count--;
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
            g_ref_count++;
            g_input_ref_count = 1;
        }
    } else if (InputHook::input_hwnd == hwnd_) {
        // 鼠标和键盘 dx 共用一个远端 Hook，等双方都释放后再卸载。
        g_input_ref_count++;
        ret = 1;
    }
    return ret;
}

long __stdcall ReleaseInputHook() {
    if (InputHook::is_hooked && g_input_ref_count > 0 && --g_input_ref_count == 0) {
        InputHook::release();
        InputHook::is_hooked = false;
        if (g_ref_count > 0)
            g_ref_count--;
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
