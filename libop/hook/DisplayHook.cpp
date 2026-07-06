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
static int is_capture;

namespace {

constexpr int kPresentIndex = 8;
constexpr int kD3D9EndSceneIndex = 42;

template <typename T> void *method_at(const std::vector<T> &methods, size_t index) {
    return index < methods.size() ? reinterpret_cast<void *>(methods[index]) : nullptr;
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
    DisplayHook::render_hwnd = hwnd_;
    DisplayHook::shared_res_name = MakeOpSharedResourceName(hwnd_);
    DisplayHook::mutex_name = MakeOpMutexName(hwnd_);

    render_type = render_type_;
    old_address = nullptr;
    hook_target = nullptr;

    void *address = nullptr;
    if (!locate_render_method(render_type_, &hook_target, &address)) {
        setlog("DisplayHook setup locate failed hwnd=%p render_type=%d target=%p detour=%p", hwnd_, render_type_,
               hook_target, address);
        return 0;
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

int DisplayHook::release() {
    set_capture_enabled(false);
    if (hook_target) {
        MH_DisableHook(hook_target);
        MH_RemoveHook(hook_target);
        ReleaseMinHook();
    }
    old_address = nullptr;
    hook_target = nullptr;
    render_hwnd = NULL;
    render_type = 0;
    return 1;
}

bool DisplayHook::capture_enabled() {
    return is_capture != 0;
}

void DisplayHook::set_capture_enabled(bool enabled) {
    is_capture = enabled ? 1 : 0;
}

} // namespace op::hook
