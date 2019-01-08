#pragma once
#ifndef __BACKBASE_H_
#define __BACKBASE_H_
#include "Bkmouse.h"
#include "Bkgdi.h"
#include "Bkdx.h"
using std::wstring;
class Bkbase
{
public:
	
	Bkbase();
	~Bkbase();
public:
	virtual long BindWindow(long hwnd, const wstring& sdisplay, const wstring& smouse, const wstring& skeypad, long mode);
	virtual long UnBindWindow();
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
	long GetDisplay();
private:
	HWND _hwnd;
	int _is_bind;
	int _display;
	int _mode;
public:
	Bkgdi _bkgdi;
	Bkdx _bkdx9;
	Bkmouse _bkmouse;
	
};
#endif


