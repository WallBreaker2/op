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

#include "../libop/core/optype.h"
#include "../libop/imageProc/compute/ThreadPool.h"
#include "../libop/libop.h"

using namespace std;

// --- Test State ---
bool gRun = false;
mutex gMutex;

// --- Helper Functions ---
void proc_shared(libop *sharedOp) {
    void *data = 0;
    long size, ret;
    while (gRun) {
        this_thread::sleep_for(chrono::microseconds(100));
        lock_guard<std::mutex> lock(gMutex);
        sharedOp->GetScreenDataBmp(0, 0, 50, 50, (size_t *)&data, &size, &ret);
    }
}

// --- Test Modules ---

// 1. Core Module
int test_core() {
    libop op;
    wstring ver = op.Ver();
    wcout << L"Version: " << ver << endl;
    if (ver.empty())
        return 1;

    wstring path;
    op.GetBasePath(path);
    wcout << L"Base Path: " << path << endl;

    long id;
    op.GetID(&id);
    cout << "Object ID: " << id << endl;

    return 0;
}

// 2. Algorithm Module
int test_algorithm() {
    libop op;
    wstring path;
    op.AStarFindPath(100, 100, L"50,50", 0, 0, 99, 99, path);
    wcout << L"AStar Path (first 20 chars): " << path.substr(0, 20) << L"..." << endl;

    wstring pos;
    op.FindNearestPos(L"10,10|20,20|30,30", 1, 15, 15, pos);
    wcout << L"Nearest Pos to (15,15): " << pos << endl;
    if (pos != L"10,10" && pos != L"20,20")
        return 2;

    return 0;
}

// 3. WinApi Module
int test_winapi() {
    libop op;
    long hwnd = 0;
    op.GetForegroundWindow(&hwnd);
    cout << "Foreground Window: " << hex << hwnd << dec << endl;

    if (hwnd) {
        wstring title;
        op.GetWindowTitle(hwnd, title);
        wcout << L"Title: " << title << endl;

        long x1, y1, x2, y2, ret;
        op.GetWindowRect(hwnd, &x1, &y1, &x2, &y2, &ret);
        cout << "Rect: (" << x1 << "," << y1 << ") - (" << x2 << "," << y2 << ")" << endl;
    }

    wstring list;
    op.EnumWindow(0, L"", L"", 1 + 2, list);
    wcout << L"EnumWindow (top 50 chars): " << list.substr(0, 50) << L"..." << endl;

    return 0;
}

// 4. Mouse & Keyboard Module
int test_mouse_key() {
    libop op;
    long ret;
    op.MoveTo(100, 100, &ret);
    if (!ret)
        return 1;

    long x, y;
    op.GetCursorPos(&x, &y, &ret);
    cout << "Cursor Pos: " << x << "," << y << endl;

    op.SetMouseDelay(L"click", 10, &ret);
    op.LeftClick(&ret);

    op.SetKeypadDelay(L"press", 10, &ret);
    op.KeyPress(VK_ESCAPE, &ret);

    return 0;
}

// 5. Image & Color Module
int test_image_color() {
    libop op;
    long ret;

    // We can't easily test capture without a valid window bond in some modes,
    // but we can test basic color functions on desktop if allowed
    wstring color;
    op.GetColor(10, 10, color);
    wcout << L"Color at (10,10): " << color << endl;
    if (color.length() != 6)
        return 1;

    op.CmpColor(10, 10, color.c_str(), 0.9, &ret);
    if (!ret)
        return 2;

    long x, y;
    op.FindColor(0, 0, 100, 100, color.c_str(), 0.9, 0, &x, &y, &ret);
    cout << "FindColor ret: " << ret << " at " << x << "," << y << endl;

    return 0;
}

