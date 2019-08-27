// dllmain.cpp: DllMain 的实现。

#include "stdafx.h"
#include "resource.h"
#include "op_i.h"
#include "dllmain.h"
#include "compreg.h"
#include "globalVar.h"
CopModule _AtlModule;

// DLL 入口点
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	gInstance= hInstance;
	return _AtlModule.DllMain(dwReason, lpReserved);
}
