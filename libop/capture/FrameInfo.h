#ifndef OP_CAPTURE_FRAME_INFO_H_
#define OP_CAPTURE_FRAME_INFO_H_
#include <Windows.h>
#include <cstring>
#include <ostream>

namespace op::capture {

#pragma pack(1)
struct FrameInfo {
    unsigned __int64 hwnd;
    unsigned __int32 frameId;
    unsigned __int32 time;
    unsigned __int32 width;
    unsigned __int32 height;
    unsigned __int32 chk;
    void fmtChk() {
        chk = (hwnd >> 32) ^ (hwnd & 0xffffffffull) ^ frameId ^ time ^ width ^ height;
    }

    void format(HWND hwnd_, int w_, int h_) {
        hwnd = (unsigned __int64)hwnd_;
        frameId++;
        time = ::GetTickCount();
        width = w_;
        height = h_;
        fmtChk();
    }
};
#pragma pack()
// 采集后端共用的帧头写入：更新自身 FrameInfo 后整块拷到共享内存头部。
// inc=false 用于同一帧被多次取用时保持 frameId 不变。
inline void WriteFrameInfo(FrameInfo &info, void *dst, HWND hwnd, int w, int h, bool inc = true) {
    info.hwnd = (unsigned __int64)hwnd;
    if (inc)
        info.frameId++;
    info.time = static_cast<unsigned __int32>(::GetTickCount64());
    info.width = w;
    info.height = h;
    info.fmtChk();
    memcpy(dst, &info, sizeof(info));
}

} // namespace op::capture

std::ostream &operator<<(std::ostream &o, op::capture::FrameInfo const &rhs);
std::wostream &operator<<(std::wostream &o, op::capture::FrameInfo const &rhs);

#endif // OP_CAPTURE_FRAME_INFO_H_
