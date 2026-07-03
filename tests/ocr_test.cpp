#include "test_support.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <windows.h>

using namespace std;
using OcrFixture = test_support::OcrTest;

namespace {

std::wstring GetGeneratedOcrBmpPath() {
    return test_support::GetTempBmpPath(L"op_test_ocr_console_like.bmp");
}

void ExpectHelloWorldDigits(const std::wstring &text) {
    const std::wstring normalized_letters = test_support::NormalizeOcrLetters(text);
    const std::wstring digits = test_support::ExtractDigits(text);
    EXPECT_LE(test_support::EditDistance(normalized_letters, L"helloworld"), 3u) << "normalized letters differ too much";
    EXPECT_NE(digits.find(L"12345"), wstring::npos) << "missing digit anchor 12345";
}

std::string WideToUtf8(const std::wstring &text) {
    if (text.empty())
        return "";
    const int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0)
        return "";
    std::string out(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), out.data(), size, nullptr, nullptr);
    return out;
}

} // namespace

TEST_F(OcrFixture, SetDictAndUseDict) {
    op.SetDict(0, L"../bin/miner/miner_num.dict", &ret);
    cout << "SetDict(0, miner_num.dict): " << ret << endl;

    op.UseDict(0, &ret);
    cout << "UseDict(0): " << ret << endl;
}

TEST_F(OcrFixture, GetDictCount) {
    op.SetDict(0, L"../bin/miner/miner_num.dict", &ret);
    op.UseDict(0, &ret);

    long dict_count = 0;
    op.GetDictCount(0, &dict_count);
    cout << "GetDictCount(0): " << dict_count << endl;
}

TEST_F(OcrFixture, GetNowDict) {
    long now_dict_idx = -1;
    op.GetNowDict(&now_dict_idx);
    cout << "GetNowDict(): " << now_dict_idx << endl;
}

TEST_F(OcrFixture, GetDictEntry) {
    op.SetDict(0, L"../bin/miner/miner_num.dict", &ret);
    op.UseDict(0, &ret);

    wstring dict_info;
    op.GetDict(0, 0, dict_info);
    wcout << L"GetDict(0, 0): " << (dict_info.empty() ? L"empty" : dict_info.substr(0, 30)) << L"..." << endl;
}

TEST_F(OcrFixture, AddAndClearDict) {
    op.AddDict(1, L"1$1$1$1$1$1$1$1$1", &ret);
    cout << "AddDict(1, test_entry): " << ret << endl;

    op.ClearDict(1, &ret);
    cout << "ClearDict(1): " << ret << endl;
}

TEST_F(OcrFixture, SaveDict) {
    op.AddDict(1, L"1$1$1$1$1$1$1$1$1", &ret);
    op.SaveDict(1, L"test_dict_output.dict", &ret);
    cout << "SaveDict(1, test_dict_output.dict): " << ret << endl;
    op.ClearDict(1, &ret);
}

TEST_F(OcrFixture, SetMemDict) {
    const wchar_t *mem_dict_data = L"2$2$2$2$2$2$2$2$2";
    op.SetMemDict(2, mem_dict_data, (long)(wcslen(mem_dict_data) * sizeof(wchar_t)), &ret);
    cout << "SetMemDict(2): " << ret << endl;
}

TEST_F(OcrFixture, Ocr) {
    wstring text;
    op.Ocr(0, 0, 100, 100, L"ffffff-000000", 0.8, text);
    wcout << L"Ocr(0,0,100,100): " << (text.empty() ? L"(empty)" : text) << endl;
}

TEST_F(OcrFixture, OcrEx) {
    wstring text;
    op.OcrEx(0, 0, 100, 100, L"ffffff-000000", 0.8, text);
    wcout << L"OcrEx(0,0,100,100): " << (text.empty() ? L"(empty)" : text.substr(0, 50)) << endl;
}

TEST_F(OcrFixture, OcrAuto) {
    wstring text;
    op.OcrAuto(0, 0, 100, 100, 0.8, text);
    wcout << L"OcrAuto(0,0,100,100): " << (text.empty() ? L"(empty)" : text) << endl;
}

