#pragma once
#ifndef OP_OCR_DICTIONARY_H_
#define OP_OCR_DICTIONARY_H_
#include "../runtime/RuntimeUtils.h"
#include "Image.h"
#include "BitFunctions.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
// #define SET_BIT(x, idx) x |= 1u << (idx)

// #define GET_BIT(x, idx) (((x )>> (idx)) & 1u)

namespace op::ocr {

const int op_dict_version = 2;

/*
第 0 代字库
*/
struct word_info_t {
    // char of word
    wchar_t _char[4];
    // char height
    __int16 width, height;
    // char bit ct
    __int32 bit_count;
    word_info_t() : width(0), height(0), bit_count(0) {
        memset(_char, 0, sizeof(_char));
    }
    bool operator==(const word_info_t &rhs) {
        return width == rhs.width && height == rhs.height;
    }
    bool operator!=(const word_info_t &rhs) {
        return width != rhs.width || height != rhs.height;
    }
};
struct word_t {

    // 32 bit a col
    using cline_t = unsigned __int32;
    word_info_t info;
    // char col line
    cline_t clines[32];
    bool operator==(const word_t &rhs) {
        if (info != rhs.info)
            return false;
        for (int i = 0; i < info.width; ++i)
            if (clines[i] != rhs.clines[i])
                return false;
        return true;
    }
    void set_chars(const std::wstring &s) {
        memcpy(info._char, s.c_str(), min(sizeof(info._char), (s.length() + 1) * sizeof(wchar_t)));
    }
    // 从 dm 字库中 的一个点阵转化为op的点阵
    void fromDm(const wchar_t *str, int ct, const std::wstring &w) {
        int bin[50] = {0};
        constexpr int DM_DICT_HEIGTH = 11;
        ct = min(ct, 88);
        int i = 0;
        auto hex2bin = [](wchar_t c) { return c <= L'9' ? c - L'0' : c - L'A' + 10; };
        while (i < ct) {

            bin[i / 2] = (hex2bin(str[i]) << 4) | (hex2bin(str[i + 1]));
            i += 2;
        }
        //
        int cols = (ct * 4) / DM_DICT_HEIGTH;
        memset(this, 0x0, sizeof(*this));
        for (int j = 0; j < cols; ++j) {
            for (int i = 0; i < 11; ++i) {
                int idx = j * 11 + i;
                if (GET_BIT(bin[idx >> 3], 7 - (idx & 7))) {
                    SET_BIT(clines[j], 31 - i);
                    ++info.bit_count;
                }
            }
        }
        info.height = DM_DICT_HEIGTH;
        info.width = cols;
        set_chars(w);
    }
};
/*
第 1 代字库
*/
struct word1_info {
    uint8_t w, h;     // max is 255 2B
    uint16_t bit_cnt; // max is 255*255=65025<65536 4B
    wchar_t name[8];  // name 12B
    word1_info() : w(0), h(0), bit_cnt(0) {
    }
};
struct word1_t {
    word1_info info;
    vector<uint8_t> data; // size is (w*h+7)/8
    bool operator==(const word1_t &rhs) {
        if (info.w != rhs.info.w || info.h != rhs.info.h || info.bit_cnt != rhs.info.bit_cnt)
            return false;
        if (data.size() != rhs.data.size())
            return false;
        for (size_t i = 0; i < data.size(); i++)
            if (data[i] != rhs.data[i])
                return false;
        return true;
    }
    void set_chars(const std::wstring &s) {
        const size_t nlen = std::min<size_t>(s.length(), 7);
        memcpy(info.name, s.c_str(), nlen * sizeof(wchar_t));
        info.name[nlen] = L'\0';
    }
    void from_word(word_t &wd) {
        info.w = (uint8_t)wd.info.width;
        info.h = static_cast<uint8_t>(wd.info.height);
        init();
        info.bit_cnt = wd.info.bit_count;
        memcpy(info.name, wd.info._char, 4 * sizeof(wchar_t));
        info.name[3] = 0;
        int idx = 0;

        for (int x = 0; x < info.w; x++) {
            for (int y = 0; y < info.h; y++) {
                if (GET_BIT(wd.clines[x], 31 - y))
                    SET_BIT(data[idx / 8], idx & 7);
                idx++;
            }
        }
    }
    void from_binary(const ImageBin &binary, const rect_t &rc) {
        int x2 = min(rc.x1 + 255, rc.x2);
        int y2 = min(rc.y1 + 255, rc.y2);
        info.w = x2 - rc.x1;
        info.h = y2 - rc.y1;
        info.bit_cnt = 0;
        init();
        int idx = 0;
        for (int j = rc.x1; j < x2; ++j) {
            for (int i = rc.y1; i < y2; ++i) {
                auto val = binary.at(i, j);
                if (val == 1) {
                    SET_BIT(data[idx / 8], idx & 7);

                    ++info.bit_cnt;
                }
                ++idx;
            }
        }
    }
    bool from_string(const std::wstring &s) {
        std::vector<std::wstring> vstr;
        split(s, vstr, L"$");
        if (vstr.size() != 3)
            return false;
        wstring name = vstr[0];
        wstring temp = vstr[1];
        wstring dataStr = vstr[2];
        int height = 0, width = 0, bit_count = 0;
        wchar_t tail = L'\0';
        if (swscanf(temp.c_str(), L"%d,%d,%d%c", &height, &width, &bit_count, &tail) != 3)
            return false;
        if (height <= 0 || height > 255 || width <= 0 || width > 255)
            return false;
        if (bit_count <= 0 || bit_count > width * height)
            return false;

        const size_t byte_count = (static_cast<size_t>(width) * static_cast<size_t>(height) + 7u) / 8u;
        // OP 文本条目必须与点阵尺寸完全一致，避免坏数据越界写入。
        if (dataStr.size() != byte_count * 2u)
            return false;
        for (wchar_t ch : dataStr) {
            if (!iswxdigit(ch))
                return false;
        }

        info.h = static_cast<uint8_t>(height);
        info.w = static_cast<uint8_t>(width);
        info.bit_cnt = static_cast<uint16_t>(bit_count);
        init();
        set_chars(name);
        for (size_t i = 0; i < dataStr.length(); i += 2) {
            wchar_t c1 = dataStr[i];
            wchar_t c2 = dataStr[i + 1];
            auto b1 = (uint8_t)hex2bin(c1);
            auto b2 = (uint8_t)hex2bin(c2);
            data[i / 2] = (b1 << 4) + b2;
        }
        return true;
    }
    std::wstring to_string() const {
        constexpr wchar_t hex_digits[] = L"0123456789ABCDEF";
        std::wstring tp = wstring(info.name) + L"$" + std::to_wstring(info.h) + L"," + std::to_wstring(info.w) +
                          L"," + std::to_wstring(info.bit_cnt) + L"$";
        tp.reserve(tp.size() + data.size() * 2);
        for (size_t j = 0; j < data.size(); ++j) {
            const auto byte = static_cast<unsigned char>(data[j]);
            tp.push_back(hex_digits[byte >> 4]);
            tp.push_back(hex_digits[byte & 0x0F]);
        }
        return tp;
    }

