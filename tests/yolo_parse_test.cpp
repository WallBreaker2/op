// YOLO 响应解析 + 请求体构造的单元测试与 A/B。
//
// 不需要 YOLO 服务：只喂字符串给 `op::yolo::parse_yolo_response`，验证结果。
// 以前这段解析埋在 `YoloDetector::detect()` 里、用 5 条 `std::regex` 逐条匹配，
// 标签还走 "UTF-8 → GBK → ACP"，非 GBK 字符会变成 '?'（defect_fixes.md §72.34）。
#include "yolo/YoloResponseParser.h"

#include "base/Utils.h"

#include <gtest/gtest.h>
#include <windows.h>

#include <chrono>
#include <cstdio>
#include <regex>
#include <string>
#include <vector>

namespace {

std::string MakeDetections(int count) {
    std::string body = "{\"code\":0,\"results\":[";
    for (int i = 0; i < count; ++i) {
        if (i)
            body += ",";
        body += "{\"label\":\"cls" + std::to_string(i) + "\",\"class_id\":" + std::to_string(i) +
                ",\"bbox\":[" + std::to_string(i) + "," + std::to_string(i + 1) + "," + std::to_string(i + 2) + "," +
                std::to_string(i + 3) + "],\"confidence\":0." + std::to_string(100 + i % 800) + "}";
    }
    body += "]}";
    return body;
}

// 旧实现的标签转换等价物：UTF-8 -> 宽 -> ACP -> 宽。非 ACP 字符在这里就变成 '?'。
// （测试目标不链 Utils.cpp，所以这里复刻一份，用来证明旧实现确实会丢字。）
std::wstring Utf8ToWideViaAcp(const std::string &utf8) {
    if (utf8.empty())
        return L"";
    const int wide_len = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (wide_len <= 0)
        return L"";
    std::wstring wide(static_cast<size_t>(wide_len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), &wide[0], wide_len);

    const int ansi_len = WideCharToMultiByte(CP_ACP, 0, wide.data(), wide_len, nullptr, 0, "?", nullptr);
    if (ansi_len <= 0)
        return L"";
    std::string ansi(static_cast<size_t>(ansi_len), '\0');
    WideCharToMultiByte(CP_ACP, 0, wide.data(), wide_len, &ansi[0], ansi_len, "?", nullptr);

    const int back_len = MultiByteToWideChar(CP_ACP, 0, ansi.data(), ansi_len, nullptr, 0);
    if (back_len <= 0)
        return L"";
    std::wstring back(static_cast<size_t>(back_len), L'\0');
    MultiByteToWideChar(CP_ACP, 0, ansi.data(), ansi_len, &back[0], back_len);
    return back;
}

double NowUs() {
    return std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// 旧实现的等价物：5 条 static regex + 逐对象 sregex_iterator。
int ParseWithRegex(const std::string &resp, op::vyolo_rec_t &out) {
    out.clear();
    static const std::regex obj_re("\\{[^\\{\\}]*\\}");
    static const std::regex label_re("\"(?:label|class_name|name)\"\\s*:\\s*\"((?:\\\\.|[^\"\\\\])*)\"");
    static const std::regex class_id_re("\"(?:class_id|cls)\"\\s*:\\s*(-?\\d+)");
    static const std::regex bbox_re("\"bbox\"\\s*:\\s*\\[\\s*(-?\\d+)\\s*,\\s*(-?\\d+)\\s*,\\s*(-?\\d+)\\s*,\\s*(-?\\d+)\\s*\\]");
    static const std::regex conf_re("\"(?:confidence|conf)\"\\s*:\\s*([-+]?\\d*\\.?\\d+(?:[eE][-+]?\\d+)?)");

    int n = 0;
    for (std::sregex_iterator it(resp.begin(), resp.end(), obj_re), end; it != end; ++it) {
        const std::string obj = it->str();
        std::smatch bbox_m;
        std::smatch conf_m;
        if (!std::regex_search(obj, bbox_m, bbox_re) || !std::regex_search(obj, conf_m, conf_re))
            continue;
        op::yolo_rec_t rec;
        std::smatch label_m;
        std::smatch class_id_m;
        if (std::regex_search(obj, label_m, label_re))
            rec.label = Utf8ToWideViaAcp(label_m[1].str());
        if (std::regex_search(obj, class_id_m, class_id_re))
            rec.class_id = atoi(class_id_m[1].str().c_str());
        rec.left_top = op::point_t(atoi(bbox_m[1].str().c_str()), atoi(bbox_m[2].str().c_str()));
        rec.right_bottom = op::point_t(atoi(bbox_m[3].str().c_str()), atoi(bbox_m[4].str().c_str()));
        rec.confidence = static_cast<float>(atof(conf_m[1].str().c_str()));
        out.push_back(rec);
        ++n;
    }
    return n;
}

// 旧写法：先编码到临时串，再整串拼进请求体（多一次 11 MB 级别的分配 + 拷贝）。
std::string BuildRequestLegacy(const unsigned char *data, size_t size) {
    DWORD needed = 0;
    if (!CryptBinaryToStringA(data, static_cast<DWORD>(size), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr,
                              &needed))
        return std::string();
    std::string b64(needed - 1, '\0');
    if (!CryptBinaryToStringA(data, static_cast<DWORD>(size), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &b64[0],
                              &needed))
        return std::string();
    std::string req = "{\"image\":\"" + b64 + "\",\"width\":1920,\"height\":1080,\"bpp\":4}";
    return req;
}

// 新写法：表驱动 base64 直接编进请求体尾部（与 `base64_encode_append` 同构）。
std::string BuildRequestAppend(const unsigned char *data, size_t size) {
    static const char kTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string req;
    req.reserve(((size + 2) / 3) * 4 + 96);
    req += "{\"image\":\"";
    const size_t offset = req.size();
    req.resize(offset + ((size + 2) / 3) * 4);
    char *dst = &req[offset];
    size_t i = 0;
    for (; i + 3 <= size; i += 3) {
        const unsigned v = (static_cast<unsigned>(data[i]) << 16) | (static_cast<unsigned>(data[i + 1]) << 8) | data[i + 2];
        *dst++ = kTable[(v >> 18) & 0x3F];
        *dst++ = kTable[(v >> 12) & 0x3F];
        *dst++ = kTable[(v >> 6) & 0x3F];
        *dst++ = kTable[v & 0x3F];
    }
    const size_t remain = size - i;
    if (remain > 0) {
        unsigned v = static_cast<unsigned>(data[i]) << 16;
        if (remain == 2)
            v |= static_cast<unsigned>(data[i + 1]) << 8;
        *dst++ = kTable[(v >> 18) & 0x3F];
        *dst++ = kTable[(v >> 12) & 0x3F];
        *dst++ = remain == 2 ? kTable[(v >> 6) & 0x3F] : '=';
        *dst++ = '=';
    }
    req += "\",\"width\":1920,\"height\":1080,\"bpp\":4}";
    return req;
}

} // namespace

TEST(YoloParseTest, ParsesLabelClassIdBboxAndConfidence) {
    const std::string body =
        "{\"code\":0,\"results\":[{\"label\":\"person\",\"class_id\":0,\"bbox\":[10,20,30,40],\"confidence\":0.75},"
        "{\"label\":\"car\",\"class_id\":2,\"bbox\":[1,2,3,4],\"confidence\":0.5}]}";
    op::vyolo_rec_t recs;
    ASSERT_EQ(op::yolo::parse_yolo_response(body, recs), 2);
    ASSERT_EQ(recs.size(), 2u);
    EXPECT_EQ(recs[0].label, L"person");
    EXPECT_EQ(recs[0].class_id, 0);
    EXPECT_EQ(recs[0].left_top.x, 10);
    EXPECT_EQ(recs[0].left_top.y, 20);
    EXPECT_EQ(recs[0].right_bottom.x, 30);
    EXPECT_EQ(recs[0].right_bottom.y, 40);
    EXPECT_FLOAT_EQ(recs[0].confidence, 0.75f);
    EXPECT_EQ(recs[1].label, L"car");
    EXPECT_EQ(recs[1].class_id, 2);
}

TEST(YoloParseTest, KeepsLabelsOutsideGbk) {
    // 韩文 + emoji：旧实现走 utf8_to_ansi 会被压成 '?'，这条在旧实现下必红。
    const std::string body =
        "{\"code\":0,\"results\":[{\"label\":\"\xED\x95\x9C\xEA\xB8\x80\",\"class_id\":1,\"bbox\":[0,0,1,1],"
        "\"confidence\":0.9},{\"label\":\"ok \xF0\x9F\x98\x80\",\"class_id\":2,\"bbox\":[2,2,3,3],"
        "\"confidence\":0.8}]}";
    op::vyolo_rec_t recs;
    ASSERT_EQ(op::yolo::parse_yolo_response(body, recs), 2);
    EXPECT_EQ(recs[0].label, L"\uD55C\uAE00");
    // 用 UTF-16 码元拼期望值，避免 MSVC 对 \U 转义在 wchar_t 里的处理差异。
    std::wstring emoji_label = L"ok ";
    emoji_label.push_back(static_cast<wchar_t>(0xD83D));
    emoji_label.push_back(static_cast<wchar_t>(0xDE00));
    EXPECT_EQ(recs[1].label, emoji_label);
}

TEST(YoloParseTest, SkipsRecordsWithoutBboxOrConfidence) {
    const std::string body =
        "{\"code\":0,\"results\":[{\"label\":\"a\",\"bbox\":[0,0,1,1],\"confidence\":0.9},"
        "{\"label\":\"no_bbox\",\"confidence\":0.9},"
        "{\"label\":\"no_conf\",\"bbox\":[0,0,1,1]}]}";
    op::vyolo_rec_t recs;
    ASSERT_EQ(op::yolo::parse_yolo_response(body, recs), 1);
    ASSERT_EQ(recs.size(), 1u);
    EXPECT_EQ(recs[0].label, L"a");
}

TEST(YoloParseTest, EmptyResultsArrayIsSuccess) {
    op::vyolo_rec_t recs;
    EXPECT_EQ(op::yolo::parse_yolo_response("{\"code\":0,\"results\":[]}", recs), 0);
    EXPECT_TRUE(recs.empty());
}

TEST(YoloParseTest, NonZeroCodeIsReportedWithValue) {
    op::vyolo_rec_t recs;
    int server_code = -1;
    EXPECT_EQ(op::yolo::parse_yolo_response("{\"code\":7,\"msg\":\"bad\"}", recs, &server_code), -8);
    EXPECT_EQ(server_code, 7);
    EXPECT_TRUE(recs.empty());
}

TEST(YoloParseTest, ScannerIsFasterThanRegexParser) {
    const std::string body = MakeDetections(50);

    op::vyolo_rec_t regex_recs;
    op::vyolo_rec_t scan_recs;
    const int regex_count = ParseWithRegex(body, regex_recs);
    const int scan_count = op::yolo::parse_yolo_response(body, scan_recs);
    ASSERT_EQ(regex_count, 50);
    ASSERT_EQ(scan_count, 50);
    ASSERT_EQ(regex_recs.size(), scan_recs.size());
    for (size_t i = 0; i < regex_recs.size(); ++i) {
        EXPECT_EQ(regex_recs[i].label, scan_recs[i].label);
        EXPECT_EQ(regex_recs[i].class_id, scan_recs[i].class_id);
        EXPECT_EQ(regex_recs[i].left_top.x, scan_recs[i].left_top.x);
        EXPECT_EQ(regex_recs[i].left_top.y, scan_recs[i].left_top.y);
        EXPECT_FLOAT_EQ(regex_recs[i].confidence, scan_recs[i].confidence);
    }

    const int kRounds = 20;
    const double t0 = NowUs();
    for (int i = 0; i < kRounds; ++i)
        ParseWithRegex(body, regex_recs);
    const double regex_us = (NowUs() - t0) / kRounds;

    const double t1 = NowUs();
    for (int i = 0; i < kRounds; ++i)
        op::yolo::parse_yolo_response(body, scan_recs);
    const double scan_us = (NowUs() - t1) / kRounds;

    std::printf("[BENCH] %zu 条结果 / %zu 字节：regex %.3f us/次，扫描 %.3f us/次，加速 %.1fx\n", scan_recs.size(),
                body.size(), regex_us, scan_us, regex_us / (scan_us > 0 ? scan_us : 1));
    EXPECT_GT(regex_us / (scan_us > 0 ? scan_us : 1), 2.0) << "扫描器应当显著快于 regex 实现";
}

TEST(YoloParseTest, AppendingBase64IntoRequestAvoidsExtraCopy) {
    // 1080p 4bpp = 8.29 MB；编码后约 11 MB。旧写法要多分配并多拷一份。
    const int width = 1920;
    const int height = 1080;
    const size_t pixel_bytes = static_cast<size_t>(width) * height * 4;
    std::vector<unsigned char> pixels(pixel_bytes);
    for (size_t i = 0; i < pixel_bytes; ++i)
        pixels[i] = static_cast<unsigned char>((i * 31) & 0xFF);

    const std::string legacy = BuildRequestLegacy(pixels.data(), pixel_bytes);
    const std::string append = BuildRequestAppend(pixels.data(), pixel_bytes);
    ASSERT_FALSE(legacy.empty());
    ASSERT_FALSE(append.empty());
    // 逐字符比对，差异处只打印局部窗口，避免 gtest 把 11 MB 字符串整个吐出来。
    ASSERT_EQ(legacy.size(), append.size()) << "两种构造方式产出的请求体长度不一致";
    size_t diff_at = legacy.size();
    for (size_t i = 0; i < legacy.size(); ++i) {
        if (legacy[i] != append[i]) {
            diff_at = i;
            break;
        }
    }
    if (diff_at != legacy.size()) {
        const size_t from = diff_at > 16 ? diff_at - 16 : 0;
        const size_t len = 48;
        std::printf("[DIFF] at=%zu legacy='%s' append='%s'\n", diff_at,
                    legacy.substr(from, len).c_str(), append.substr(from, len).c_str());
    }
    ASSERT_EQ(diff_at, legacy.size()) << "两种构造方式必须产出完全一致的请求体";

    const int kRounds = 5;
    const double t0 = NowUs();
    for (int i = 0; i < kRounds; ++i) {
        const std::string req = BuildRequestLegacy(pixels.data(), pixel_bytes);
        ASSERT_FALSE(req.empty());
    }
    const double legacy_us = (NowUs() - t0) / kRounds;

    const double t1 = NowUs();
    for (int i = 0; i < kRounds; ++i) {
        const std::string req = BuildRequestAppend(pixels.data(), pixel_bytes);
        ASSERT_FALSE(req.empty());
    }
    const double append_us = (NowUs() - t1) / kRounds;

    std::printf("[BENCH] 1080p4bpp 请求体 %.2f MB：CryptoAPI+拼接 %.1f us，表驱动就地追加 %.1f us（省 %.1f%%）\n",
                static_cast<double>(append.size()) / (1024.0 * 1024.0), legacy_us, append_us,
                (legacy_us - append_us) * 100.0 / (legacy_us > 0 ? legacy_us : 1));
    EXPECT_LT(append_us, legacy_us) << "表驱动编码应当快于“CryptBinaryToStringA + 拼接”";
}