TEST_F(OcrFixture, OcrFromFile) {
    const std::wstring path = GetGeneratedOcrBmpPath();
    ASSERT_TRUE(test_support::CreateConsoleLikeBmp(path, L"Hello World! 12345"));

    wstring text;
    op.OcrFromFile(path.c_str(), L"ffffff-000000", 0.8, text);
    wcout << L"OcrFromFile(" << path << L"): " << (text.empty() ? L"(empty)" : text) << endl;
    ExpectHelloWorldDigits(text);
}

TEST_F(OcrFixture, OcrFromFileWithColorFilter) {
    const std::wstring path = GetGeneratedOcrBmpPath();
    ASSERT_TRUE(test_support::CreateConsoleLikeBmp(path, L"Hello World! 12345"));

    std::wstring text;
    op.OcrFromFile(path.c_str(), L"ffffff-000000", 0.8, text);
    wcout << L"OcrFromFile(color filtered, " << path << L"): " << (text.empty() ? L"(empty)" : text) << endl;
    ExpectHelloWorldDigits(text);
}

TEST_F(OcrFixture, OcrAutoFromFile) {
    const std::wstring path = GetGeneratedOcrBmpPath();
    ASSERT_TRUE(test_support::CreateConsoleLikeBmp(path, L"Hello World! 12345"));

    wstring text;
    op.OcrAutoFromFile(path.c_str(), 0.8, text);
    wcout << L"OcrAutoFromFile(" << path << L"): " << (text.empty() ? L"(empty)" : text) << endl;
    ExpectHelloWorldDigits(text);
}

TEST_F(OcrFixture, FetchWord) {
    wstring word_data;
    op.FetchWord(10, 10, 50, 50, L"ffffff-000000", L"A", word_data);
    wcout << L"FetchWord(10,10,50,50,A): " << (word_data.empty() ? L"(empty)" : word_data.substr(0, 30)) << L"..."
          << endl;
}

TEST_F(OcrFixture, GetWordsNoDict) {
    wstring words_result;
    op.GetWordsNoDict(0, 0, 100, 100, L"ffffff-000000", words_result);
    wcout << L"GetWordsNoDict(0,0,100,100): " << (words_result.empty() ? L"(empty)" : words_result.substr(0, 30))
          << L"..." << endl;
}

TEST_F(OcrFixture, FindStr) {
    long find_x = 0;
    long find_y = 0;
    op.FindStr(0, 0, 200, 200, L"test", L"ffffff-000000", 0.8, &find_x, &find_y, &ret);
    cout << "FindStr(test): ret=" << ret << " at (" << find_x << "," << find_y << ")" << endl;
}

TEST_F(OcrFixture, FindStrEx) {
    wstring find_results;
    op.FindStrEx(0, 0, 200, 200, L"test|demo", L"ffffff-000000", 0.8, find_results);
    wcout << L"FindStrEx(test|demo): " << (find_results.empty() ? L"(empty)" : find_results.substr(0, 30))
          << L"..." << endl;
}

TEST_F(OcrFixture, WordResultParsing) {
    wstring parse_test_result;
    op.GetWordsNoDict(0, 0, 50, 50, L"ffffff-000000", parse_test_result);

    long word_count = 0;
    op.GetWordResultCount(parse_test_result.c_str(), &word_count);
    cout << "GetWordResultCount: " << word_count << endl;

    if (word_count > 0) {
        long word_x = 0;
        long word_y = 0;
        op.GetWordResultPos(parse_test_result.c_str(), 0, &word_x, &word_y, &ret);
        cout << "GetWordResultPos(0): ret=" << ret << " pos=(" << word_x << "," << word_y << ")" << endl;

        wstring word_str;
        op.GetWordResultStr(parse_test_result.c_str(), 0, word_str);
        wcout << L"GetWordResultStr(0): " << (word_str.empty() ? L"(empty)" : word_str) << endl;
    }
}

