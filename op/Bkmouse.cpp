#include "stdafx.h"
#include "Bkmouse.h"
#include "globalVar.h"

bkmouse::bkmouse():_hwnd(NULL)
{
}


bkmouse::~bkmouse()
{
	_hwnd = NULL;
}

long bkmouse::Bind(HWND h,int mode) {
	_hwnd = h;
	_mode = mode;
	return 1;
}

long bkmouse::UnBind() {
	_hwnd = 0; _mode = 0;
	return 1;
}

long bkmouse::GetCursorPos(long& x, long& y) {
	POINT pt;
	::GetCursorPos(&pt);
	x = pt.x; y = pt.y;
	return 1;
}

long bkmouse::MoveR(int rx, int ry) {
	return MoveTo(_x + rx, _y + ry);
}

long bkmouse::MoveTo(int x, int y) {

	long ret = 0;
	switch (_mode) {
	case INPUT_TYPE::IN_NORMAL:
	{
		POINT pt;
		pt.x = x, pt.y = y;
		if (_hwnd)
			::ClientToScreen(_hwnd, &pt);
		x = pt.x, y = pt.y;
		//setlog(L"hwnd:%d,pt:%d,%d",_hwnd, 0, y);
		static double fScreenWidth = ::GetSystemMetrics(SM_CXSCREEN) - 1;
		static double fScreenHeight = ::GetSystemMetrics(SM_CYSCREEN) - 1;
		double fx = x * (65535.0f / fScreenWidth);
		double fy = y * (65535.0f / fScreenHeight);
		INPUT Input = { 0 };
		Input.type = INPUT_MOUSE;
		Input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
		Input.mi.dx = static_cast<LONG>(fx);
		Input.mi.dy = static_cast<LONG>(fy);
		/*The function returns the number of events that it successfully inserted into the keyboard or mouse input stream.
		If the function returns zero, the input was already blocked by another thread.
		To get extended error information, call GetLastError.
		This function fails when it is blocked by UIPI.
		Note that neither GetLastError nor the return value will indicate the failure was caused by UIPI blocking.
		*/
		ret = ::SendInput(1, &Input, sizeof(INPUT));
		break;
	}

	case INPUT_TYPE::IN_WINDOWS: {
		ret = ::SendMessage(_hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(x, y));

		break;
	}
	}
	_x = x, _y = y;
	return ret;
}

long bkmouse::MoveToEx(int x, int y, int w, int h) {
	if (w >= 2 && h >= 2)
		return MoveTo(x + rand() % w, y + rand() % h);
	else
		return MoveTo(x, y);
}

long bkmouse::LeftClick() {
	long ret = 0, ret2 = 0;
	switch (_mode) {
	case INPUT_TYPE::IN_NORMAL: {
		INPUT Input = { 0 };
		// left down 
		Input.type = INPUT_MOUSE;
		Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		ret = ::SendInput(1, &Input, sizeof(INPUT));

		// left up
		::ZeroMemory(&Input, sizeof(INPUT));
		Input.type = INPUT_MOUSE;
		Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
		ret2 = ::SendInput(1, &Input, sizeof(INPUT));
		break;
	}

	case INPUT_TYPE::IN_WINDOWS: {
		///ret=::PostMessage(_hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(_x, _y));
		ret = ::SendMessageTimeout(_hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(_x, _y), SMTO_BLOCK, 2000, nullptr);
		//ret = ::SendNotifyMessage(_hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(_x, _y));
		//::Sleep(100);
		//ret = ::SendMessage(_hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(_x, _y));
		ret2=::SendMessageTimeout(_hwnd, WM_LBUTTONUP, 0, MAKELPARAM(_x, _y), SMTO_BLOCK, 2000, nullptr);
		//ret2 = ::SendMessage(_hwnd, WM_LBUTTONUP, 0, MAKELPARAM(_x, _y));
		//::SendMessage(_hwnd, WM_CAPTURECHANGED, 0, 0);
		break;
	}
	}
	return ret && ret2 ? 1 : 0;
}

long bkmouse::LeftDoubleClick() {
	long r1, r2;
	r1=LeftClick();
	::Sleep(1);
	r2=LeftClick();
	return r1 & r2 ? 1 : 0;
}

long bkmouse::LeftDown() {
	long ret = 0;
	switch (_mode) {
	case INPUT_TYPE::IN_NORMAL: {
		INPUT Input = { 0 };
		// left down 
		Input.type = INPUT_MOUSE;
		Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		ret = ::SendInput(1, &Input, sizeof(INPUT));
		break;
	}

	case INPUT_TYPE::IN_WINDOWS: {
		ret = ::PostMessage(_hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(_x, _y));
		break;
	}
	}
	return ret;
}

long bkmouse::LeftUp() {
	long ret = 0;
	switch (_mode) {
	case INPUT_TYPE::IN_NORMAL: {
		INPUT Input = { 0 };
		// left up
		::ZeroMemory(&Input, sizeof(INPUT));
		Input.type = INPUT_MOUSE;
		Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
		ret = ::SendInput(1, &Input, sizeof(INPUT));
		break;
	}

	case INPUT_TYPE::IN_WINDOWS: {
		ret = ::SendMessage(_hwnd, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(_x, _y));
		break;
	}
	}
	return ret;
}

long bkmouse::MiddleClick() {
	long r1, r2;
	r1=MiddleDown();
	::Sleep(1);
	r2=MiddleUp();
	return r1 & r2 ? 1 : 0;
}

