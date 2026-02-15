#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>

#include <gtest/gtest.h>

#include "../libop/core/optype.h"
#include "../libop/imageProc/compute/ThreadPool.h"
#include "../libop/libop.h"

using namespace std;

// ============================================================
// Global Environment: runs SetShowErrorMsg once before all tests
// ============================================================
class OpEnvironment : public ::testing::Environment {
  public:
    void SetUp() override {
        libop op;
        long ret = 0;
        op.SetShowErrorMsg(3, &ret);
    }
};

// ============================================================
// 1. Core Module
// ============================================================
TEST(CoreTest, VersionIsNotEmpty) {
    libop op;
    wstring ver = op.Ver();
    EXPECT_FALSE(ver.empty()) << "Version string should not be empty";
    wcout << L"Version: " << ver << endl;
}

TEST(CoreTest, GetBasePath) {
    libop op;
    wstring path;
    op.GetBasePath(path);
    wcout << L"Base Path: " << path << endl;
    // Just verify it doesn't crash; path may be empty in test env
}

TEST(CoreTest, GetID) {
    libop op;
    long id = 0;
    op.GetID(&id);
    cout << "Object ID: " << id << endl;
}

// ============================================================
// 2. Algorithm Module
// ============================================================
TEST(AlgorithmTest, AStarFindPath) {
    libop op;
    wstring path;
    op.AStarFindPath(100, 100, L"50,50", 0, 0, 99, 99, path);
    wcout << L"AStar Path (first 20 chars): " << path.substr(0, 20) << L"..." << endl;
}

TEST(AlgorithmTest, FindNearestPos) {
    libop op;
    wstring pos;
    op.FindNearestPos(L"10,10|20,20|30,30", 1, 15, 15, pos);
    wcout << L"Nearest Pos to (15,15): " << pos << endl;
    EXPECT_TRUE(pos == L"10,10" || pos == L"20,20")
        << "Expected nearest pos to be 10,10 or 20,20, got: " << std::string(pos.begin(), pos.end());
}

// ============================================================
// 3. WinApi Module
// ============================================================
TEST(WinApiTest, GetForegroundWindow) {
    libop op;
    long hwnd = 0;
    op.GetForegroundWindow(&hwnd);
    cout << "Foreground Window: " << hex << hwnd << dec << endl;
}

TEST(WinApiTest, GetWindowTitleAndRect) {
    libop op;
    long hwnd = 0;
    op.GetForegroundWindow(&hwnd);
    if (hwnd) {
        wstring title;
        op.GetWindowTitle(hwnd, title);
        wcout << L"Title: " << title << endl;

        long x1, y1, x2, y2, ret;
        op.GetWindowRect(hwnd, &x1, &y1, &x2, &y2, &ret);
        cout << "Rect: (" << x1 << "," << y1 << ") - (" << x2 << "," << y2 << ")" << endl;
    }
}

TEST(WinApiTest, EnumWindow) {
    libop op;
    wstring list;
    op.EnumWindow(0, L"", L"", 1 + 2, list);
    wcout << L"EnumWindow (top 50 chars): " << list.substr(0, 50) << L"..." << endl;
}

// ============================================================
// 4. Mouse & Keyboard Module
// ============================================================
TEST(MouseKeyTest, MoveToAndGetCursorPos) {
    libop op;
    long ret;
    op.MoveTo(100, 100, &ret);
    EXPECT_TRUE(ret) << "MoveTo should succeed";

    long x, y;
    op.GetCursorPos(&x, &y, &ret);
    cout << "Cursor Pos: " << x << "," << y << endl;
}

TEST(MouseKeyTest, ClickAndKeyPress) {
    libop op;
    long ret;
    op.SetMouseDelay(L"click", 10, &ret);
    op.LeftClick(&ret);

    op.SetKeypadDelay(L"press", 10, &ret);
    op.KeyPress(VK_ESCAPE, &ret);
}

