// #include "stdafx.h"
#include "WinMouse.h"
#include "CursorShape.h"
#include "../../runtime/AutomationModes.h"
#include "../../runtime/RuntimeUtils.h"

namespace {
long send_message_result(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    return ::SendMessageTimeout(hwnd, message, wparam, lparam, SMTO_BLOCK, 2000, nullptr) ? 1L : 0L;
}
}

namespace op::input {

WinMouse::WinMouse() : _hwnd(NULL), _mode(0), _x(0), _y(0) {
}

WinMouse::~WinMouse() {
    _hwnd = NULL;
}

long WinMouse::Bind(HWND h, int mode) {
    _hwnd = h;
    _mode = mode;
    _x = _y = 0;
    return 1;
}

long WinMouse::UnBind() {
    _hwnd = 0;
    _mode = 0;
    _x = _y = 0;
    return 1;
}

long WinMouse::GetCursorPos(long &x, long &y) {
    BOOL ret = FALSE;
    POINT pt;
    ret = ::GetCursorPos(&pt);
    if (_hwnd && _hwnd != ::GetDesktopWindow()) {
        ret = ::ScreenToClient(_hwnd, &pt);
    }
    x = pt.x;
    y = pt.y;
    return ret;
}

long WinMouse::GetCursorShape(std::wstring &ret) {
    CursorShapeInfo info;
    if (!cursor_shape::FromSystem(info)) {
        ret.clear();
        return 0;
    }

    ret = cursor_shape::Format(info);
    return 1;
}

long WinMouse::MoveR(int rx, int ry) {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        // https://learn.microsoft.com/zh-cn/windows/win32/api/winuser/ns-winuser-mouseinput
        _x += rx;
        _y += ry;

        INPUT Input = {0};
        Input.type = INPUT_MOUSE;
        Input.mi.dwFlags = MOUSEEVENTF_MOVE;
        Input.mi.dx = static_cast<LONG>(rx);
        Input.mi.dy = static_cast<LONG>(ry);
        return ::SendInput(1, &Input, sizeof(INPUT)) > 0 ? 1 : 0;
    }
    }
    return MoveTo(_x + rx, _y + ry);
}

long WinMouse::MoveTo(int x, int y) {
    long ret = 0;
    POINT client_pt{x, y};
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        POINT pt = client_pt;
        if (_hwnd)
            ::ClientToScreen(_hwnd, &pt);

        const int screen_x = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
        const int screen_y = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
        const int screen_width = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
        const int screen_height = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
        const double width = screen_width > 1 ? static_cast<double>(screen_width - 1) : 1.0;
        const double height = screen_height > 1 ? static_cast<double>(screen_height - 1) : 1.0;
        const double fx = (pt.x - screen_x) * (65535.0 / width);
        const double fy = (pt.y - screen_y) * (65535.0 / height);

        INPUT Input = {0};
        Input.type = INPUT_MOUSE;
        Input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
        Input.mi.dx = static_cast<LONG>(fx);
        Input.mi.dy = static_cast<LONG>(fy);
        ret = ::SendInput(1, &Input, sizeof(INPUT)) > 0 ? 1 : 0;
        break;
    }
    case INPUT_TYPE::IN_WINDOWS: {
        ret = send_message_result(_hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(client_pt.x, client_pt.y));
        break;
    }
    }
    _x = client_pt.x, _y = client_pt.y;
    return ret;
}

POINT WinMouse::current_client_point() const {
    return POINT{_x, _y};
}

long WinMouse::sync_system_cursor() {
    POINT pt = current_client_point();
    if (_hwnd)
        ::ClientToScreen(_hwnd, &pt);
    return ::SetCursorPos(pt.x, pt.y) ? 1 : 0;
}

long WinMouse::MoveToEx(int x, int y, int w, int h, int &dst_x, int &dst_y) {
    auto random_offset = [](int value) {
        if (value == 0)
            return 0;
        const int span = std::abs(value);
        const int offset = rand() % span;
        return value > 0 ? offset : -offset;
    };

    dst_x = x + random_offset(w);
    dst_y = y + random_offset(h);
    return MoveTo(dst_x, dst_y);
}

long WinMouse::LeftClick() {
    long ret = 0, ret2 = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        if (!sync_system_cursor())
            return 0;
        INPUT Input = {0};
        Input.type = INPUT_MOUSE;
        Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        ret = ::SendInput(1, &Input, sizeof(INPUT));
        ::Delay(MOUSE_NORMAL_DELAY);
        ::ZeroMemory(&Input, sizeof(INPUT));
        Input.type = INPUT_MOUSE;
        Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        ret2 = ::SendInput(1, &Input, sizeof(INPUT));
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        const POINT pt = current_client_point();
        ret = send_message_result(_hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(pt.x, pt.y));
        ::Delay(MOUSE_WINDOWS_DELAY);
        ret2 = send_message_result(_hwnd, WM_LBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
        break;
    }
    }
    return ret && ret2 ? 1 : 0;
}

long WinMouse::LeftDoubleClick() {
    long r1, r2;
    r1 = LeftClick();
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        ::Delay(MOUSE_NORMAL_DELAY);
        break;
    }
    case INPUT_TYPE::IN_WINDOWS: {
        ::Delay(MOUSE_WINDOWS_DELAY);
        break;
    }
    }
    r2 = LeftClick();
    return r1 && r2 ? 1 : 0;
}

