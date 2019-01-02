#include "stdafx.h"
#include "BackGround.h"


Background::Background() :_hwnd(0),_is_bind(0)
{
	_display = _keypad = _mode = 0;
}


Background::~Background()
{
}

long Background::Bind(int hwnd, int display, int mouse, int keypad, int mode) {
	_hwnd = (HWND)hwnd;
	long ret;
	if (!::IsWindow(_hwnd)) {
		ret = 0; _hwnd = 0;
	}
	else {
		ret = 1;
		_display = display;
		_keypad = keypad; _mode = mode;
		if (!_bkdisplay.Bind(_hwnd, mode))
			return 0;
		if (!_bkmouse.Bind(_hwnd, mouse))
			return 0;
		Sleep(20);
		
	}
	return ret;

}

long Background::UnBind() {
	_hwnd = NULL;
	_is_bind = 0;
	_display = _keypad = _mode = 0;
	_bkdisplay.UnBind();
	_bkmouse.UnBind();
	return 1;
}

long Background::GetBindWindow() {
	return (long)_hwnd;
}

long Background::IsBind() {
	return _is_bind==1?1:0;
}

long Background::GetCursorPos(int&x, int&y) {
	POINT pt;
	auto r=::GetCursorPos(&pt);
	x = pt.x; y = pt.y;
	return r;
}

long Background::GetKeyState(int vk_code) {
	return ::GetAsyncKeyState(vk_code);
}

long Background::KeyDown(int vk_code) {
	return 0;
}

long Background::KeyUp(int vk_code) {
	return 0;
}

long Background::LeftClick() {
	long ret = 0;
	ret = _bkmouse.LeftClick();
	return ret;
}

long Background::RightClick() {
	
	return _bkmouse.RightClick();
}

long Background::MoveTo(long x, long y) {
	
	return _bkmouse.MoveTo(x,y);
}

long Background::Capture(const std::wstring& file_name) {
	return _bkdisplay.capture(file_name);
}


