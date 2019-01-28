#include "stdafx.h"
#include "Bkkeypad.h"


bkkeypad::bkkeypad()
{
}


bkkeypad::~bkkeypad()
{
}

long bkkeypad::Bind(HWND hwnd, long mode) {
	if (!::IsWindow(hwnd))
		return 0;
	_hwnd = hwnd;
	_mode = mode;
	return 1;
}

long bkkeypad::UnBind() {
	return 1;
}

long bkkeypad::GetKeyState(long vk_code) {
	return 0x8000 & ::GetAsyncKeyState(vk_code);
}

long bkkeypad::KeyDown(long vk_code) {
	return 0;
}

long bkkeypad::KeyUp(long vk_code) {
	return 0;
}

long bkkeypad::WaitKey(long vk_code, long time_out) {
	auto deadline = ::GetTickCount() + time_out;
	while (::GetTickCount() < deadline) {
		if (GetKeyState(vk_code))
			return 1;
		::Sleep(1);
	}
	return 0;
}
