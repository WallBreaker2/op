#include "opMouseDx.h"
#include "CursorShape.h"
#include "../Hook/InputHookClient.h"
#include "../Hook/opMessage.h"
#include "../core/globalVar.h"
#include "../core/helpfunc.h"

namespace {
long send_op_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    return ::SendMessageTimeout(hwnd, message, wparam, lparam, SMTO_BLOCK, 2000, nullptr) ? 1L : 0L;
}
}

opMouseDx::opMouseDx() {
}

opMouseDx::~opMouseDx() {
    UnBind();
}

long opMouseDx::Bind(HWND h, int mode) {
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
    return ret;
}

long opMouseDx::UnBind() {
    const long ret = input_hook_client::UnBind(_hwnd);
    _hwnd = 0;
    _mode = 0;
    _x = _y = 0;
    return ret;
}

long opMouseDx::GetCursorPos(long &x, long &y) {
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

long opMouseDx::GetCursorShape(std::wstring &ret) {
    unsigned long long hash = 0;
    unsigned long long meta = 0;
    CursorShapeInfo info;
    if (input_hook_client::GetCursorShape(_hwnd, hash, meta) && cursor_shape::UnpackMeta(meta, hash, info)) {
        ret = cursor_shape::Format(info);
        return 1;
    }

    return opMouseWin::GetCursorShape(ret);
}

long opMouseDx::MoveR(int rx, int ry) {
    return MoveTo(_x + rx, _y + ry);
}

long opMouseDx::MoveTo(int x, int y) {
    const POINT pt{x, y};
    long ret = send_op_message(_hwnd, OP_WM_MOUSEMOVE, 0, MAKELPARAM(pt.x, pt.y));

    _x = pt.x, _y = pt.y;
    return ret;
}

long opMouseDx::MoveToEx(int x, int y, int w, int h, int &dst_x, int &dst_y) {
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

long opMouseDx::LeftClick() {
    long ret = 0, ret2 = 0;
    const POINT pt = current_client_point();

    ret = send_op_message(_hwnd, OP_WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(pt.x, pt.y));
    ::Delay(MOUSE_DX_DELAY);
    ret2 = send_op_message(_hwnd, OP_WM_LBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));

    return ret && ret2 ? 1 : 0;
}

long opMouseDx::LeftDoubleClick() {
    long r1, r2;
    r1 = LeftClick();
    ::Delay(MOUSE_DX_DELAY);
    r2 = LeftClick();
    return r1 & r2 ? 1 : 0;
}

long opMouseDx::LeftDown() {
    const POINT pt = current_client_point();
    return send_op_message(_hwnd, OP_WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(pt.x, pt.y));
}

long opMouseDx::LeftUp() {
    const POINT pt = current_client_point();
    return send_op_message(_hwnd, OP_WM_LBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
}

long opMouseDx::MiddleClick() {
    long r1, r2;
    r1 = MiddleDown();
    ::Delay(MOUSE_DX_DELAY);
    r2 = MiddleUp();
    return r1 & r2 ? 1 : 0;
}

long opMouseDx::MiddleDown() {
    const POINT pt = current_client_point();
    return send_op_message(_hwnd, OP_WM_MBUTTONDOWN, MK_MBUTTON, MAKELPARAM(pt.x, pt.y));
}

long opMouseDx::MiddleUp() {
    const POINT pt = current_client_point();
    return send_op_message(_hwnd, OP_WM_MBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
}

long opMouseDx::RightClick() {
    long ret = 0;
    long r1, r2;
    const POINT pt = current_client_point();

    r1 = send_op_message(_hwnd, OP_WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(pt.x, pt.y));
    ::Delay(MOUSE_DX_DELAY);
    r2 = send_op_message(_hwnd, OP_WM_RBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
    ret = r1 && r2 ? 1 : 0;

    return ret;
}

long opMouseDx::RightDown() {
    const POINT pt = current_client_point();
    return send_op_message(_hwnd, OP_WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(pt.x, pt.y));
}

long opMouseDx::RightUp() {
    const POINT pt = current_client_point();
    return send_op_message(_hwnd, OP_WM_RBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
}

long opMouseDx::WheelDown() {
    const POINT pt = current_client_point();
    return send_op_message(_hwnd, OP_WM_MOUSEWHEEL, MAKEWPARAM(0, -WHEEL_DELTA), MAKELPARAM(pt.x, pt.y));
}

long opMouseDx::WheelUp() {
    const POINT pt = current_client_point();
    return send_op_message(_hwnd, OP_WM_MOUSEWHEEL, MAKEWPARAM(0, WHEEL_DELTA), MAKELPARAM(pt.x, pt.y));
}
