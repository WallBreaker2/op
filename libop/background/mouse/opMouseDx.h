#pragma once
#include "../core/optype.h"
#include "opMouseWin.h"
class opMouseDx: public opMouseWin
{
public:
	opMouseDx();
	virtual ~opMouseDx();

	virtual long Bind(HWND h,int mode);

	virtual long UnBind();

	virtual long GetCursorPos(long&x, long& y);

	virtual long MoveR(int rx, int ry);

	virtual long MoveTo(int x, int y);

	virtual long MoveToEx(int x, int y,int w,int h);

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
private:
	HWND _hwnd;
	int _mode;
	int _x,_y;
	float _dpi;//screen dpi
};

