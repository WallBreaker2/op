#pragma once
#ifndef __BACKGROUND_H_
#define __BACKGROUND_H_
#include "Bkmouse.h"
#include "Bkdisplay.h"
using std::wstring;
class Background
{
public:
	
	Background();
	~Background();
public:
	virtual long Bind(long hwnd, const wstring& sdisplay, const wstring& smouse, const wstring& skeypad, long mode);
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
	virtual long Capture(const std::wstring& file_name);
private:
	HWND _hwnd;
	int _is_bind;
	int _mode;
public:
	Bkdisplay _bkdisplay;
	Bkmouse _bkmouse;
	
};
#endif


