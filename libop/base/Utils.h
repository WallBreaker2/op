#pragma once
#ifndef OP_BASE_UTILS_H_
#define OP_BASE_UTILS_H_
#include "Types.h"
std::wstring _s2wstring(const std::string &s);
std::string _ws2string(const std::wstring &s);

std::string utf8_to_ansi(std::string strUTF8);
// 将路径转化为全局路径
long Path2GlobalPath(const std::wstring &file, const std::wstring &curr_path, std::wstring &out);

void split(const std::wstring &s, std::vector<std::wstring> &v, const std::wstring &c);
void split(const std::string &s, std::vector<std::string> &v, const std::string &c);

void wstring2lower(std::wstring &s);

void replacew(std::wstring &str, const std::wstring &oldval, const std::wstring &newval);

// for debug
long setlog(const wchar_t *format, ...);
//
long setlog(const char *format, ...);

// Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetLastErrorAsString();

int inline hex2bin(int c) {
    return c <= L'9' ? c - L'0' : c - L'A' + 10;
};

int inline bin2hex(int c) {
    int ans = 0;
    int c1 = c >> 4 & 0xf;
    int c2 = c & 0xf;
    ans |= (c1 <= 9 ? c1 + L'0' : c1 + 'A' - 10) << 8;
    ans |= c2 <= 9 ? c2 + L'0' : c2 + 'A' - 10;
    return ans;
};

constexpr int PTY(op::uint pt) {
    return pt >> 16;
}

constexpr int PTX(op::uint pt) {
    return pt & 0xffff;
}

namespace op {
std::ostream &operator<<(std::ostream &o, point_t const &rhs);
std::wostream &operator<<(std::wostream &o, point_t const &rhs);
} // namespace op

bool Delay(long mis);
bool Delays(long mis_min, long mis_max);

#endif // OP_BASE_UTILS_H_
