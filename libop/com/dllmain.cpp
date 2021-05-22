// dllmain.cpp: DllMain 的实现。


#include "resource.h"
#include "op_i.h"
#include "dllmain.h"
#include "compreg.h"
#include "../libop/libop.h"
#include "../libop/background/xhook.h"
CopModule _AtlModule;

// DLL 入口点
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	libop::init(hInstance);
	return _AtlModule.DllMain(dwReason, lpReserved);
}