long WinMouse::LeftDown() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        if (!sync_system_cursor())
            return 0;
        INPUT Input = {0};
        Input.type = INPUT_MOUSE;
        Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        ret = ::SendInput(1, &Input, sizeof(INPUT));
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        const POINT pt = current_client_point();
        ret = send_message_result(_hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(pt.x, pt.y));
        break;
    }
    }
    return ret;
}

long WinMouse::LeftUp() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        if (!sync_system_cursor())
            return 0;
        INPUT Input = {0};
        ::ZeroMemory(&Input, sizeof(INPUT));
        Input.type = INPUT_MOUSE;
        Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        ret = ::SendInput(1, &Input, sizeof(INPUT));
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        const POINT pt = current_client_point();
        ret = send_message_result(_hwnd, WM_LBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
        break;
    }
    }
    return ret;
}

long WinMouse::MiddleClick() {
    long r1, r2;
    r1 = MiddleDown();
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        ::Delay(MOUSE_NORMAL_DELAY);
        break;
    }
    case INPUT_TYPE::IN_WINDOWS: {
        ::Delay(MOUSE_WINDOWS_DELAY);
        break;
    }
    }
    r2 = MiddleUp();
    return r1 && r2 ? 1 : 0;
}

long WinMouse::MiddleDown() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        if (!sync_system_cursor())
            return 0;
        INPUT Input = {0};
        Input.type = INPUT_MOUSE;
        Input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
        ret = ::SendInput(1, &Input, sizeof(INPUT));
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        const POINT pt = current_client_point();
        ret = send_message_result(_hwnd, WM_MBUTTONDOWN, MK_MBUTTON, MAKELPARAM(pt.x, pt.y));
        break;
    }
    }
    return ret;
}

long WinMouse::MiddleUp() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        if (!sync_system_cursor())
            return 0;
        INPUT Input = {0};
        ::ZeroMemory(&Input, sizeof(INPUT));
        Input.type = INPUT_MOUSE;
        Input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
        ret = ::SendInput(1, &Input, sizeof(INPUT));
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        const POINT pt = current_client_point();
        ret = send_message_result(_hwnd, WM_MBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
        break;
    }
    }
    return ret;
}

long WinMouse::RightClick() {
    long ret = 0;
    long r1, r2;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        if (!sync_system_cursor())
            return 0;
        INPUT Input = {0};
        Input.type = INPUT_MOUSE;
        Input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        r1 = ::SendInput(1, &Input, sizeof(INPUT));
        ::Delay(MOUSE_NORMAL_DELAY);
        ::ZeroMemory(&Input, sizeof(INPUT));
        Input.type = INPUT_MOUSE;
        Input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        r2 = ::SendInput(1, &Input, sizeof(INPUT));
        ret = r1 > 0 && r2 > 0 ? 1 : 0;
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        const POINT pt = current_client_point();
        r1 = send_message_result(_hwnd, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(pt.x, pt.y));
        ::Delay(MOUSE_WINDOWS_DELAY);
        r2 = send_message_result(_hwnd, WM_RBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
        ret = r1 && r2 ? 1 : 0;
        break;
    }
    }
    return ret;
}

long WinMouse::RightDown() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        if (!sync_system_cursor())
            return 0;
        INPUT Input = {0};
        Input.type = INPUT_MOUSE;
        Input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        ret = ::SendInput(1, &Input, sizeof(INPUT)) > 0 ? 1 : 0;
        break;
    }
    case INPUT_TYPE::IN_WINDOWS: {
        const POINT pt = current_client_point();
        ret = send_message_result(_hwnd, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(pt.x, pt.y));
        break;
    }
    }
    return ret;
}

long WinMouse::RightUp() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        if (!sync_system_cursor())
            return 0;
        INPUT Input = {0};
        ::ZeroMemory(&Input, sizeof(INPUT));
        Input.type = INPUT_MOUSE;
        Input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        ret = ::SendInput(1, &Input, sizeof(INPUT)) > 0 ? 1 : 0;
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        const POINT pt = current_client_point();
        ret = send_message_result(_hwnd, WM_RBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
        break;
    }
    }
    return ret;
}

long WinMouse::WheelDown() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        if (!sync_system_cursor())
            return 0;
        INPUT Input = {0};
        Input.type = INPUT_MOUSE;
        Input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        Input.mi.mouseData = -WHEEL_DELTA;
        ret = ::SendInput(1, &Input, sizeof(INPUT)) > 0 ? 1 : 0;
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        // WM_MOUSEWHEEL 的 lParam 使用屏幕坐标。
        POINT pt = current_client_point();
        ::ClientToScreen(_hwnd, &pt);
        ret = send_message_result(_hwnd, WM_MOUSEWHEEL, MAKEWPARAM(0, -WHEEL_DELTA), MAKELPARAM(pt.x, pt.y));
        break;
    }
    }

    return ret;
}

long WinMouse::WheelUp() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        if (!sync_system_cursor())
            return 0;
        INPUT Input = {0};
        Input.type = INPUT_MOUSE;
        Input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        Input.mi.mouseData = WHEEL_DELTA;
        ret = ::SendInput(1, &Input, sizeof(INPUT)) > 0 ? 1 : 0;
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        // WM_MOUSEWHEEL 的 lParam 使用屏幕坐标。
        POINT pt = current_client_point();
        ::ClientToScreen(_hwnd, &pt);
        ret = send_message_result(_hwnd, WM_MOUSEWHEEL, MAKEWPARAM(0, WHEEL_DELTA), MAKELPARAM(pt.x, pt.y));
        break;
    }
    }
    return ret;
}

} // namespace op::input
