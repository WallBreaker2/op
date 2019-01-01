#pragma once
#include "Type.h"
class Bkmouse
{
public:
	Bkmouse();
	~Bkmouse();
	long Bind(HWND h,int mode);
	long MoveTo(int x, int y);
	long LeftClick();
	long RightClick();
private:
	HWND _hwnd;
	int _mode;
	int _x,_y;
};

