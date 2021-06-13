//#include "stdafx.h"
#include <algorithm>
#include "./core/globalVar.h"
#include "./core/helpfunc.h"
#include "opBackground.h"

#include "./display/opGDI.h"
#include "./display/opDxGL.h""

#include "./keypad/winkeypad.h""
#include "./mouse/opMouseDx.h"
opBackground::opBackground() : _hwnd(0), _is_bind(0), _pbkdisplay(nullptr), _bkmouse(new opMouseWin), _keypad(new winkeypad)
{
	_display_method = std::make_pair<wstring, wstring>(L"screen", L"");
}

opBackground::~opBackground()
{
	/*_hwnd = NULL;
	_is_bind = 0;
	_mode = 0;
	_bkmouse->UnBind();
	if (_pbkdisplay) {
		_pbkdisplay->UnBind();
		delete _pbkdisplay;
		_pbkdisplay = nullptr;
	}*/
	UnBindWindow();
	SAFE_DELETE(_bkmouse);
	SAFE_DELETE(_keypad);
}

long opBackground::BindWindow(long hwnd, const wstring &sdisplay, const wstring &smouse, const wstring &skeypad, long mode)
{
	//step 1.避免重复绑定
	UnBindWindow();
	//step 2.check hwnd
	if (!::IsWindow(HWND(hwnd)))
	{
		setlog("Invalid window handles");
		return 0;
	}

	int display, mouse, keypad;
	//step 3.check display... mode
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
	else if (sdisplay == L"opengl.fi")//glFinish
		display = RDT_GL_FI;
	else
	{
		setlog(L"error display:%s", sdisplay.c_str());
		return 0;
	}
	//check mouse
	if (smouse == L"normal")
		mouse = INPUT_TYPE::IN_NORMAL;
	else if (smouse == L"windows")
		mouse = INPUT_TYPE::IN_WINDOWS;
	else if (smouse == L"dx")
		mouse = INPUT_TYPE::IN_DX;
	else
	{
		setlog(L"error mouse:%s", smouse.c_str());
		return 0;
	}
	//check keypad
	if (skeypad == L"normal")
		keypad = INPUT_TYPE::IN_NORMAL;
	else if (skeypad == L"windows")
		keypad = INPUT_TYPE::IN_WINDOWS;
	else
	{
		setlog(L"error keypad:%s", sdisplay.c_str());
		return 0;
	}
	//step 4.init
	_mode = mode;
	_display = display;
	_hwnd = (HWND)hwnd;
	set_display_method(L"screen");

	//step 5. create instance
	_pbkdisplay = createDisplay(display);
	_bkmouse = createMouse(mouse);
	_keypad = createKeypad(keypad);

	if (!_pbkdisplay || !_bkmouse || !_keypad)
	{
		setlog("create instance error!");
		UnBindWindow();
		return 0;
	}
	//step 6.try bind
	if (_pbkdisplay->Bind((HWND)hwnd, display) != 1 ||
		_bkmouse->Bind((HWND)hwnd, mouse) != 1 ||
		_keypad->Bind((HWND)hwnd, keypad) != 1)
	{
		UnBindWindow();
		return 0;
	}

	//等待线程创建好
	Sleep(200);

	_is_bind = 1;
	return 1;
}

long opBackground::UnBindWindow()
{
	//to do
	//clear ....
	_hwnd = NULL;
	_is_bind = 0;
	_mode = 0;

	if (_pbkdisplay)
	{
		_pbkdisplay->UnBind();
		SAFE_DELETE(_pbkdisplay);
	}
	if (_bkmouse)
	{
		_bkmouse->UnBind();
		SAFE_DELETE(_bkmouse);
	}
	if (_keypad)
	{
		_keypad->UnBind();
		SAFE_DELETE(_keypad);
	}
	//恢复为前台(默认)
	_bkmouse = new opMouseWin;
	_keypad = new winkeypad;

	return 1;
}

long opBackground::GetBindWindow()
{
	return (long)_hwnd;
}

long opBackground::IsBind()
{
	return _pbkdisplay ? 1 : 0;
}

//long bkbase::GetCursorPos(int& x, int& y) {
//	POINT pt;
//	auto r = ::GetCursorPos(&pt);
//	x = pt.x; y = pt.y;
//	return r;
//}

