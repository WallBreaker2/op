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

WinMouse::WinMouse() : _hwnd(NULL), _mode(0), _x(0), _y(0), _button_state(0) {
}

WinMouse::~WinMouse() {
    _hwnd = NULL;
}

long WinMouse::Bind(HWND h, int mode) {
    _hwnd = h;
    _mode = mode;
    _x = _y = 0;
    _button_state = 0;
    return 1;
}

long WinMouse::UnBind() {
    _hwnd = 0;
    _mode = 0;
    _x = _y = 0;
    _button_state = 0;
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
        ret = send_message_result(_hwnd, WM_MOUSEMOVE, button_state(), MAKELPARAM(client_pt.x, client_pt.y));
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

WPARAM WinMouse::button_state() const {
    return _button_state;
}

WPARAM WinMouse::button_state_with(WPARAM button, bool down) const {
    return down ? (_button_state | button) : (_button_state & ~button);
}

void WinMouse::set_button_state(WPARAM button, bool down) {
    _button_state = button_state_with(button, down);
}

long WinMouse::send_input_mouse(DWORD flags, DWORD mouse_data, bool sync_cursor) {
    if (sync_cursor && !sync_system_cursor())
        return 0;

    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = flags;
    input.mi.mouseData = mouse_data;
    return ::SendInput(1, &input, sizeof(INPUT)) > 0 ? 1 : 0;
}

long WinMouse::send_input_click(DWORD down_flags, DWORD up_flags, DWORD mouse_data, long delay) {
    if (!sync_system_cursor())
        return 0;

    const long r1 = send_input_mouse(down_flags, mouse_data, false);
    ::Delay(delay);
    const long r2 = send_input_mouse(up_flags, mouse_data, false);
    return r1 && r2 ? 1 : 0;
}

long WinMouse::send_windows_button(UINT message, WPARAM button, bool down) {
    const POINT pt = current_client_point();
    const WPARAM state = button_state_with(button, down);
    const long ret = send_message_result(_hwnd, message, state, MAKELPARAM(pt.x, pt.y));
    if (ret)
        set_button_state(button, down);
    return ret;
}

long WinMouse::send_windows_xbutton(UINT message, WORD xbutton, WPARAM button, bool down) {
    const POINT pt = current_client_point();
    const WPARAM state = button_state_with(button, down);
    const long ret = send_message_result(_hwnd, message, MAKEWPARAM(static_cast<WORD>(state), xbutton),
                                         MAKELPARAM(pt.x, pt.y));
    if (ret)
        set_button_state(button, down);
    return ret;
}

long WinMouse::button_click(long (WinMouse::*down)(), long (WinMouse::*up)(), long delay) {
    const long r1 = (this->*down)();
    ::Delay(delay);
    const long r2 = (this->*up)();
    return r1 && r2 ? 1 : 0;
}

long WinMouse::normal_double_click(long (WinMouse::*click)(), long delay) {
    const long r1 = (this->*click)();
    ::Delay(delay);
    const long r2 = (this->*click)();
    return r1 && r2 ? 1 : 0;
}

long WinMouse::button_double_click(long (WinMouse::*click)(), UINT message, UINT up_message, WPARAM button,
                                   long delay) {
    const long r1 = (this->*click)();
    ::Delay(delay);
    const POINT pt = current_client_point();
    const WPARAM state = button_state_with(button, true);
    const long r2 = send_message_result(_hwnd, message, state, MAKELPARAM(pt.x, pt.y));
    if (r2)
        set_button_state(button, true);
    ::Delay(delay);
    const long r3 = send_windows_button(up_message, button, false);
    return r1 && r2 && r3 ? 1 : 0;
}

long WinMouse::xbutton(WORD xbutton_id, WPARAM button, bool down) {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL:
        return send_input_mouse(down ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP, xbutton_id);
    case INPUT_TYPE::IN_WINDOWS:
        return send_windows_xbutton(down ? WM_XBUTTONDOWN : WM_XBUTTONUP, xbutton_id, button, down);
    }
    return 0;
}

