#include "test_support.h"

#include <iostream>

using namespace std;
using test_support::MouseEventWindow;

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
