#include "../runtime/AutomationModes.h"

// 描述： 设置显示Hook
// 返回值:1 成功，0失败
DLL_API long __stdcall SetDisplayHook(HWND hwnd_, int render_type_);

// 描述： 释放显示Hook
// 返回值:1 成功，0失败
DLL_API long __stdcall ReleaseDisplayHook();

// 描述： 设置输入Hook
// 返回值:1 成功，0失败
DLL_API long __stdcall SetInputHook(HWND hwnd_, int input_type_);

// 描述： 释放输入Hook
// 返回值:1 成功，0失败
DLL_API long __stdcall ReleaseInputHook();

// 描述： 锁定目标窗口的外部输入，仅作用于输入 Hook。
// 返回值:1 成功，0失败
DLL_API long __stdcall SetInputLock(int lock);

// 描述： 获取目标进程内最近一次 SetCursor 的光标 hash。
DLL_API unsigned long long __stdcall GetInputCursorShapeHash();

// 描述： 获取目标进程内最近一次 SetCursor 的光标元数据。
DLL_API unsigned long long __stdcall GetInputCursorShapeMeta();

DLL_API unsigned long __stdcall GetInputCursorShapeHashLow();
DLL_API unsigned long __stdcall GetInputCursorShapeHashHigh();
DLL_API unsigned long __stdcall GetInputCursorShapeMetaLow();
DLL_API unsigned long __stdcall GetInputCursorShapeMetaHigh();
