#include "stdafx.h"
#include "BackGround.h"


Background::Background() :_hwnd(0),_is_bind(0)
{
}


Background::~Background()
{
}

long Background::Bind(int hwnd, int display, int mouse, int keypad, int mode) {
	_hwnd = hwnd;
	return 0;
}

long Background::UnBind() {
	_is_bind = _hwnd = 0;
	return 1;
}

long Background::GetBindWindow() {
	return _hwnd;
}

long Background::IsBind() {
	return _is_bind==1;
}

long Background::GetCursorPos(int&x, int&y) {
	POINT pt;
	auto r=::GetCursorPos(&pt);
	x = pt.x; y = pt.y;
	return r;
}

long Background::GetKeyState(int vk_code) {
	return ::GetAsyncKeyState(vk_code);
}

long Background::KeyDown(int vk_code) {
	return 0;
}

long Background::KeyUp(int vk_code) {
	return 0;
}

long Background::LeftClick() {
	INPUT Input = { 0 };
	// left down 
	Input.type = INPUT_MOUSE;
	Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	::SendInput(1, &Input, sizeof(INPUT));

	// left up
	::ZeroMemory(&Input, sizeof(INPUT));
	Input.type =INPUT_MOUSE;
	Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	::SendInput(1, &Input, sizeof(INPUT));
	return 1;
}

long Background::RightClick() {
	INPUT Input = { 0 };
	// left down 
	Input.type = INPUT_MOUSE;
	Input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
	::SendInput(1, &Input, sizeof(INPUT));

	// left up
	::ZeroMemory(&Input, sizeof(INPUT));
	Input.type = INPUT_MOUSE;
	Input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
	::SendInput(1, &Input, sizeof(INPUT));
	return 1;
}

long Background::MoveTo(long x, long y) {
	double fScreenWidth = ::GetSystemMetrics(SM_CXSCREEN) - 1;
	double fScreenHeight = ::GetSystemMetrics(SM_CYSCREEN) - 1;
	double fx = x * (65535.0f / fScreenWidth);
	double fy = y * (65535.0f / fScreenHeight);
	INPUT Input = { 0 };
	Input.type=INPUT_MOUSE;
	Input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
	Input.mi.dx = static_cast<LONG>(fx);
	Input.mi.dy = static_cast<LONG>(fy);
	::SendInput(1, &Input, sizeof(INPUT));
	return 0;
}


