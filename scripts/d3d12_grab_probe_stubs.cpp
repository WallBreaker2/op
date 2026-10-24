// 只为 scripts/d3d12_grab_probe.cpp 提供 DisplayHook 里被 D3D12Capture.cpp 引用到的符号。
// D3D12Capture.cpp 在真实构建里和 DisplayHook.cpp 一起编进 op_x64.dll；探针不链接
// DisplayHook.cpp（它带 MinHook 与一大堆 D3D9/11/12 hook 代码），所以在这里补最小定义。
// 注意：CopyImageData / GetImageBufferFormat（DxCaptureCommon.cpp）和 SharedFrameChannelLease
// （SharedFrameChannel.cpp）是编真代码进来的，这里不要再写桩，否则重复定义。
#include "../libop/hook/DisplayHook.h"

namespace op::hook {

HWND DisplayHook::render_hwnd = nullptr;
int DisplayHook::render_type = 0;
std::wstring DisplayHook::shared_res_name;
std::wstring DisplayHook::mutex_name;
void *DisplayHook::old_address = nullptr;
void *DisplayHook::hook_target = nullptr;
bool DisplayHook::is_hooked = false;
// detour_depth / DetourScope 是解绑时等 detour 走完用的（见 DisplayHook.h）。
// dx12_hkPresent 里用到了 DetourScope，探针不装钩子但要能链接。
std::atomic<long> DisplayHook::detour_depth{0};

bool DisplayHook::capture_enabled() {
    return true;
}

void DisplayHook::set_capture_enabled(bool enabled) {
    (void)enabled;
}

int DisplayHook::setup(HWND hwnd_, int render_type_) {
    (void)hwnd_;
    (void)render_type_;
    return 0;
}

int DisplayHook::release() {
    return 0;
}

} // namespace op::hook
