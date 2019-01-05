#include "stdafx.h"
#include "Bkkeypad.h"


Bkkeypad::Bkkeypad()
{
}


Bkkeypad::~Bkkeypad()
{
}

long Bkkeypad::Bind(HWND hwnd, long mode) {
	if (!::IsWindow(hwnd))
		return 0;
	_hwnd = hwnd;
	_mode = mode;
}

long Bkkeypad::UnBind() {
	return 1;
}