// 6. OCR Module
int test_ocr() {
    libop op;
    long ret;
    wstring text;

    cout << "=== OCR Module Tests ===" << endl;

    // --- 1. Dictionary Management Tests ---
    cout << "\n[Dictionary Management]" << endl;

    // Test SetDict with actual dictionary file
    op.SetDict(0, L"../bin/miner/miner_num.dict", &ret);
    cout << "SetDict(0, miner_num.dict): " << ret << endl;

    // Test UseDict to activate dictionary
    op.UseDict(0, &ret);
    cout << "UseDict(0): " << ret << endl;

    // Test GetDictCount
    long dict_count = 0;
    op.GetDictCount(0, &dict_count);
    cout << "GetDictCount(0): " << dict_count << endl;

    // Test GetNowDict
    long now_dict_idx = -1;
    op.GetNowDict(&now_dict_idx);
    cout << "GetNowDict(): " << now_dict_idx << endl;

    // Test GetDict to retrieve specific entry
    wstring dict_info;
    op.GetDict(0, 0, dict_info);
    wcout << L"GetDict(0, 0): " << (dict_info.empty() ? L"empty" : dict_info.substr(0, 30)) << L"..." << endl;

    // Test AddDict to add custom entry
    op.AddDict(1, L"1$1$1$1$1$1$1$1$1", &ret);
    cout << "AddDict(1, test_entry): " << ret << endl;

    // Test SaveDict
    op.SaveDict(1, L"test_dict_output.txt", &ret);
    cout << "SaveDict(1, test_dict_output.txt): " << ret << endl;

    // Test ClearDict
    op.ClearDict(1, &ret);
    cout << "ClearDict(1): " << ret << endl;

    // Test SetMemDict (with minimal valid data)
    const wchar_t *mem_dict_data = L"2$2$2$2$2$2$2$2$2";
    op.SetMemDict(2, mem_dict_data, (long)(wcslen(mem_dict_data) * sizeof(wchar_t)), &ret);
    cout << "SetMemDict(2): " << ret << endl;

    // --- 2. OCR Operations Tests ---
    cout << "\n[OCR Operations]" << endl;

    // Test Ocr - basic OCR with color
    text.clear();
    op.Ocr(0, 0, 100, 100, L"ffffff-000000", 0.8, text);
    wcout << L"Ocr(0,0,100,100): " << (text.empty() ? L"(empty)" : text) << endl;

    // Test OcrEx - OCR with coordinates
    text.clear();
    op.OcrEx(0, 0, 100, 100, L"ffffff-000000", 0.8, text);
    wcout << L"OcrEx(0,0,100,100): " << (text.empty() ? L"(empty)" : text.substr(0, 50)) << endl;

    // Test OcrAuto - auto color detection
    text.clear();
    op.OcrAuto(0, 0, 100, 100, 0.8, text);
    wcout << L"OcrAuto(0,0,100,100): " << (text.empty() ? L"(empty)" : text) << endl;

    // Test OcrFromFile (may fail if file doesn't exist, that's OK)
    text.clear();
    op.OcrFromFile(L"test_image.bmp", L"ffffff-000000", 0.8, text);
    wcout << L"OcrFromFile(test_image.bmp): " << (text.empty() ? L"(empty)" : text) << endl;

    // Test OcrAutoFromFile
    text.clear();
    op.OcrAutoFromFile(L"test_image.bmp", 0.8, text);
    wcout << L"OcrAutoFromFile(test_image.bmp): " << (text.empty() ? L"(empty)" : text) << endl;

    // --- 3. Word Extraction Tests ---
    cout << "\n[Word Extraction]" << endl;

    // Test FetchWord - extract character pixel data
    wstring word_data;
    op.FetchWord(10, 10, 50, 50, L"ffffff-000000", L"A", word_data);
    wcout << L"FetchWord(10,10,50,50, 'A'): " << (word_data.empty() ? L"(empty)" : word_data.substr(0, 30)) << L"..."
          << endl;

    // Test GetWordsNoDict - identify word shapes without dictionary
    wstring words_result;
    op.GetWordsNoDict(0, 0, 100, 100, L"ffffff-000000", words_result);
    wcout << L"GetWordsNoDict(0,0,100,100): " << (words_result.empty() ? L"(empty)" : words_result.substr(0, 30))
          << L"..." << endl;

    // Test FindStr - locate specific string
    long find_x = 0, find_y = 0;
    op.FindStr(0, 0, 200, 200, L"test", L"ffffff-000000", 0.8, &find_x, &find_y, &ret);
    cout << "FindStr('test'): ret=" << ret << " at (" << find_x << "," << find_y << ")" << endl;

    // Test FindStrEx - find all instances
    wstring find_results;
    op.FindStrEx(0, 0, 200, 200, L"test|demo", L"ffffff-000000", 0.8, find_results);
    wcout << L"FindStrEx('test|demo'): " << (find_results.empty() ? L"(empty)" : find_results.substr(0, 30)) << L"..."
          << endl;

    // --- 4. Result Parsing Tests ---
    cout << "\n[Result Parsing]" << endl;

    // Generate some test result data using GetWordsNoDict
    wstring parse_test_result;
    op.GetWordsNoDict(0, 0, 50, 50, L"ffffff-000000", parse_test_result);

    // Test GetWordResultCount
    long word_count = 0;
    op.GetWordResultCount(parse_test_result.c_str(), &word_count);
    cout << "GetWordResultCount: " << word_count << endl;

    if (word_count > 0) {
        // Test GetWordResultPos
        long word_x = 0, word_y = 0;
        op.GetWordResultPos(parse_test_result.c_str(), 0, &word_x, &word_y, &ret);
        cout << "GetWordResultPos(0): ret=" << ret << " pos=(" << word_x << "," << word_y << ")" << endl;

        // Test GetWordResultStr
        wstring word_str;
        op.GetWordResultStr(parse_test_result.c_str(), 0, word_str);
        wcout << L"GetWordResultStr(0): " << (word_str.empty() ? L"(empty)" : word_str) << endl;
    }

    // --- 5. OCR Engine Configuration Test ---
    cout << "\n[OCR Engine]" << endl;

    // Test SetOcrEngine (basic interface stability test)
    long engine_ret = op.SetOcrEngine(L".", L"test_engine.dll", L"");
    cout << "SetOcrEngine: " << engine_ret << endl;

    cout << "\n=== OCR Tests Complete ===" << endl;
    return 0;
}

