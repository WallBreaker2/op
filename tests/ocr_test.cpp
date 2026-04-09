#include "test_support.h"

#include <filesystem>
#include <iostream>
#include <string>

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
    op.SaveDict(1, L"test_dict_output.txt", &ret);
    cout << "SaveDict(1, test_dict_output.txt): " << ret << endl;
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
