#pragma once

#include "DisplayHook.h"
#include "../capture/FrameInfo.h"
#include "../ipc/SharedMemory.h"
#include <cstddef>

// 只用裸指针 + 长度，不依赖 std::span：注入侧的捕获源分两种编译单元
// （D3D10/D3D11 是 C++20，D3D9/D3D12/OpenGL 是 C++17），头文件必须两者都能包含。
// 以前这里用 std::span，导致 C++17 的那三个后端只能各写一份"发布一帧"的骨架，
// 已经分叉出过缺陷（OpenGL 双翻转、缺 request_in_bounds 校验）。
namespace op::hook {

// 共享内存的一帧视图：帧头 + 像素区。
struct SharedFrameView {
    std::byte *data = nullptr;
    size_t size = 0;

    bool valid() const { return data != nullptr; }
};

inline SharedFrameView make_shared_frame_span(op::SharedMemory &mem, UINT width, UINT height) {
    const auto pixelBytes = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    return {mem.data<std::byte>(), sizeof(op::capture::FrameInfo) + pixelBytes};
}

inline void write_shared_frame(SharedFrameView sharedFrame, HWND hwnd, UINT width, UINT height, const void *source,
                               int sourceRows, int sourceCols, int rowPitch, int format) {
    auto *frameInfo = reinterpret_cast<op::capture::FrameInfo *>(sharedFrame.data);
    frameInfo->format(hwnd, width, height);
    CopyImageData(reinterpret_cast<char *>(sharedFrame.data) + sizeof(op::capture::FrameInfo),
                  static_cast<const char *>(source), sourceRows, sourceCols, rowPitch, format);
}

// 只搬请求区域时用：帧头仍记**整窗**尺寸（协商区偏移、旧宿主都靠它），
// 像素区只放请求的那块（rows×cols，行方向与该后端的整窗一致）。
inline void write_shared_frame_region(SharedFrameView sharedFrame, HWND hwnd, UINT windowWidth, UINT windowHeight,
                                      const void *source, int rows, int cols, int rowPitch, int format) {
    auto *frameInfo = reinterpret_cast<op::capture::FrameInfo *>(sharedFrame.data);
    frameInfo->format(hwnd, windowWidth, windowHeight);
    CopyImageData(reinterpret_cast<char *>(sharedFrame.data) + sizeof(op::capture::FrameInfo),
                  static_cast<const char *>(source), rows, cols, rowPitch, format);
}

} // namespace op::hook
