//#include "stdafx.h"
#include "winkeypad.h"
#include "./core/globalVar.h"
#include "./core/helpfunc.h"

static uint oem_code(uint key) {
	short code[256] = { 0 };
	code['q'] = 0x10; code['a'] = 0x1e;
	code['w'] = 0x11; code['s'] = 0x1f;
	code['e'] = 0x12; code['d'] = 0x20;
	code['r'] = 0x13; code['f'] = 0x21;
	code['t'] = 0x14; code['g'] = 0x22;
	code['y'] = 0x15; code['h'] = 0x23;
	code['u'] = 0x16; code['j'] = 0x24;
	code['i'] = 0x17; code['k'] = 0x25;
	code['o'] = 0x18; code['l'] = 0x26;
	code['p'] = 0x19; code[':'] = 0x27; code[';'] = 0x27;

	code['z'] = 0x2c;
	code['x'] = 0x2d;
	code['c'] = 0x2e;
	code['v'] = 0x2f;
	code['b'] = 0x30;
	code['n'] = 0x31;
	code['m'] = 0x32;
	return code[key & 0xffu];

}

winkeypad::winkeypad():bkkeypad()
{
}


winkeypad::~winkeypad()
{
	//UnBind();
}

long winkeypad::Bind(HWND hwnd, long mode) {
	if (!::IsWindow(hwnd))
		return 0;
	_hwnd = hwnd;
	_mode = mode;
	return 1;
}

long winkeypad::UnBind() {
	_hwnd = NULL;
	_mode = 0;
	return 1;
}

long winkeypad::GetKeyState(long vk_code) {
	vk_code = toupper(vk_code);
	return 0x8000 & ::GetAsyncKeyState(vk_code);
}

long winkeypad::KeyDown(long vk_code) {
	long ret = 0;
	vk_code = toupper(vk_code);

	switch (_mode) {
	case INPUT_TYPE::IN_NORMAL:
	{
	
		INPUT Input = { 0 };
		Input.type = INPUT_KEYBOARD;
		Input.ki.wVk = (WORD)vk_code;
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
		/*Specification of WM_KEYDOWN :*/

		/*wParam

			Specifies the virtual - key code of the nonsystem key.
		lParam
			Specifies the repeat count, scan code, extended - key flag, context code,
			previous key - state flag,
			and transition - state flag, as shown in the following table.
			0 - 15
			Specifies the repeat count for the current message.The value is
			the number of times the keystroke is
			autorepeated as a result of the user holding down the key.If the
			keystroke is held long enough, multiple messages are sent.However,
			the repeat count is not cumulative.
			16 - 23
			Specifies the scan code.The value depends on the OEM.
			24
			Specifies whether the key is an extended key, such as the
			right - hand ALT and CTRL keys that
			appear on an enhanced 101 - or 102 - key keyboard.The value
			is 1 if it is an extended key; otherwise, it is 0.
			25 - 28
			Reserved; do not use.
			29
			Specifies the context code.The value is always 0 for a WM_KEYDOWN message.
			30
			Specifies the previous key state.The value is 1 if the key
			is down before the message is sent, or it is zero if the key is up.
			31
			Specifies the transition state.The value is always zero for a WM_KEYDOWN message.*/

		DWORD lparam = 1u;
		if (vk_code == VK_RCONTROL)
			lparam |= 1u << 24;
		lparam |= oem_code(vk_code) << 16;
		ret = ::PostMessageW(_hwnd, WM_KEYDOWN, vk_code, lparam);
		//ret = ::SendMessageW(_hwnd, WM_KEYDOWN, vk_code, 0);
		if (ret == 0)setlog("error code=%d", GetLastError());
		break;
	}
	}


	return ret;
}

long winkeypad::KeyUp(long vk_code) {
	long ret = 0;
	vk_code = toupper(vk_code);
	switch (_mode) {
	case INPUT_TYPE::IN_NORMAL:
	{
		
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
		/*Specification of WM_KEYUP
		wParam
			Specifies the virtual - key code of the nonsystem key.
			lParam
			Specifies the repeat count, scan code, extended - key flag, context code,
			previous key - state flag, and transition - state flag, as shown in the following table.
			0 - 15
			Specifies the repeat count for the current message.The value is the number of times the keystroke is
			autorepeated as a result of the user holding down the key.
			The repeat count is always one for a WM_KEYUP message.
			16 - 23
			Specifies the scan code.The value depends on the OEM.
			24
			Specifies whether the key is an extended key, such as the right - hand ALT and CTRL keys that
			appear on an enhanced 101 - or 102 - key keyboard.The value is 1 if it is an extended key; otherwise, it is 0.
			25 - 28
			Reserved; do not use.
			29
			Specifies the context code.The value is always 0 for a WM_KEYUP message.
			30
			Specifies the previous key state.The value is always 1 for a WM_KEYUP message.
			31
			Specifies the transition state.The value is always 1 for a WM_KEYUP message.*/
			//ret = ::SendMessageW(_hwnd, WM_KEYUP, vk_code, 0);
		DWORD lparam = 1u;
		if (vk_code == VK_RCONTROL)lparam |= 1u << 24;
		lparam |= oem_code(vk_code) << 16;
		lparam |= 1u << 30;
		lparam |= 1u << 31;
		ret = ::PostMessageW(_hwnd, WM_KEYUP, vk_code, lparam);
		if (ret == 0)setlog("error2 code=%d", GetLastError());
		break;
	}
	}


	return ret;
}

long winkeypad::WaitKey(long vk_code, long time_out) {
	auto deadline = ::GetTickCount() + time_out;
	while (::GetTickCount() < deadline) {
		if (GetKeyState(vk_code))
			return 1;
		::Sleep(1);
	}
	return 0;
}

long winkeypad::KeyPress(long vk_code) {
	KeyDown(vk_code);

	return KeyUp(vk_code);
}
