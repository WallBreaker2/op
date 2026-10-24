#pragma once
#ifndef OP_BASE_JSON_SCANNER_H_
#define OP_BASE_JSON_SCANNER_H_

// 服务端响应（OCR / YOLO）用的极简 JSON 扫描原语。
//
// 为什么不引第三方 JSON 库：这两条路径的响应形状是固定的扁平结构，字段少、层级浅，
// 而且一次响应可能有几十条结果。早先的实现用 std::regex 逐条匹配，50 条结果要 800+ µs
// （doc/defect_fixes.md §72.33）；换成单遍扫描后是 11 µs。这里把原语抽出来给两个解析器共用，
// 避免各写一份。

#include "Types.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>

namespace op::json {

inline const char *skip_ws(const char *p, const char *end) {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'))
        ++p;
    return p;
}

// 从 p 起找字面量 needle，返回命中位置；找不到返回 nullptr。
inline const char *find_literal(const char *p, const char *end, const char *needle) {
    const size_t n = std::strlen(needle);
    if (n == 0 || static_cast<size_t>(end - p) < n)
        return nullptr;
    const char *hit = std::search(p, end, needle, needle + n);
    return hit == end ? nullptr : hit;
}

inline bool scan_long(const char *&p, const char *end, long long *out) {
    p = skip_ws(p, end);
    bool negative = false;
    if (p < end && (*p == '-' || *p == '+')) {
        negative = *p == '-';
        ++p;
    }
    if (p >= end || *p < '0' || *p > '9')
        return false;
    // §72.51：原来 value * 10 不做溢出检查 —— 超长数字串是未定义行为（实践中回绕），
    // 而 -value 碰到 LLONG_MIN 又是 UB。这里限 18 位有效数字：足够放任何真实坐标、
    // 置信度、错误码，第 19 位直接判失败（宁可拒绝畸形输入，也不接受回绕后的值）。
    long long value = 0;
    int digits = 0;
    while (p < end && *p >= '0' && *p <= '9') {
        if (++digits > 18)
            return false;
        value = value * 10 + (*p - '0');
        ++p;
    }
    *out = negative ? -value : value;
    return true;
}

inline bool scan_double(const char *&p, const char *end, double *out) {
    p = skip_ws(p, end);
    const char *begin = p;
    if (p < end && (*p == '-' || *p == '+'))
        ++p;
    while (p < end) {
        const char ch = *p;
        if ((ch >= '0' && ch <= '9') || ch == '.' || ch == 'e' || ch == 'E') {
            ++p;
            continue;
        }
        // 指数里的正负号只紧跟在 e/E 后面才算数字的一部分。
        if ((ch == '+' || ch == '-') && p != begin && (*(p - 1) == 'e' || *(p - 1) == 'E')) {
            ++p;
            continue;
        }
        break;
    }
    if (p == begin)
        return false;
    const std::string text(begin, p);
    *out = std::strtod(text.c_str(), nullptr);
    return true;
}

// 读一个 JSON 字符串字面量（p 指向开头的引号），返回**未反转义**的内容。
inline bool scan_json_string(const char *&p, const char *end, std::string *raw) {
    p = skip_ws(p, end);
    if (p >= end || *p != '"')
        return false;
    ++p;
    raw->clear();
    while (p < end) {
        const char ch = *p;
        if (ch == '\\') {
            if (p + 1 >= end)
                return false;
            raw->push_back(ch);
            raw->push_back(*(p + 1));
            p += 2;
            continue;
        }
        if (ch == '"') {
            ++p;
            return true;
        }
        raw->push_back(ch);
        ++p;
    }
    return false;
}

inline int hex_digit(char ch) {
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    return -1;
}

inline void append_utf8(std::string *out, unsigned code) {
    if (code < 0x80) {
        out->push_back(static_cast<char>(code));
    } else if (code < 0x800) {
        out->push_back(static_cast<char>(0xC0 | (code >> 6)));
        out->push_back(static_cast<char>(0x80 | (code & 0x3F)));
    } else if (code < 0x10000) {
        out->push_back(static_cast<char>(0xE0 | (code >> 12)));
        out->push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
        out->push_back(static_cast<char>(0x80 | (code & 0x3F)));
    } else {
        out->push_back(static_cast<char>(0xF0 | (code >> 18)));
        out->push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3F)));
        out->push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
        out->push_back(static_cast<char>(0x80 | (code & 0x3F)));
    }
}

