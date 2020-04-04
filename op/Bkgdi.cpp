#include "stdafx.h"
#include "Bkgdi.h"
#include "globalVar.h"
#include "helpfunc.h"
#include <fstream>

bkgdi::bkgdi()
{
	_render_type = 0;

	//4*2^22=16*2^20=16MB
	//_image_data = new byte[MAX_IMAGE_WIDTH*MAX_IMAGE_WIDTH * 4];
}

bkgdi::~bkgdi()
{
	//SAFE_DELETE_ARRAY(_image_data);
}

long bkgdi::Bind(HWND hwnd, long render_type) {
	if (!::IsWindow(hwnd))
		return 0;
	_hwnd = hwnd; _render_type = render_type;
	bind_init();
	RECT rc;
	//step 3.获取截图区域，大小
	::GetWindowRect(_hwnd, &rc);
	_width = rc.right - rc.left;
	_height = rc.bottom - rc.top;
	//step 2.获得 设备句柄
	_hdc = NULL;
	if (_render_type == RDT_NORMAL) {
		_hdc = ::GetDC(::GetDesktopWindow());
		//hdc = ::CreateDCW(L"DISPLAY", nullptr, nullptr, nullptr);
	}
	else if (_render_type == RDT_GDI) {
		_hdc = ::GetDC(_hwnd);
	}
	else if (_render_type == RDT_GDI2 || _render_type == RDT_GDI_DX2) {
		_hdc = ::GetDCEx(_hwnd, NULL, DCX_PARENTCLIP);
	}
	
	if (_hdc == NULL) {
		setlog("hdc == NULL", _hdc);
		return 0;
	}
	
	
		 //创建一个与指定设备兼容的内存设备上下文环境	
	_hmdc = CreateCompatibleDC(_hdc);
	if (_hmdc == NULL) {
		setlog("CreateCompatibleDC false");
		return -2;
	}
	_hbmpscreen = CreateCompatibleBitmap(_hdc, _width, _height); //创建与指定的设备环境相关的设备兼容的位图

	updata_screen();
	return 1;
}

long bkgdi::UnBind(HWND hwnd) {
	_hwnd = hwnd;
	return UnBind();
}

long bkgdi::UnBind() {
	//delete[dwLen_2]hDib;
	if (_hdc)DeleteDC(_hdc); _hdc = NULL;
	if (_hmdc)DeleteDC(_hmdc); _hmdc = NULL;

	if (_hbmpscreen)DeleteObject(_hbmpscreen); _hbmpscreen = NULL;
	//if (_hbmp_old)DeleteObject(_hbmp_old); _hbmp_old = NULL;
	bkdisplay::bind_release();
	return 1;
}







byte* bkgdi::get_data() {
	//首先  刷新数据
	updata_screen();

	return _shmem->data<byte>();
}

long bkgdi::updata_screen() {
	//step 1.判断 窗口是否存在
	if (!::IsWindow(_hwnd))
		return 0;

	RECT rc;
	//step 3.获取截图区域，大小
	::GetWindowRect(_hwnd, &rc);
	_width = rc.right - rc.left;
	_height = rc.bottom - rc.top;
	//::MoveWindow(_hwnd, 0, 0, _width, _height, 1);
	POINT pt;
	pt.x = rc.left; pt.y = rc.top;

	
	if (_render_type != RDT_NORMAL) {
		::ScreenToClient(_hwnd, &pt);
		//设置偏移
		_client_x = -pt.x;
		_client_y = -pt.y;
	}
	else {
		_client_x = _client_y = 0;
	}


	_bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	_bfh.bfSize = _bfh.bfOffBits + _width * _height * 4;
	_bfh.bfType = static_cast<WORD>(0x4d42);

	_bih.biBitCount = 32;//每个像素字节大小
	_bih.biCompression = BI_RGB;
	_bih.biHeight = _height;//高度
	_bih.biPlanes = 1;
	_bih.biSize = sizeof(BITMAPINFOHEADER);
	_bih.biSizeImage = _width * 4 * _height;//图像数据大小
	_bih.biWidth = _width;//宽度


	_hbmp_old = (HBITMAP)SelectObject(_hmdc, _hbmpscreen); //选择一对象到指定的设备上下文环境中

	//对指定的源设备环境区域中的像素进行位块（bit_block）转换

	if (_render_type == RDT_GDI)
		::PrintWindow(_hwnd, _hdc, 0);
	else if (_render_type == RDT_GDI2) {
		::UpdateWindow(_hwnd);
		::RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
		::PrintWindow(_hwnd, _hdc, 0);
	}
	int x1 = 0, y1 = 0;
	if (_render_type == RDT_NORMAL) {
		x1 = rc.left; y1 = rc.top;
	}

	BitBlt(_hmdc, 0, 0, _width, _height, _hdc, x1, y1,  SRCCOPY);

	//_hbmp_old = (HBITMAP)SelectObject(_hmdc, _hbmpscreen);

	_hbmpscreen = (HBITMAP)SelectObject(_hmdc, _hbmp_old);

	//函数获取指定兼容位图的位，然后将其作一个DIB—设备无关位图（Device-Independent Bitmap）使用的指定格式复制到一个缓冲区中
	_pmutex->lock();
	GetDIBits(_hmdc, _hbmpscreen, 0L, (DWORD)_height, _shmem->data<byte>(), (LPBITMAPINFO)&_bih, (DWORD)DIB_RGB_COLORS);
	_pmutex->unlock();


	return 1;

}





