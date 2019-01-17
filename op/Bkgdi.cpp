#include "stdafx.h"
#include "Bkgdi.h"
#include "Common.h"
#include <fstream>
Bkgdi::Bkgdi() :_is_cap(0), _pthread(nullptr)
{
	_hwnd = NULL; _mode = 0;
	_hdc = _hmdc = NULL;
	_hbmpscreen = _holdbmp = NULL;
	//4*2^22=16*2^20=16MB
	_image_data = new byte[MAX_IMAGE_WIDTH*MAX_IMAGE_WIDTH * 4];
}

Bkgdi::~Bkgdi()
{
	SAFE_DELETE_ARRAY(_image_data);
}

long Bkgdi::Bind(HWND hwnd, int mode) {
	if (!::IsWindow(hwnd))
		return 0;
	_hwnd = hwnd; _mode = mode;
	long ret = 0;
	
	_pthread = new std::thread(&Bkgdi::cap_thread, this);
	ret = 1;
	return ret;
}

long Bkgdi::UnBind() {
	_is_cap = 0;
	if (_pthread) {
		_pthread->join();
		SAFE_DELETE(_pthread);
	}
	return 1;
}

int Bkgdi::cap_thread() {
	_is_cap = 1;
	cap_init();
	while (_is_cap) {
		_mutex.lock();
		//do cap
		cap_image();
		_mutex.unlock();
		//sleep some time to free cpu
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	cap_release();
	return 0;
}

long Bkgdi::cap_init() {
	if (!IsWindow(_hwnd)) { _is_cap = 0; return 0; }
	_hdc = ::GetWindowDC(_hwnd);
	RECT rc;
	
	::GetWindowRect(_hwnd, &rc);
	_width = rc.right - rc.left;
	_height = rc.bottom - rc.top;
	POINT pt;
	pt.x = rc.left; pt.y = rc.top;
	::ScreenToClient(_hwnd, &pt);
	//setlog("WindowRect:%d,%d", pt.x,pt.y);
	//setlog("Window:%d,%d", _width, _height);
	//设置偏移
	::GetClientRect(_hwnd, &rc);
	_client_x = -pt.x;
	_client_y = -pt.y;
	//setlog("ClientRect:%d,%d,%d,%d", rc.left,rc.top,rc.right,rc.bottom);

	_hmdc = CreateCompatibleDC(_hdc); //创建一个与指定设备兼容的内存设备上下文环境		
	_hbmpscreen = CreateCompatibleBitmap(_hdc, _width, _height); //创建与指定的设备环境相关的设备兼容的位图

	_holdbmp = (HBITMAP)SelectObject(_hmdc, _hbmpscreen); //选择一对象到指定的设备上下文环境中


	GetObject(_hbmpscreen, sizeof(_bm), (LPSTR)&_bm); //得到指定图形对象的信息	

	_bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	_bfh.bfSize = _bfh.bfOffBits + _bm.bmWidthBytes*_bm.bmHeight;
	_bfh.bfType = static_cast<WORD>(0x4d42);

	_bih.biBitCount = _bm.bmBitsPixel;//每个像素字节大小
	_bih.biCompression = BI_RGB;
	_bih.biHeight = _bm.bmHeight;//高度
	_bih.biPlanes = 1;
	_bih.biSize = sizeof(BITMAPINFOHEADER);
	_bih.biSizeImage = _bm.bmWidthBytes * _bm.bmHeight;//图像数据大小
	_bih.biWidth = _bm.bmWidth;//宽度
	//dwLen_1 = bi.biSize + ncolors * sizeof(RGBQUAD);
	//dwLen_2 = dwLen_1 + bi.biSizeImage;
	//hDib = new char[dwLen_2];
	//memcpy(hDib, &bi, sizeof(bi));

	//buf = hDib + dwLen_1;
	//buf_len = bi.biSizeImage;
	//setlog(L"check bih:biBitCount=%d,biCompression=%d,biHeight=%d,biWidth=%d",
	//	_bih.biBitCount, _bih.biCompression, _bih.biHeight, _bih.biWidth);
	return 1;
}

long Bkgdi::cap_release() {
	if (_holdbmp&&_hmdc)
		_hbmpscreen = (HBITMAP)SelectObject(_hmdc, _holdbmp);
	//delete[dwLen_2]hDib;
	if (_hdc)DeleteDC(_hdc); _hdc = NULL;
	if (_hmdc)DeleteDC(_hmdc); _hmdc = NULL;

	if (_hbmpscreen)DeleteObject(_hbmpscreen); _hbmpscreen = NULL;
	if (_holdbmp)DeleteObject(_holdbmp); _holdbmp = NULL;
	//setlog(L"cap_release");
	return 0;
}

long Bkgdi::cap_image() {
	if (!IsWindow(_hwnd)) { _is_cap = 0; return 0; }
	//对指定的源设备环境区域中的像素进行位块（bit_block）转换
	if (_mode == BACKTYPE::NORMAL)
		BitBlt(_hmdc, 0, 0, _width, _height, _hdc, 0, 0, SRCCOPY);
	else if (_mode == BACKTYPE::GDI)
		::PrintWindow(_hwnd, _hmdc, 0);

	//函数获取指定兼容位图的位，然后将其作一个DIB—设备无关位图（Device-Independent Bitmap）使用的指定格式复制到一个缓冲区中
	GetDIBits(_hmdc, _hbmpscreen, 0L, (DWORD)_height, (LPBYTE)_image_data, (LPBITMAPINFO)&_bih, (DWORD)DIB_RGB_COLORS);
	return 1;

}

long Bkgdi::capture(const std::wstring& file_name) {
	//setlog(L"Bkgdi::capture");
	std::fstream file;
	file.open(file_name, std::ios::out | std::ios::binary);
	if (!file.is_open())return 0;
	_mutex.lock();
	file.write((char*)&_bfh, sizeof(BITMAPFILEHEADER));
	file.write((char*)&_bih, sizeof(BITMAPINFOHEADER));
	file.write((char*)_image_data, _bih.biSizeImage);
	_mutex.unlock();
	file.close();
	return 1;
}



