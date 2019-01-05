#include "stdafx.h"
#include "BackGround.h"


Background::Background() :_hwnd(0),_is_bind(0)
{
	_mode = 0;
}


Background::~Background()
{
}

long Background::Bind(long hwnd, const wstring& sdisplay, const wstring& smouse, const wstring& skeypad, long mode) {
	_hwnd = (HWND)hwnd;
	long ret;
	int display, mouse, keypad;
	//check display
	if (sdisplay == L"normal")
		display = BACKTYPE::NORMAL;
	else if (sdisplay == L"gdi")
		display = BACKTYPE::GDI;
	else if (sdisplay == L"DX")
		display = BACKTYPE::DX;
	else if (sdisplay == L"opengl")
		display = BACKTYPE::OPENGL;
	else {
		setlog(L"error sdisplay=%s", sdisplay.c_str());
		return 0;
	}
	//check mouse
	if (smouse == L"normal")
		mouse = BACKTYPE::NORMAL;
	else if (smouse == L"windows")
		mouse = BACKTYPE::WINDOWS;
	else if (smouse == L"DX")
		mouse = BACKTYPE::DX;
	else if (smouse == L"opengl")
		mouse = BACKTYPE::OPENGL;
	else {
		setlog(L"error smouse=%s", smouse.c_str());
		return 0;
	}
	//check keypad
	if (skeypad == L"normal")
		keypad = BACKTYPE::NORMAL;
	else if (skeypad == L"windows")
		keypad = BACKTYPE::WINDOWS;
	else if (skeypad == L"DX")
		keypad = BACKTYPE::DX;
	else if (skeypad == L"opengl")
		keypad = BACKTYPE::OPENGL;
	else {
		setlog(L"error sdisplay=%s", sdisplay.c_str());
		return 0;
	}
	//check hwnd
	if (!::IsWindow(_hwnd)) {
		setlog(L"invalid window hwnd.");
		ret = 0; _hwnd = 0;
	}
	else {
		ret = 1;
		_mode = mode;
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
	_mode = 0;
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