TEST(OcrParsing, WordResultParsingHandlesBadInput) {
    op::Op op;
    const wchar_t *result = L"12,34-first/56,78-second/";

    long word_count = -1;
    op.GetWordResultCount(nullptr, &word_count);
    EXPECT_EQ(word_count, 0);
    op.GetWordResultCount(result, &word_count);
    EXPECT_EQ(word_count, 2);
    op.GetWordResultCount(result, nullptr);

    long word_x = -1;
    long word_y = -1;
    long parse_ret = -1;
    op.GetWordResultPos(result, 1, &word_x, &word_y, &parse_ret);
    EXPECT_EQ(parse_ret, 1);
    EXPECT_EQ(word_x, 56);
    EXPECT_EQ(word_y, 78);

    word_x = word_y = parse_ret = -1;
    op.GetWordResultPos(nullptr, 0, &word_x, &word_y, &parse_ret);
    EXPECT_EQ(parse_ret, 0);
    EXPECT_EQ(word_x, 0);
    EXPECT_EQ(word_y, 0);

    word_x = word_y = parse_ret = -1;
    op.GetWordResultPos(result, -1, &word_x, &word_y, &parse_ret);
    EXPECT_EQ(parse_ret, 0);
    EXPECT_EQ(word_x, 0);
    EXPECT_EQ(word_y, 0);

    word_x = word_y = parse_ret = -1;
    op.GetWordResultPos(L"12,34broken/", 0, &word_x, &word_y, &parse_ret);
    EXPECT_EQ(parse_ret, 0);
    EXPECT_EQ(word_x, 0);
    EXPECT_EQ(word_y, 0);

    // 缺少 '-' 的坏格式不能越界读取，只返回空字符串。
    wstring word_str;
    op.GetWordResultStr(result, 1, word_str);
    EXPECT_EQ(word_str, L"second");
    op.GetWordResultStr(nullptr, 0, word_str);
    EXPECT_TRUE(word_str.empty());
    op.GetWordResultStr(result, -1, word_str);
    EXPECT_TRUE(word_str.empty());
    op.GetWordResultStr(L"12,34broken/", 0, word_str);
    EXPECT_TRUE(word_str.empty());

    word_x = word_y = parse_ret = 0;
    op.GetWordResultPos(L"-12,-34-negative/", 0, &word_x, &word_y, &parse_ret);
    EXPECT_EQ(parse_ret, 1);
    EXPECT_EQ(word_x, -12);
    EXPECT_EQ(word_y, -34);
    op.GetWordResultStr(L"-12,-34-negative/", 0, word_str);
    EXPECT_EQ(word_str, L"negative");
}

TEST_F(OcrFixture, SetOcrEngine) {
    const std::wstring endpoint = test_support::GetConfiguredOcrEndpoint();
    const long engine_ret = op.SetOcrEngine(endpoint.c_str(), L"", L"--timeout=5000");
    cout << "SetOcrEngine: " << engine_ret << endl;
    EXPECT_EQ(engine_ret, 1);
}

TEST_F(OcrFixture, SetOcrEngineBaseUrl) {
    const long engine_ret = op.SetOcrEngine(L"http://127.0.0.1:8080", L"", L"");
    cout << "SetOcrEngine(base url): " << engine_ret << endl;
    EXPECT_EQ(engine_ret, 1);
}

TEST_F(OcrFixture, SetOcrEngineInvalidUrl) {
    const long engine_ret = op.SetOcrEngine(L"http://", L"", L"--timeout=5000");
    cout << "SetOcrEngine(invalid): " << engine_ret << endl;
    EXPECT_EQ(engine_ret, 0);
}

TEST_F(OcrFixture, SetOcrEngineViaDllNameArg) {
    const std::wstring endpoint = test_support::GetConfiguredOcrEndpoint();
    const long engine_ret = op.SetOcrEngine(L"", endpoint.c_str(), L"--timeout=2000");
    cout << "SetOcrEngine(via dll_name): " << engine_ret << endl;
    EXPECT_EQ(engine_ret, 1);
}

TEST_F(OcrFixture, SetOcrEngineTesseractAlias) {
    const long engine_ret = op.SetOcrEngine(L"tesseract", L"", L"--timeout=2000");
    cout << "SetOcrEngine(tesseract): " << engine_ret << endl;
    EXPECT_EQ(engine_ret, 1);
}

