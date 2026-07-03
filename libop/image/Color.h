#pragma once
#ifndef OP_IMAGE_COLOR_H_
#define OP_IMAGE_COLOR_H_
#include "../runtime/Types.h"
#include <algorithm>
#define WORD_BKCOLOR 0
#define WORD_COLOR 1
// #include "../Tool.h"
#include <math.h>
#define color2uint(color) (*(uint *)&color)

namespace op {

template <typename T> constexpr T OP_ABS(T x) {
    return x < 0 ? -x : x;
}
template <typename T> constexpr bool IN_RANGE(T lhs, T rhs, T df) {
    return OP_ABS(lhs.b - rhs.b) <= df.b && OP_ABS(lhs.g - rhs.g) <= df.g && OP_ABS(lhs.r - rhs.r) <= df.r;
}
constexpr char HEX_DIGIT_A(unsigned char value) {
    return value < 10 ? static_cast<char>('0' + value) : static_cast<char>('A' + value - 10);
}
constexpr wchar_t HEX_DIGIT_W(unsigned char value) {
    return value < 10 ? static_cast<wchar_t>(L'0' + value) : static_cast<wchar_t>(L'A' + value - 10);
}
// 颜色结构

// #pragma pack(push)
#pragma pack(1)
struct color_t {
    // b is in low address ,alpha is in high address
    uchar b, g, r, alpha;
    color_t() : b(0), g(0), r(0), alpha(0) {
    }
    color_t(int b_, int g_, int r_) : b(b_), g(g_), r(r_), alpha(0xffu) {
    }

    color_t &str2color(const std::wstring &s) {
        int r = 0, g = 0, b = 0;
        std::wstring ss = s;
        std::transform(ss.begin(), ss.end(), ss.begin(), ::towupper);
        swscanf(ss.c_str(), L"%02X%02X%02X", &r, &g, &b);
        this->b = b;
        this->r = r;
        this->g = g;
        return *this;
    }
    color_t &str2color(const std::string &s) {
        int r = 0, g = 0, b = 0;
        std::string ss = s;
        std::transform(ss.begin(), ss.end(), ss.begin(), ::toupper);
        sscanf(ss.c_str(), "%02X%02X%02X", &r, &g, &b);
        this->b = b;
        this->r = r;
        this->g = g;
        return *this;
    }
    std::string tostr() {
        std::string result(6, '0');
        result[0] = HEX_DIGIT_A(r >> 4);
        result[1] = HEX_DIGIT_A(r & 0x0F);
        result[2] = HEX_DIGIT_A(g >> 4);
        result[3] = HEX_DIGIT_A(g & 0x0F);
        result[4] = HEX_DIGIT_A(b >> 4);
        result[5] = HEX_DIGIT_A(b & 0x0F);
        return result;
    }
    std::wstring towstr() {
        std::wstring result(6, L'0');
        result[0] = HEX_DIGIT_W(r >> 4);
        result[1] = HEX_DIGIT_W(r & 0x0F);
        result[2] = HEX_DIGIT_W(g >> 4);
        result[3] = HEX_DIGIT_W(g & 0x0F);
        result[4] = HEX_DIGIT_W(b >> 4);
        result[5] = HEX_DIGIT_W(b & 0x0F);
        return result;
    }
    uchar toGray() const {
        return (r * 299 + g * 587 + b * 114 + 500) / 1000;
    }
};
#pragma pack()
// 颜色-偏色结构
struct color_df_t {
    // 颜色
    color_t color;
    // 偏色
    color_t df;
};
// 坐标-颜色-偏色结构
struct pt_cr_df_t {
    int x, y;
    std::vector<color_df_t> crdfs;
};

} // namespace op

#endif // OP_IMAGE_COLOR_H_
