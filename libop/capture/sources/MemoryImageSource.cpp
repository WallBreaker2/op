#include "MemoryImageSource.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <cwctype>
#include <cstdint>
#include <span>
#include <vector>

namespace op::capture {

namespace {

// 外部缓冲区的尺寸上限：位图/BMP 都要在这之内，避免用畸形参数去构造一个几十 GB 的 span。
constexpr long long kMaxImagePixels = 1LL << 28;

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
    // 不接受符号位：wcstoull(L"-1") 会回绕成 ULLONG_MAX 且不报错，
    // 等于把一个野指针放进来，随后按 w*h*4 读取就会 AV。
    if (input.front() == L'-' || input.front() == L'+') {
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
    // x86 构建里 uintptr_t 只有 32 位：超出宽度的地址会被静默截断成另一个地址，必须显式拒绝。
    if (value > static_cast<unsigned long long>(UINTPTR_MAX)) {
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
    // 原来只判了 <=0/ERANGE，然后 static_cast<int>：x64 上 "99999999999" 能通过 wcstol
    // （long 是 64 位），截断成 int 后变成一个完全不同的尺寸。
    if (*end != L'\0' || errno == ERANGE || parsed <= 0 || parsed > INT_MAX) {
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

bool copy_raw_image(std::span<const std::byte> src, int width, int height, const std::wstring &fmt, Image &dst) {
    dst.create(width, height);
    if (fmt == L"bgra") {
        const auto rowBytes = static_cast<size_t>(width) * 4;
        for (int y = 0; y < height; ++y) {
            const auto row = src.subspan(static_cast<size_t>(y) * rowBytes, rowBytes);
            memcpy(dst.ptr<uchar>(y), row.data(), row.size());
        }
        return true;
    }
    if (fmt == L"bgr") {
        const auto rowBytes = static_cast<size_t>(width) * 3;
        for (int y = 0; y < height; ++y) {
            const auto src_row = src.subspan(static_cast<size_t>(y) * rowBytes, rowBytes);
            auto dst_row = dst.ptr<uchar>(y);
            for (int x = 0; x < width; ++x) {
                dst_row[x * 4 + 0] = std::to_integer<uchar>(src_row[x * 3 + 0]);
                dst_row[x * 4 + 1] = std::to_integer<uchar>(src_row[x * 3 + 1]);
                dst_row[x * 4 + 2] = std::to_integer<uchar>(src_row[x * 3 + 2]);
                dst_row[x * 4 + 3] = 0xff;
            }
        }
        return true;
    }
    return false;
}

bool read_bmp_image(const std::byte *ptr, Image &dst) {
    std::span<const std::byte> header(ptr, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
    BITMAPFILEHEADER bfh = {0};
    BITMAPINFOHEADER bih = {0};
    memcpy(&bfh, header.data(), sizeof(bfh));
    memcpy(&bih, header.subspan(sizeof(bfh)).data(), sizeof(bih));

    if (bfh.bfType != static_cast<WORD>(0x4d42) || bih.biWidth <= 0 || bih.biHeight == 0) {
        return false;
    }
    // 只认 BITMAPINFOHEADER：BITMAPCOREHEADER(12 字节) 会被按 40 字节解释成垃圾宽高。
    if (bih.biSize != sizeof(BITMAPINFOHEADER) || bih.biPlanes != 1) {
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
    // biHeight == INT_MIN 时 -bih.biHeight 是 UB，先拒掉。
    if (bih.biHeight == INT_MIN) {
        return false;
    }

    // 全程用 64 位算：原来 width*biBitCount 是 int 乘法，超大宽度会溢出成负数，
    // 随后再转 size_t 就变成一个巨大的跨度。
    const long long width64 = bih.biWidth;
    const long long height64 = bih.biHeight < 0 ? -static_cast<long long>(bih.biHeight) : bih.biHeight;
    if (width64 <= 0 || height64 <= 0 || width64 * height64 > kMaxImagePixels) {
        return false;
    }

    const int width = static_cast<int>(width64);
    const int height = static_cast<int>(height64);
    const long long src_stride64 = ((width64 * bih.biBitCount + 31) / 32) * 4;
    const auto pixelBytes = static_cast<size_t>(src_stride64 * height64);
    std::span<const std::byte> pixels(ptr + bfh.bfOffBits, pixelBytes);
    bool top_down = bih.biHeight < 0;

    dst.create(width, height);
    for (int y = 0; y < height; ++y) {
        const auto src_row = pixels.subspan(static_cast<size_t>(top_down ? y : (height - 1 - y)) *
                                                static_cast<size_t>(src_stride64),
                                            static_cast<size_t>(src_stride64));
        auto dst_row = dst.ptr<uchar>(y);
        if (bih.biBitCount == 32) {
            memcpy(dst_row, src_row.data(), static_cast<size_t>(width) * 4);
        } else {
            for (int x = 0; x < width; ++x) {
                dst_row[x * 4 + 0] = std::to_integer<uchar>(src_row[x * 3 + 0]);
                dst_row[x * 4 + 1] = std::to_integer<uchar>(src_row[x * 3 + 1]);
                dst_row[x * 4 + 2] = std::to_integer<uchar>(src_row[x * 3 + 2]);
                dst_row[x * 4 + 3] = 0xff;
            }
        }
    }
    return true;
}

} // namespace

bool ParseMemoryImageSource(const std::wstring &method, Image &output, std::wstring &normalized_method) {
    const std::wstring candidate_method = trim_ws(method);
    auto parts = split_csv(candidate_method);
    Image parsed;

    if (parts.size() >= 3) {
        if (parts.size() != 3 && parts.size() != 4) {
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
        if (static_cast<long long>(width) * height > kMaxImagePixels) {
            return false;
        }

        std::wstring fmt = L"bgra";
        if (parts.size() == 4 && !parts[3].empty()) {
            fmt = parts[3];
            std::transform(fmt.begin(), fmt.end(), fmt.begin(), ::towlower);
        }
        // 外部只提供首地址和尺寸参数，内部用 span 约束本次需要读取的字节范围。
        const auto bytesPerPixel = fmt == L"bgr" ? 3 : 4;
        const auto rawBytes = static_cast<size_t>(width) * static_cast<size_t>(height) * bytesPerPixel;
        if (!copy_raw_image({reinterpret_cast<const std::byte *>(ptr), rawBytes}, width, height, fmt, parsed)) {
            return false;
        }
        output = parsed;
        normalized_method = candidate_method;
        return true;
    }

    byte *ptr = nullptr;
    if (!parse_ptr(candidate_method, ptr)) {
        return false;
    }
    if (!read_bmp_image(reinterpret_cast<const std::byte *>(ptr), parsed)) {
        return false;
    }
    output = parsed;
    normalized_method = candidate_method;
    return true;
}

} // namespace op::capture