long bkmouse::MiddleDown() {
	long ret = 0;
	switch (_mode) {
	case INPUT_TYPE::IN_NORMAL: {
		INPUT Input = { 0 };
		// left down 
		Input.type = INPUT_MOUSE;
		Input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
		ret = ::SendInput(1, &Input, sizeof(INPUT));
		break;
	}

	case INPUT_TYPE::IN_WINDOWS: {
		ret = ::SendMessage(_hwnd, WM_MBUTTONDOWN, MK_MBUTTON, MAKELPARAM(_x, _y));
		break;
	}

	}
	return ret;
}

long bkmouse::MiddleUp() {
	long ret = 0;
	switch (_mode) {
	case INPUT_TYPE::IN_NORMAL: {
		INPUT Input = { 0 };
		// left up
		::ZeroMemory(&Input, sizeof(INPUT));
		Input.type = INPUT_MOUSE;
		Input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
		ret = ::SendInput(1, &Input, sizeof(INPUT));
		break;
	}

	case INPUT_TYPE::IN_WINDOWS: {
		ret = ::SendMessage(_hwnd, WM_MBUTTONUP, MK_MBUTTON, MAKELPARAM(_x, _y));
		break;
	}
	}
	return ret;
}


long bkmouse::RightClick() {
	long ret = 0;
	switch (_mode) {
	case INPUT_TYPE::IN_NORMAL: {
		INPUT Input = { 0 };
		// left down 
		Input.type = INPUT_MOUSE;
		Input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
		ret=::SendInput(1, &Input, sizeof(INPUT));

		// left up
		::ZeroMemory(&Input, sizeof(INPUT));
		Input.type = INPUT_MOUSE;
		Input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
		ret=::SendInput(1, &Input, sizeof(INPUT));
		break;
	}

	case INPUT_TYPE::IN_WINDOWS: {
		ret=::SendMessage(_hwnd, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(_x, _y));
		ret=::SendMessage(_hwnd, WM_RBUTTONUP, MK_RBUTTON, MAKELPARAM(_x, _y));
		break;
	}

	}
	return ret;
}

long bkmouse::RightDown() {
	long ret = 0;
	switch (_mode) {
	case INPUT_TYPE::IN_NORMAL: {
		INPUT Input = { 0 };
		// left down 
		Input.type = INPUT_MOUSE;
		Input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
		ret = ::SendInput(1, &Input, sizeof(INPUT));
		break;
	}
	case	INPUT_TYPE::IN_WINDOWS: {
		ret = ::PostMessage(_hwnd, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(_x, _y));
		break;
	}

	}
	return ret;
}

long bkmouse::RightUp() {
	long ret = 0;
	switch (_mode) {
	case INPUT_TYPE::IN_NORMAL: {
		INPUT Input = { 0 };
		// left up
		::ZeroMemory(&Input, sizeof(INPUT));
		Input.type = INPUT_MOUSE;
		Input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
		ret = ::SendInput(1, &Input, sizeof(INPUT));
		break;
	}

	case INPUT_TYPE::IN_WINDOWS: {
		ret = ::PostMessage(_hwnd, WM_RBUTTONUP, MK_RBUTTON, MAKELPARAM(_x, _y));
		break;
	}
	}
	return ret;
}

long bkmouse::WheelDown() {
	long ret = 0;
	switch (_mode) {
	case INPUT_TYPE::IN_NORMAL: {
		INPUT Input = { 0 };
		//down 
		/*
		If dwFlags contains MOUSEEVENTF_WHEEL, then dwData specifies the amount of wheel movement.
		A positive value indicates that the wheel was rotated forward, away from the user;
		a negative value indicates that the wheel was rotated backward, toward the user. 
		One wheel click is defined as WHEEL_DELTA, which is 120.
		*/
		Input.type = INPUT_MOUSE;
		Input.mi.dwFlags = MOUSEEVENTF_WHEEL;
		Input.mi.mouseData = -WHEEL_DELTA;
		ret = ::SendInput(1, &Input, sizeof(INPUT));
		break;
	}

	case INPUT_TYPE::IN_WINDOWS: {
		/*
		wParam
		The high-order word indicates the distance the wheel is rotated, 
		expressed in multiples or divisions of WHEEL_DELTA, which is 120. 
		A positive value indicates that the wheel was rotated forward, away from the user;
		a negative value indicates that the wheel was rotated backward, toward the user.
		The low-order word indicates whether various virtual keys are down.
		This parameter can be one or more of the following values.
		lParam
		The low-order word specifies the x-coordinate of the pointer,
		relative to the upper-left corner of the screen.
		The high-order word specifies the y-coordinate of the pointer, 
		relative to the upper-left corner of the screen.
		*/
		//If an application processes this message, it should return zero.
		ret = ::SendMessage(_hwnd, WM_MOUSEWHEEL, MAKEWPARAM(-WHEEL_DELTA,0), MAKELPARAM(_x, _y));
		break;
	}

	
	}
	
	return ret;
}

long bkmouse::WheelUp() {
	long ret = 0;
	switch (_mode) {
	case INPUT_TYPE::IN_NORMAL: {
		INPUT Input = { 0 };
		// left up
		Input.type = INPUT_MOUSE;
		Input.mi.dwFlags = MOUSEEVENTF_WHEEL;
		Input.mi.mouseData = WHEEL_DELTA;
		ret = ::SendInput(1, &Input, sizeof(INPUT));
		break;
	}

	case INPUT_TYPE::IN_WINDOWS: {
		ret = ::SendMessage(_hwnd, WM_MOUSEWHEEL, MAKEWPARAM(WHEEL_DELTA, 0), MAKELPARAM(_x, _y));
		break;
	}
	}
	return ret;
}
