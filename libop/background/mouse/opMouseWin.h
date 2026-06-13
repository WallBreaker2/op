#pragma once
#include "core/optype.h"
#include <string>
class opMouseWin {
  public:
    opMouseWin();
    virtual ~opMouseWin();

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

    virtual long MiddleDown();

    virtual long MiddleUp();

    virtual long RightClick();

    virtual long RightDown();

    virtual long RightUp();

    virtual long WheelDown();

    virtual long WheelUp();

  protected:
    POINT current_client_point() const;
    long sync_system_cursor();

    HWND _hwnd;
    int _mode;
    int _x, _y;
};
