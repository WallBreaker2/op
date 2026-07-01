#pragma once
#include "../../runtime/Types.h"
#include <string>

namespace op::input {

class WinMouse {
  public:
    WinMouse();
    virtual ~WinMouse();

    virtual long Bind(HWND h, int mode);

    virtual long UnBind();

    virtual long GetCursorPos(long &x, long &y);

    virtual long GetCursorShape(std::wstring &ret);

    virtual long MoveR(int rx, int ry);

    virtual long MoveTo(int x, int y);

    virtual long MoveToEx(int x, int y, int w, int h, int &dst_x, int &dst_y);

    virtual long LeftClick();

    virtual long LeftDoubleClick();

    virtual long LeftDown();

    virtual long LeftUp();

    virtual long MiddleClick();

    virtual long MiddleDoubleClick();

    virtual long MiddleDown();

    virtual long MiddleUp();

    virtual long RightClick();

    virtual long RightDoubleClick();

    virtual long RightDown();

    virtual long RightUp();

    virtual long XButton1Click();

    virtual long XButton1DoubleClick();

    virtual long XButton1Down();

    virtual long XButton1Up();

    virtual long XButton2Click();

    virtual long XButton2DoubleClick();

    virtual long XButton2Down();

    virtual long XButton2Up();

    virtual long Wheel(int delta);

    virtual long HWheel(int delta);

    virtual long WheelDown();

    virtual long WheelUp();

  protected:
    POINT current_client_point() const;
    long sync_system_cursor();
    WPARAM button_state() const;
    WPARAM button_state_with(WPARAM button, bool down) const;
    void set_button_state(WPARAM button, bool down);
    long send_input_mouse(DWORD flags, DWORD mouse_data = 0, bool sync_cursor = true);
    long send_input_click(DWORD down_flags, DWORD up_flags, DWORD mouse_data, long delay);
    long send_windows_button(UINT message, WPARAM button, bool down);
    long send_windows_xbutton(UINT message, WORD xbutton, WPARAM button, bool down);
    long button_click(long (WinMouse::*down)(), long (WinMouse::*up)(), long delay);
    long normal_double_click(long (WinMouse::*click)(), long delay);
    long button_double_click(long (WinMouse::*click)(), UINT message, UINT up_message, WPARAM button, long delay);
    long xbutton(WORD xbutton, WPARAM button, bool down);
    long xbutton_double_click(long (WinMouse::*click)(), WORD xbutton, WPARAM button, long delay);
    long send_wheel(DWORD input_flag, UINT window_message, int delta);

    HWND _hwnd;
    int _mode;
    int _x, _y;
    WPARAM _button_state;
};

} // namespace op::input
