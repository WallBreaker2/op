#pragma once
#include "../../base/Types.h"
#include "WinMouse.h"

namespace op::input {

class DxMouse : public WinMouse {
  public:
    DxMouse();
    ~DxMouse() override;

    long Bind(HWND h, int mode) override;

    long UnBind() override;

    long GetCursorPos(long &x, long &y) override;

    long GetCursorShape(std::wstring &ret) override;

    long MoveR(int rx, int ry) override;

    long MoveTo(int x, int y) override;

    long LeftClick() override;

    long LeftDoubleClick() override;

    long LeftDown() override;

    long LeftUp() override;

    long MiddleClick() override;

    long MiddleDoubleClick() override;

    long MiddleDown() override;

    long MiddleUp() override;

    long RightClick() override;

    long RightDoubleClick() override;

    long RightDown() override;

    long RightUp() override;

    long XButton1Click() override;

    long XButton1DoubleClick() override;

    long XButton1Down() override;

    long XButton1Up() override;

    long XButton2Click() override;

    long XButton2DoubleClick() override;

    long XButton2Down() override;

    long XButton2Up() override;

    long Wheel(int delta) override;

    long HWheel(int delta) override;

    long WheelDown() override;

    long WheelUp() override;

  private:
    long send_button(UINT message, WPARAM button, bool down);
    long send_xbutton(UINT message, WORD xbutton, WPARAM button, bool down);
    long click(long (DxMouse::*down)(), long (DxMouse::*up)());
    long send_double_click(UINT message, UINT up_message, WPARAM button);
    long double_click(long (DxMouse::*click)(), UINT message, UINT up_message, WPARAM button);
    long xbutton(WORD xbutton, WPARAM button, bool down);
    long xbutton_double_click(long (DxMouse::*click)(), WORD xbutton, WPARAM button);
    long wheel(UINT message, int delta);
};

} // namespace op::input