// 7. Utils / Shared
int test_utils() {
    ThreadPool pool(4);
    auto fut = pool.enqueue([] { return 42; });
    if (fut.get() != 42)
        return 1;

    rect_t rc(0, 0, 100, 100);
    vector<rect_t> blocks;
    rc.divideBlock(2, false, blocks);
    if (blocks.size() != 2)
        return 2;

    return 0;
}

// --- Test Runner ---

void print_usage(const char *prog) {
    cout << "Usage: " << prog << " <test_module>" << endl;
    cout << "Available modules: core, algorithm, winapi, mouse_key, image_color, ocr, utils" << endl;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    libop op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);
    string module_name = argv[1];
    map<string, function<int()>> modules = {
        {"core", test_core},           {"algorithm", test_algorithm},     {"winapi", test_winapi},
        {"mouse_key", test_mouse_key}, {"image_color", test_image_color}, {"ocr", test_ocr},
        {"utils", test_utils}};

    if (modules.find(module_name) == modules.end()) {
        cout << "Unknown module: " << module_name << endl;
        print_usage(argv[0]);
        return 1;
    }

    cout << "--- Running Module: " << module_name << " ---" << endl;
    int result = modules[module_name]();

    if (result == 0) {
        cout << "Module " << module_name << " PASSED" << endl;
    } else {
        cout << "Module " << module_name << " FAILED with code " << result << endl;
    }

    return result;
}
