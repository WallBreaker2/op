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

long DxMouse::LeftClick() {
    return click(&DxMouse::LeftDown, &DxMouse::LeftUp);
}

long DxMouse::LeftDoubleClick() {
    return double_click(&DxMouse::LeftClick, OP_WM_LBUTTONDBLCLK, OP_WM_LBUTTONUP, MK_LBUTTON);
}

long DxMouse::LeftDown() {
    return send_button(OP_WM_LBUTTONDOWN, MK_LBUTTON, true);
}

long DxMouse::LeftUp() {
    return send_button(OP_WM_LBUTTONUP, MK_LBUTTON, false);
}

long DxMouse::MiddleClick() {
    return click(&DxMouse::MiddleDown, &DxMouse::MiddleUp);
}

long DxMouse::MiddleDoubleClick() {
    return double_click(&DxMouse::MiddleClick, OP_WM_MBUTTONDBLCLK, OP_WM_MBUTTONUP, MK_MBUTTON);
}

long DxMouse::MiddleDown() {
    return send_button(OP_WM_MBUTTONDOWN, MK_MBUTTON, true);
}

long DxMouse::MiddleUp() {
    return send_button(OP_WM_MBUTTONUP, MK_MBUTTON, false);
}

long DxMouse::RightClick() {
    return click(&DxMouse::RightDown, &DxMouse::RightUp);
}

long DxMouse::RightDoubleClick() {
    return double_click(&DxMouse::RightClick, OP_WM_RBUTTONDBLCLK, OP_WM_RBUTTONUP, MK_RBUTTON);
}

long DxMouse::RightDown() {
    return send_button(OP_WM_RBUTTONDOWN, MK_RBUTTON, true);
}

long DxMouse::RightUp() {
    return send_button(OP_WM_RBUTTONUP, MK_RBUTTON, false);
}

long DxMouse::XButton1Click() {
    return click(&DxMouse::XButton1Down, &DxMouse::XButton1Up);
}

long DxMouse::XButton1DoubleClick() {
    return xbutton_double_click(&DxMouse::XButton1Click, XBUTTON1, MK_XBUTTON1);
}

long DxMouse::XButton1Down() {
    return xbutton(XBUTTON1, MK_XBUTTON1, true);
}

long DxMouse::XButton1Up() {
    return xbutton(XBUTTON1, MK_XBUTTON1, false);
}

long DxMouse::XButton2Click() {
    return click(&DxMouse::XButton2Down, &DxMouse::XButton2Up);
}

long DxMouse::XButton2DoubleClick() {
    return xbutton_double_click(&DxMouse::XButton2Click, XBUTTON2, MK_XBUTTON2);
}

long DxMouse::XButton2Down() {
    return xbutton(XBUTTON2, MK_XBUTTON2, true);
}

long DxMouse::XButton2Up() {
    return xbutton(XBUTTON2, MK_XBUTTON2, false);
}

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
