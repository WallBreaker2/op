// #pragma once
#ifndef OP_HOOK_DISPLAY_HOOK_H_
#define OP_HOOK_DISPLAY_HOOK_H_
#include "../runtime/AutomationModes.h"
#include <string>

namespace op::hook {

class DisplayHook {
  public:
    /*target window hwnd*/
    static HWND render_hwnd;
    static int render_type;
    /*name of ...*/
    static std::wstring shared_res_name;
    static std::wstring mutex_name;
    static void *old_address;
    static void *hook_target;
    static bool is_hooked;
    //
    static int setup(HWND hwnd_, int render_type_);
    static int release();
};
// 以下函数用于HOOK DX9

// 此函数做以下工作
/*
1.hook相关函数
2.设置共享内存,互斥量
3.截图(hook)至共享内存
*/
void CopyImageData(char *dst_, const char *src_, int rows_, int cols_, int rowPitch, int fmt_);

} // namespace op::hook

#endif // OP_HOOK_DISPLAY_HOOK_H_
