#include "CursorShape.h"

#include <iomanip>
#include <sstream>
#include <vector>

namespace {

struct ScopedIconInfo {
    ICONINFO value = {};

    ~ScopedIconInfo() {
        if (value.hbmColor)
            ::DeleteObject(value.hbmColor);
        if (value.hbmMask)
            ::DeleteObject(value.hbmMask);
    }
};

struct ScopedBitmap {
    HBITMAP handle = nullptr;

    ~ScopedBitmap() {
        if (handle)
            ::DeleteObject(handle);
    }
};

struct ScopedDc {
    HDC handle = nullptr;

    ~ScopedDc() {
        if (handle)
            ::DeleteDC(handle);
    }
};

struct SelectBitmapGuard {
    HDC dc = nullptr;
    HGDIOBJ old = nullptr;

    SelectBitmapGuard(HDC dc_, HBITMAP bitmap) : dc(dc_), old(::SelectObject(dc_, bitmap)) {
    }

    ~SelectBitmapGuard() {
        if (dc && old)
            ::SelectObject(dc, old);
    }
};

std::uint64_t fnv1a(const void *data, size_t size, std::uint64_t hash = 14695981039346656037ull) {
    const auto *bytes = static_cast<const unsigned char *>(data);
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 1099511628211ull;
    }
    return hash;
}

void hash_u32(std::uint64_t &hash, std::uint32_t value) {
    hash = fnv1a(&value, sizeof(value), hash);
}

bool bitmap_size(HBITMAP bitmap, long &width, long &height) {
    BITMAP bm = {};
    if (!bitmap || !::GetObject(bitmap, sizeof(bm), &bm))
        return false;

    width = bm.bmWidth;
    height = bm.bmHeight;
    return width > 0 && height > 0;
}

} // namespace

namespace cursor_shape {

bool FromCursor(HCURSOR cursor, bool visible, CursorShapeInfo &info) {
    info = {};
    info.visible = visible;
    if (!cursor)
        return false;

    HICON copied = ::CopyIcon(cursor);
    if (!copied)
        return false;

    ScopedIconInfo icon_info;
    if (!::GetIconInfo(copied, &icon_info.value)) {
        ::DestroyIcon(copied);
        return false;
    }

    long width = 0;
    long height = 0;
    if (!bitmap_size(icon_info.value.hbmColor ? icon_info.value.hbmColor : icon_info.value.hbmMask, width, height)) {
        ::DestroyIcon(copied);
        return false;
    }

    if (!icon_info.value.hbmColor)
        height /= 2;
    if (height <= 0) {
        ::DestroyIcon(copied);
        return false;
    }

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void *bits = nullptr;
    ScopedBitmap dib{::CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0)};
    if (!dib.handle || !bits) {
        ::DestroyIcon(copied);
        return false;
    }

    ScopedDc dc{::CreateCompatibleDC(nullptr)};
    if (!dc.handle) {
        ::DestroyIcon(copied);
        return false;
    }

    {
        SelectBitmapGuard select(dc.handle, dib.handle);
        // 统一渲染成 32 位 BGRA，hash 才能跨系统颜色深度稳定比较。
        ::PatBlt(dc.handle, 0, 0, width, height, BLACKNESS);
        if (!::DrawIconEx(dc.handle, 0, 0, copied, width, height, 0, nullptr, DI_NORMAL)) {
            ::DestroyIcon(copied);
            return false;
        }
    }
    ::DestroyIcon(copied);

    info.visible = visible;
    info.width = width;
    info.height = height;
    info.hot_x = static_cast<long>(icon_info.value.xHotspot);
    info.hot_y = static_cast<long>(icon_info.value.yHotspot);

    std::uint64_t hash = 14695981039346656037ull;
    hash_u32(hash, static_cast<std::uint32_t>(width));
    hash_u32(hash, static_cast<std::uint32_t>(height));
    hash_u32(hash, static_cast<std::uint32_t>(info.hot_x));
    hash_u32(hash, static_cast<std::uint32_t>(info.hot_y));
    hash = fnv1a(bits, static_cast<size_t>(width) * static_cast<size_t>(height) * 4, hash);
    info.hash = hash;
    return true;
}

bool FromSystem(CursorShapeInfo &info) {
    CURSORINFO cursor = {};
    cursor.cbSize = sizeof(cursor);
    if (!::GetCursorInfo(&cursor))
        return false;

    const bool visible = (cursor.flags & CURSOR_SHOWING) != 0;
    return FromCursor(cursor.hCursor, visible, info);
}

std::uint64_t PackMeta(const CursorShapeInfo &info, bool valid) {
    std::uint64_t meta = valid ? 1ull : 0ull;
    meta |= (info.visible ? 1ull : 0ull) << 1;
    meta |= (static_cast<std::uint64_t>(info.width) & 0xFFFFull) << 2;
    meta |= (static_cast<std::uint64_t>(info.height) & 0xFFFFull) << 18;
    meta |= (static_cast<std::uint64_t>(info.hot_x) & 0x7FFFull) << 34;
    meta |= (static_cast<std::uint64_t>(info.hot_y) & 0x7FFFull) << 49;
    return meta;
}

bool UnpackMeta(std::uint64_t meta, std::uint64_t hash, CursorShapeInfo &info) {
    if ((meta & 1ull) == 0)
        return false;

    info.visible = ((meta >> 1) & 1ull) != 0;
    info.width = static_cast<long>((meta >> 2) & 0xFFFFull);
    info.height = static_cast<long>((meta >> 18) & 0xFFFFull);
    info.hot_x = static_cast<long>((meta >> 34) & 0x7FFFull);
    info.hot_y = static_cast<long>((meta >> 49) & 0x7FFFull);
    info.hash = hash;
    if (info.width > 0 && info.height > 0)
        return info.hash != 0;
    return !info.visible && info.hash == 0;
}

std::wstring Format(const CursorShapeInfo &info) {
    std::wstringstream ss;
    ss << (info.visible ? 1 : 0) << L"," << std::uppercase << std::hex << std::setw(16) << std::setfill(L'0')
       << info.hash << std::dec << L"," << info.width << L"," << info.height << L"," << info.hot_x << L","
       << info.hot_y;
    return ss.str();
}

} // namespace cursor_shape
