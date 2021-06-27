#include "HookExport.h"
#include "DisplayHook.h"
#include "InputHook.h"
#include "../../core/opEnv.h"
int refCount = 0;
//--------------export function--------------------------
long __stdcall SetDisplayHook(HWND hwnd_, int render_type_)
{
    int ret = 0;
    opEnv::m_showErrorMsg = 2; //this code is excuate in hookde process,so its better not show meesageBox(avoid suspend the work thread)
    if (!DisplayHook::is_hooked)
    {
      ret  = DisplayHook::setup(hwnd_, render_type_);
      DisplayHook::is_hooked = ret == 1;
      refCount += DisplayHook::is_hooked;
    }
    else {
        //setlog("warning: ")
        ret = 1;
    }
    return ret;
}

long __stdcall ReleaseDisplayHook()
{
    int ret = 0;
    if (DisplayHook::is_hooked){
        DisplayHook::is_hooked = false;
        ret = DisplayHook::release();
        refCount--;
        if(refCount==0){
            ::FreeLibraryAndExitThread(static_cast<HMODULE>(opEnv::getInstance()), 0);
        }
        
    }
  
  
    return ret;
}

long __stdcall SetInputHook(HWND hwnd_, int input_type_)
{
    int ret = 0;
    if (!InputHook::is_hooked)
    {
        ret = InputHook::setup(hwnd_, input_type_);
        InputHook::is_hooked = ret == 1;
         refCount += InputHook::is_hooked;
    }
    return ret;
}

long __stdcall ReleaseInputHook()
{
    if (InputHook::is_hooked)
    {
        InputHook::release();
        InputHook::is_hooked = false;
         refCount--;
        if(refCount==0){
            ::FreeLibraryAndExitThread(static_cast<HMODULE>(opEnv::getInstance()), 0);
        }
    }
    return 1;
}
