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
	_hwnd = NULL;
	_mode = 0;
	return 1;
}

long bkkeypad::GetKeyState(long vk_code) {
	return 0x8000 & ::GetAsyncKeyState(vk_code);
}

long bkkeypad::KeyDown(long vk_code) {
	auto ret=::PostMessageW(_hwnd, WM_KEYDOWN, vk_code, 0);
	return ret;
}

long bkkeypad::KeyUp(long vk_code) {
	auto ret=::PostMessageW(_hwnd, WM_KEYUP, vk_code, 1);
	return ret;
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

long bkkeypad::KeyPress(long vk_code) {
	KeyDown(vk_code);
	Sleep(1);
	return KeyUp(vk_code);
}
