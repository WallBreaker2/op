#ifndef __INPUT_HOOK_H
#define __INPUT_HOOK_H

#include "../../core/globalVar.h"


namespace InputHook {
	
	/*target window hwnd*/
	extern HWND input_hwnd;
	extern int input_type;
	/*name of ...*/
	extern wchar_t shared_res_name[256];
	extern wchar_t mutex_name[256];
	extern void* old_address;
	//
	int setup(HWND hwnd_, int input_type_);
	int release();
    int x,y;

};
//以下函数用于HOOK DX9

//此函数做以下工作
/*
1.hook相关函数
2.设置共享内存,互斥量
3.截图(hook)至共享内存
*/

//返回值:1 成功，0失败
DLL_API long __stdcall SetInputHook(HWND hwnd_, int input_type_);

DLL_API long __stdcall ReleaseInputHook();

#endif