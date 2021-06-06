#include "../../core/globalVar.h"


//描述： 设置显示Hook
//返回值:1 成功，0失败
DLL_API long __stdcall SetDisplayHook(HWND hwnd_, int render_type_);

//描述： 释放显示Hook
//返回值:1 成功，0失败
DLL_API long __stdcall ReleaseDisplayHook();

//描述： 设置输入Hook
//返回值:1 成功，0失败
DLL_API long __stdcall SetInputHook(HWND hwnd_, int input_type_);

//描述： 释放输入Hook
//返回值:1 成功，0失败
DLL_API long __stdcall ReleaseInputHook();
