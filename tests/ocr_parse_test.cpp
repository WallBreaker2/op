// OCR 响应解析的单元测试 + A/B。
//
// 这里**不需要 OCR 服务**：只喂字符串给 `op::ocr::parse_ocr_response`，验证解析结果。
// 以前这段解析埋在 `HttpOcrService::ocr()` 里、用 `std::regex` 实现，既没法单测（要起服务），
// 又慢（50 条结果上百次正则搜索，见 defect_fixes.md §72.33）。
#include "ocr/OcrResponseParser.h"

#include "base/Utils.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>
#include <regex>
#include <string>

namespace {

std::string MakeResponse(int count) {
    std::string body = "{\"code\":0,\"results\":[";
    for (int i = 0; i < count; ++i) {
        if (i)
            body += ',';
        body += "{\"text\":\"行" + std::to_string(i) + "\",\"bbox\":[" + std::to_string(i) + "," +
                std::to_string(i + 1) + "," + std::to_string(i + 2) + "," + std::to_string(i + 3) +
                "],\"confidence\":0." + std::to_string(90 + i % 10) + "}";
    }
    body += "]}";
    return body;
}

// 旧实现的等价物：整段用正则扫，作为 A/B 的对照（只在本测试里保留，不参与产品）。
int ParseWithRegex(const std::string &resp, op::vocr_rec_t &result) {
    result.clear();
    static const std::regex code_re("\"code\"\\s*:\\s*(-?\\d+)");
    std::smatch code_match;
    if (!std::regex_search(resp, code_match, code_re))
        return -7;
    if (atoi(code_match[1].str().c_str()) != 0)
        return -8;

    static const std::regex obj_re("\\{[^\\{\\}]*\\}");
    static const std::regex text_re("\"text\"\\s*:\\s*\"((?:\\\\.|[^\"\\\\])*)\"");
    static const std::regex bbox_re("\"bbox\"\\s*:\\s*\\[\\s*(-?\\d+)\\s*,\\s*(-?\\d+)\\s*,\\s*(-?\\d+)\\s*,\\s*(-?\\d+)\\s*\\]");
    static const std::regex conf_re("\"confidence\"\\s*:\\s*([-+]?\\d*\\.?\\d+(?:[eE][-+]?\\d+)?)");

    for (std::sregex_iterator it(resp.begin(), resp.end(), obj_re), end; it != end; ++it) {
        const std::string obj = it->str();
        std::smatch text_m;
        std::smatch bbox_m;
        std::smatch conf_m;
        if (!std::regex_search(obj, text_m, text_re) || !std::regex_search(obj, bbox_m, bbox_re) ||
            !std::regex_search(obj, conf_m, conf_re)) {
            continue;
        }
        op::ocr_rec_t rec;
        rec.left_top = op::point_t(atoi(bbox_m[1].str().c_str()), atoi(bbox_m[2].str().c_str()));
        rec.right_bottom = op::point_t(atoi(bbox_m[3].str().c_str()), atoi(bbox_m[4].str().c_str()));
        rec.confidence = static_cast<float>(atof(conf_m[1].str().c_str()));
        rec.text = utf8_to_wstring(text_m[1].str());
        result.push_back(rec);
    }
    return static_cast<int>(result.size());
}

} // namespace

TEST(OcrParseTest, ParsesTextBboxAndConfidence) {
    const std::string body =
        "{\"code\":0,\"results\":["
        "{\"text\":\"hello\",\"bbox\":[1,2,30,40],\"confidence\":0.98},"
        "{\"text\":\"世界\",\"bbox\":[5,6,70,80],\"confidence\":0.5}"
        "]}";

    op::vocr_rec_t result;
    ASSERT_EQ(op::ocr::parse_ocr_response(body, result), 2);
    ASSERT_EQ(result.size(), 2u);

    EXPECT_EQ(result[0].text, L"hello");
    EXPECT_EQ(result[0].left_top.x, 1);
    EXPECT_EQ(result[0].left_top.y, 2);
    EXPECT_EQ(result[0].right_bottom.x, 30);
    EXPECT_EQ(result[0].right_bottom.y, 40);
    EXPECT_FLOAT_EQ(result[0].confidence, 0.98f);

    EXPECT_EQ(result[1].text, L"世界");
    EXPECT_FLOAT_EQ(result[1].confidence, 0.5f);
}