long WinMouse::xbutton_double_click(long (WinMouse::*click)(), WORD xbutton_id, WPARAM button, long delay) {
    const long r1 = (this->*click)();
    ::Delay(delay);
    const long r2 = send_windows_xbutton(WM_XBUTTONDBLCLK, xbutton_id, button, true);
    ::Delay(delay);
    const long r3 = send_windows_xbutton(WM_XBUTTONUP, xbutton_id, button, false);
    return r1 && r2 && r3 ? 1 : 0;
}

long WinMouse::send_wheel(DWORD input_flag, UINT window_message, int delta) {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL:
        return send_input_mouse(input_flag, static_cast<DWORD>(delta));
    case INPUT_TYPE::IN_WINDOWS: {
        POINT pt = current_client_point();
        ::ClientToScreen(_hwnd, &pt);
        return send_message_result(_hwnd, window_message,
                                   MAKEWPARAM(static_cast<WORD>(button_state()), static_cast<WORD>(delta)),
                                   MAKELPARAM(pt.x, pt.y));
    }
    }
    return 0;
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
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        return send_input_click(MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP, 0, MOUSE_NORMAL_DELAY);
    }

    case INPUT_TYPE::IN_WINDOWS: {
        return button_click(&WinMouse::LeftDown, &WinMouse::LeftUp, MOUSE_WINDOWS_DELAY);
    }
    }
    return 0;
}

long WinMouse::LeftDoubleClick() {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        return normal_double_click(&WinMouse::LeftClick, MOUSE_NORMAL_DELAY);
    }
    case INPUT_TYPE::IN_WINDOWS: {
        return button_double_click(&WinMouse::LeftClick, WM_LBUTTONDBLCLK, WM_LBUTTONUP, MK_LBUTTON,
                                   MOUSE_WINDOWS_DELAY);
    }
    }
    return 0;
}

long WinMouse::LeftDown() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        ret = send_input_mouse(MOUSEEVENTF_LEFTDOWN);
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        ret = send_windows_button(WM_LBUTTONDOWN, MK_LBUTTON, true);
        break;
    }
    }
    return ret;
}

long WinMouse::LeftUp() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        ret = send_input_mouse(MOUSEEVENTF_LEFTUP);
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        ret = send_windows_button(WM_LBUTTONUP, MK_LBUTTON, false);
        break;
    }
    }
    return ret;
}

long WinMouse::MiddleClick() {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL:
        return send_input_click(MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_MIDDLEUP, 0, MOUSE_NORMAL_DELAY);
    case INPUT_TYPE::IN_WINDOWS:
        return button_click(&WinMouse::MiddleDown, &WinMouse::MiddleUp, MOUSE_WINDOWS_DELAY);
    }
    return 0;
}

long WinMouse::MiddleDoubleClick() {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        return normal_double_click(&WinMouse::MiddleClick, MOUSE_NORMAL_DELAY);
    }
    case INPUT_TYPE::IN_WINDOWS:
        return button_double_click(&WinMouse::MiddleClick, WM_MBUTTONDBLCLK, WM_MBUTTONUP, MK_MBUTTON,
                                   MOUSE_WINDOWS_DELAY);
    }
    return 0;
}

long WinMouse::MiddleDown() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        ret = send_input_mouse(MOUSEEVENTF_MIDDLEDOWN);
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        ret = send_windows_button(WM_MBUTTONDOWN, MK_MBUTTON, true);
        break;
    }
    }
    return ret;
}

long WinMouse::MiddleUp() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        ret = send_input_mouse(MOUSEEVENTF_MIDDLEUP);
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        ret = send_windows_button(WM_MBUTTONUP, MK_MBUTTON, false);
        break;
    }
    }
    return ret;
}

