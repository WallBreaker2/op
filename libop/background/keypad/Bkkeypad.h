#pragma once
#include "core/optype.h"
class bkkeypad
{
public:
	bkkeypad();

	virtual ~bkkeypad();

	virtual long Bind(HWND hwnd, long mode) = 0;

	virtual long UnBind();

	virtual long GetKeyState(long vk_code) = 0;

	virtual long KeyDown(long vk_code) = 0;

	//virtual long GetKeyState(long vk_code);

	virtual long KeyUp(long vk_code) = 0;

	virtual long WaitKey(long vk_code, long time_out) = 0;

	virtual long KeyPress(long vk_code) = 0;
protected:
	HWND _hwnd;
	int _mode;
};