// 服务端给 UTF-8；旧实现走 "UTF-8 → GBK(936) → ACP → 宽字符"，非 GBK 字符会被压成 '?'。
// 这条用例锁住"不丢字"。
TEST(OcrParseTest, KeepsCharactersOutsideGbk) {
    const std::string body =
        "{\"code\":0,\"results\":[{\"text\":\"\xED\x95\x9C\xEA\xB8\x80 \xF0\x9F\x98\x80\",\"bbox\":[0,0,10,10],"
        "\"confidence\":0.9}]}";

    op::vocr_rec_t result;
    ASSERT_EQ(op::ocr::parse_ocr_response(body, result), 1);
    // U+D55C U+AE00 U+0020 U+1F600
    EXPECT_EQ(result[0].text, std::wstring(L"\uD55C\uAE00 \U0001F600"));
}

TEST(OcrParseTest, EmptyResultsArrayIsSuccess) {
    op::vocr_rec_t result;
    EXPECT_EQ(op::ocr::parse_ocr_response("{\"code\":0,\"results\":[]}", result), 0);
    EXPECT_TRUE(result.empty());
}

TEST(OcrParseTest, NonZeroCodeIsReportedWithValue) {
    op::vocr_rec_t result;
    int server_code = 0;
    EXPECT_EQ(op::ocr::parse_ocr_response("{\"code\":7,\"message\":\"boom\"}", result, &server_code), -8);
    EXPECT_EQ(server_code, 7);
    EXPECT_TRUE(result.empty());
}

TEST(OcrParseTest, MissingCodeAndGarbageFail) {
    op::vocr_rec_t result;
    EXPECT_EQ(op::ocr::parse_ocr_response("{\"results\":[]}", result), -7) << "没有 code 字段";
    EXPECT_EQ(op::ocr::parse_ocr_response("not json at all", result), -7);
    EXPECT_EQ(op::ocr::parse_ocr_response("{\"code\":0,\"foo\":1}", result), -9)
        << "既没结果也没 results:[] 标记，应当报解析失败";
}

TEST(OcrParseTest, UnescapesQuotesAndNewlines) {
    const std::string body =
        "{\"code\":0,\"results\":[{\"text\":\"a\\\"b\\nc\",\"bbox\":[0,0,1,1],\"confidence\":1}]}";
    op::vocr_rec_t result;
    ASSERT_EQ(op::ocr::parse_ocr_response(body, result), 1);
    EXPECT_EQ(result[0].text, L"a\"b\nc");
}

// 只打印、不断言时间（机器差异会让时间断言 flaky）。关注点是数量级：
// 正则版每解析一条都要新建 smatch 并拷子串，扫描版只走一遍字节。
TEST(OcrParseTest, ScannerIsFasterThanRegexParser) {
    const std::string body = MakeResponse(50);

    op::vocr_rec_t regex_result;
    ASSERT_EQ(ParseWithRegex(body, regex_result), 50);
    op::vocr_rec_t scan_result;
    ASSERT_EQ(op::ocr::parse_ocr_response(body, scan_result), 50);
    ASSERT_EQ(regex_result.size(), scan_result.size());
    for (size_t i = 0; i < regex_result.size(); ++i) {
        EXPECT_EQ(regex_result[i].text, scan_result[i].text) << "第 " << i << " 条文本不一致";
        EXPECT_EQ(regex_result[i].left_top.x, scan_result[i].left_top.x);
        EXPECT_EQ(regex_result[i].right_bottom.y, scan_result[i].right_bottom.y);
    }

    constexpr int kRounds = 200;
    const auto regex_start = std::chrono::steady_clock::now();
    for (int i = 0; i < kRounds; ++i) {
        op::vocr_rec_t out;
        ParseWithRegex(body, out);
    }
    const auto regex_end = std::chrono::steady_clock::now();

    const auto scan_start = std::chrono::steady_clock::now();
    for (int i = 0; i < kRounds; ++i) {
        op::vocr_rec_t out;
        op::ocr::parse_ocr_response(body, out);
    }
    const auto scan_end = std::chrono::steady_clock::now();

    const double regex_us = std::chrono::duration<double, std::micro>(regex_end - regex_start).count() / kRounds;
    const double scan_us = std::chrono::duration<double, std::micro>(scan_end - scan_start).count() / kRounds;
    std::cout << "[BENCH] 50 条结果 / " << body.size() << " 字节：regex " << regex_us << " us/次，扫描 "
              << scan_us << " us/次，加速 " << (scan_us > 0 ? regex_us / scan_us : 0.0) << "x" << std::endl;
    EXPECT_GT(regex_us, scan_us) << "扫描版应当快于正则版";
}
