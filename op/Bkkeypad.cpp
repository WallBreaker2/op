#include "stdafx.h"
#include "Bkkeypad.h"
#include "globalVar.h"
#include "helpfunc.h"
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
	long ret = 0;
	switch (_mode) {
	case INPUT_TYPE::IN_NORMAL:
	{
		POINT pt;
	
	
		INPUT Input = { 0 };
		Input.type = INPUT_KEYBOARD;
		Input.ki.wVk = vk_code;
		Input.ki.dwFlags = 0;
		
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
		ret = ::PostMessageW(_hwnd, WM_KEYDOWN, vk_code, 1u|(3u<<30));
		//ret = ::SendMessageW(_hwnd, WM_KEYDOWN, vk_code, 0);
		if (ret == 0)setlog("error code=%d", GetLastError());
		break;
	}
	}
	
	
	return ret;
}

long bkkeypad::KeyUp(long vk_code) {
	long ret = 0;
	switch (_mode) {
	case INPUT_TYPE::IN_NORMAL:
	{
		POINT pt;


		INPUT Input = { 0 };
		Input.type = INPUT_KEYBOARD;
		Input.ki.wVk = vk_code;
		Input.ki.dwFlags = KEYEVENTF_KEYUP;

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
		//ret = ::SendMessageW(_hwnd, WM_KEYUP, vk_code, 0);
		ret = ::PostMessageW(_hwnd, WM_KEYUP, vk_code, 1);
		if (ret == 0)setlog("error2 code=%d", GetLastError());
		break;
	}
	}


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
	//Sleep(50);
	return KeyUp(vk_code);
}
