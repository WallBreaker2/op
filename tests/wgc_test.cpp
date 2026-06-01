#include "test_support.h"

#include <thread>

using test_support::ColorPulseWindow;

#ifdef OP_ENABLE_WGC

namespace {

void PumpMessagesFor(int milliseconds) {
    const auto deadline = GetTickCount64() + milliseconds;
    MSG msg = {};
    while (GetTickCount64() < deadline) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        Sleep(10);
    }
}

} // namespace

TEST(WgcTest, CapturesStaticWindowFromStart) {
    ColorPulseWindow window;
    ASSERT_TRUE(window.Create(false));

    libop op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    ret = 0;
    op.BindWindow((long)(intptr_t)window.hwnd, L"normal.wgc", L"windows", L"windows", 0, &ret);
    ASSERT_EQ(ret, 1);

    PumpMessagesFor(1000);

    std::wstring color;
    op.GetColor(60, 60, color);
    EXPECT_EQ(color, L"FF0000");

    const std::wstring capture_path = test_support::GetTempBmpPath(L"op_wgc_static_capture.bmp");
    long cap_ret = 0;
    op.Capture(0, 0, 200, 160, capture_path.c_str(), &cap_ret);
    EXPECT_EQ(cap_ret, 1);

    long unbind_ret = 0;
    op.UnBindWindow(&unbind_ret);
    EXPECT_EQ(unbind_ret, 1);
    DestroyWindow(window.hwnd);
    window.hwnd = nullptr;
    PumpMessagesFor(400);
}

// This scenario is useful for local WGC stress validation, but it is still
// flaky when multiple capture sessions are created back-to-back in the same process.
TEST(WgcTest, DISABLED_CapturesAnimatedWindowLatestFrame) {
    ColorPulseWindow window;
    ASSERT_TRUE(window.Create(true));

    libop op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    ret = 0;
    op.BindWindow((long)(intptr_t)window.hwnd, L"normal.wgc", L"windows", L"windows", 0, &ret);
    ASSERT_EQ(ret, 1);

    PumpMessagesFor(1400);

    std::wstring color;
    op.GetColor(60, 60, color);
    EXPECT_TRUE(color == L"FF0000" || color == L"00FF00" || color == L"0000FF");

    long cap_ret = 0;
    const std::wstring capture_path = test_support::GetTempBmpPath(L"op_wgc_animated_capture.bmp");
    op.Capture(0, 0, 200, 160, capture_path.c_str(), &cap_ret);
    EXPECT_EQ(cap_ret, 1);

    long unbind_ret = 0;
    op.UnBindWindow(&unbind_ret);
    EXPECT_EQ(unbind_ret, 1);
    DestroyWindow(window.hwnd);
    window.hwnd = nullptr;
    PumpMessagesFor(400);
}

#endif
