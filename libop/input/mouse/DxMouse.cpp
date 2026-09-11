#include "DxMouse.h"
#include "CursorShape.h"
#include "../InputMessageUtils.h"
#include "../../hook/HookProtocol.h"
#include "../../hook/InputHookClient.h"
#include "../../base/AutomationModes.h"
#include "../../base/Utils.h"

namespace input_hook_client = op::hook::input_hook_client;

namespace op::input {

DxMouse::DxMouse() {
}

DxMouse::~DxMouse() {
    UnBind();
}

long DxMouse::Bind(HWND h, int mode) {
    if (_hwnd == h && _mode == mode)
        return 1;

    UnBind();
    long ret = input_hook_client::Bind(h, mode);
    if (ret != 1) {
        _hwnd = NULL;
        _mode = 0;
        return ret;
    }
    _hwnd = h;
    _mode = mode;
    _x = _y = 0;
    _button_state = 0;
    return ret;
}

long DxMouse::UnBind() {
    const long ret = input_hook_client::UnBind(_hwnd);
    _hwnd = 0;
    _mode = 0;
    _x = _y = 0;
    _button_state = 0;
    return ret;
}

long DxMouse::GetCursorPos(long &x, long &y) {
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

long DxMouse::GetCursorShape(std::wstring &ret) {
    unsigned long long hash = 0;
    unsigned long long meta = 0;
    CursorShapeInfo info;
    if (input_hook_client::GetCursorShape(_hwnd, hash, meta) && cursor_shape::UnpackMeta(meta, hash, info)) {
        ret = cursor_shape::Format(info);
        return 1;
    }

    return WinMouse::GetCursorShape(ret);
}

long DxMouse::MoveR(int rx, int ry) {
    return MoveTo(_x + rx, _y + ry);
}

long DxMouse::MoveTo(int x, int y) {
    const POINT pt{x, y};
    long ret = message::SendTimeout(_hwnd, OP_WM_MOUSEMOVE, button_state(), MAKELPARAM(pt.x, pt.y));

    _x = pt.x, _y = pt.y;
    return ret;
}

long DxMouse::send_button(UINT message, WPARAM button, bool down) {
    const POINT pt = current_client_point();
    const WPARAM state = button_state_with(button, down);
    const long ret = message::SendTimeout(_hwnd, message, state, MAKELPARAM(pt.x, pt.y));
    if (ret)
        set_button_state(button, down);
    return ret;
}

long DxMouse::send_xbutton(UINT message, WORD xbutton, WPARAM button, bool down) {
    const POINT pt = current_client_point();
    const WPARAM state = button_state_with(button, down);
    const long ret =
        message::SendTimeout(_hwnd, message, MAKEWPARAM(static_cast<WORD>(state), xbutton), MAKELPARAM(pt.x, pt.y));
    if (ret)
        set_button_state(button, down);
    return ret;
}

long DxMouse::click(long (DxMouse::*down)(), long (DxMouse::*up)()) {
    const long r1 = (this->*down)();
    ::Delay(MOUSE_DX_DELAY);
    const long r2 = (this->*up)();
    return r1 && r2 ? 1 : 0;
}

long DxMouse::send_double_click(UINT message, UINT up_message, WPARAM button) {
    const POINT pt = current_client_point();
    const WPARAM state = button_state_with(button, true);
    const long r1 = message::SendTimeout(_hwnd, message, state, MAKELPARAM(pt.x, pt.y));
    if (r1)
        set_button_state(button, true);
    ::Delay(MOUSE_DX_DELAY);
    const long r2 = send_button(up_message, button, false);
    return r1 && r2 ? 1 : 0;
}

long DxMouse::double_click(long (DxMouse::*click_func)(), UINT message, UINT up_message, WPARAM button) {
    const long r1 = (this->*click_func)();
    ::Delay(MOUSE_DX_DELAY);
    const long r2 = send_double_click(message, up_message, button);
    return r1 && r2 ? 1 : 0;
}

long DxMouse::xbutton(WORD xbutton_id, WPARAM button, bool down) {
    return send_xbutton(down ? OP_WM_XBUTTONDOWN : OP_WM_XBUTTONUP, xbutton_id, button, down);
}

long DxMouse::xbutton_double_click(long (DxMouse::*click_func)(), WORD xbutton_id, WPARAM button) {
    const long r1 = (this->*click_func)();
    ::Delay(MOUSE_DX_DELAY);
    const long r2 = send_xbutton(OP_WM_XBUTTONDBLCLK, xbutton_id, button, true);
    ::Delay(MOUSE_DX_DELAY);
    const long r3 = send_xbutton(OP_WM_XBUTTONUP, xbutton_id, button, false);
    return r1 && r2 && r3 ? 1 : 0;
}

long DxMouse::wheel(UINT message, int delta) {
    const POINT pt = current_client_point();
    return message::SendTimeout(_hwnd, message,
                                MAKEWPARAM(static_cast<WORD>(button_state()), static_cast<WORD>(delta)),
                                MAKELPARAM(pt.x, pt.y));
}

// DxMouse 的按钮只走窗口消息一条路径，访问器本身已是一行转发；
// 这里按按钮收敛成宏，省去 20 个同构签名各写一遍的样板。
#define OP_DX_MOUSE_BUTTON(Name, down_msg, up_msg, dbl_msg, mk) \
    long DxMouse::Name##Click() { \
        return click(&DxMouse::Name##Down, &DxMouse::Name##Up); \
    } \
    long DxMouse::Name##DoubleClick() { \
        return double_click(&DxMouse::Name##Click, dbl_msg, up_msg, mk); \
    } \
    long DxMouse::Name##Down() { \
        return send_button(down_msg, mk, true); \
    } \
    long DxMouse::Name##Up() { \
        return send_button(up_msg, mk, false); \
    }

