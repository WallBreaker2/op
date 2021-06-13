#pragma once
#ifndef __BKDISPLAY_H_
#define __BKDISPLAY_H_
#include <thread>
#include "optype.h"
#include "IDisplay.h"
struct Image;
class opGDI:public IDisplay
{
public:
	opGDI();
	~opGDI();
	//绑定
	long BindEx(HWND _hwnd, long render_type) override;
	//long UnBind(HWND hwnd);
	//解绑
	long UnBindEx() override;
	
	
	//long updata_screen();

	//byte* get_data() override;

	virtual bool requestCapture(int x1, int y1, int w, int h, Image& img)override;
	
private:
	//设备句柄
	HDC _hdc = NULL;
	int _device_caps = 0;
	HDC _hmdc = NULL;
	//位图句柄
	HBITMAP _hbmpscreen = NULL;
	HBITMAP _hbmp_old = NULL;
	//bmp 文件头
	BITMAPFILEHEADER _bfh = { 0 };
	BITMAPINFOHEADER _bih = { 0 };//位图信息头
	int dx_, dy_;//去除标题栏
	//bytearray temp_src;
	FrameInfo m_frameInfo;
	void fmtFrameInfo(void* dst,HWND hwnd, int w, int h);
};

#endif