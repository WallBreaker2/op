#include "stdafx.h"
#include "Bkgdi.h"
#include "globalVar.h"
#include "helpfunc.h"
#include <fstream>

bkgdi::bkgdi()
{
	_render_type = 0;
	dx_ = 0; dy_ = 0;
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
	//
	if (render_type == RDT_NORMAL) {
		RECT rc, rc2;
		::GetWindowRect(_hwnd, &rc);
		::GetClientRect(hwnd, &rc2);

		_width = rc2.right - rc2.left;
		_height = rc2.bottom - rc2.top;
		POINT pt = { 0 };
		::ClientToScreen(hwnd, &pt);
		dx_ = pt.x - rc.left;
		dy_ = pt.y - rc.top;
		_hdc = ::GetDC(::GetDesktopWindow());
	}
	else {//client size
		RECT rc,rc2;
		::GetWindowRect(_hwnd, &rc);
		::GetClientRect(hwnd, &rc2);
		_width = rc2.right - rc2.left;
		_height = rc2.bottom - rc2.top;
		POINT pt = { 0 };
		::ClientToScreen(hwnd, &pt);
		dx_ = pt.x - rc.left;
		dy_ = pt.y - rc.top;
		if (_render_type == RDT_GDI) {
			_hdc = ::GetDC(_hwnd);
		}
		else {
			_hdc = ::GetDCEx(_hwnd, NULL, DCX_PARENTCLIP);
		}
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

	updata_screen();
	return 1;
}

long bkgdi::UnBind(HWND hwnd) {
	_hwnd = hwnd;
	return UnBind();
}

long bkgdi::UnBind() {
	_hbmpscreen = (HBITMAP)SelectObject(_hmdc, _hbmp_old);
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
	if (_render_type == RDT_NORMAL) {//normal 拷贝的大小为实际需要的大小
		//
		int w = rect.right - rect.left;
		int h = rect.bottom - rect.top;
		_hbmpscreen = CreateCompatibleBitmap(_hdc, w, h); //创建与指定的设备环境相关的设备兼容的位图
		_hbmp_old = (HBITMAP)SelectObject(_hmdc, _hbmpscreen); //选择一对象到指定的设备上下文环境中

		_bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		_bfh.bfSize = _bfh.bfOffBits + w * h * 4;
		_bfh.bfType = static_cast<WORD>(0x4d42);

		_bih.biBitCount = 32;//每个像素字节大小
		_bih.biCompression = BI_RGB;
		_bih.biHeight = h;//高度
		_bih.biPlanes = 1;
		_bih.biSize = sizeof(BITMAPINFOHEADER);
		_bih.biSizeImage = w * h * 4;//图像数据大小
		_bih.biWidth = w;//宽度

		//对指定的源设备环境区域中的像素进行位块（bit_block）转换

		RECT rc;
		::GetWindowRect(_hwnd, &rc);
		int src_x = rect.left + rc.left + dx_;
		int src_y = rect.top + rc.top + dy_;
		if (BitBlt(_hmdc, 0, 0, w, h, _hdc, src_x, src_y, SRCCOPY)) {
			//ok
		}
		else {
			setlog("error in bitbit");
		}



		//函数获取指定兼容位图的位，然后将其作一个DIB—设备无关位图（Device-Independent Bitmap）使用的指定格式复制到一个缓冲区中
		_pmutex->lock();
		GetDIBits(_hmdc, _hbmpscreen, 0L, (DWORD)h, _shmem->data<byte>(), (LPBITMAPINFO)&_bih, (DWORD)DIB_RGB_COLORS);

		_pmutex->unlock();

		if (_hbmpscreen)DeleteObject(_hbmpscreen); _hbmpscreen = NULL;
	}
	else {//gdi ... 由于printwindow 函数的原因 截取大小为实际的窗口大小，在后续的处理中，需要转化成客户区大小
		//
		RECT rc;
		::GetWindowRect(_hwnd, &rc);
		int w = rc.right - rc.left;
		int h = rc.bottom - rc.top;
		//setlog("_w w=%d %d _h h=%d %d,dx=%d dy=%d", _width, w, _height, h, dx_, dy_);
		_hbmpscreen = CreateCompatibleBitmap(_hdc, w, h); //创建与指定的设备环境相关的设备兼容的位图
		_hbmp_old = (HBITMAP)SelectObject(_hmdc, _hbmpscreen); //选择一对象到指定的设备上下文环境中

		_bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		_bfh.bfSize = _bfh.bfOffBits + w * h * 4;
		_bfh.bfType = static_cast<WORD>(0x4d42);

		_bih.biBitCount = 32;//每个像素字节大小
		_bih.biCompression = BI_RGB;
		_bih.biHeight = h;//高度
		_bih.biPlanes = 1;
		_bih.biSize = sizeof(BITMAPINFOHEADER);
		_bih.biSizeImage = w * h * 4;//图像数据大小
		_bih.biWidth = w;//宽度

		//对指定的源设备环境区域中的像素进行位块（bit_block）转换
	
		if (_render_type == RDT_GDI) {
			::PrintWindow(_hwnd, _hmdc, 0);

		}
		else {
			::UpdateWindow(_hwnd);
			//::RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
			::PrintWindow(_hwnd, _hmdc, 0);
		}
		//函数获取指定兼容位图的位，然后将其作一个DIB—设备无关位图（Device-Independent Bitmap）使用的指定格式复制到一个缓冲区中
		temp_src.resize(_bih.biSizeImage);
		GetDIBits(_hmdc, _hbmpscreen, 0L, (DWORD)h, temp_src.data(), (LPBITMAPINFO)&_bih, (DWORD)DIB_RGB_COLORS);
		_pmutex->lock();
		auto pdst = _shmem->data<uchar>();
		for (int i = 0; i < _height; i++) {
			memcpy(pdst + i * _width * 4,
				temp_src.data() + (i + 0) * w * 4 + dx_ * 4,
				_width * 4);
		}
			
		_pmutex->unlock();

		if (_hbmpscreen)DeleteObject(_hbmpscreen); _hbmpscreen = NULL;
		
	}
	return 1;

}





