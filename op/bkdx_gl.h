#pragma once
#ifndef __DXBACKGROUND_H_
#define __DXBACKGROUND_H_


#include "bkdisplay.h"

class bkdo:public bkdisplay
{
public:
	bkdo();
	~bkdo();
	//1
	long Bind(HWND hwnd,long render_type) override;

	long UnBind(HWND hwnd);

	long UnBind() override;

	//nox mode
	long BindNox(HWND hwnd, long render_type);
	//
	long UnBindNox();
private:
	//blackbone::Process _process;

};

#endif

