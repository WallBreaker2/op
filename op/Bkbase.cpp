#include "stdafx.h"
#include "Bkbase.h"


Bkbase::Bkbase() :_hwnd(0),_is_bind(0)
{
	_mode = 0;
}


Bkbase::~Bkbase()
{
}

long Bkbase::BindWindow(long hwnd, const wstring& sdisplay, const wstring& smouse, const wstring& skeypad, long mode) {
	_hwnd = (HWND)hwnd;
	long ret;
	int display, mouse, keypad;
	//check display
	if (sdisplay == L"normal")
		display = BACKTYPE::NORMAL;
	else if (sdisplay == L"gdi")
		display = BACKTYPE::GDI;
	else if (sdisplay == L"dx")
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
	else if (smouse == L"dx")
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
	else if (skeypad == L"dx")
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
		_mode = mode;
		_display = display;
		setlog("bind info:%d,%d", _display, mouse);
		
		if (display == BACKTYPE::NORMAL || display == BACKTYPE::GDI) {
			ret = _bkgdi.Bind(_hwnd, display);
		}
		else if (display == BACKTYPE::DX) {
			ret = _bkdx9.Bind(_hwnd);
		}
		else {
			ret = 0;
		}
		if (!ret)
			return 0;
		if (!_bkmouse.Bind(_hwnd, mouse))
			return 0;
		Sleep(20);
		
	}
	_display = display;
	return ret;

}

long Bkbase::UnBindWindow() {
	_hwnd = NULL;
	_is_bind = 0;
	_mode = 0;
	if (_display == BACKTYPE::NORMAL || _display == BACKTYPE::GDI)
		_bkgdi.UnBind();
	else
		_bkdx9.UnBind();
	_bkmouse.UnBind();
	return 1;
}

long Bkbase::GetBindWindow() {
	return (long)_hwnd;
}

long Bkbase::IsBind() {
	return _is_bind==1?1:0;
}

long Bkbase::GetCursorPos(int&x, int&y) {
	POINT pt;
	auto r=::GetCursorPos(&pt);
	x = pt.x; y = pt.y;
	return r;
}

long Bkbase::GetKeyState(int vk_code) {
	return ::GetAsyncKeyState(vk_code);
}

long Bkbase::KeyDown(int vk_code) {
	return 0;
}

long Bkbase::KeyUp(int vk_code) {
	return 0;
}

long Bkbase::LeftClick() {
	long ret = 0;
	ret = _bkmouse.LeftClick();
	return ret;
}

long Bkbase::RightClick() {
	
	return _bkmouse.RightClick();
}

long Bkbase::MoveTo(long x, long y) {
	
	return _bkmouse.MoveTo(x,y);
}

long Bkbase::Capture(const std::wstring& file_name) {
	if (_display == BACKTYPE::NORMAL || _display == BACKTYPE::GDI)
		return _bkgdi.capture(file_name);
	else
		return _bkdx9.capture(file_name);
}

long Bkbase::GetDisplay() {
	return _display;
}

byte* Bkbase::GetScreenData() {
	if (_display == BACKTYPE::NORMAL || _display == BACKTYPE::GDI) {
		return _bkgdi.get_data();
	}
	return nullptr;
}

void Bkbase::lock_data() {
	if (_display == BACKTYPE::NORMAL || _display == BACKTYPE::GDI) {
		_bkgdi.get_mutex().lock();
	}
}

void Bkbase::unlock_data() {
	if (_display == BACKTYPE::NORMAL || _display == BACKTYPE::GDI) {
		_bkgdi.get_mutex().unlock();
	}
}

long Bkbase::get_height() {
	return _bkgdi.get_height();
}

long Bkbase::get_widht() {
	return _bkgdi.get_widht();
}


