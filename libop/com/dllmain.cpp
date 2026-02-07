// dllmain.cpp: DllMain 的实现。

#include "dllmain.h"
#include "../core/opEnv.h"
#include "compreg.h"
#include "op_i.h"
#include "resource.h"
CopModule _AtlModule;

// DLL 入口点
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
    opEnv::setInstance(hInstance);
    return _AtlModule.DllMain(dwReason, lpReserved);
}
