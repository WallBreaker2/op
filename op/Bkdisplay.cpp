#include "stdafx.h"
#include "Bkdisplay.h"
#include "Common.h"
#include <fstream>
Bkdisplay::Bkdisplay():_is_cap(0)
{
	_hwnd = NULL; _mode = 0;
	_hdc = _hmdc = NULL;
	_hbmpscreen = _holdbmp = NULL;
	//4*2^22=16*2^20=16MB
	_image_data = new byte[MAX_IMAGE_WIDTH*MAX_IMAGE_WIDTH*4];
}

Bkdisplay::~Bkdisplay()
{
	SAFE_DELETE_ARRAY(_image_data);
}

long Bkdisplay::Bind(HWND hwnd, int mode) {
	if (!::IsWindow(hwnd))
		return 0;
	_hwnd = hwnd; _mode = mode;
	long ret = 0;
	RECT rc;
	::GetWindowRect(_hwnd, &rc);
	_width = rc.right - rc.left;
	_height = rc.bottom - rc.top;
	if (_mode == BACKTYPE::NORMAL) {
		_pthread = new std::thread(&Bkdisplay::cap_thread, this);
		ret = 1;
	}
	else if (_mode == BACKTYPE::GDI || _mode == BACKTYPE::WINDOWS) {
		_pthread = new std::thread(&Bkdisplay::cap_thread, this);
		ret = 1;
	}
	else if (_mode == BACKTYPE::DX) {

	}
	else {//opengl

	}
	return ret;
}

long Bkdisplay::UnBind() {
	_is_cap = 0;
	if (_pthread) {
		_pthread->join();
		SAFE_DELETE(_pthread);
	}
	return 1;
}

int Bkdisplay::cap_thread() {
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

long Bkdisplay::cap_init() {
	if (!IsWindow(_hwnd)) { _is_cap = 0; return 0; }
	_hdc = ::GetWindowDC(_hwnd);
	//_width = GetDeviceCaps(_hdc, HORZRES);    //屏幕宽度
	//_height = GetDeviceCaps(_hdc, VERTRES);   //屏幕高度		
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

long Bkdisplay::cap_release() {
	if (_holdbmp&&_hmdc)
		_hbmpscreen = (HBITMAP)SelectObject(_hmdc, _holdbmp);
	//delete[dwLen_2]hDib;
	if (_hdc)DeleteDC(_hdc); _hdc = NULL;
	if (_hmdc)DeleteDC(_hmdc); _hmdc = NULL;
	
	if (_hbmpscreen)DeleteObject(_hbmpscreen); _hbmpscreen = NULL;
	if (_holdbmp)DeleteObject(_holdbmp); _holdbmp = NULL;
	setlog(L"cap_release");
	return 0;
}

long Bkdisplay::cap_image() {
	if (!IsWindow(_hwnd)) { _is_cap = 0; return 0; }
	//对指定的源设备环境区域中的像素进行位块（bit_block）转换
	if (_mode == BACKTYPE::NORMAL)
		BitBlt(_hmdc, 0, 0, _width, _height, _hdc, 0, 0, SRCCOPY);
	else if (_mode == BACKTYPE::WINDOWS)
		::PrintWindow(_hwnd, _hmdc, 0);

	//函数获取指定兼容位图的位，然后将其作一个DIB—设备无关位图（Device-Independent Bitmap）使用的指定格式复制到一个缓冲区中
	GetDIBits(_hmdc, _hbmpscreen, 0L, (DWORD)_height, (LPBYTE)_image_data, (LPBITMAPINFO)&_bih, (DWORD)DIB_RGB_COLORS);
	return 1;
	
}

long Bkdisplay::capture(const std::wstring& file_name) {
	//setlog(L"capture");
	std::fstream file;
	file.open(file_name, std::ios::out|std::ios::binary);
	if (!file.is_open())return 0;
	_mutex.lock();
	file.write((char*)&_bfh, sizeof(BITMAPFILEHEADER));
	file.write((char*)&_bih, sizeof(BITMAPINFOHEADER));
	file.write((char*)_image_data, _bih.biSizeImage);
	_mutex.unlock();
	file.close();
	return 1;
}

long Bkdisplay::FindPic(long x1, long y1, long x2, long y2, const std::wstring& files, double sim, long& x, long &y) {
	long ret = 0;
	//setlog(L"Bkdisplay::FindPic,files=%s", files.c_str());
	if (!_is_cap) {
		return 0;

	}
	else {
		_mutex.lock();
		_imageloc.input_image(_image_data, _width, _height, _bih.biBitCount>>3);
		_mutex.unlock();
		std::vector<std::wstring> images;
		split(files, images, L"|");
		ret = _imageloc.imageloc(images, sim, x, y);
	}
	return ret;
}

