#pragma once
#ifndef __BKDISPLAY_H_
#define __BKDISPLAY_H_
#include <thread>
#include "bkdisplay.h"
class bkgdi:public bkdisplay
{
public:
	bkgdi();
	~bkgdi();
	//°ó¶¨
	long Bind(HWND _hwnd, long render_type) override;
	long UnBind(HWND hwnd);
	//½â°ó
	long UnBind() override;
	
	
	long updata_screen();

	byte* get_data() override;
	
	
private:
	
	
};

#endif