TEST_F(OcrFixture, SetOcrEnginePaddleAlias) {
    const long engine_ret = op.SetOcrEngine(L"paddle", L"", L"--timeout=2000");
    cout << "SetOcrEngine(paddle): " << engine_ret << endl;
    EXPECT_EQ(engine_ret, 1);
}

TEST_F(OcrFixture, OcrAutoFromGeneratedConsoleLikeBmpContainsExpectedText) {
    const std::wstring path = GetGeneratedOcrBmpPath();
    ASSERT_TRUE(test_support::CreateConsoleLikeBmp(path, L"Hello World! 12345"));

    std::wstring text;
    op.OcrAutoFromFile(path.c_str(), 0.8, text);
    wcout << L"Generated OCR result: " << (text.empty() ? L"(empty)" : text) << endl;

    ASSERT_FALSE(text.empty()) << "Expected OCR text from generated BMP";
    ExpectHelloWorldDigits(text);

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(OcrDiagnostics, DISABLED_ProgramManagerDmSoftOcrDiagnostics) {
    op::Op op;
    long ret = 0;

    LONG_PTR hwnd = 0;
    op.FindWindow(L"", L"Program Manager", &hwnd);
    wcout << L"FindWindow(Program Manager): " << hwnd << endl;
    ASSERT_NE(hwnd, 0);

    op.BindWindow(hwnd, L"normal.wgc", L"normal", L"normal", 0, &ret);
    cout << "BindWindow: " << ret << endl;
    ASSERT_EQ(ret, 1);

    const wchar_t *target_entry =
        L"1FE000007FF08010420840FFE08020080206000001FF24448891122FFE48891122244488FF001000003FFC488"
        L"91FFC0013F2844928E33C2CC4089F900$此电脑$1.0.202$13";
    op.AddDict(0, target_entry, &ret);
    cout << "AddDict(dm 此电脑): " << ret << endl;
    ASSERT_EQ(ret, 1);

    long dict_count = 0;
    op.GetDictCount(0, &dict_count);
    cout << "DictCount: " << dict_count << endl;
    ASSERT_GT(dict_count, 0);

    wstring dict_info;
    op.GetDict(0, 0, dict_info);
    cout << "Dictionary[0]: " << (dict_info.empty() ? "(empty)" : WideToUtf8(dict_info.substr(0, 120))) << endl;

    wstring ocr;
    op.Ocr(0, 0, 800, 600, L"ffffff-000000", 0.8, ocr);
    cout << "Ocr: " << (ocr.empty() ? "(empty)" : WideToUtf8(ocr)) << endl;

    wstring ocr_ex;
    op.OcrEx(0, 0, 800, 600, L"ffffff-000000", 0.8, ocr_ex);
    cout << "OcrEx: " << (ocr_ex.empty() ? "(empty)" : WideToUtf8(ocr_ex)) << endl;

    long find_x = -1;
    long find_y = -1;
    op.FindStr(0, 0, 800, 600, L"此电脑", L"ffffff-000000", 0.8, &find_x, &find_y, &ret);
    cout << "FindStr(full dict, 此电脑): ret=" << ret << " x=" << find_x << " y=" << find_y << endl;

    wstring find_ex;
    op.FindStrEx(0, 0, 800, 600, L"此电脑", L"ffffff-000000", 0.8, find_ex);
    cout << "FindStrEx(full dict, 此电脑): " << (find_ex.empty() ? "(empty)" : WideToUtf8(find_ex)) << endl;

    op.FindStr(0, 0, 800, 600, L"此电脑", L"ffffff-000000", 0.8, &find_x, &find_y, &ret);
    cout << "FindStr(single entry, 此电脑): ret=" << ret << " x=" << find_x << " y=" << find_y << endl;

    find_ex.clear();
    op.FindStrEx(0, 0, 800, 600, L"此电脑", L"ffffff-000000", 0.8, find_ex);
    cout << "FindStrEx(single entry, 此电脑): " << (find_ex.empty() ? "(empty)" : WideToUtf8(find_ex)) << endl;

    long unbind_ret = 0;
    op.UnBindWindow(&unbind_ret);
    cout << "UnBindWindow: " << unbind_ret << endl;
}

TEST(OcrDiagnostics, DISABLED_ProgramManagerOpDictOcrDiagnostics) {
    op::Op op;
    long ret = 0;

    LONG_PTR hwnd = 0;
    op.FindWindow(L"", L"Program Manager", &hwnd);
    wcout << L"FindWindow(Program Manager): " << hwnd << endl;
    ASSERT_NE(hwnd, 0);

    op.BindWindow(hwnd, L"normal.wgc", L"normal", L"normal", 0, &ret);
    cout << "BindWindow: " << ret << endl;
    ASSERT_EQ(ret, 1);

    const wchar_t *dict_path = L"E:\\github-qiannian\\op\\examples\\op.dict";
    op.SetDict(0, dict_path, &ret);
    cout << "SetDict(op.dict): " << ret << endl;
    ASSERT_EQ(ret, 1);

    long dict_count = 0;
    op.GetDictCount(0, &dict_count);
    cout << "DictCount: " << dict_count << endl;
    ASSERT_GT(dict_count, 0);

    for (long i = 0; i < dict_count; ++i) {
        wstring dict_info;
        op.GetDict(0, i, dict_info);
        cout << "Dictionary[" << i << "]: " << (dict_info.empty() ? "(empty)" : WideToUtf8(dict_info.substr(0, 120))) << endl;
    }

    wstring ocr;
    op.Ocr(0, 0, 800, 600, L"ffffff-000000", 0.8, ocr);
    cout << "Ocr(op.dict): " << (ocr.empty() ? "(empty)" : WideToUtf8(ocr)) << endl;

    wstring ocr_ex;
    op.OcrEx(0, 0, 800, 600, L"ffffff-000000", 0.8, ocr_ex);
    cout << "OcrEx(op.dict): " << (ocr_ex.empty() ? "(empty)" : WideToUtf8(ocr_ex)) << endl;

    long find_x = -1;
    long find_y = -1;
    op.FindStr(0, 0, 800, 600, L"此电脑", L"ffffff-000000", 0.8, &find_x, &find_y, &ret);
    cout << "FindStr(op.dict, 此电脑): ret=" << ret << " x=" << find_x << " y=" << find_y << endl;

    wstring find_ex;
    op.FindStrEx(0, 0, 800, 600, L"此电脑", L"ffffff-000000", 0.8, find_ex);
    cout << "FindStrEx(op.dict, 此电脑): " << (find_ex.empty() ? "(empty)" : WideToUtf8(find_ex)) << endl;

    const struct {
        const wchar_t *color;
        double sim;
    } color_cases[] = {
        {L"ffffff-000000", 0.8},
        {L"ffffff", 0.8},
        {L"ffffff-303030", 0.8},
        {L"ffffff-606060", 0.8},
        {L"ffffff-808080", 0.8},
        {L"f0f0f0-404040", 0.8},
        {L"e0e0e0-505050", 0.8},
    };

    const struct {
        long x1, y1, x2, y2;
        const char *name;
    } regions[] = {
        {0, 0, 120, 130, "top-left icon"},
        {0, 55, 120, 130, "top-left text"},
        {0, 0, 800, 600, "wide"},
    };

    for (const auto &region : regions) {
        cout << "Region: " << region.name << " (" << region.x1 << "," << region.y1 << "," << region.x2 << ","
             << region.y2 << ")" << endl;
        for (const auto &color_case : color_cases) {
            find_x = -1;
            find_y = -1;
            ret = -999;
            op.FindStr(region.x1, region.y1, region.x2, region.y2, L"此电脑", color_case.color, color_case.sim,
                       &find_x, &find_y, &ret);
            wstring local_ocr_ex;
            op.OcrEx(region.x1, region.y1, region.x2, region.y2, color_case.color, color_case.sim, local_ocr_ex);
            cout << "  color=" << WideToUtf8(color_case.color) << " sim=" << color_case.sim << " FindStr ret=" << ret
                 << " x=" << find_x << " y=" << find_y << " OcrEx="
                 << (local_ocr_ex.empty() ? "(empty)" : WideToUtf8(local_ocr_ex)) << endl;
        }
    }

    long unbind_ret = 0;
    op.UnBindWindow(&unbind_ret);
    cout << "UnBindWindow: " << unbind_ret << endl;
}
