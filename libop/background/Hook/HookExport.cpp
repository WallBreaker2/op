#include "HookExport.h"
#include "../../core/opEnv.h"
#include "DisplayHook.h"
#include "InputHook.h"
#include "../../../3rd_party/include/kiero.h"
int refCount = 0;

static int to_kiero_render_type(int render_type_) {
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

//--------------export function--------------------------
long __stdcall SetDisplayHook(HWND hwnd_, int render_type_) {
    int ret = 0;
    opEnv::m_showErrorMsg =
        2; // this code is excuate in hookde process,so its better not show meesageBox(avoid suspend the work thread)
    if (!DisplayHook::is_hooked) {
        ret = DisplayHook::setup(hwnd_, render_type_);
        DisplayHook::is_hooked = ret == 1;
        refCount += DisplayHook::is_hooked;
    } else {
        if (DisplayHook::render_hwnd == hwnd_ && DisplayHook::render_type == to_kiero_render_type(render_type_)) {
            ret = 1;
        } else {
            DisplayHook::release();
            ret = DisplayHook::setup(hwnd_, render_type_);
            DisplayHook::is_hooked = ret == 1;
            if (!DisplayHook::is_hooked && refCount > 0) {
                refCount--;
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
        if (refCount > 0) {
            refCount--;
        }
        if (refCount == 0) {
            ::FreeLibraryAndExitThread(static_cast<HMODULE>(opEnv::getInstance()), 0);
        }
    }

    return ret;
}

long __stdcall SetInputHook(HWND hwnd_, int input_type_) {
    int ret = 0;
    if (!InputHook::is_hooked) {
        ret = InputHook::setup(hwnd_, input_type_);
        InputHook::is_hooked = ret == 1;
        refCount += InputHook::is_hooked;
    }
    return ret;
}

long __stdcall ReleaseInputHook() {
    if (InputHook::is_hooked) {
        InputHook::release();
        InputHook::is_hooked = false;
        refCount--;
        if (refCount == 0) {
            ::FreeLibraryAndExitThread(static_cast<HMODULE>(opEnv::getInstance()), 0);
        }
    }
    return 1;
}