// 把 JSON 字符串内容反转义成 UTF-8 字节串。
// 只处理这两条路径需要的转义；`\uXXXX` 按 UTF-16 码元处理，代理对合并成一个码点。
inline std::string unescape(const std::string &raw) {
    std::string out;
    out.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        const char ch = raw[i];
        if (ch != '\\') {
            out.push_back(ch);
            continue;
        }
        if (++i >= raw.size())
            break;
        switch (raw[i]) {
        case '"':
            out.push_back('"');
            break;
        case '\\':
            out.push_back('\\');
            break;
        case '/':
            out.push_back('/');
            break;
        case 'b':
            out.push_back('\b');
            break;
        case 'f':
            out.push_back('\f');
            break;
        case 'n':
            out.push_back('\n');
            break;
        case 'r':
            out.push_back('\r');
            break;
        case 't':
            out.push_back('\t');
            break;
        case 'u': {
            if (i + 4 >= raw.size())
                return out;
            unsigned code = 0;
            for (int k = 0; k < 4; ++k) {
                const int digit = hex_digit(raw[i + 1 + static_cast<size_t>(k)]);
                if (digit < 0)
                    return out;
                code = (code << 4) | static_cast<unsigned>(digit);
            }
            i += 4;
            // 高代理后面跟低代理时合成一个补充平面码点。
            if (code >= 0xD800 && code <= 0xDBFF && i + 6 < raw.size() && raw[i + 1] == '\\' && raw[i + 2] == 'u') {
                unsigned low = 0;
                bool ok = true;
                for (int k = 0; k < 4; ++k) {
                    const int digit = hex_digit(raw[i + 3 + static_cast<size_t>(k)]);
                    if (digit < 0) {
                        ok = false;
                        break;
                    }
                    low = (low << 4) | static_cast<unsigned>(digit);
                }
                if (ok && low >= 0xDC00 && low <= 0xDFFF) {
                    i += 6;
                    code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                }
            }
            append_utf8(&out, code);
            break;
        }
        default:
            out.push_back(raw[i]);
            break;
        }
    }
    return out;
}

// 在 [begin,end) 里找 "key"，跳过冒号，把值的位置写到 value_begin。
inline bool locate_value(const char *begin, const char *end, const char *quoted_key, const char **value_begin) {
    const char *hit = find_literal(begin, end, quoted_key);
    if (!hit)
        return false;
    const char *p = hit + std::strlen(quoted_key);
    p = skip_ws(p, end);
    if (p >= end || *p != ':')
        return false;
    ++p;
    *value_begin = skip_ws(p, end);
    return true;
}

// 依次试多个别名键，第一个命中的写进 value_begin。
inline bool locate_first_value(const char *begin, const char *end, const char *const *quoted_keys, int key_count,
                               const char **value_begin) {
    for (int i = 0; i < key_count; ++i) {
        if (locate_value(begin, end, quoted_keys[i], value_begin))
            return true;
    }
    return false;
}

// 读 `[a, b, c, d]` 形状的整数数组（bbox）。
inline bool scan_int_array4(const char *p, const char *end, long long values[4]) {
    p = skip_ws(p, end);
    if (p >= end || *p != '[')
        return false;
    ++p;
    for (int i = 0; i < 4; ++i) {
        if (!scan_long(p, end, &values[i]))
            return false;
        p = skip_ws(p, end);
        if (i < 3) {
            if (p >= end || *p != ',')
                return false;
            ++p;
        }
    }
    return true;
}

// 取 bbox：`"bbox":[x1,y1,x2,y2]`。
inline bool parse_bbox(const char *begin, const char *end, point_t *left_top, point_t *right_bottom) {
    const char *p = nullptr;
    if (!locate_value(begin, end, "\"bbox\"", &p))
        return false;
    long long values[4] = {0, 0, 0, 0};
    if (!scan_int_array4(p, end, values))
        return false;
    *left_top = point_t(static_cast<long>(values[0]), static_cast<long>(values[1]));
    *right_bottom = point_t(static_cast<long>(values[2]), static_cast<long>(values[3]));
    return true;
}

// 校验 `"code":0`；成功时把服务端码回填到 server_code。
// 返回 0 = 正常，-7 = 没有 code 字段，-8 = 服务端报错（码已回填）。
inline int check_code(const char *begin, const char *end, int *server_code, const char **code_pos) {
    const char *pos = nullptr;
    if (!locate_value(begin, end, "\"code\"", &pos))
        return -7;
    long long code = 0;
    if (!scan_long(pos, end, &code))
        return -7;
    if (server_code)
        *server_code = static_cast<int>(code);
    if (code != 0)
        return -8;
    if (code_pos)
        *code_pos = pos;
    return 0;
}

// 明确的空结果数组 `"results":[]` 才算“空是正常的”。
inline bool has_empty_results_array(const char *begin, const char *end) {
    const char *pos = nullptr;
    if (!locate_value(begin, end, "\"results\"", &pos) || pos >= end || *pos != '[')
        return false;
    const char *q = skip_ws(pos + 1, end);
    return q < end && *q == ']';
}

} // namespace op::json

#endif // OP_BASE_JSON_SCANNER_H_
