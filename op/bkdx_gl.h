#pragma once
#ifndef __DXBACKGROUND_H_
#define __DXBACKGROUND_H_


#include "DisplayBase.h"
struct Image;
class bkdo:public DisplayBase
{
public:
	bkdo();
	~bkdo();
	//1
	long BindEx(HWND hwnd,long render_type) override;

	/*long UnBind(HWND hwnd);*/

	long UnBindEx() override;

	virtual bool requestCapture(int x1, int y1, int w, int h, Image& img)override;

	//nox mode
	long BindNox(HWND hwnd, long render_type);
	//
	long UnBindNox();
private:
	//blackbone::Process _process;

};

#endif

