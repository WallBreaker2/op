// #pragma once
#ifndef OP_HOOK_DISPLAY_HOOK_H_
#define OP_HOOK_DISPLAY_HOOK_H_
#include "../base/AutomationModes.h"
#include <atomic>
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
    static bool capture_enabled();
    static void set_capture_enabled(bool enabled);

    // 正在 detour 里的线程数。release() 用它判断「现在能不能安全地摘钩」：
    // 目标进程每秒几十次进 detour，解绑请求随时可能落在某次捕获中间。
    // 以前 release() 直接 MH_RemoveHook + 清空 old_address/render_hwnd，
    // 于是那次已经在跑的 detour 回去调用空的 old_address（崩溃/挂死），
    // 或者拿 render_hwnd=NULL 去 GetClientRect（读到未初始化的宽高，越界写共享段）。
    static std::atomic<long> detour_depth;

    // RAII：detour 一进入就登记，离开时注销。写在 detour 的第一行。
    struct DetourScope {
        DetourScope() {
            detour_depth.fetch_add(1);
        }
        ~DetourScope() {
            detour_depth.fetch_sub(1);
        }
        DetourScope(const DetourScope &) = delete;
        DetourScope &operator=(const DetourScope &) = delete;
    };

    // 等所有在跑的 detour 走完；超时返回 false（调用方此时必须放弃释放 trampoline）。
    static bool wait_for_detours(int timeout_ms);

    // release() 没等到静默时，trampoline/钩子/窗口信息都被刻意留着（否则在跑的
    // detour 会跳进已回收的代码），这次释放就变成"待清理"。下次 setup() 或
    // ReleaseDisplayHook() 会再来一次 finish_pending_release()：
    // 静默了就真摘钩 + 清状态，否则继续留着，绝不覆盖。没有这条，残留的
    // MH 钩子会让下一次 setup() 撞 MH_ERROR_ALREADY_CREATED（同一入口）而绑定失败。
    static bool pending_release;
    static bool finish_pending_release(int timeout_ms);
    static bool has_pending_release();

    // 捕获失败计数：一次瞬时失败（设备重置、GPU 抢占、驱动升级、DWM 重建交换链）
    // 不该让后台截图永久停摆 —— 原来每个失败点都是 set_capture_enabled(false)，
    // 意味着 GetBuffer/Map 里任何一个瞬时 HRESULT 就把整条通道关到重新绑定为止。
    // 现在连续失败 kCaptureFailureLimit 次才真正停抓，任何一帧成功即清零。
    static void note_capture_failure();
    static void note_capture_success();
    static constexpr int kCaptureFailureLimit = 30;
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
