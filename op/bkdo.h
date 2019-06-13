#pragma once
#ifndef __DXBACKGROUND_H_
#define __DXBACKGROUND_H_

#include "Common.h"

#include "bkdisplay.h"

using std::wstring;
class bkdo:public bkdisplay
{
public:
	bkdo();
	~bkdo();
	//1
	long Bind(HWND hwnd,long render_type) override;

	long UnBind() override;

	
	//╫ьм╪жанд╪Ч
	long capture(const std::wstring& file_name);
private:
	//blackbone::Process _process;

	

	std::wstring _dllname;
	
};

#endif

