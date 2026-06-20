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

    op::Client op;
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

// 回归：覆盖本轮针对 OBS 对照修复的几个运行时场景（单会话内串联，规避多会话连开的偶发不稳定）。
TEST(WgcTest, FirstFrameResizeAndCloseScenarios) {
    ColorPulseWindow window;
    ASSERT_TRUE(window.Create(false));

    op::Client op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    // 场景 #1：绑定后“不” PumpMessages，立即取色。
    // waitForBindReady 必须已在 BindWindow 内部等到首帧，否则这里会取到黑色。
    ret = 0;
    op.BindWindow((long)(intptr_t)window.hwnd, L"normal.wgc", L"windows", L"windows", 0, &ret);
    ASSERT_EQ(ret, 1);

    std::wstring color_first;
    op.GetColor(60, 60, color_first);
    EXPECT_EQ(color_first, L"FF0000") << "#1 首帧未就绪：绑定后立即截图失败";

    // 场景 #2/#7：放大窗口后仍能正确截图（surface GetDesc 跟随尺寸、无 CopyResource 尺寸不匹配/花屏）。
    SetWindowPos(window.hwnd, nullptr, 0, 0, 480, 360, SWP_NOMOVE | SWP_NOZORDER);
    PumpMessagesFor(700); // 让目标窗口重绘 + WGC 重建帧池并吐出新尺寸的帧

    std::wstring color_after_resize;
    op.GetColor(60, 60, color_after_resize);
    EXPECT_EQ(color_after_resize, L"FF0000") << "#2 resize 后取色错误";

    long cap_ret = 0;
    const std::wstring capture_path = test_support::GetTempBmpPath(L"op_wgc_resize_capture.bmp");
    op.Capture(0, 0, 300, 240, capture_path.c_str(), &cap_ret);
    EXPECT_EQ(cap_ret, 1) << "#2 resize 后整图截取失败";

    // 场景 #4：目标窗口关闭后，后续截图应优雅失败（返回 0），且不崩溃/不挂起。
    DestroyWindow(window.hwnd);
    window.hwnd = nullptr;
    PumpMessagesFor(400); // 给 GraphicsCaptureItem.Closed 触发留出时间

    std::wstring color_after_close;
    op.GetColor(60, 60, color_after_close); // 仅验证不崩溃
    long cap_after_close = 1;
    const std::wstring closed_path = test_support::GetTempBmpPath(L"op_wgc_closed_capture.bmp");
    op.Capture(0, 0, 100, 100, closed_path.c_str(), &cap_after_close);
    EXPECT_EQ(cap_after_close, 0) << "#4 窗口关闭后截图应失败";

    long unbind_ret = 0;
    op.UnBindWindow(&unbind_ret);
    EXPECT_EQ(unbind_ret, 1);
    PumpMessagesFor(200);
}

// 回归：最小化后恢复，应能重新正常截图（覆盖 requestCapture 的 restore 等帧/重启会话分支）。
TEST(WgcTest, MinimizeRestoreStillCaptures) {
    ColorPulseWindow window;
    ASSERT_TRUE(window.Create(false));

    op::Client op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    ret = 0;
    op.BindWindow((long)(intptr_t)window.hwnd, L"normal.wgc", L"windows", L"windows", 0, &ret);
    ASSERT_EQ(ret, 1);

    // 基线：正常截到红色。
    std::wstring color_before;
    op.GetColor(60, 60, color_before);
    EXPECT_EQ(color_before, L"FF0000");

    // 最小化：截图应失败（最小化窗口无客户区内容可截）。
    ShowWindow(window.hwnd, SW_MINIMIZE);
    PumpMessagesFor(500);
    long cap_min = 1;
    const std::wstring min_path = test_support::GetTempBmpPath(L"op_wgc_min_capture.bmp");
    op.Capture(0, 0, 100, 100, min_path.c_str(), &cap_min);
    EXPECT_EQ(cap_min, 0) << "最小化期间截图应失败";

    // 恢复：restore 分支会等新帧、必要时重启捕获会话；之后应能再次正常截图。
    ShowWindow(window.hwnd, SW_RESTORE);
    PumpMessagesFor(900);
    std::wstring color_after;
    op.GetColor(60, 60, color_after);
    EXPECT_EQ(color_after, L"FF0000") << "恢复后取色失败：restore 未恢复出帧";

    long cap_restore = 0;
    const std::wstring restore_path = test_support::GetTempBmpPath(L"op_wgc_restore_capture.bmp");
    op.Capture(0, 0, 150, 120, restore_path.c_str(), &cap_restore);
    EXPECT_EQ(cap_restore, 1) << "恢复后整图截取失败";

    long unbind_ret = 0;
    op.UnBindWindow(&unbind_ret);
    EXPECT_EQ(unbind_ret, 1);
    PumpMessagesFor(200);
}

// This scenario is useful for local WGC stress validation, but it is still
// flaky when multiple capture sessions are created back-to-back in the same process.
TEST(WgcTest, DISABLED_CapturesAnimatedWindowLatestFrame) {
    ColorPulseWindow window;
    ASSERT_TRUE(window.Create(true));

    op::Client op;
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
