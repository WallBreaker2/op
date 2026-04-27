#include "test_support.h"

#include <chrono>
#include <iostream>
#include <thread>

using namespace std;
using test_support::MouseEventWindow;

namespace {

bool SendMouseInput(DWORD flags, DWORD mouseData = 0) {
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = flags;
    input.mi.mouseData = mouseData;
    return ::SendInput(1, &input, sizeof(INPUT)) == 1;
}

bool SendKeyboardInput(WORD vk, DWORD flags = 0) {
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.dwFlags = flags;
    return ::SendInput(1, &input, sizeof(INPUT)) == 1;
}

struct InputResetGuard {
    bool left_down = false;
    bool right_down = false;
    bool key_down = false;
    WORD key_vk = 0;

    ~InputResetGuard() {
        if (left_down)
            SendMouseInput(MOUSEEVENTF_LEFTUP);
        if (right_down)
            SendMouseInput(MOUSEEVENTF_RIGHTUP);
        if (key_down)
            SendKeyboardInput(key_vk, KEYEVENTF_KEYUP);
    }
};

} // namespace

TEST(MouseKeyTest, MoveToAndGetCursorPos) {
    libop op;
    long ret;
    op.MoveTo(100, 100, &ret);
    EXPECT_TRUE(ret) << "MoveTo should succeed";

    long x, y;
    op.GetCursorPos(&x, &y, &ret);
    cout << "Cursor Pos: " << x << "," << y << endl;
}

TEST(MouseKeyTest, ClickAndKeyPress) {
    libop op;
    long ret;
    op.SetMouseDelay(L"click", 10, &ret);
    op.LeftClick(&ret);

    op.SetKeypadDelay(L"press", 10, &ret);
    op.KeyPress(VK_ESCAPE, &ret);
}

TEST(MouseKeyTest, EscKeyMapsToEscapeNotCancel) {
    libop op;
    long ret = 0;
    InputResetGuard guard;

    ASSERT_TRUE(SendKeyboardInput(VK_ESCAPE, 0));
    guard.key_down = true;
    guard.key_vk = VK_ESCAPE;
    ::Sleep(20);
    op.GetKeyState(VK_ESCAPE, &ret);
    EXPECT_EQ(ret, 1);
}

TEST(MouseKeyTest, NumpadKeyMappings) {
    EXPECT_NE(VK_NUMPAD0, '0');
    EXPECT_NE(VK_NUMPAD1, '1');

    libop op;
    long ret = 0;
    InputResetGuard guard;

    ASSERT_TRUE(SendKeyboardInput(VK_NUMPAD5, 0));
    guard.key_down = true;
    guard.key_vk = VK_NUMPAD5;
    ::Sleep(20);
    op.GetKeyState(VK_NUMPAD5, &ret);
    EXPECT_EQ(ret, 1);
}

TEST(MouseKeyTest, GetKeyStateTracksMouseButtons) {
    libop op;
    long ret = 0;
    InputResetGuard guard;

    ASSERT_TRUE(SendMouseInput(MOUSEEVENTF_LEFTDOWN));
    guard.left_down = true;
    ::Sleep(20);
    op.GetKeyState(VK_LBUTTON, &ret);
    EXPECT_EQ(ret, 1);

    ASSERT_TRUE(SendMouseInput(MOUSEEVENTF_LEFTUP));
    guard.left_down = false;
    ::Sleep(20);
    op.GetKeyState(VK_LBUTTON, &ret);
    EXPECT_EQ(ret, 0);

    ASSERT_TRUE(SendMouseInput(MOUSEEVENTF_RIGHTDOWN));
    guard.right_down = true;
    ::Sleep(20);
    op.GetKeyState(VK_RBUTTON, &ret);
    EXPECT_EQ(ret, 1);

    ASSERT_TRUE(SendMouseInput(MOUSEEVENTF_RIGHTUP));
    guard.right_down = false;
    ::Sleep(20);
    op.GetKeyState(VK_RBUTTON, &ret);
    EXPECT_EQ(ret, 0);
}

TEST(MouseKeyTest, WaitKeyUsesNormalizedKeyStateForMouseButtons) {
    libop op;
    long ret = 0;
    InputResetGuard guard;

    std::thread worker([] {
        ::Sleep(50);
        SendMouseInput(MOUSEEVENTF_LEFTDOWN);
        ::Sleep(50);
        SendMouseInput(MOUSEEVENTF_LEFTUP);
    });

    op.WaitKey(VK_LBUTTON, 1000, &ret);
    worker.join();
    EXPECT_EQ(ret, 1);

    ::Sleep(20);
    op.GetKeyState(VK_LBUTTON, &ret);
    EXPECT_EQ(ret, 0);
}

