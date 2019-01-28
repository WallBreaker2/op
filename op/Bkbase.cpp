#include "stdafx.h"
#include "Bkbase.h"
#include "Tool.h"
#include "Bkgdi.h"
#include "Bkdx.h"
#include "bkopengl.h"
Bkbase::Bkbase() :_hwnd(0),_is_bind(0)
{
	_mode = 0;
}


Bkbase::~Bkbase()
{
}

long Bkbase::BindWindow(long hwnd, const wstring& sdisplay, const wstring& smouse, const wstring& skeypad, long mode) {
	Tool::setlog(L"Bkbase::BindWindow(%d,%s,%s,%s,%d",
		hwnd, sdisplay.c_str(), smouse.c_str(), skeypad.c_str(), mode);
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
		Tool::setlog(L"error sdisplay=%s", sdisplay.c_str());
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
		Tool::setlog(L"error smouse=%s", smouse.c_str());
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
		Tool::setlog(L"error sdisplay=%s", sdisplay.c_str());
		return 0;
	}
	//check hwnd
	if (!::IsWindow(_hwnd)) {
		Tool::setlog(L"invalid window hwnd.");
		ret = 0; _hwnd = 0;
	}
	else {
		_mode = mode;
		_display = display;
		if (!_bkmouse.Bind(_hwnd, mouse))
			return 0;
		if (!_keypad.Bind(_hwnd, keypad))
			return 0;
		
		if (display == BACKTYPE::NORMAL || display == BACKTYPE::GDI) {
			_pbkdisplay = new bkgdi();
		}
		else if (display == BACKTYPE::DX) {
			_pbkdisplay = new Bkdx;
		}
		else if(display==BACKTYPE::OPENGL)
			_pbkdisplay = new bkopengl;
		
		ret = _pbkdisplay->Bind((HWND)hwnd, display);
		if (!ret) {
			SAFE_DELETE(_pbkdisplay);
			return 0;
		}
			
		
		Sleep(20);
		
	}
	_is_bind = 1;
	_display = display;
	return ret;

}

long Bkbase::UnBindWindow() {
	_hwnd = NULL;
	_is_bind = 0;
	_mode = 0;
	
	_bkmouse.UnBind();
	if (_pbkdisplay) {
		_pbkdisplay->UnBind();
		delete _pbkdisplay;
		_pbkdisplay = nullptr;
	}
	
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
	if (_is_bind)
		return _pbkdisplay->capture(file_name);
	else
		return 0;
}

long Bkbase::GetDisplay() {
	return _display;
}

byte* Bkbase::GetScreenData() {
	return _pbkdisplay->get_data();
}

void Bkbase::lock_data() {
	_pbkdisplay->get_mutex()->lock();
}

void Bkbase::unlock_data() {
	_pbkdisplay->get_mutex()->unlock();
}

long Bkbase::get_height() {
	return _pbkdisplay->get_height();
}

long Bkbase::get_widht() {
	return _pbkdisplay->get_width();
}

long Bkbase::RectConvert(long&x1, long&y1, long&x2, long&y2) {
	if (x1 > x2 || y1 > y2) {
		Tool::setlog("Invalid rect:%d %d %d %d", x1, y1, x2, y2);
		return 0;
	}
		
	if (_display == BACKTYPE::NORMAL || _display == BACKTYPE::GDI) {
		x1 += _pbkdisplay->_client_x; y1 += _pbkdisplay->_client_y;
		x2 += _pbkdisplay->_client_x; y2 += _pbkdisplay->_client_y;
	}
	else {
		//to do...
	}
	if (x1<0 || x2>get_widht() || y1<0 || y2>get_height()) {
		Tool::setlog("Invalid rect:%d %d %d %d", x1, y1, x2, y2);
		return 0;
	}
	return 1;
}


