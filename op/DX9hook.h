#pragma once
#ifndef __DX9HOOK_H_
#define __DX9HOOK_H_
#include "Common.h"
//以下函数用于HOOK DX9
//此函数做以下工作
/*
1.hook相关函数
2.设置共享内存,互斥量
3.截图(hook)至共享内存
*/
DLL_API long SetDX9Hook(HWND hwnd);
//恢复原状态，释放共享内存
DLL_API long UnDX9Hook();


#endif // !__DX9HOOK_H_
