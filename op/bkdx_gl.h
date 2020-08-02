#pragma once
#ifndef __DXBACKGROUND_H_
#define __DXBACKGROUND_H_


#include "bkdisplay.h"
struct Image;
class bkdo:public bkdisplay
{
public:
	bkdo();
	~bkdo();
	//1
	long Bind(HWND hwnd,long render_type) override;

	long UnBind(HWND hwnd);

	long UnBind() override;

	virtual bool requestCapture(int x1, int y1, int w, int h, Image& img)override;

	//nox mode
	long BindNox(HWND hwnd, long render_type);
	//
	long UnBindNox();
private:
	//blackbone::Process _process;

};

#endif

