#include "displayInputHelper.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cwctype>
#include <cstdint>
#include <vector>

namespace {

std::wstring trim_ws(const std::wstring &value) {
    size_t start = 0;
    while (start < value.size() && iswspace(value[start])) {
        ++start;
    }
    size_t end = value.size();
    while (end > start && iswspace(value[end - 1])) {
        --end;
    }
    return value.substr(start, end - start);
}

bool parse_ptr(const std::wstring &text, byte *&ptr) {
    auto input = trim_ws(text);
    if (input.empty()) {
        return false;
    }
    errno = 0;
    wchar_t *end = nullptr;
    auto value = wcstoull(input.c_str(), &end, 0);
    if (end == input.c_str()) {
        return false;
    }
    while (*end != L'\0' && iswspace(*end)) {
        ++end;
    }
    if (*end != L'\0' || errno == ERANGE || value == 0) {
        return false;
    }
    ptr = reinterpret_cast<byte *>(static_cast<uintptr_t>(value));
    return ptr != nullptr;
}

bool parse_positive_int(const std::wstring &text, int &value) {
    auto input = trim_ws(text);
    if (input.empty()) {
        return false;
    }
    errno = 0;
    wchar_t *end = nullptr;
    auto parsed = wcstol(input.c_str(), &end, 10);
    if (end == input.c_str()) {
        return false;
    }
    while (*end != L'\0' && iswspace(*end)) {
        ++end;
    }
    if (*end != L'\0' || errno == ERANGE || parsed <= 0) {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

std::vector<std::wstring> split_csv(const std::wstring &text) {
    std::vector<std::wstring> tokens;
    size_t start = 0;
    while (start <= text.size()) {
        size_t pos = text.find(L',', start);
        if (pos == std::wstring::npos) {
            tokens.emplace_back(trim_ws(text.substr(start)));
            break;
        }
        tokens.emplace_back(trim_ws(text.substr(start, pos - start)));
        start = pos + 1;
    }
    return tokens;
}

bool copy_raw_image(byte *src, int width, int height, const std::wstring &fmt, Image &dst) {
    dst.create(width, height);
    if (fmt == L"bgra") {
        for (int y = 0; y < height; ++y) {
            memcpy(dst.ptr<uchar>(y), src + static_cast<size_t>(y) * width * 4, static_cast<size_t>(width) * 4);
        }
        return true;
    }
    if (fmt == L"bgr") {
        for (int y = 0; y < height; ++y) {
            auto src_row = src + static_cast<size_t>(y) * width * 3;
            auto dst_row = dst.ptr<uchar>(y);
            for (int x = 0; x < width; ++x) {
                dst_row[x * 4 + 0] = src_row[x * 3 + 0];
                dst_row[x * 4 + 1] = src_row[x * 3 + 1];
                dst_row[x * 4 + 2] = src_row[x * 3 + 2];
                dst_row[x * 4 + 3] = 0xff;
            }
        }
        return true;
    }
    return false;
}

bool read_bmp_image(byte *ptr, Image &dst) {
    BITMAPFILEHEADER bfh = {0};
    BITMAPINFOHEADER bih = {0};
    memcpy(&bfh, ptr, sizeof(bfh));
    memcpy(&bih, ptr + sizeof(bfh), sizeof(bih));

    if (bfh.bfType != static_cast<WORD>(0x4d42) || bih.biWidth <= 0 || bih.biHeight == 0) {
        return false;
    }
    if (bih.biBitCount != 24 && bih.biBitCount != 32) {
        return false;
    }
    if (bih.biCompression != BI_RGB) {
        return false;
    }
    if (bfh.bfOffBits < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)) {
        return false;
    }

    int width = bih.biWidth;
    int height = bih.biHeight < 0 ? -bih.biHeight : bih.biHeight;
    int src_stride = ((width * bih.biBitCount + 31) / 32) * 4;
    auto pixel_ptr = ptr + bfh.bfOffBits;
    bool top_down = bih.biHeight < 0;

    dst.create(width, height);
    for (int y = 0; y < height; ++y) {
        auto src_row = pixel_ptr + static_cast<size_t>(top_down ? y : (height - 1 - y)) * src_stride;
        auto dst_row = dst.ptr<uchar>(y);
        if (bih.biBitCount == 32) {
            memcpy(dst_row, src_row, static_cast<size_t>(width) * 4);
        } else {
            for (int x = 0; x < width; ++x) {
                dst_row[x * 4 + 0] = src_row[x * 3 + 0];
                dst_row[x * 4 + 1] = src_row[x * 3 + 1];
                dst_row[x * 4 + 2] = src_row[x * 3 + 2];
                dst_row[x * 4 + 3] = 0xff;
            }
        }
    }
    return true;
}

} // namespace

namespace display_input_helper {

bool parse_mem_display_input(const std::wstring &method, Image &output, std::wstring &normalized_method) {
    normalized_method = trim_ws(method);
    auto parts = split_csv(normalized_method);

    if (parts.size() >= 3) {
        if (parts.size() > 4) {
            return false;
        }
        byte *ptr = nullptr;
        if (!parse_ptr(parts[0], ptr)) {
            return false;
        }
        int width = 0;
        int height = 0;
        if (!parse_positive_int(parts[1], width) || !parse_positive_int(parts[2], height)) {
            return false;
        }

        std::wstring fmt = L"bgra";
        if (parts.size() == 4 && !parts[3].empty()) {
            fmt = parts[3];
            std::transform(fmt.begin(), fmt.end(), fmt.begin(), ::towlower);
        }
        return copy_raw_image(ptr, width, height, fmt, output);
    }

    byte *ptr = nullptr;
    if (!parse_ptr(normalized_method, ptr)) {
        return false;
    }
    return read_bmp_image(ptr, output);
}

} // namespace display_input_helper