    void init() {
        data.resize((info.w * info.h + 7) / 8);
        std::fill(data.begin(), data.end(), 0);
    }
};

namespace dm_dict_compat {

// 大漠字库兼容格式：
// 点阵HEX$文本$左.右.数量$高度
// 这里只负责在导入边界把大漠点阵转换成 OP 的 word1_t，OP 内部仍然只保存自己的字库格式。
inline bool parse_dm_text_word(const std::wstring &s, word1_t &word) {
    std::vector<std::wstring> vstr;
    split(s, vstr, L"$");
    if (vstr.size() != 4 || vstr[0].empty() || vstr[1].empty() || vstr[3].empty())
        return false;

    // 第三个字段是大漠的左/右/数量信息；OP 匹配只需要点阵本身，所以校验格式后不直接使用。
    int left = 0, right = 0, declared_count = 0;
    if (swscanf(vstr[2].c_str(), L"%d.%d.%d", &left, &right, &declared_count) < 3)
        return false;
    (void)left;
    (void)right;
    (void)declared_count;

    wchar_t *end = nullptr;
    const long parsed_height = wcstol(vstr[3].c_str(), &end, 10);
    if (end == vstr[3].c_str() || (end && *end != L'\0') || parsed_height <= 0 || parsed_height > 255)
        return false;

    for (wchar_t ch : vstr[0]) {
        if (!iswxdigit(ch))
            return false;
    }

    (void)parsed_height;

    constexpr int dm_lattice_height = 11;
    const int height = dm_lattice_height;
    const size_t total_bits = vstr[0].size() * 4u;
    if (total_bits < static_cast<size_t>(height))
        return false;

    // 保持和大漠 parserDict 一致：点阵按 MaxHight 切列，默认 MaxHight 为 11。
    // 最后一段“高度”是大漠字库的附加信息，不参与 OP 本地点阵匹配的高宽计算。
    const size_t width = (total_bits % static_cast<size_t>(height)) != 0u
                             ? (total_bits - 1u) / static_cast<size_t>(height)
                             : total_bits / static_cast<size_t>(height);
    if (width == 0 || width > 255u)
        return false;

    word.info.w = static_cast<uint8_t>(width);
    word.info.h = static_cast<uint8_t>(height);
    word.info.bit_cnt = 0;
    word.init();
    word.set_chars(vstr[1]);

    size_t bit_index = 0;
    const size_t bit_count = static_cast<size_t>(word.info.w) * static_cast<size_t>(word.info.h);
    for (wchar_t ch : vstr[0]) {
        const int value = hex2bin(ch);
        for (int bit = 3; bit >= 0; --bit, ++bit_index) {
            if (bit_index >= bit_count)
                continue;
            if (((value >> bit) & 1) == 0)
                continue;

            // 转成 OP 的列优先、低位在前的 bit 存储。
            SET_BIT(word.data[bit_index / 8], bit_index & 7);
            ++word.info.bit_cnt;
        }
    }

    return word.info.bit_cnt > 0;
}

} // namespace dm_dict_compat

namespace dict_entry_importer {

enum class text_entry_format {
    unknown,
    op_entry,
    dm_txt_entry,
};

// 文本字库行/单条字形文本导入分发层。
// 注意：OP 正式字库文件是二进制 .dict，由 Dictionary::read_dict 读取，不经过这里。
// 这里的 OP 三段格式只表示 txt 文本字库、AddDict、GetDict、FetchWord 使用的单条字形文本数据。
inline text_entry_format detect_text_entry_format(const std::wstring &s) {
    std::vector<std::wstring> vstr;
    split(s, vstr, L"$");
    if (vstr.size() == 3)
        return text_entry_format::op_entry;
    if (vstr.size() == 4)
        return text_entry_format::dm_txt_entry;
    return text_entry_format::unknown;
}

inline bool parse_op_text_entry(const std::wstring &s, word1_t &word) {
    return word.from_string(s);
}

inline bool parse_text_dict_entry(const std::wstring &s, word1_t &word) {
    switch (detect_text_entry_format(s)) {
    case text_entry_format::op_entry:
        return parse_op_text_entry(s, word);
    case text_entry_format::dm_txt_entry:
        return dm_dict_compat::parse_dm_text_word(s, word);
    default:
        return false;
    }
}

} // namespace dict_entry_importer

struct Dictionary {
    // SetMemDict 没有文件名，不能靠后缀判断；这里按内存内容识别字库格式。
    enum class memory_format {
        invalid,
        op_binary,
        text,
    };