// 侧键的 WPARAM 高字要携带 xbutton 标识，因此走 send_xbutton / xbutton 这条独立通道。
#define OP_DX_MOUSE_XBUTTON(Name, xbtn, mk) \
    long DxMouse::Name##Click() { \
        return click(&DxMouse::Name##Down, &DxMouse::Name##Up); \
    } \
    long DxMouse::Name##DoubleClick() { \
        return xbutton_double_click(&DxMouse::Name##Click, xbtn, mk); \
    } \
    long DxMouse::Name##Down() { \
        return xbutton(xbtn, mk, true); \
    } \
    long DxMouse::Name##Up() { \
        return xbutton(xbtn, mk, false); \
    }

OP_DX_MOUSE_BUTTON(Left, OP_WM_LBUTTONDOWN, OP_WM_LBUTTONUP, OP_WM_LBUTTONDBLCLK, MK_LBUTTON)
OP_DX_MOUSE_BUTTON(Middle, OP_WM_MBUTTONDOWN, OP_WM_MBUTTONUP, OP_WM_MBUTTONDBLCLK, MK_MBUTTON)
OP_DX_MOUSE_BUTTON(Right, OP_WM_RBUTTONDOWN, OP_WM_RBUTTONUP, OP_WM_RBUTTONDBLCLK, MK_RBUTTON)
OP_DX_MOUSE_XBUTTON(XButton1, XBUTTON1, MK_XBUTTON1)
OP_DX_MOUSE_XBUTTON(XButton2, XBUTTON2, MK_XBUTTON2)

#undef OP_DX_MOUSE_BUTTON
#undef OP_DX_MOUSE_XBUTTON

long DxMouse::Wheel(int delta) {
    return wheel(OP_WM_MOUSEWHEEL, delta);
}

long DxMouse::HWheel(int delta) {
    return wheel(OP_WM_MOUSEHWHEEL, delta);
}

long DxMouse::WheelDown() {
    return Wheel(-WHEEL_DELTA);
}

long DxMouse::WheelUp() {
    return Wheel(WHEEL_DELTA);
}

} // namespace op::input