long opBackground::GetDisplay()
{
	return _display;
}

//byte* bkbase::GetScreenData() {
//	if (get_display_method().first == L"screen") {
//		return _pbkdisplay ? _pbkdisplay->get_data() : nullptr;
//	}
//	else {
//		if (get_display_method().first == L"pic") {
//			return _pic.pdata;
//		}
//		if (get_display_method().first == L"mem") {
//
//#if OP64==1
//			auto ptr= (byte*)_wtoi64(get_display_method().second.data());
//#else
//			auto ptr = (byte*)_wtoi(get_display_method().second.data());
//#endif //
//
//			auto pbfh = (BITMAPFILEHEADER*)ptr;
//			return ptr + pbfh->bfOffBits;
//	}
//		return nullptr;
//}
//
//
//}

void opBackground::lock_data()
{
	if (_pbkdisplay)
	{
		auto p = _pbkdisplay->get_mutex();
		if (p)
			p->lock();
	}
}

void opBackground::unlock_data()
{
	if (_pbkdisplay)
	{
		auto p = _pbkdisplay->get_mutex();
		if (p)
			p->unlock();
	}
}

long opBackground::get_height()
{
	auto &displayMethod = get_display_method();
	if (displayMethod.first == L"pic")
	{
		return _pic.height;
	}
	else if (displayMethod.first == L"mem")
	{
		auto strPtr = displayMethod.second;
#if OP64 == 1
		auto ptr = (byte *)_wtoi64(strPtr.data());
#else
		auto ptr = (byte *)_wtoi(strPtr.data());
#endif //

		auto bih = (BITMAPINFOHEADER *)(ptr + sizeof(BITMAPFILEHEADER));
		return bih->biHeight < 0 ? -bih->biHeight : bih->biHeight;
	}
	else
	{
		return _pbkdisplay ? _pbkdisplay->get_height() : 0;
	}
}

long opBackground::get_width()
{
	auto &displayMethod = get_display_method();
	if (displayMethod.first == L"pic")
	{
		return _pic.width;
	}
	else if (displayMethod.first == L"mem")
	{
		auto strPtr = displayMethod.second;
#if OP64 == 1
		auto ptr = (byte *)_wtoi64(strPtr.data());
#else
		auto ptr = (byte *)_wtoi(strPtr.data());
#endif //

		auto bih = (BITMAPINFOHEADER *)(ptr + sizeof(BITMAPFILEHEADER));
		return bih->biWidth;
	}
	else
	{
		return _pbkdisplay ? _pbkdisplay->get_width() : 0;
	}
}

long opBackground::RectConvert(long &x1, long &y1, long &x2, long &y2)
{

	/*if (_pbkdisplay && (_display == RENDER_TYPE::NORMAL || _display == RENDER_TYPE::GDI)) {
		x1 += _pbkdisplay->_client_x; y1 += _pbkdisplay->_client_y;
		x2 += _pbkdisplay->_client_x; y2 += _pbkdisplay->_client_y;
	}*/

	x2 = std::min<long>(this->get_width(), x2);
	y2 = std::min<long>(this->get_height(), y2);
	if (x1 < 0 || y1 < 0 || x1 >= x2 || y1 >= y2)
	{
		setlog(L"invalid rectangle:%d %d %d %d", x1, y1, x2, y2);
		return 0;
	}
	//if (_pbkdisplay) {
	//	if (_display == RDT_NORMAL) {//cap rect
	//		_pbkdisplay->rect.left = x1;
	//		_pbkdisplay->rect.top = y1;
	//		_pbkdisplay->rect.right = x2;
	//		_pbkdisplay->rect.bottom = y2;
	//	}
	//	else {
	//		_pbkdisplay->rect.left = 0;
	//		_pbkdisplay->rect.top = 0;
	//		_pbkdisplay->rect.right = _pbkdisplay->get_width();
	//		_pbkdisplay->rect.bottom =_pbkdisplay->get_height();
	//	}
	//
	//}
	return 1;
}

