//#pragma once
#ifndef __DX9HOOK_H_
#define __DX9HOOK_H_
#include "../../core/globalVar.h"

class DisplayHook
{
public:
	/*target window hwnd*/
	static HWND render_hwnd;
	static int render_type;
	/*name of ...*/
	static wchar_t shared_res_name[256];
	static wchar_t mutex_name[256];
	static void *old_address;
	static bool is_hooked ;
	//
	static int setup(HWND hwnd_, int render_type_);
	static int release();
};
//以下函数用于HOOK DX9

//此函数做以下工作
/*
1.hook相关函数
2.设置共享内存,互斥量
3.截图(hook)至共享内存
*/

#endif // !__DX9HOOK_H_