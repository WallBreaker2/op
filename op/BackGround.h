#pragma once
#ifndef __BACKGROUND_H_
#define __BACKGROUND_H_
#include "Bkmouse.h"
class Background
{
public:
	
	Background();
	~Background();
public:
	virtual long Bind(int hwnd, int display, int mouse, int keypad, int mode);
	virtual long UnBind();
	virtual long GetBindWindow();
	virtual long IsBind();
	virtual long GetCursorPos(int& x, int& y);
	virtual long GetKeyState(int vk_code);
	virtual long KeyDown(int vk_code);
	virtual long KeyUp(int vk_code);
	virtual long LeftClick();
	virtual long RightClick();
	virtual long MoveTo(long x, long y);
private:
	HWND _hwnd;
	int _is_bind;
	int _display;
	int _keypad;
	int _mode;
	Bkmouse _bkmouse;
};
#endif