long opBackground::get_image_type()
{

	if (_display_method.first == L"pic")
		return 0;
	else if (_display_method.first == L"mem")
	{

		return 1;
	}
	else
	{
		switch (GET_RENDER_TYPE(_display))
		{
		case RENDER_TYPE::NORMAL:
			return -2;
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
}

bool opBackground::check_bind()
{
	//已绑定
	if (IsBind())
		return true;
	//显示模式非屏幕
	if (get_display_method().first != L"screen")
	{
		if (get_display_method().first == L"pic")
		{ //load pic first
			wstring fullpath;
			if (Path2GlobalPath(get_display_method().second, _curr_path, fullpath))
			{
				_pic.read(fullpath.data());
			}
		}
		return true;
	}

	//绑定前台桌面
	return BindWindow((long)::GetDesktopWindow(), L"normal", L"normal", L"normal", 0);
}

const std::pair<wstring, wstring> &opBackground::get_display_method() const
{
	return _display_method;
}

long opBackground::set_display_method(const wstring &method)
{
	if (method == L"screen")
	{
		_display_method.first = method;
		_display_method.second.clear();
		return 1;
	}
	else
	{
		auto idx = method.find(L"pic:");
		if (idx != wstring::npos)
		{
			_display_method.first = L"pic";
			_display_method.second = method.substr(idx + 4);
			_pic.read(_display_method.second.data());
			return 1;
		}
		idx = method.find(L"mem:");
		if (idx != wstring::npos)
		{
			auto strPtr = method.substr(idx + 4);
#if OP64 == 1
			auto ptr = (byte *)_wtoi64(strPtr.data());
#else
			auto ptr = (byte *)_wtoi(strPtr.data());
#endif //

			if (ptr == nullptr)
			{
				return 0;
			}
			BITMAPFILEHEADER bfh = {0}; //bmp file header
			BITMAPINFOHEADER bih = {0}; //bmp info header
			memcpy(&bfh, ptr, sizeof(bfh));
			memcpy(&bih, ptr + sizeof(bfh), sizeof(bih));

			if (bfh.bfType != static_cast<WORD>(0x4d42))
				return 0;

			if (bih.biHeight < 0)
			{ //正常拷贝
				int h = -bih.biHeight;
				_pic.create(bih.biWidth, h);
				/*setlog("mem w=%d h=%d chk=%d",
					bih.biWidth, h,
					_pic.size() * 4 == bih.biSizeImage ? 1 : 0);*/
				memcpy(_pic.pdata, ptr + sizeof(bfh) + sizeof(bih), _pic.size() * 4);
			}
			else
			{ //倒过来拷贝
				int h = bih.biHeight;
				_pic.create(bih.biWidth, h);
				for (int i = 0; i < h; i++)
				{
					memcpy(_pic.ptr<uchar>(i),
						   ptr + sizeof(bfh) + sizeof(bih) + (h - 1 - i) * bih.biWidth * 4,
						   bih.biWidth * 4);
				}
			}
			_display_method.first = L"mem";
			_display_method.second = strPtr;

			return 1;
		}
		return 0;
	}
}

bool opBackground::requestCapture(int x1, int y1, int w, int h, Image &img)
{
	wstring method = get_display_method().first;
	if (method == L"screen")
		return _pbkdisplay->requestCapture(x1, y1, w, h, img);
	else if (method == L"pic" || method == L"mem")
	{
		img.create(w, h);
		for (int i = 0; i < h; i++)
			memcpy(img.ptr<uchar>(i), _pic.ptr<uchar>(i + y1) + x1 * 4, w * 4);
		return true;
	}
	return false;
}

IDisplay *opBackground::createDisplay(int mode)
{
	IDisplay *pans = 0;

	if (mode == RDT_NORMAL || GET_RENDER_TYPE(mode) == RENDER_TYPE::GDI)
	{
		pans = new opGDI();
	}
	else if (GET_RENDER_TYPE(mode) == RENDER_TYPE::DX)
	{
		pans = new opDxGL;
	}
	else if (GET_RENDER_TYPE(mode) == RENDER_TYPE::OPENGL)
		pans = new opDxGL;
	else
		pans = 0;
	return pans;
}

opMouseWin *opBackground::createMouse(int mode)
{
	if (mode == INPUT_TYPE::IN_NORMAL || mode == INPUT_TYPE::IN_WINDOWS)
		return new opMouseWin();
	else if (mode == INPUT_TYPE::IN_DX)
	{
		return new opMouseDx();
	}
	//return 0;
}

bkkeypad *opBackground::createKeypad(int mode)
{
	return new winkeypad();
	//return 0;
}