    // v0 v1
    struct dict_info_t {
        __int16 _this_ver; // 0 1
        __int16 _word_count;
        // check code=_this_ver^_word_count
        __int32 _check_code;
        dict_info_t() : _this_ver(1), _word_count(0) {
            _check_code = _word_count ^ _this_ver;
        }
    };
    dict_info_t info;
    Dictionary() {
    }
    std::vector<word1_t> words;
    static bool read_bytes(const unsigned char *buf, size_t size, size_t &offset, void *out, size_t out_size) {
        if (!buf || !out || offset > size || out_size > size - offset)
            return false;
        std::memcpy(out, buf + offset, out_size);
        offset += out_size;
        return true;
    }
    static bool valid_word_info(const word1_info &head) {
        if (head.w == 0 || head.h == 0)
            return false;
        const size_t bit_count = static_cast<size_t>(head.w) * static_cast<size_t>(head.h);
        return head.bit_cnt > 0 && static_cast<size_t>(head.bit_cnt) <= bit_count;
    }
    // OP 二进制字库没有 magic，这里用头部校验和完整长度校验来避免文本误判。
    static bool is_binary_dict_memory(const void *data, size_t size) {
        const auto *buf = static_cast<const unsigned char *>(data);
        size_t offset = 0;
        dict_info_t file_info;
        if (!read_bytes(buf, size, offset, &file_info, sizeof(file_info)))
            return false;
        if (file_info._word_count <= 0)
            return false;
        if (file_info._check_code != (file_info._this_ver ^ file_info._word_count))
            return false;

        const size_t word_count = static_cast<size_t>(file_info._word_count);
        if (file_info._this_ver == 0) {
            if (word_count > (size - offset) / sizeof(word_t))
                return false;
            for (size_t i = 0; i < word_count; ++i) {
                word_t old_word;
                if (!read_bytes(buf, size, offset, &old_word, sizeof(old_word)))
                    return false;
                if (old_word.info.width <= 0 || old_word.info.height <= 0 || old_word.info.width > 255 ||
                    old_word.info.height > 255 || old_word.info.bit_count <= 0 ||
                    old_word.info.bit_count > old_word.info.width * old_word.info.height)
                    return false;
            }
            return offset == size;
        }

        if (file_info._this_ver != 1)
            return false;

        for (size_t i = 0; i < word_count; ++i) {
            word1_info head;
            if (!read_bytes(buf, size, offset, &head, sizeof(head)))
                return false;
            if (!valid_word_info(head))
                return false;
            const size_t data_size = (static_cast<size_t>(head.w) * static_cast<size_t>(head.h) + 7u) / 8u;
            if (offset > size || data_size > size - offset)
                return false;
            offset += data_size;
        }
        return offset == size;
    }
    // 先识别合法 OP 二进制字库；不符合时，再按文本字库内容处理。
    static memory_format detect_memory_format(const void *data, size_t size) {
        if (!data || size == 0)
            return memory_format::invalid;
        if (is_binary_dict_memory(data, size))
            return memory_format::op_binary;

        const auto *buf = static_cast<const unsigned char *>(data);
        for (size_t i = 0; i < size; ++i) {
            if (buf[i] == '$')
                return memory_format::text;
        }
        return memory_format::invalid;
    }
    void read_dict(const std::wstring &s) {
        if (s.empty())
            return;

        // SetDict 的入口：优先识别 OP 二进制 .dict；不符合时再按大漠文本字库读取。
        if (read_binary_dict(s))
            return;
        read_dict_dm(s);
    }