// ============================================================
// 5. Image & Color Module
// ============================================================
TEST(ImageColorTest, GetColor) {
    libop op;
    wstring color;
    op.GetColor(10, 10, color);
    wcout << L"Color at (10,10): " << color << endl;
    EXPECT_EQ(color.length(), 6u) << "Color string should be 6 hex characters";
}

TEST(ImageColorTest, CmpColor) {
    libop op;
    wstring color;
    op.GetColor(10, 10, color);
    ASSERT_EQ(color.length(), 6u);

    long ret;
    op.CmpColor(10, 10, color.c_str(), 0.9, &ret);
    EXPECT_TRUE(ret) << "CmpColor should match the same color just read";
}

TEST(ImageColorTest, FindColor) {
    libop op;
    wstring color;
    op.GetColor(10, 10, color);
    ASSERT_EQ(color.length(), 6u);

    long x, y, ret;
    op.FindColor(0, 0, 100, 100, color.c_str(), 0.9, 0, &x, &y, &ret);
    cout << "FindColor ret: " << ret << " at " << x << "," << y << endl;
}

// ============================================================
// 6. OCR Module
// ============================================================
class OcrTest : public ::testing::Test {
  protected:
    libop op;
    long ret = 0;
};

// --- Dictionary Management ---
TEST_F(OcrTest, SetDictAndUseDict) {
    op.SetDict(0, L"../bin/miner/miner_num.dict", &ret);
    cout << "SetDict(0, miner_num.dict): " << ret << endl;

    op.UseDict(0, &ret);
    cout << "UseDict(0): " << ret << endl;
}

TEST_F(OcrTest, GetDictCount) {
    op.SetDict(0, L"../bin/miner/miner_num.dict", &ret);
    op.UseDict(0, &ret);

    long dict_count = 0;
    op.GetDictCount(0, &dict_count);
    cout << "GetDictCount(0): " << dict_count << endl;
}

TEST_F(OcrTest, GetNowDict) {
    long now_dict_idx = -1;
    op.GetNowDict(&now_dict_idx);
    cout << "GetNowDict(): " << now_dict_idx << endl;
}

TEST_F(OcrTest, GetDictEntry) {
    op.SetDict(0, L"../bin/miner/miner_num.dict", &ret);
    op.UseDict(0, &ret);

    wstring dict_info;
    op.GetDict(0, 0, dict_info);
    wcout << L"GetDict(0, 0): " << (dict_info.empty() ? L"empty" : dict_info.substr(0, 30)) << L"..." << endl;
}

TEST_F(OcrTest, AddAndClearDict) {
    op.AddDict(1, L"1$1$1$1$1$1$1$1$1", &ret);
    cout << "AddDict(1, test_entry): " << ret << endl;

    op.ClearDict(1, &ret);
    cout << "ClearDict(1): " << ret << endl;
}

TEST_F(OcrTest, SaveDict) {
    op.AddDict(1, L"1$1$1$1$1$1$1$1$1", &ret);
    op.SaveDict(1, L"test_dict_output.txt", &ret);
    cout << "SaveDict(1, test_dict_output.txt): " << ret << endl;
    op.ClearDict(1, &ret);
}

TEST_F(OcrTest, SetMemDict) {
    const wchar_t *mem_dict_data = L"2$2$2$2$2$2$2$2$2";
    op.SetMemDict(2, mem_dict_data, (long)(wcslen(mem_dict_data) * sizeof(wchar_t)), &ret);
    cout << "SetMemDict(2): " << ret << endl;
}

// --- OCR Operations ---
TEST_F(OcrTest, Ocr) {
    wstring text;
    op.Ocr(0, 0, 100, 100, L"ffffff-000000", 0.8, text);
    wcout << L"Ocr(0,0,100,100): " << (text.empty() ? L"(empty)" : text) << endl;
}

TEST_F(OcrTest, OcrEx) {
    wstring text;
    op.OcrEx(0, 0, 100, 100, L"ffffff-000000", 0.8, text);
    wcout << L"OcrEx(0,0,100,100): " << (text.empty() ? L"(empty)" : text.substr(0, 50)) << endl;
}

