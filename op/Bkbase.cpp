#include "stdafx.h"
#include <algorithm>
#include "globalVar.h"
#include "helpfunc.h"
#include "Bkbase.h"

bkbase::bkbase() :_hwnd(0), _is_bind(0), _pbkdisplay(nullptr)
{
	_display_method = std::make_pair<wstring, wstring>(L"screen", L"");
}

bkbase::~bkbase()
{
	_hwnd = NULL;
	_is_bind = 0;
	_mode = 0;
	_bkmouse.UnBind();
	if (_pbkdisplay) {
		_pbkdisplay->UnBind();
		delete _pbkdisplay;
		_pbkdisplay = nullptr;
	}
}

long bkbase::BindWindow(long hwnd, const wstring& sdisplay, const wstring& smouse, const wstring& skeypad, long mode) {

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
		set_display_method(L"screen");
		_mode = mode;
		_display = display;
		if (!_bkmouse.Bind(_hwnd, mouse))
			return 0;
		if (!_keypad.Bind(_hwnd, keypad))
			return 0;

		if (display == RDT_NORMAL || GET_RENDER_TYPE(display) == RENDER_TYPE::GDI) {
			_pbkdisplay = new bkgdi();
		}
		else if (GET_RENDER_TYPE(display) == RENDER_TYPE::DX) {
			_pbkdisplay = new bkdo;
		}
		else if (GET_RENDER_TYPE(display) == RENDER_TYPE::OPENGL)
			_pbkdisplay = new bkdo;

		ret = _pbkdisplay->Bind((HWND)hwnd, display);
		if (!ret) {
			_pbkdisplay->UnBind();
			ret = _pbkdisplay->Bind((HWND)hwnd, display);
			if (!ret) {
				SAFE_DELETE(_pbkdisplay);
				return 0;
			}
		}
		//等待线程创建好
		Sleep(200);

	}
	_is_bind = 1;
	_display = display;
	return ret;

}

long bkbase::UnBindWindow() {
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

long bkbase::GetBindWindow() {
	return (long)_hwnd;
}

long bkbase::IsBind() {
	return _pbkdisplay ? 1 : 0;
}

long bkbase::GetCursorPos(int&x, int&y) {
	POINT pt;
	auto r = ::GetCursorPos(&pt);
	x = pt.x; y = pt.y;
	return r;
}

long bkbase::GetDisplay() {
	return _display;
}

byte* bkbase::GetScreenData() {
	if (get_display_method().first == L"screen") {
		return _pbkdisplay ? _pbkdisplay->get_data() : nullptr;
	}
	else {
		if (get_display_method().first == L"pic") {
			return _pic.pdata;
		}
		if (get_display_method().first == L"mem") {
#if OP64==1
			return (byte*)_wtoi64(get_display_method().second.data());
#endif // 
			return (byte*)_wtoi(get_display_method().second.data());

	}
		return nullptr;
}


}

void bkbase::lock_data() {
	if (_pbkdisplay) {
		auto p = _pbkdisplay->get_mutex();
		if (p)
			p->lock();
	}

}

void bkbase::unlock_data() {
	if (_pbkdisplay) {
		auto p = _pbkdisplay->get_mutex();
		if (p)
			p->unlock();
	}

}

long bkbase::get_height() {
	if (get_display_method().first == L"pic")
		return _pic.height;
	return _pbkdisplay ? _pbkdisplay->get_height() : 0;
}

long bkbase::get_width() {
	if (get_display_method().first == L"pic")
		return _pic.width;
	return _pbkdisplay ? _pbkdisplay->get_width() : 0;
}

long bkbase::RectConvert(long&x1, long&y1, long&x2, long&y2) {
	if (x1 > x2 || y1 > y2) {
		setlog("无效的窗口坐标:%d %d %d %d", x1, y1, x2, y2);
		return 0;
	}


	if (_pbkdisplay && (_display == RENDER_TYPE::NORMAL || _display == RENDER_TYPE::GDI)) {
		x1 += _pbkdisplay->_client_x; y1 += _pbkdisplay->_client_y;
		x2 += _pbkdisplay->_client_x; y2 += _pbkdisplay->_client_y;
	}


	x2 = std::min<long>(get_width(), x2);
	y2 = std::min<long>(get_height(), y2);
	if (x1 < 0 || y1 < 0) {
		setlog("无效的窗口坐标:%d %d %d %d", x1, y1, x2, y2);
		return 0;
	}
	return 1;
}

long bkbase::get_image_type() {
	if (get_display_method().first != L"screen")
		return 0;
	switch (GET_RENDER_TYPE(_display))
	{
	case RENDER_TYPE::NORMAL:
		return -1;
	case RENDER_TYPE::GDI:
		return -1;
	case RENDER_TYPE::DX:
		return 0;
	case RENDER_TYPE::OPENGL:
		return -1;
	default:
		return 0;
	}

}

bool bkbase::check_bind() {
	//已绑定
	if (IsBind())
		return true;
	//显示模式非屏幕
	if (get_display_method().first != L"screen") {
		if (get_display_method().first == L"pic") {//load pic first
			wstring fullpath;
			if (Path2GlobalPath(get_display_method().second, _curr_path, fullpath)) {
				_pic.read(fullpath.data());
			}
		}
		return true;
	}

	//绑定前台桌面
	return BindWindow((long)::GetDesktopWindow(), L"normal", L"normal", L"normal", 0);
}


const std::pair<wstring, wstring>& bkbase::get_display_method()const {
	return _display_method;
}

long bkbase::set_display_method(const wstring& method) {
	if (method == L"screen") {
		_display_method.first = method;
		_display_method.second.clear();
		return 1;
	}
	else {
		auto idx = method.find(L"pic:");
		if (idx != wstring::npos) {
			_display_method.first = L"pic";
			_display_method.second = method.substr(idx + 4);
			return 1;
		}
		idx = method.find(L"mem:");
		if (idx != wstring::npos) {
			_display_method.first = L"mem";
			_display_method.second = method.substr(idx + 4);
			return 1;
		}
		return 0;
	}
}

