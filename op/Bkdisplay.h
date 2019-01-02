#pragma once
#ifndef __BKDISPLAY_H_
#define __BKDISPLAY_H_
#include <thread>
#include <mutex>
#include "ImageLoc.h"
class Bkdisplay
{
public:
	Bkdisplay();
	~Bkdisplay();
	long Bind(HWND _hwnd, int mode);
	long UnBind();
	long cap_init();
	long cap_release();
	long cap_image();
	long capture(const std::wstring& file_name);
	long FindPic(long x1, long y1, long x2, long y2, const std::wstring& files, double sim, long& x, long &y);
private:
	HWND _hwnd;
	int _mode;
	byte* _image_data;
	long _width, _height;
	HDC _hdc;
	HDC _hmdc;
	HDC _hhh;
	HBITMAP _hbmpscreen;
	HBITMAP _holdbmp;
	BITMAP           _bm;    //bmp图像效果好，非压缩；jpg压缩，便于网络传输	
	BITMAPFILEHEADER _bfh = { 0 };
	BITMAPINFOHEADER _bih = { 0 };//位图信息头
	std::thread* _pthread;
	std::mutex _mutex;
	int _is_cap;
	ImageLoc _imageloc;
	int cap_thread();
};

#endif

