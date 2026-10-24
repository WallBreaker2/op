// 只为 scripts/hook_capture_probe.cpp 提供 DisplayHook 与 setlog 的符号。
// D3D11Capture.cpp 在真实构建里和 DisplayHook.cpp 一起编进 op_x64.dll；探针不链接
// DisplayHook.cpp（它带 MinHook 和 D3D9/10/12/GL 的一堆 hook 代码），所以在这里补最小定义。
#include "../libop/base/Utils.h"
#include "../libop/hook/DisplayHook.h"

#include <cstdarg>
#include <cstdio>

namespace op::hook {

HWND DisplayHook::render_hwnd = nullptr;
int DisplayHook::render_type = 0;
std::wstring DisplayHook::shared_res_name;
std::wstring DisplayHook::mutex_name;
void *DisplayHook::old_address = nullptr;
void *DisplayHook::hook_target = nullptr;
bool DisplayHook::is_hooked = false;
// detour_depth 是解绑时用来等 detour 走完的计数器（见 DisplayHook.h）。探针不装钩子，
// 但它把 D3D11Capture.cpp 编进来了，而 dx11_hkPresent 里用到了 DetourScope，所以要补定义。
std::atomic<long> DisplayHook::detour_depth{0};

namespace {
bool g_capture_enabled = true;
int g_capture_failures = 0;
}

bool DisplayHook::capture_enabled() {
    return g_capture_enabled;
}

void DisplayHook::set_capture_enabled(bool enabled) {
    // 产品里这里会停掉整个后台截图；探针需要看见"被停掉"这件事，所以打印出来。
    if (g_capture_enabled && !enabled) {
        std::printf("  [WARN] 捕获被后端自己关掉了（set_capture_enabled(false)）\n");
    }
    g_capture_enabled = enabled;
}

int DisplayHook::setup(HWND hwnd_, int render_type_) {
    (void)hwnd_;
    (void)render_type_;
    return 0;
}

int DisplayHook::release() {
    return 0;
}

void DisplayHook::note_capture_failure() {
    // 探针里保留与产品一致语义：连续失败到阈值才停抓。
    if (++g_capture_failures >= kCaptureFailureLimit) {
        std::printf("  [WARN] 连续失败 %d 次，产品会停抓（note_capture_failure）\n", g_capture_failures);
        set_capture_enabled(false);
    }
}

void DisplayHook::note_capture_success() {
    g_capture_failures = 0;
}

} // namespace op::hook

long setlog(const char *format, ...) {
    va_list args;
    va_start(args, format);
    std::printf("  [log] ");
    std::vprintf(format, args);
    std::printf("\n");
    va_end(args);
    return 0;
}

long setlog(const wchar_t *format, ...) {
    va_list args;
    va_start(args, format);
    std::printf("  [log] ");
    std::vwprintf(format, args);
    std::printf("\n");
    va_end(args);
    return 0;
}
