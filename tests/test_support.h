#pragma once

#include <gtest/gtest.h>

#include <string>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <winhttp.h>

#include "../libop/hook/HookProtocol.h"
#include "../libop/base/Types.h"
#include "../include/libop.h"

namespace test_support {

using op::uchar;

class OpEnvironment : public ::testing::Environment {
  public:
    void SetUp() override;
};

std::wstring TrimCopy(const std::wstring &value);
std::wstring GetEnvString(const wchar_t *name);

// 严格模式：设了 OP_REQUIRE_HOOK_E2E 时，本该"因环境缺失而跳过"的用例改成失败。
// 跨进程注入用例在缺少交互式桌面 / ANGLE / x64 产物时会 GTEST_SKIP()，而 gtest 把
// 跳过算通过 —— 没有这个开关，整条注入链路根本没跑也会显示全绿（见 §72.51 R-02/R-03）。
bool RequireHookE2E();
std::wstring GetConfiguredOcrEndpoint();
bool IsOcrServerHealthy();
std::wstring PtrToWString(const void *ptr, bool hex = false);
std::vector<uchar> BuildBmp32TopDown(int width, int height, const std::vector<uchar> &bgra_pixels);
std::wstring ToLowerCopy(std::wstring value);
bool ContainsInsensitive(const std::wstring &haystack, const std::wstring &needle);
std::wstring NormalizeOcrLetters(const std::wstring &text);
std::wstring ExtractDigits(const std::wstring &text);
size_t EditDistance(const std::wstring &lhs, const std::wstring &rhs);
std::wstring GetTempBmpPath(const wchar_t *file_name);
bool CreateConsoleLikeBmp(const std::wstring &path, const std::wstring &text);
// 用 GDI 直接从桌面读一个像素（RRGGBB），作为"屏幕此刻到底是什么颜色"的独立基准。
// DXGI/WGC 截的是合成后的桌面，会被其它窗口遮挡；拿它对比比假设"测试窗口在最上面"稳。
std::wstring ReadDesktopPixelHex(int screen_x, int screen_y);

struct SendStringWindow {
    HWND parent = nullptr;
    HWND edit = nullptr;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    bool Create();
    std::wstring GetEditText() const;
    ~SendStringWindow();
};

struct MouseMoveEvent {
    long x = 0;
    long y = 0;
    WPARAM wparam = 0;
};

struct MouseButtonEvent {
    UINT message = 0;
    long x = 0;
    long y = 0;
    WPARAM wparam = 0;
};

struct MouseEventWindow {
    HWND hwnd = nullptr;
    int left_down = 0;
    int left_up = 0;
    int left_double = 0;
    int middle_down = 0;
    int middle_up = 0;
    int middle_double = 0;
    int right_down = 0;
    int right_up = 0;
    int right_double = 0;
    int xbutton1_down = 0;
    int xbutton1_up = 0;
    int xbutton1_double = 0;
    int xbutton2_down = 0;
    int xbutton2_up = 0;
    int xbutton2_double = 0;
    int wheel_count = 0;
    int wheel_delta_sum = 0;
    int hwheel_count = 0;
    int hwheel_delta_sum = 0;
    int move_count = 0;
    int move_with_left_count = 0;
    int raw_mouse_count = 0;
    int raw_keyboard_count = 0;
    int raw_left_down = 0;
    int raw_left_up = 0;
    int raw_xbutton1_down = 0;
    int raw_xbutton1_up = 0;
    int raw_hwheel_delta_sum = 0;
    int raw_wheel_delta_sum = 0;
    int raw_key_down = 0;
    int raw_key_up = 0;
    int raw_device_info_count = 0;
    int raw_device_name_count = 0;
    HCURSOR test_cursor = nullptr;

    int op_left_down = 0;
    int op_left_up = 0;
    int op_left_double = 0;
    int op_right_down = 0;
    int op_right_up = 0;
    int op_right_double = 0;
    int op_xbutton1_down = 0;
    int op_xbutton1_up = 0;
    int op_xbutton1_double = 0;
    int op_wheel_count = 0;
    int op_wheel_delta_sum = 0;
    int op_hwheel_count = 0;
    int op_hwheel_delta_sum = 0;
    int op_move_count = 0;

    long last_x = 0;
    long last_y = 0;
    WPARAM last_move_wparam = 0;
    std::vector<MouseMoveEvent> move_events;
    std::vector<MouseButtonEvent> button_events;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    bool Create();
    void SetTestCursor(HCURSOR cursor);
    bool HasMove(long x, long y, WPARAM required_state, WPARAM blocked_state = 0) const;
    bool HasButton(UINT message, long x, long y) const;
    void ResetCounts();
    ~MouseEventWindow();
};

struct ColorPulseWindow {
    HWND hwnd = nullptr;
    COLORREF current_color = RGB(255, 0, 0);
    std::vector<COLORREF> colors;
    size_t color_index = 0;
    bool animate = false;

    static constexpr UINT_PTR kTimerId = 1;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    // 默认仍用 CW_USEDEFAULT；真实截屏类用例可以指定位置，把窗口摆进显示器内部，
    // 否则窗口被摆到屏幕外时，DXGI 会按输出边界夹取并补黑，用例就会随机取到 000000。
    bool Create(bool enable_animation, int width = 220, int height = 180, int x = CW_USEDEFAULT,
                int y = CW_USEDEFAULT);
    void SetColor(COLORREF color);
    // 强制同步重绘一次。几何变化（最大化/还原/缩放）后客户区可能还没被 WM_PAINT 覆盖，
    // 此刻截到的是未绘制区域（实测是白色 FFFFFF），会让"取色=窗口本色"的断言随机变红。
    void Repaint();
    std::wstring CurrentColorHex() const;
    ~ColorPulseWindow();
};

class OcrTest : public ::testing::Test {
  protected:
    op::Op op;
    long ret = 0;

    void SetUp() override;
    void TearDown() override;
};

} // namespace test_support

// 环境不具备时的统一出口：严格模式下失败，否则跳过。
// 让 ctest 的 hook_require_e2e 能在环境缺失时变红，而不是静默通过。
#define OP_HOOK_E2E_SKIP_OR_FAIL(message_expr) \
    do {                                       \
        if (test_support::RequireHookE2E()) {  \
            FAIL() << (message_expr);          \
        } else {                               \
            GTEST_SKIP() << (message_expr);    \
        }                                      \
    } while (0)
