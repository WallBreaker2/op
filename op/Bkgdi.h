#pragma once
#ifndef __BKDISPLAY_H_
#define __BKDISPLAY_H_
#include <thread>
#include "optype.h"
#include "bkdisplay.h"
class bkgdi:public bkdisplay
{
public:
	bkgdi();
	~bkgdi();
	//绑定
	long Bind(HWND _hwnd, long render_type) override;
	long UnBind(HWND hwnd);
	//解绑
	long UnBind() override;
	
	
	long updata_screen();

	byte* get_data() override;
	
	
private:
	//设备句柄
	HDC _hdc = NULL;

	HDC _hmdc = NULL;
	//位图句柄
	HBITMAP _hbmpscreen = NULL;
	HBITMAP _hbmp_old = NULL;
	//bmp 文件头
	BITMAPFILEHEADER _bfh = { 0 };
	BITMAPINFOHEADER _bih = { 0 };//位图信息头
	int dx_, dy_;//去除标题栏
	bytearray temp_src;
};

#endif

