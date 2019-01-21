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
//dx9
DLL_API long SetDX9Hook(HWND hwnd);
//恢复原状态，释放共享内存
DLL_API long UnDX9Hook();
//dx10
DLL_API long SetDX10Hook(HWND hwnd);
DLL_API long UnDX10Hook();
//dx11
DLL_API long SetDX11Hook(HWND hwnd);
DLL_API long UnDX11Hook();
//opengl
DLL_API long SetOpenglHook(HWND hwnd);
DLL_API long UnOpenglHook();

#endif // !__DX9HOOK_H_
