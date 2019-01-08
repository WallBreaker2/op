#pragma once
#ifndef __BKDISPLAY_H_
#define __BKDISPLAY_H_
#include <thread>
#include <mutex>
#include "ImageLoc.h"
class Bkgdi
{
public:
	Bkgdi();
	~Bkgdi();
	//绑定
	long Bind(HWND _hwnd, int mode);
	//解绑
	long UnBind();
	//截图初始化
	long cap_init();
	//截图释放
	long cap_release();
	//截图
	long cap_image();
	//截图至文件
	long capture(const std::wstring& file_name);
	//图形定位
	long FindPic(long x1, long y1, long x2, long y2, const std::wstring& files, double sim, long& x, long &y);
private:
	//截图窗口句柄
	HWND _hwnd;
	//截图模式
	int _mode;
	//屏幕像素指针
	byte* _image_data;
	//图像宽高
	long _width, _height;
	//设备句柄
	HDC _hdc;
	HDC _hmdc;
	//位图句柄
	HBITMAP _hbmpscreen;
	HBITMAP _holdbmp;
	//位图信息
	BITMAP           _bm;
	//bmp 文件头
	BITMAPFILEHEADER _bfh = { 0 };
	BITMAPINFOHEADER _bih = { 0 };//位图信息头
	//截图线程指针
	std::thread* _pthread;
	//互斥量
	std::mutex _mutex;
	//截图标识
	int _is_cap;
	//图形匹配
	ImageLoc _imageloc;
	//截图线程函数
	int cap_thread();
};

#endif

