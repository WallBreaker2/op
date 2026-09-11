#pragma once

#include "DisplayHook.h"
#include "../capture/FrameInfo.h"
#include "../ipc/SharedMemory.h"
#include <cstddef>
#include <span>

// 依赖 std::span，只给以 C++20 编译的捕获源（D3D10/D3D11）用，
// 避免把 C++20 要求传染给同样包含 SharedFrame.h 的 C++17 源文件。
namespace op::hook {

// 共享内存 = 帧头 + 像素区，用 span 切分避免裸指针偏移散落在捕获逻辑里。
inline std::span<std::byte> make_shared_frame_span(op::SharedMemory &mem, UINT width, UINT height) {
    const auto pixelBytes = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    return {mem.data<std::byte>(), sizeof(op::capture::FrameInfo) + pixelBytes};
}

inline void write_shared_frame(std::span<std::byte> sharedFrame, HWND hwnd, UINT width, UINT height,
                               const void *source, int sourceRows, int sourceCols, int rowPitch, int format) {
    auto frameInfoBytes = sharedFrame.first(sizeof(op::capture::FrameInfo));
    auto pixelBytes = sharedFrame.subspan(sizeof(op::capture::FrameInfo));

    reinterpret_cast<op::capture::FrameInfo *>(frameInfoBytes.data())->format(hwnd, width, height);
    CopyImageData(reinterpret_cast<char *>(pixelBytes.data()), static_cast<const char *>(source), sourceRows,
                  sourceCols, rowPitch, format);
}

} // namespace op::hook