TEST_F(OcrTest, OcrAuto) {
    wstring text;
    op.OcrAuto(0, 0, 100, 100, 0.8, text);
    wcout << L"OcrAuto(0,0,100,100): " << (text.empty() ? L"(empty)" : text) << endl;
}

TEST_F(OcrTest, OcrFromFile) {
    wstring text;
    op.OcrFromFile(L"test_image.bmp", L"ffffff-000000", 0.8, text);
    wcout << L"OcrFromFile(test_image.bmp): " << (text.empty() ? L"(empty)" : text) << endl;
}

TEST_F(OcrTest, OcrAutoFromFile) {
    wstring text;
    op.OcrAutoFromFile(L"test_image.bmp", 0.8, text);
    wcout << L"OcrAutoFromFile(test_image.bmp): " << (text.empty() ? L"(empty)" : text) << endl;
}

// --- Word Extraction ---
TEST_F(OcrTest, FetchWord) {
    wstring word_data;
    op.FetchWord(10, 10, 50, 50, L"ffffff-000000", L"A", word_data);
    wcout << L"FetchWord(10,10,50,50, 'A'): " << (word_data.empty() ? L"(empty)" : word_data.substr(0, 30)) << L"..."
          << endl;
}

TEST_F(OcrTest, GetWordsNoDict) {
    wstring words_result;
    op.GetWordsNoDict(0, 0, 100, 100, L"ffffff-000000", words_result);
    wcout << L"GetWordsNoDict(0,0,100,100): " << (words_result.empty() ? L"(empty)" : words_result.substr(0, 30))
          << L"..." << endl;
}

TEST_F(OcrTest, FindStr) {
    long find_x = 0, find_y = 0;
    op.FindStr(0, 0, 200, 200, L"test", L"ffffff-000000", 0.8, &find_x, &find_y, &ret);
    cout << "FindStr('test'): ret=" << ret << " at (" << find_x << "," << find_y << ")" << endl;
}

TEST_F(OcrTest, FindStrEx) {
    wstring find_results;
    op.FindStrEx(0, 0, 200, 200, L"test|demo", L"ffffff-000000", 0.8, find_results);
    wcout << L"FindStrEx('test|demo'): " << (find_results.empty() ? L"(empty)" : find_results.substr(0, 30)) << L"..."
          << endl;
}

// --- Result Parsing ---
TEST_F(OcrTest, WordResultParsing) {
    wstring parse_test_result;
    op.GetWordsNoDict(0, 0, 50, 50, L"ffffff-000000", parse_test_result);

    long word_count = 0;
    op.GetWordResultCount(parse_test_result.c_str(), &word_count);
    cout << "GetWordResultCount: " << word_count << endl;

    if (word_count > 0) {
        long word_x = 0, word_y = 0;
        op.GetWordResultPos(parse_test_result.c_str(), 0, &word_x, &word_y, &ret);
        cout << "GetWordResultPos(0): ret=" << ret << " pos=(" << word_x << "," << word_y << ")" << endl;

        wstring word_str;
        op.GetWordResultStr(parse_test_result.c_str(), 0, word_str);
        wcout << L"GetWordResultStr(0): " << (word_str.empty() ? L"(empty)" : word_str) << endl;
    }
}

// --- OCR Engine ---
TEST_F(OcrTest, SetOcrEngine) {
    long engine_ret = op.SetOcrEngine(L".", L"test_engine.dll", L"");
    cout << "SetOcrEngine: " << engine_ret << endl;
}

// ============================================================
// 7. Utils / Shared
// ============================================================
TEST(UtilsTest, ThreadPool) {
    ThreadPool pool(4);
    auto fut = pool.enqueue([] { return 42; });
    EXPECT_EQ(fut.get(), 42);
}

TEST(UtilsTest, RectDivideBlock) {
    rect_t rc(0, 0, 100, 100);
    vector<rect_t> blocks;
    rc.divideBlock(2, false, blocks);
    EXPECT_EQ(blocks.size(), 2u);
}

// ============================================================
// main: register environment & run all tests
// ============================================================
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new OpEnvironment);
    return RUN_ALL_TESTS();
}