long WinMouse::RightClick() {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        return send_input_click(MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP, 0, MOUSE_NORMAL_DELAY);
    }

    case INPUT_TYPE::IN_WINDOWS: {
        return button_click(&WinMouse::RightDown, &WinMouse::RightUp, MOUSE_WINDOWS_DELAY);
    }
    }
    return 0;
}

long WinMouse::RightDoubleClick() {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        return normal_double_click(&WinMouse::RightClick, MOUSE_NORMAL_DELAY);
    }
    case INPUT_TYPE::IN_WINDOWS:
        return button_double_click(&WinMouse::RightClick, WM_RBUTTONDBLCLK, WM_RBUTTONUP, MK_RBUTTON,
                                   MOUSE_WINDOWS_DELAY);
    }
    return 0;
}

long WinMouse::RightDown() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        ret = send_input_mouse(MOUSEEVENTF_RIGHTDOWN);
        break;
    }
    case INPUT_TYPE::IN_WINDOWS: {
        ret = send_windows_button(WM_RBUTTONDOWN, MK_RBUTTON, true);
        break;
    }
    }
    return ret;
}

long WinMouse::RightUp() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        ret = send_input_mouse(MOUSEEVENTF_RIGHTUP);
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        ret = send_windows_button(WM_RBUTTONUP, MK_RBUTTON, false);
        break;
    }
    }
    return ret;
}

long WinMouse::XButton1Click() {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL:
        return send_input_click(MOUSEEVENTF_XDOWN, MOUSEEVENTF_XUP, XBUTTON1, MOUSE_NORMAL_DELAY);
    case INPUT_TYPE::IN_WINDOWS:
        return button_click(&WinMouse::XButton1Down, &WinMouse::XButton1Up, MOUSE_WINDOWS_DELAY);
    }
    return 0;
}

long WinMouse::XButton1DoubleClick() {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        return normal_double_click(&WinMouse::XButton1Click, MOUSE_NORMAL_DELAY);
    }
    case INPUT_TYPE::IN_WINDOWS:
        return xbutton_double_click(&WinMouse::XButton1Click, XBUTTON1, MK_XBUTTON1, MOUSE_WINDOWS_DELAY);
    }
    return 0;
}

long WinMouse::XButton1Down() {
    return xbutton(XBUTTON1, MK_XBUTTON1, true);
}

long WinMouse::XButton1Up() {
    return xbutton(XBUTTON1, MK_XBUTTON1, false);
}

long WinMouse::XButton2Click() {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL:
        return send_input_click(MOUSEEVENTF_XDOWN, MOUSEEVENTF_XUP, XBUTTON2, MOUSE_NORMAL_DELAY);
    case INPUT_TYPE::IN_WINDOWS:
        return button_click(&WinMouse::XButton2Down, &WinMouse::XButton2Up, MOUSE_WINDOWS_DELAY);
    }
    return 0;
}

long WinMouse::XButton2DoubleClick() {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        return normal_double_click(&WinMouse::XButton2Click, MOUSE_NORMAL_DELAY);
    }
    case INPUT_TYPE::IN_WINDOWS:
        return xbutton_double_click(&WinMouse::XButton2Click, XBUTTON2, MK_XBUTTON2, MOUSE_WINDOWS_DELAY);
    }
    return 0;
}

long WinMouse::XButton2Down() {
    return xbutton(XBUTTON2, MK_XBUTTON2, true);
}

long WinMouse::XButton2Up() {
    return xbutton(XBUTTON2, MK_XBUTTON2, false);
}

long WinMouse::Wheel(int delta) {
    return send_wheel(MOUSEEVENTF_WHEEL, WM_MOUSEWHEEL, delta);
}

long WinMouse::HWheel(int delta) {
    return send_wheel(MOUSEEVENTF_HWHEEL, WM_MOUSEHWHEEL, delta);
}

long WinMouse::WheelDown() {
    return Wheel(-WHEEL_DELTA);
}

long WinMouse::WheelUp() {
    return Wheel(WHEEL_DELTA);
}

} // namespace op::input
