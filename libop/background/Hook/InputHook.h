#ifndef __INPUT_HOOK_H
#define __INPUT_HOOK_H

#include "../../core/globalVar.h"

struct opMouseState
{
	LONG lAxisX;
	LONG lAxisY;
	BYTE abButtons[3];
	BYTE bPadding; // Structure must be DWORD multiple in size.
};

class InputHook
{
public:
	/*target window hwnd*/
	static HWND input_hwnd;
	static int input_type;
	/*name of ...*/
	static wchar_t shared_res_name[256];
	static wchar_t mutex_name[256];
	static void *old_address;
	static bool is_hooked;
	//
	static int setup(HWND hwnd_, int input_type_);
	static int release();
	//LParam is pos,key:-1-2,means null, left mid and right, down means keyState
	static void upDataPos(LPARAM, int key, bool down);
	static opMouseState m_mouseState;
};

#endif