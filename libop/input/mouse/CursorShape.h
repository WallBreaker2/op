#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>

struct CursorShapeInfo {
    bool visible = false;
    std::uint64_t hash = 0;
    long width = 0;
    long height = 0;
    long hot_x = 0;
    long hot_y = 0;
};

namespace cursor_shape {

bool FromCursor(HCURSOR cursor, bool visible, CursorShapeInfo &info);
bool FromSystem(CursorShapeInfo &info);
std::uint64_t PackMeta(const CursorShapeInfo &info, bool valid);
bool UnpackMeta(std::uint64_t meta, std::uint64_t hash, CursorShapeInfo &info);
std::wstring Format(const CursorShapeInfo &info);

} // namespace cursor_shape
