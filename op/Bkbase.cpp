#include "stdafx.h"
#include "Bkbase.h"
#include "Tool.h"
#include "Bkgdi.h"
#include "bkdx_gl.h"

#include <algorithm>
Bkbase::Bkbase() :_hwnd(0),_is_bind(0)
{
	_mode = 0;
}


Bkbase::~Bkbase()
{
}

long Bkbase::BindWindow(long hwnd, const wstring& sdisplay, const wstring& smouse, const wstring& skeypad, long mode) {
	//setlog(L"Bkbase::BindWindow(%d,%s,%s,%s,%d",
	//	hwnd, sdisplay.c_str(), smouse.c_str(), skeypad.c_str(), mode);
	_pbkdisplay = nullptr;
	_hwnd = (HWND)hwnd;
	long ret;
	int display, mouse, keypad;
	//check display
	if (sdisplay == L"normal")
		display = RDT_NORMAL;
	else if (sdisplay == L"gdi")
		display = RDT_GDI;
	else if (sdisplay == L"gdi2")
		display = RDT_GDI2;
	else if (sdisplay == L"dx2")
		display = RDT_GDI_DX2;
	else if (sdisplay == L"dx")
		display = RDT_DX_DEFAULT;
	else if (sdisplay == L"dx.d3d9")
		display = RDT_DX_D3D9;
	else if (sdisplay == L"dx.d3d10")
		display = RDT_DX_D3D10;
	else if (sdisplay == L"dx.d3d11")
		display = RDT_DX_D3D11;
	else if (sdisplay == L"opengl")
		display = RDT_GL_DEFAULT;
	else if (sdisplay == L"opengl.std")
		display = RDT_GL_STD;
	else if (sdisplay == L"opengl.nox")
		display = RDT_GL_NOX;
	else if (sdisplay == L"opengl.es")
		display = RDT_GL_ES;
	else {
		setlog(L"错误的display:%s", sdisplay.c_str());
		return 0;
	}
	//check mouse
	if (smouse == L"normal")
		mouse = INPUT_TYPE::IN_NORMAL;
	else if (smouse == L"windows")
		mouse = INPUT_TYPE::IN_WINDOWS;
	else {
		setlog(L"错误mouse:%s", smouse.c_str());
		return 0;
	}
	//check keypad
	if (skeypad == L"normal")
		keypad = INPUT_TYPE::IN_NORMAL;
	else if (skeypad == L"windows")
		keypad = INPUT_TYPE::IN_WINDOWS;
	else {
		setlog(L"错误的keypad:%s", sdisplay.c_str());
		return 0;
	}
	//check hwnd
	if (!::IsWindow(_hwnd)) {
		setlog(L"无效的窗口句柄:%d", _hwnd);
		ret = 0; _hwnd = 0;
	}
	else {
		_mode = mode;
		_display = display;
		if (!_bkmouse.Bind(_hwnd, mouse))
			return 0;
		if (!_keypad.Bind(_hwnd, keypad))
			return 0;
		
		if (display ==RDT_NORMAL || GET_RENDER_TYPE(display) == RENDER_TYPE::GDI) {
			_pbkdisplay = new bkgdi();
		}
		else if (GET_RENDER_TYPE(display) == RENDER_TYPE::DX) {
			_pbkdisplay = new bkdo;
		}
		else if (GET_RENDER_TYPE(display) == RENDER_TYPE::OPENGL)
			_pbkdisplay = new bkdo;
		
		ret = _pbkdisplay->Bind((HWND)hwnd, display);
		if (!ret) {
			SAFE_DELETE(_pbkdisplay);
			return 0;
		}
		//等待线程创建好
		Sleep(200);
		
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
	return _is_bind && ::IsWindow(_hwnd);
}

long Bkbase::GetCursorPos(int&x, int&y) {
	POINT pt;
	auto r=::GetCursorPos(&pt);
	x = pt.x; y = pt.y;
	return r;
}

//long Bkbase::GetKeyState(int vk_code) {
//	return ::GetAsyncKeyState(vk_code);
//}





long Bkbase::GetDisplay() {
	return _display;
}

byte* Bkbase::GetScreenData() {
	return _pbkdisplay->get_data();
}

void Bkbase::lock_data() {
	auto p = _pbkdisplay->get_mutex();
	if (p)
		p->lock();
}

void Bkbase::unlock_data() {
	auto p = _pbkdisplay->get_mutex();
	if (p)
		p->unlock();
}

long Bkbase::get_height() {
	return _pbkdisplay->get_height();
}

long Bkbase::get_widht() {
	return _pbkdisplay->get_width();
}

long Bkbase::RectConvert(long&x1, long&y1, long&x2, long&y2) {
	if (x1 > x2 || y1 > y2) {
		setlog("无效的窗口坐标:%d %d %d %d", x1, y1, x2, y2);
		return 0;
	}
		
	if (_display == RENDER_TYPE::NORMAL || _display == RENDER_TYPE::GDI) {
		x1 += _pbkdisplay->_client_x; y1 += _pbkdisplay->_client_y;
		x2 += _pbkdisplay->_client_x; y2 += _pbkdisplay->_client_y;
	}
	else {
		//to do...
	}
	x2 = std::min<long>(get_widht()-1, x2);
	y2 = std::min<long>(get_height()-1, y2);
	if (x1<0 || y1<0) {
		setlog("无效的窗口坐标:%d %d %d %d", x1, y1, x2, y2);
		return 0;
	}
	return 1;
}