    bool read_binary_dict(const std::wstring &s) {
        clear();
        std::fstream file;
        file.open(s, std::ios::in | std::ios::binary);
        if (!file.is_open())
            return false;
        // 读取头信息
        dict_info_t file_info;
        if (!file.read((char *)&file_info, sizeof(file_info))) {
            file.close();
            return false;
        }

        // 校验
        if (file_info._word_count < 0 || file_info._check_code != (file_info._this_ver ^ file_info._word_count)) {
            file.close();
            return false;
        }

        info = file_info;
        if (file_info._this_ver == 0) {
            // old dict format
            words.resize(file_info._word_count);
            info._this_ver = 1;
            word_t tmp;
            for (size_t i = 0; i < words.size(); i++) {
                if (!file.read((char *)&tmp, sizeof(tmp))) {
                    clear();
                    file.close();
                    return false;
                }
                words[i].from_word(tmp);
            }
            info._check_code = info._this_ver ^ info._word_count;
            // file.read((char*)&words[0], sizeof(word_t)*info._word_count);
        } else if (file_info._this_ver == 1) {
            // new dict format
            words.resize(file_info._word_count);
            word1_info head;
            for (size_t i = 0; i < words.size(); i++) {
                if (!file.read((char *)&head, sizeof(head))) {
                    clear();
                    file.close();
                    return false;
                }

                words[i].info = head;
                const size_t nlen = (static_cast<size_t>(head.w) * static_cast<size_t>(head.h) + 7u) / 8u;
                words[i].data.resize(nlen);
                if (!file.read((char *)words[i].data.data(), static_cast<std::streamsize>(nlen))) {
                    clear();
                    file.close();
                    return false;
                }
            }
        } else {
            file.close();
            return false;
        }
        file.close();
        sort_dict();
        return true;
    }
    bool read_binary_dict_memory(const void *data, size_t size) {
        clear();
        if (!is_binary_dict_memory(data, size))
            return false;

        const auto *buf = static_cast<const unsigned char *>(data);
        size_t offset = 0;
        dict_info_t file_info;
        if (!read_bytes(buf, size, offset, &file_info, sizeof(file_info)))
            return false;

        info = file_info;
        words.resize(static_cast<size_t>(file_info._word_count));
        if (file_info._this_ver == 0) {
            info._this_ver = 1;
            word_t tmp;
            for (size_t i = 0; i < words.size(); ++i) {
                if (!read_bytes(buf, size, offset, &tmp, sizeof(tmp))) {
                    clear();
                    return false;
                }
                words[i].from_word(tmp);
            }
            info._check_code = info._this_ver ^ info._word_count;
        } else if (file_info._this_ver == 1) {
            word1_info head;
            for (size_t i = 0; i < words.size(); ++i) {
                if (!read_bytes(buf, size, offset, &head, sizeof(head))) {
                    clear();
                    return false;
                }
                words[i].info = head;
                const size_t data_size = (static_cast<size_t>(head.w) * static_cast<size_t>(head.h) + 7u) / 8u;
                words[i].data.resize(data_size);
                if (!read_bytes(buf, size, offset, words[i].data.data(), data_size)) {
                    clear();
                    return false;
                }
            }
        } else {
            clear();
            return false;
        }

        if (offset != size) {
            clear();
            return false;
        }
        sort_dict();
        return true;
    }
    void read_dict_dm(const std::wstring &s) {
        clear();
        std::fstream file;
        file.open(s, std::ios::in);
        if (!file.is_open())
            return;
        // 读取信息
        std::wstring ss;
        std::string str;
        while (std::getline(file, str)) {
            std::string strLocale = setlocale(LC_ALL, "");
            const char *chSrc = str.c_str();
            size_t nDestSize = mbstowcs(NULL, chSrc, 0) + 1;
            wchar_t *wchDest = new wchar_t[nDestSize];
            wmemset(wchDest, 0, nDestSize);
            mbstowcs(wchDest, chSrc, nDestSize);
            std::wstring wstrResult = wchDest;
            delete[] wchDest;
            setlocale(LC_ALL, strLocale.c_str());
            ss = wstrResult;
            size_t idx1 = ss.find(L'$');
            auto idx2 = ss.find(L'$', idx1 + 1);
            word1_t wd1;
            // 文件导入只接受大漠文本格式；OP 三段文本用于 AddDict 和内存文本字库。
            if (idx1 != -1 && idx2 != -1 && dm_dict_compat::parse_dm_text_word(ss, wd1))
                add_word(wd1);
        }
        file.close();
        sort_dict();
    }
    void read_memory_dict_dm(const char *buf, size_t size) {
        clear();
        std::stringstream file;
        file.write(buf, size);

        // 读取信息
        std::wstring ss;
        std::string str;
        while (std::getline(file, str)) {
            std::string strLocale = setlocale(LC_ALL, "");
            const char *chSrc = str.c_str();
            size_t nDestSize = mbstowcs(NULL, chSrc, 0) + 1;
            wchar_t *wchDest = new wchar_t[nDestSize];
            wmemset(wchDest, 0, nDestSize);
            mbstowcs(wchDest, chSrc, nDestSize);
            std::wstring wstrResult = wchDest;
            delete[] wchDest;
            setlocale(LC_ALL, strLocale.c_str());
            ss = wstrResult;
            size_t idx1 = ss.find(L'$');
            auto idx2 = ss.find(L'$', idx1 + 1);
            word1_t wd1;
            if (idx1 != -1 && idx2 != -1 && dict_entry_importer::parse_text_dict_entry(ss, wd1))
                add_word(wd1);
        }
        sort_dict();
    }
    bool read_memory_dict(const void *data, size_t size) {
        // 内存入口支持完整 .dict 字节，也支持按行保存的文本字库。
        switch (detect_memory_format(data, size)) {
        case memory_format::op_binary:
            return read_binary_dict_memory(data, size);
        case memory_format::text:
            read_memory_dict_dm(static_cast<const char *>(data), size);
            return !empty();
        default:
            clear();
            return false;
        }
    }

