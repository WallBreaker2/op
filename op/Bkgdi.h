#pragma once
#ifndef __BKDISPLAY_H_
#define __BKDISPLAY_H_
#include <thread>
#include "bkdisplay.h"
class Bkgdi:public bkdisplay
{
public:
	Bkgdi();
	~Bkgdi();
	//绑定
	long Bind(HWND _hwnd, long flag);
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
	
	
private:
	//截图模式
	int _mode;
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
	//截图标识
	int _is_cap;
	
	//截图线程函数
	int cap_thread();
};

#endif