TEST(MouseKeyTest, WaitKeyImmediateCheckReturnsQuickly) {
    libop op;
    long ret = -1;
    auto start = std::chrono::steady_clock::now();
    op.WaitKey(VK_LBUTTON, 0, &ret);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    EXPECT_LT(elapsed, 50);
    EXPECT_TRUE(ret == 0 || ret == 1);
}

TEST(MouseKeyTest, WaitKeyScanAllImmediate) {
    libop op;
    long ret = -1;
    auto start = std::chrono::steady_clock::now();
    op.WaitKey(0, 0, &ret);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    EXPECT_LT(elapsed, 50);
    EXPECT_TRUE(ret == 0 || (ret >= 1 && ret <= 254));
}

TEST(MouseKeyTest, WaitKeyScanAllWithWaitFindsKey) {
    libop op;
    long ret = 0;
    std::thread worker([] {
        ::Sleep(100);
        INPUT input = {0};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_SPACE;
        ::SendInput(1, &input, sizeof(INPUT));
        ::Sleep(50);
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        ::SendInput(1, &input, sizeof(INPUT));
    });
    op.WaitKey(0, 500, &ret);
    worker.join();
    EXPECT_TRUE(ret >= 1 && ret <= 254);
}

TEST(MouseKeyTest, WaitKeySpecificKeyZeroTimeout) {
    libop op;
    long ret = 0;
    InputResetGuard guard;

    ASSERT_TRUE(SendKeyboardInput('A', 0));
    guard.key_down = true;
    guard.key_vk = 'A';
    ::Sleep(20);
    op.WaitKey('A', 0, &ret);
    EXPECT_TRUE(ret == 0 || ret == 'A');
}

TEST(MouseKeyTest, WindowsModeMouseReturnAndWheel) {
    libop op;
    MouseEventWindow window;
    ASSERT_TRUE(window.Create());

    long ret = 0;
    op.BindWindow((long)(intptr_t)window.hwnd, L"normal", L"windows", L"windows", 0, &ret);
    ASSERT_EQ(ret, 1) << "BindWindow windows mode should succeed";

    op.MoveTo(30, 40, &ret);
    EXPECT_EQ(ret, 1);
    op.LeftClick(&ret);
    EXPECT_EQ(ret, 1);
    op.RightClick(&ret);
    EXPECT_EQ(ret, 1);
    op.WheelDown(&ret);
    EXPECT_EQ(ret, 1);
    op.WheelUp(&ret);
    EXPECT_EQ(ret, 1);

    EXPECT_GE(window.left_down, 1);
    EXPECT_GE(window.left_up, 1);
    EXPECT_GE(window.right_down, 1);
    EXPECT_GE(window.right_up, 1);
    EXPECT_GE(window.wheel_count, 2);
    EXPECT_EQ(window.wheel_delta_sum, 0);

    long unbind_ret = 0;
    op.UnBindWindow(&unbind_ret);
    EXPECT_EQ(unbind_ret, 1);
}

TEST(MouseKeyTest, DxModeWheelAndCustomMessages) {
    libop op;
    MouseEventWindow window;
    ASSERT_TRUE(window.Create());

    long ret = 0;
    op.BindWindow((long)(intptr_t)window.hwnd, L"normal", L"dx", L"windows", 0, &ret);
    if (ret != 1) {
        GTEST_SKIP() << "DX mouse bind unavailable on current environment";
    }

    op.MoveTo(18, 26, &ret);
    EXPECT_EQ(ret, 1);
    op.LeftClick(&ret);
    EXPECT_EQ(ret, 1);
    op.RightClick(&ret);
    EXPECT_EQ(ret, 1);
    op.WheelDown(&ret);
    EXPECT_EQ(ret, 1);
    op.WheelUp(&ret);
    EXPECT_EQ(ret, 1);

    EXPECT_GE(window.op_left_down, 1);
    EXPECT_GE(window.op_left_up, 1);
    EXPECT_GE(window.op_right_down, 1);
    EXPECT_GE(window.op_right_up, 1);
    EXPECT_GE(window.op_wheel_count, 2);
    EXPECT_EQ(window.op_wheel_delta_sum, 0);

    long unbind_ret = 0;
    op.UnBindWindow(&unbind_ret);
    EXPECT_EQ(unbind_ret, 1);
}