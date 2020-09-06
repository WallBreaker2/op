#pragma once
#ifndef __BACKBASE_H_
#define __BACKBASE_H_
#include <string>
#include "include/Image.hpp"

#include "IDisplay.h"

#include "Bkmouse.h"
#include "Bkkeypad.h"


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
	/*byte* GetScreenData();*/
	void lock_data();
	void unlock_data();
	long get_height();
	long get_width();
	long RectConvert(long&x1, long&y1, long&x2, long&y2);
	//0:normal;-1 reserve 1 need cut
	long get_image_type();
	//检查是否绑定或者桌面前台
	bool check_bind();
	const std::pair<wstring, wstring>& get_display_method()const;
	long set_display_method(const wstring& method);

	bool requestCapture(int x1,int y1,int w,int h,Image& img);
private:
	HWND _hwnd;
	int _is_bind;
	int _display;
	int _mode;
	std::pair<wstring,wstring> _display_method;
	Image _pic;

	IDisplay* createDisplay(int mode);
	bkmouse* createMouse(int mode);
	bkkeypad* createKeypad(int mode);
public:
	IDisplay* _pbkdisplay;
	bkmouse* _bkmouse;
	bkkeypad* _keypad;
	wstring _curr_path;
	
};
#endif


