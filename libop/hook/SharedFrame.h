#pragma once

#include "../capture/FrameInfo.h"
#include "../ipc/SharedMemory.h"
#include <cstddef>
#include <cstdint>
#include <limits>

namespace op::hook {

inline size_t RequiredSharedFrameBytes(std::uint64_t width, std::uint64_t height) {
    if (width == 0 || height == 0)
        return 0;

    constexpr std::uint64_t bytes_per_pixel = 4;
    constexpr std::uint64_t header_bytes = sizeof(op::capture::FrameInfo);
    constexpr std::uint64_t max_size = (std::numeric_limits<size_t>::max)();
    if (width > max_size / height)
        return 0;
    const std::uint64_t pixels = width * height;
    if (pixels > (max_size - header_bytes) / bytes_per_pixel)
        return 0;
    return static_cast<size_t>(header_bytes + pixels * bytes_per_pixel);
}

inline bool SharedFrameHasCapacity(const op::SharedMemory &mem, std::uint64_t width, std::uint64_t height) {
    const size_t required = RequiredSharedFrameBytes(width, height);
    return required != 0 && mem.size() >= required;
}

inline bool WriteSharedFrameHeader(op::SharedMemory &mem, HWND hwnd, int width, int height) {
    if (mem.size() < sizeof(op::capture::FrameInfo))
        return false;
    auto *info = reinterpret_cast<op::capture::FrameInfo *>(mem.data<unsigned char>());
    info->format(hwnd, width, height);
    return true;
}

} // namespace op::hook
