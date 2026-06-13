#pragma once
#include "../core/optype.h"
#include "opMouseWin.h"
class opMouseDx : public opMouseWin {
  public:
    opMouseDx();
    ~opMouseDx() override;

    long Bind(HWND h, int mode) override;

    long UnBind() override;

    long GetCursorPos(long &x, long &y) override;

    long GetCursorShape(std::wstring &ret) override;

    long MoveR(int rx, int ry) override;

    long MoveTo(int x, int y) override;

    long MoveToEx(int x, int y, int w, int h, int &dst_x, int &dst_y) override;

    long LeftClick() override;

    long LeftDoubleClick() override;

    long LeftDown() override;

    long LeftUp() override;

    long MiddleClick() override;

    long MiddleDown() override;

    long MiddleUp() override;

    long RightClick() override;

    long RightDown() override;

    long RightUp() override;

    long WheelDown() override;

    long WheelUp() override;

};