    bool write_dict(const std::wstring &s) {
        std::fstream file;
        file.open(s, std::ios::out | std::ios::binary);
        if (!file.is_open())
            return false;
        // 删除所有空的字符
        auto it = words.begin();
        while (it != words.end()) {
            if (it->info.name[0] == L'\0')
                it = words.erase(it);
            else
                ++it;
        }
        info._word_count = static_cast<decltype(info._word_count)>(words.size());
        // 设置校验

        info._check_code = info._this_ver ^ info._word_count;
        // 写入信息
        file.write((char *)&info, sizeof(info));
        // 写入数据
        for (size_t i = 0; i < words.size(); i++) {
            file.write((char *)&words[i].info, sizeof(word1_info));
            file.write((char *)words[i].data.data(), static_cast<std::streamsize>(words[i].data.size()));
        }
        file.close();
        return true;
    }
    void add_word(const ImageBin &binary, const rect_t &rc) {
        word1_t word;
        word.from_binary(binary, rc);
        auto it = find(word);
        if (words.empty() || it == words.end()) {
            word.set_chars(L"");

            words.push_back(word);
            info._word_count = static_cast<decltype(info._word_count)>(words.size());
        } else { // only change char
            // word.set_chars(c);
        }
    }

    void sort_dict() {
        // sort dict(size: big --> small ,cnt: small -->big)
        std::stable_sort(words.begin(), words.end(), [](const word1_t &lhs, const word1_t &rhs) {
            int dh = lhs.info.h - rhs.info.h;
            int dw = lhs.info.w - rhs.info.w;
            return dh > 0 || (dh == 0 && dw > 0) || (dh == 0 && dw == 0 && lhs.info.bit_cnt < rhs.info.bit_cnt);
        });
    }

    void add_word(const word1_t &word) {
        auto it = find(word);
        if (words.empty() || it == words.end()) {
            words.push_back(word);
        } else {
            it->set_chars(word.info.name);
        }
        info._word_count = static_cast<decltype(info._word_count)>(words.size());
    }
    void clear() {
        info._word_count = 0;
        words.clear();
    }
    std::vector<word1_t>::iterator find(const word1_t &word) {
        for (auto it = words.begin(); it != words.end(); ++it)
            if (*it == word)
                return it;
        return words.end();
    }

    void erase(const word1_t &word) {
        auto it = find(word);
        if (!words.empty() && it != words.end())
            words.erase(it);
        info._word_count = static_cast<decltype(info._word_count)>(words.size());
    }

    int size() const {
        return info._word_count;
    }

    bool empty() const {
        return size() == 0;
    }
};

} // namespace op::ocr

#endif // OP_OCR_DICTIONARY_H_
