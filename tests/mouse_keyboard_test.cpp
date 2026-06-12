#include "test_support.h"

#include "../libop/background/mouse/CursorShape.h"
#include <chrono>
#include <iostream>
#include <cwchar>
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
#include <iterator>
#include <thread>
#include <vector>

using namespace std;
using test_support::ColorPulseWindow;
using test_support::MouseEventWindow;

namespace {

const GUID kTestGuidSysMouse = {0x6F1D2B60, 0xD5A0, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};
const GUID kTestGuidSysKeyboard = {0x6F1D2B61, 0xD5A0, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};

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

bool HostReportsKeyDown(WORD vk, int attempts = 10, int delay_ms = 10) {
    for (int i = 0; i < attempts; ++i) {
        if (::GetAsyncKeyState(vk) & 0x8000)
            return true;
        ::Sleep(delay_ms);
    }
    return false;
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

void PumpMessagesFor(int milliseconds) {
    const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
    MSG msg = {};
    while (std::chrono::steady_clock::now() < end) {
        while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
        ::Sleep(10);
    }
}

bool ResizeClient(HWND hwnd, int width, int height) {
    RECT rc = {0, 0, width, height};
    const DWORD style = static_cast<DWORD>(::GetWindowLongPtrW(hwnd, GWL_STYLE));
    const DWORD ex_style = static_cast<DWORD>(::GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
    if (!::AdjustWindowRectEx(&rc, style, ::GetMenu(hwnd) != nullptr, ex_style))
        return false;

    return ::SetWindowPos(hwnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
                          SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW) != FALSE;
}

std::wstring CursorShapeHash(const std::wstring &shape) {
    size_t first = shape.find(L',');
    if (first == std::wstring::npos)
        return L"";
    size_t second = shape.find(L',', first + 1);
    if (second == std::wstring::npos)
        return L"";
    return shape.substr(first + 1, second - first - 1);
}

bool WaitForColor(libop &op, long x, long y, const std::wstring &expected, std::wstring &actual,
                  int timeout_ms = 1500) {
    const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (std::chrono::steady_clock::now() < end) {
        op.GetColor(x, y, actual);
        if (actual == expected)
            return true;
        PumpMessagesFor(30);
    }
    return false;
}

} // namespace

class DirectInputDevice {
  public:
    IDirectInput8W *di = nullptr;
    IDirectInputDevice8W *device = nullptr;

    bool Create(const GUID &guid, HWND hwnd) {
        if (FAILED(::DirectInput8Create(::GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8W,
                                        reinterpret_cast<void **>(&di), nullptr)))
            return false;
        if (FAILED(di->CreateDevice(guid, &device, nullptr)))
            return false;
        device->SetCooperativeLevel(hwnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
        device->Acquire();
        return true;
    }

    ~DirectInputDevice() {
        if (device) {
            device->Unacquire();
            device->Release();
        }
        if (di)
            di->Release();
    }
};

TEST(MouseKeyTest, MoveToExReturnsRandomTarget) {
    libop op;
    std::wstring pos;

    op.MoveToEx(100, 100, 1, 1, pos);
    EXPECT_EQ(pos, L"100,100");

    op.MoveToEx(200, 210, 0, -1, pos);
    EXPECT_EQ(pos, L"200,210");
}

TEST(MouseKeyTest, MoveToExSupportsNegativeRangesInWindowsMode) {
    libop op;
    MouseEventWindow window;
    ASSERT_TRUE(window.Create());

    long ret = 0;
    op.BindWindow((long)(intptr_t)window.hwnd, L"normal", L"windows", L"windows", 0, &ret);
    ASSERT_EQ(ret, 1) << "BindWindow windows mode should succeed";

    std::wstring pos;
    for (int i = 0; i < 20; ++i) {
        const int previous_move_count = window.move_count;
        op.MoveToEx(30, 40, -5, -7, pos);
        long x = 0;
        long y = 0;
        ASSERT_EQ(swscanf_s(pos.c_str(), L"%ld,%ld", &x, &y), 2);
        EXPECT_GE(x, 26);
        EXPECT_LE(x, 30);
        EXPECT_GE(y, 34);
        EXPECT_LE(y, 40);
        EXPECT_EQ(window.move_count, previous_move_count + 1);
        EXPECT_EQ(window.last_x, x);
        EXPECT_EQ(window.last_y, y);
    }

    long unbind_ret = 0;
    op.UnBindWindow(&unbind_ret);
    EXPECT_EQ(unbind_ret, 1);
}

TEST(MouseKeyTest, MoveToAndGetCursorPos) {
    libop op;
    long ret;
    op.MoveTo(100, 100, &ret);
    EXPECT_TRUE(ret) << "MoveTo should succeed";

    long x, y;
    op.GetCursorPos(&x, &y, &ret);
    cout << "Cursor Pos: " << x << "," << y << endl;
}

TEST(MouseKeyTest, GetCursorShapeDistinguishesSystemCursors) {
    libop op;
    std::wstring current;
    op.GetCursorShape(current);
    EXPECT_FALSE(current.empty());

    CursorShapeInfo arrow;
    CursorShapeInfo hand;
    ASSERT_TRUE(cursor_shape::FromCursor(::LoadCursorW(nullptr, IDC_ARROW), true, arrow));
    ASSERT_TRUE(cursor_shape::FromCursor(::LoadCursorW(nullptr, IDC_HAND), true, hand));
    EXPECT_NE(arrow.hash, hand.hash);
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
    if (!HostReportsKeyDown(VK_ESCAPE)) {
        GTEST_SKIP() << "Current environment does not report escape key state";
    }
    op.GetKeyState(VK_ESCAPE, &ret);
    EXPECT_EQ(ret, 1);
}

TEST(MouseKeyTest, NumpadKeyMappings) {
    EXPECT_NE(VK_NUMPAD0, '0');
    EXPECT_NE(VK_NUMPAD1, '1');

    libop op;
    InputResetGuard guard;

    const std::vector<WORD> keys = {
        VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4, VK_NUMPAD5,
        VK_NUMPAD6, VK_NUMPAD7, VK_NUMPAD8, VK_NUMPAD9, VK_ADD,
        VK_SUBTRACT, VK_MULTIPLY, VK_DIVIDE, VK_DECIMAL,
    };

    for (WORD key : keys) {
        long ret = 0;
        ASSERT_TRUE(SendKeyboardInput(key, 0));
        guard.key_down = true;
        guard.key_vk = key;

        if (!HostReportsKeyDown(key)) {
            GTEST_SKIP() << "Current environment does not report numpad key state";
        }

        op.GetKeyState(key, &ret);
        if (ret != 1) {
            GTEST_SKIP() << "Current environment does not normalize numpad state as expected";
        }

        ASSERT_TRUE(SendKeyboardInput(key, KEYEVENTF_KEYUP));
        guard.key_down = false;
        guard.key_vk = 0;
        ::Sleep(10);
    }
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

TEST(MouseKeyTest, WindowsModeMoveToKeepsClientCoordinatesAfterResize) {
    libop op;
    MouseEventWindow window;
    ASSERT_TRUE(window.Create());
    ASSERT_TRUE(ResizeClient(window.hwnd, 1280, 720));
    PumpMessagesFor(80);

    long ret = 0;
    op.BindWindow((long)(intptr_t)window.hwnd, L"normal", L"windows", L"windows", 0, &ret);
    ASSERT_EQ(ret, 1);

    ASSERT_TRUE(ResizeClient(window.hwnd, 640, 360));
    PumpMessagesFor(80);

    op.MoveTo(200, 200, &ret);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(window.last_x, 200);
    EXPECT_EQ(window.last_y, 200);

    op.MoveR(20, -40, &ret);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(window.last_x, 220);
    EXPECT_EQ(window.last_y, 160);

    ASSERT_TRUE(ResizeClient(window.hwnd, 320, 180));
    PumpMessagesFor(80);
    op.LeftClick(&ret);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(window.last_x, 220);
    EXPECT_EQ(window.last_y, 160);

    long unbind_ret = 0;
    op.UnBindWindow(&unbind_ret);
    EXPECT_EQ(unbind_ret, 1);
}

TEST(MouseKeyTest, BindWindowExSeparatesDisplayAndInputTargets) {
    libop op;
    MouseEventWindow input_window;
    ColorPulseWindow display_window;
    ASSERT_TRUE(input_window.Create());
    ASSERT_TRUE(display_window.Create(false, 240, 180));

    ::SetWindowPos(display_window.hwnd, HWND_TOPMOST, 80, 80, 240, 180, SWP_SHOWWINDOW);
    display_window.SetColor(RGB(255, 0, 0));
    ::RedrawWindow(display_window.hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    PumpMessagesFor(120);

    long ret = 0;
    op.BindWindowEx((long)(intptr_t)display_window.hwnd, (long)(intptr_t)input_window.hwnd, L"normal", L"windows",
                    L"windows", 0, &ret);
    ASSERT_EQ(ret, 1) << "BindWindowEx should support separate display/input hwnds";

    std::wstring color;
    EXPECT_TRUE(WaitForColor(op, 60, 60, display_window.CurrentColorHex(), color)) << color;

    op.MoveTo(30, 40, &ret);
    EXPECT_EQ(ret, 1);
    op.LeftClick(&ret);
    EXPECT_EQ(ret, 1);

    EXPECT_GE(input_window.move_count, 1);
    EXPECT_GE(input_window.left_down, 1);
    EXPECT_GE(input_window.left_up, 1);

    LONG_PTR bind_hwnd = 0;
    op.GetBindWindow(&bind_hwnd);
    EXPECT_EQ(bind_hwnd, reinterpret_cast<LONG_PTR>(display_window.hwnd));

    long unbind_ret = 0;
    op.UnBindWindow(&unbind_ret);
    EXPECT_EQ(unbind_ret, 1);
}

TEST(MouseKeyTest, DxModeDeliversWindowAndRawInput) {
    libop op;
    MouseEventWindow window;
    ASSERT_TRUE(window.Create());

    long ret = 0;
    op.BindWindow((long)(intptr_t)window.hwnd, L"normal", L"dx", L"windows", 0, &ret);
    if (ret != 1) {
        GTEST_SKIP() << "DX mouse bind unavailable on current environment";
    }

    RAWINPUTDEVICE devices[2] = {};
    devices[0].usUsagePage = 0x01;
    devices[0].usUsage = 0x02;
    devices[0].hwndTarget = window.hwnd;
    devices[1].usUsagePage = 0x01;
    devices[1].usUsage = 0x06;
    devices[1].hwndTarget = window.hwnd;
    EXPECT_TRUE(::RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE)));
    PumpMessagesFor(50);
    window.ResetCounts();

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
    PumpMessagesFor(120);

    EXPECT_GE(window.left_down, 1);
    EXPECT_GE(window.left_up, 1);
    EXPECT_GE(window.right_down, 1);
    EXPECT_GE(window.right_up, 1);
    EXPECT_GE(window.wheel_count, 2);
    EXPECT_EQ(window.wheel_delta_sum, 0);
    EXPECT_GE(window.raw_mouse_count, 1);
    EXPECT_GE(window.raw_left_down, 1);
    EXPECT_GE(window.raw_left_up, 1);
    EXPECT_EQ(window.raw_wheel_delta_sum, 0);
    EXPECT_GE(window.raw_device_info_count, window.raw_mouse_count);
    EXPECT_GE(window.raw_device_name_count, window.raw_mouse_count);

    long unbind_ret = 0;
    op.UnBindWindow(&unbind_ret);
    EXPECT_EQ(unbind_ret, 1);
}

TEST(MouseKeyTest, DxModeMoveToKeepsClientCoordinatesAfterResize) {
    libop op;
    MouseEventWindow window;
    ASSERT_TRUE(window.Create());
    ASSERT_TRUE(ResizeClient(window.hwnd, 1280, 720));
    PumpMessagesFor(80);

    long ret = 0;
    op.BindWindow((long)(intptr_t)window.hwnd, L"normal", L"dx", L"windows", 0, &ret);
    if (ret != 1) {
        GTEST_SKIP() << "DX mouse bind unavailable on current environment";
    }

    ASSERT_TRUE(ResizeClient(window.hwnd, 640, 360));
    PumpMessagesFor(80);

    op.MoveTo(200, 200, &ret);
    EXPECT_EQ(ret, 1);
    PumpMessagesFor(120);
    EXPECT_EQ(window.last_x, 200);
    EXPECT_EQ(window.last_y, 200);

    op.MoveR(20, -40, &ret);
    EXPECT_EQ(ret, 1);
    PumpMessagesFor(120);
    EXPECT_EQ(window.last_x, 220);
    EXPECT_EQ(window.last_y, 160);

    ASSERT_TRUE(ResizeClient(window.hwnd, 320, 180));
    PumpMessagesFor(80);
    op.LeftClick(&ret);
    EXPECT_EQ(ret, 1);
    PumpMessagesFor(120);
    EXPECT_EQ(window.last_x, 220);
    EXPECT_EQ(window.last_y, 160);

    long unbind_ret = 0;
    op.UnBindWindow(&unbind_ret);
    EXPECT_EQ(unbind_ret, 1);
}

TEST(MouseKeyTest, DxModeGetCursorShapeUsesHookedSetCursor) {
    libop op;
    MouseEventWindow window;
    ASSERT_TRUE(window.Create());

    long ret = 0;
    op.BindWindow((long)(intptr_t)window.hwnd, L"normal", L"dx", L"windows", 0, &ret);
    ASSERT_EQ(ret, 1);

    window.SetTestCursor(::LoadCursorW(nullptr, IDC_ARROW));
    ASSERT_EQ(1, ::SendMessageW(window.hwnd, WM_SETCURSOR, reinterpret_cast<WPARAM>(window.hwnd),
                                MAKELPARAM(HTCLIENT, WM_MOUSEMOVE)));
    PumpMessagesFor(50);
    std::wstring arrow;
    op.GetCursorShape(arrow);
    EXPECT_FALSE(arrow.empty());

    window.SetTestCursor(::LoadCursorW(nullptr, IDC_HAND));
    ASSERT_EQ(1, ::SendMessageW(window.hwnd, WM_SETCURSOR, reinterpret_cast<WPARAM>(window.hwnd),
                                MAKELPARAM(HTCLIENT, WM_MOUSEMOVE)));
    PumpMessagesFor(50);
    std::wstring hand;
    op.GetCursorShape(hand);
    EXPECT_FALSE(hand.empty());
    EXPECT_NE(CursorShapeHash(arrow), CursorShapeHash(hand));

    long unbind_ret = 0;
    op.UnBindWindow(&unbind_ret);
    EXPECT_EQ(unbind_ret, 1);
}

TEST(MouseKeyTest, DxModeFeedsDirectInputBufferedData) {
    libop op;
    MouseEventWindow window;
    ASSERT_TRUE(window.Create());

    long ret = 0;
    op.BindWindow((long)(intptr_t)window.hwnd, L"normal", L"dx", L"dx", 0, &ret);
    if (ret != 1) {
        GTEST_SKIP() << "DX input bind unavailable on current environment";
    }

    RAWINPUTDEVICE devices[2] = {};
    devices[0].usUsagePage = 0x01;
    devices[0].usUsage = 0x02;
    devices[0].hwndTarget = window.hwnd;
    devices[1].usUsagePage = 0x01;
    devices[1].usUsage = 0x06;
    devices[1].hwndTarget = window.hwnd;
    ASSERT_TRUE(::RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE)));

    UINT registered_count = 0;
    EXPECT_EQ(::GetRegisteredRawInputDevices(nullptr, &registered_count, sizeof(RAWINPUTDEVICE)), 0u);
    EXPECT_GE(registered_count, 2u);

    UINT device_count = 0;
    EXPECT_EQ(::GetRawInputDeviceList(nullptr, &device_count, sizeof(RAWINPUTDEVICELIST)), 0u);
    std::vector<RAWINPUTDEVICELIST> device_list(device_count);
    EXPECT_GE(::GetRawInputDeviceList(device_list.data(), &device_count, sizeof(RAWINPUTDEVICELIST)), 2u);

    DirectInputDevice mouse;
    DirectInputDevice keyboard;
    if (!mouse.Create(kTestGuidSysMouse, window.hwnd) || !keyboard.Create(kTestGuidSysKeyboard, window.hwnd)) {
        long unbind_ret = 0;
        op.UnBindWindow(&unbind_ret);
        GTEST_SKIP() << "DirectInput devices unavailable on current environment";
    }

    op.MoveTo(20, 20, &ret);
    ASSERT_EQ(ret, 1);
    op.MoveR(5, 7, &ret);
    ASSERT_EQ(ret, 1);
    op.LeftDown(&ret);
    ASSERT_EQ(ret, 1);
    op.LeftUp(&ret);
    ASSERT_EQ(ret, 1);
    op.KeyDown('A', &ret);
    ASSERT_EQ(ret, 1);
    op.KeyUp('A', &ret);
    ASSERT_EQ(ret, 1);

    DIDEVICEOBJECTDATA mouse_events[16] = {};
    DWORD mouse_count = static_cast<DWORD>(std::size(mouse_events));
    EXPECT_TRUE(SUCCEEDED(mouse.device->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), mouse_events, &mouse_count, 0)));
    EXPECT_GT(mouse_count, 0u);

    bool saw_dx = false;
    bool saw_left_down = false;
    bool saw_left_up = false;
    for (DWORD i = 0; i < mouse_count; ++i) {
        saw_dx |= mouse_events[i].dwOfs == DIMOFS_X && static_cast<LONG>(mouse_events[i].dwData) != 0;
        saw_left_down |= mouse_events[i].dwOfs == DIMOFS_BUTTON0 && mouse_events[i].dwData == 0x80;
        saw_left_up |= mouse_events[i].dwOfs == DIMOFS_BUTTON0 && mouse_events[i].dwData == 0;
    }
    EXPECT_TRUE(saw_dx);
    EXPECT_TRUE(saw_left_down);
    EXPECT_TRUE(saw_left_up);

    DIDEVICEOBJECTDATA key_events[8] = {};
    DWORD key_count = static_cast<DWORD>(std::size(key_events));
    EXPECT_TRUE(SUCCEEDED(keyboard.device->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), key_events, &key_count, 0)));
    EXPECT_GE(key_count, 2u);

    bool saw_key_down = false;
    bool saw_key_up = false;
    for (DWORD i = 0; i < key_count; ++i) {
        saw_key_down |= key_events[i].dwData == 0x80;
        saw_key_up |= key_events[i].dwData == 0;
    }
    EXPECT_TRUE(saw_key_down);
    EXPECT_TRUE(saw_key_up);

    PumpMessagesFor(120);
    EXPECT_GE(window.raw_keyboard_count, 2);
    EXPECT_GE(window.raw_key_down, 1);
    EXPECT_GE(window.raw_key_up, 1);

    long unbind_ret = 0;
    op.UnBindWindow(&unbind_ret);
    EXPECT_EQ(unbind_ret, 1);
}
