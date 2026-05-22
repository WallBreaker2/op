#pragma once

#include <gtest/gtest.h>

#include <string>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <winhttp.h>

#include "../libop/background/Hook/opMessage.h"
#include "../libop/core/optype.h"
#include "../libop/libop.h"

namespace test_support {

class OpEnvironment : public ::testing::Environment {
  public:
    void SetUp() override;
};

std::wstring TrimCopy(const std::wstring &value);
std::wstring GetEnvString(const wchar_t *name);
std::wstring GetConfiguredOcrEndpoint();
bool IsOcrServerHealthy();
std::wstring PtrToWString(const void *ptr, bool hex = false);
std::vector<uchar> BuildBmp32TopDown(int width, int height, const std::vector<uchar> &bgra_pixels);
std::wstring ToLowerCopy(std::wstring value);
bool ContainsInsensitive(const std::wstring &haystack, const std::wstring &needle);
std::wstring NormalizeOcrLetters(const std::wstring &text);
std::wstring ExtractDigits(const std::wstring &text);
size_t EditDistance(const std::wstring &lhs, const std::wstring &rhs);
std::wstring GetTempBmpPath(const wchar_t *file_name);
bool CreateConsoleLikeBmp(const std::wstring &path, const std::wstring &text);

struct SendStringWindow {
    HWND parent = nullptr;
    HWND edit = nullptr;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    bool Create();
    std::wstring GetEditText() const;
    ~SendStringWindow();
};

struct MouseEventWindow {
    HWND hwnd = nullptr;
    int left_down = 0;
    int left_up = 0;
    int right_down = 0;
    int right_up = 0;
    int wheel_count = 0;
    int wheel_delta_sum = 0;

    int op_left_down = 0;
    int op_left_up = 0;
    int op_right_down = 0;
    int op_right_up = 0;
    int op_wheel_count = 0;
    int op_wheel_delta_sum = 0;

    long last_x = 0;
    long last_y = 0;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    bool Create();
    ~MouseEventWindow();
};

class OcrTest : public ::testing::Test {
  protected:
    libop op;
    long ret = 0;

    void SetUp() override;
    void TearDown() override;
};

} // namespace test_support
