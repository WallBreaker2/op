#include "HookExport.h"
#include "../runtime/RuntimeEnvironment.h"
#include "DisplayHook.h"
#include "InputHook.h"
#include "../../3rd_party/include/kiero.h"

using op::hook::DisplayHook;
using op::hook::InputHook;

namespace {

int g_ref_count = 0;
int g_input_ref_count = 0;

int to_kiero_render_type(int render_type_) {
    if (render_type_ == RDT_DX_DEFAULT || render_type_ == RDT_DX_D3D9)
        return kiero::RenderType::D3D9;
    if (render_type_ == RDT_DX_D3D10)
        return kiero::RenderType::D3D10;
    if (render_type_ == RDT_DX_D3D11)
        return kiero::RenderType::D3D11;
    if (render_type_ == RDT_DX_D3D12)
        return kiero::RenderType::D3D12;
    if (render_type_ == RDT_GL_DEFAULT || render_type_ == RDT_GL_NOX || render_type_ == RDT_GL_STD ||
        render_type_ == RDT_GL_FI)
        return kiero::RenderType::OpenGL;
    if (render_type_ == RDT_GL_ES)
        return kiero::RenderType::OpenglES;
    return kiero::RenderType::None;
}

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
        if (DisplayHook::render_hwnd == hwnd_ && DisplayHook::render_type == to_kiero_render_type(render_type_)) {
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
        release_module_if_idle();
    }

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
