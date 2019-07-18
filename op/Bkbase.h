#pragma once
#ifndef __BACKBASE_H_
#define __BACKBASE_H_
#include <string>
#include "bkgdi.h"
#include "Bkmouse.h"
#include "Bkkeypad.h"
#include "bkdx_gl.h"

using std::wstring;

/*
后台处理类，包含以下功能:
1.窗口绑定
2.后台截图
3.鼠标键盘操作
*/
class bkbase
{
public:
	
	bkbase();
	~bkbase();
public:
	virtual long BindWindow(long hwnd, const wstring& sdisplay, const wstring& smouse, const wstring& skeypad, long mode);
	virtual long UnBindWindow();
	virtual long GetBindWindow();
	virtual long IsBind();
	virtual long GetCursorPos(int& x, int& y);
	
	long GetDisplay();
	byte* GetScreenData();
	void lock_data();
	void unlock_data();
	long get_height();
	long get_widht();
	long RectConvert(long&x1, long&y1, long&x2, long&y2);
	long get_image_type();
	//检查是否绑定或者桌面前台
	bool check_bind();
private:
	HWND _hwnd;
	int _is_bind;
	int _display;
	int _mode;
public:
	bkdisplay* _pbkdisplay;
	bkmouse _bkmouse;
	bkkeypad _keypad;
	
};
#endif


