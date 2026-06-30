#include "DxMouse.h"
#include "CursorShape.h"
#include "../../hook/HookProtocol.h"
#include "../../hook/InputHookClient.h"
#include "../../runtime/AutomationModes.h"
#include "../../runtime/RuntimeUtils.h"

namespace input_hook_client = op::hook::input_hook_client;

namespace {
long send_op_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    return ::SendMessageTimeout(hwnd, message, wparam, lparam, SMTO_BLOCK, 2000, nullptr) ? 1L : 0L;
}
}

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
    long ret = send_op_message(_hwnd, OP_WM_MOUSEMOVE, button_state(), MAKELPARAM(pt.x, pt.y));

    _x = pt.x, _y = pt.y;
    return ret;
}

long DxMouse::MoveToEx(int x, int y, int w, int h, int &dst_x, int &dst_y) {
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

long DxMouse::send_button(UINT message, WPARAM button, bool down) {
    const POINT pt = current_client_point();
    const WPARAM state = button_state_with(button, down);
    const long ret = send_op_message(_hwnd, message, state, MAKELPARAM(pt.x, pt.y));
    if (ret)
        set_button_state(button, down);
    return ret;
}

long DxMouse::LeftClick() {
    long ret = 0, ret2 = 0;

    ret = send_button(OP_WM_LBUTTONDOWN, MK_LBUTTON, true);
    ::Delay(MOUSE_DX_DELAY);
    ret2 = send_button(OP_WM_LBUTTONUP, MK_LBUTTON, false);

    return ret && ret2 ? 1 : 0;
}

long DxMouse::LeftDoubleClick() {
    long r1, r2;
    r1 = LeftClick();
    ::Delay(MOUSE_DX_DELAY);
    r2 = LeftClick();
    return r1 & r2 ? 1 : 0;
}

long DxMouse::LeftDown() {
    return send_button(OP_WM_LBUTTONDOWN, MK_LBUTTON, true);
}

long DxMouse::LeftUp() {
    return send_button(OP_WM_LBUTTONUP, MK_LBUTTON, false);
}

long DxMouse::MiddleClick() {
    long r1, r2;
    r1 = MiddleDown();
    ::Delay(MOUSE_DX_DELAY);
    r2 = MiddleUp();
    return r1 & r2 ? 1 : 0;
}

long DxMouse::MiddleDown() {
    return send_button(OP_WM_MBUTTONDOWN, MK_MBUTTON, true);
}

long DxMouse::MiddleUp() {
    return send_button(OP_WM_MBUTTONUP, MK_MBUTTON, false);
}

long DxMouse::RightClick() {
    long ret = 0;
    long r1, r2;

    r1 = send_button(OP_WM_RBUTTONDOWN, MK_RBUTTON, true);
    ::Delay(MOUSE_DX_DELAY);
    r2 = send_button(OP_WM_RBUTTONUP, MK_RBUTTON, false);
    ret = r1 && r2 ? 1 : 0;

    return ret;
}

long DxMouse::RightDown() {
    return send_button(OP_WM_RBUTTONDOWN, MK_RBUTTON, true);
}

long DxMouse::RightUp() {
    return send_button(OP_WM_RBUTTONUP, MK_RBUTTON, false);
}

long DxMouse::WheelDown() {
    const POINT pt = current_client_point();
    return send_op_message(_hwnd, OP_WM_MOUSEWHEEL, MAKEWPARAM(static_cast<WORD>(button_state()), -WHEEL_DELTA),
                           MAKELPARAM(pt.x, pt.y));
}

long DxMouse::WheelUp() {
    const POINT pt = current_client_point();
    return send_op_message(_hwnd, OP_WM_MOUSEWHEEL, MAKEWPARAM(static_cast<WORD>(button_state()), WHEEL_DELTA),
                           MAKELPARAM(pt.x, pt.y));
}

} // namespace op::input
