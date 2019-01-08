#pragma once
#ifndef __DXBACKGROUND_H_
#define __DXBACKGROUND_H_
#include <BlackBone/Process/Process.h>
//#include <BlackBone/Patterns/PatternSearch.h>
#include <BlackBone/Process/RPC/RemoteFunction.hpp>
//#include <BlackBone/Syscalls/Syscall.h>
#include "Common.h"

long CreateSharedMemory(const std::string& name);

long ReleaseSharedMemory(const std::string& name);

//此函数做以下工作
/*
1.hook相关函数
2.设置共享内存
3.截图(hook)至共享内存
*/
DLL_API long SetDX9Hook(HWND hwnd);
//恢复原状态，释放共享内存
DLL_API long UnDX9Hook();




using std::wstring;
class DXBackground
{
public:
	DXBackground();
	~DXBackground();
	long Bind(HWND hwnd);
	long UnBind();
	//截图至文件
	long capture(const std::wstring& file_name);
private:
	HWND _hwnd;
	wstring _dllname;
	long _width, _height;
	blackbone::Process _process;
};

#endif

