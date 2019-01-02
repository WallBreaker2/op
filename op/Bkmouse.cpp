#include "stdafx.h"
#include "Bkmouse.h"


Bkmouse::Bkmouse():_hwnd(NULL)
{
}


Bkmouse::~Bkmouse()
{
	_hwnd = NULL;
}

long Bkmouse::Bind(HWND h,int mode) {
	_hwnd = h;
	_mode = mode;
	return 1;
}

long Bkmouse::UnBind() {
	_hwnd = 0; _mode = 0;
	return 1;
}

long Bkmouse::MoveTo(int x, int y) {
	
	long ret = 0;
	switch (_mode) {
	case BACKTYPE::NORMAL:
	{
		POINT pt;
		pt.x = x, pt.y = y;
		::ClientToScreen(_hwnd, &pt);
		x = pt.x, y = pt.y;
		setlog(L"hwnd:%d,pt:%d,%d",_hwnd, 0, y);
		static double fScreenWidth = ::GetSystemMetrics(SM_CXSCREEN) - 1;
		static double fScreenHeight = ::GetSystemMetrics(SM_CYSCREEN) - 1;
		double fx = x * (65535.0f / fScreenWidth);
		double fy = y * (65535.0f / fScreenHeight);
		INPUT Input = { 0 };
		Input.type = INPUT_MOUSE;
		Input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
		Input.mi.dx = static_cast<LONG>(fx);
		Input.mi.dy = static_cast<LONG>(fy);
		ret=::SendInput(1, &Input, sizeof(INPUT));
		break;
	}
		
	case BACKTYPE::WINDOWS: {
		ret=::PostMessage(_hwnd, WM_MOUSEMOVE, 0,MAKELPARAM(x,y));
		
		break;
	}
		
	case BACKTYPE::DX: {
		break;
	}
		
	case BACKTYPE::OPENGL: {
		break;
	}
	}
	_x = x, _y = y;
	return ret;
}


long Bkmouse::LeftClick() {
	long ret = 0;
	switch (_mode) {
	case BACKTYPE::NORMAL: {
		INPUT Input = { 0 };
		// left down 
		Input.type = INPUT_MOUSE;
		Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		ret=::SendInput(1, &Input, sizeof(INPUT));

		// left up
		::ZeroMemory(&Input, sizeof(INPUT));
		Input.type = INPUT_MOUSE;
		Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
		ret=::SendInput(1, &Input, sizeof(INPUT));
		break;
	}

	case BACKTYPE::WINDOWS: {
		ret=::PostMessage(_hwnd, WM_LBUTTONDOWN, MK_LBUTTON,MAKELPARAM(_x, _y));

		ret=::SendMessage(_hwnd, WM_LBUTTONUP, MK_LBUTTON,  MAKELPARAM(_x, _y));
		break;
	}

	case BACKTYPE::DX: {
		break;
	}

	case BACKTYPE::OPENGL: {
		break;
	}
	}
	return ret;
}


long Bkmouse::RightClick() {
	long ret = 0;
	switch (_mode) {
	case BACKTYPE::NORMAL: {
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

	case BACKTYPE::WINDOWS: {
		ret=::PostMessage(_hwnd, WM_RBUTTONDOWN, MK_RBUTTON, MAKELPARAM(_x, _y));
		ret=::SendMessage(_hwnd, WM_RBUTTONUP, MK_RBUTTON, MAKELPARAM(_x, _y));
		break;
	}

	case BACKTYPE::DX: {
		break;
	}

	case BACKTYPE::OPENGL: {
		break;
	}
	}
	return ret;
}
