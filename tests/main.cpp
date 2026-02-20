#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>
#include <windowsx.h>
#include <winhttp.h>

#include <gtest/gtest.h>

#pragma comment(lib, "winhttp.lib")

#include "../libop/background/Hook/opMessage.h"
#include "../libop/core/optype.h"
#include "../libop/imageProc/compute/ThreadPool.h"
#include "../libop/libop.h"

using namespace std;

static bool IsOcrServerHealthy() {
    HINTERNET hSession = WinHttpOpen(L"op-test-health/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
        return false;

    HINTERNET hConnect = WinHttpConnect(hSession, L"127.0.0.1", 8080, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/health", nullptr, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    WinHttpSetTimeouts(hRequest, 1000, 1000, 1000, 1000);

    bool ok = false;
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, nullptr)) {
        DWORD status_code = 0;
        DWORD size = sizeof(status_code);
        if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &size, WINHTTP_NO_HEADER_INDEX)) {
            ok = (status_code == 200);
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return ok;
}

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

static std::wstring PtrToWString(const void *ptr, bool hex = false) {
    std::wstringstream ss;
    if (hex) {
        ss << L"0x" << std::hex << reinterpret_cast<uintptr_t>(ptr);
    } else {
        ss << reinterpret_cast<uintptr_t>(ptr);
    }
    return ss.str();
}

static vector<uchar> BuildBmp32TopDown(int width, int height, const vector<uchar> &bgra_pixels) {
    const size_t pixel_bytes = static_cast<size_t>(width) * height * 4;
    const size_t header_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    vector<uchar> buffer(header_size + pixel_bytes, 0);

    auto *bfh = reinterpret_cast<BITMAPFILEHEADER *>(buffer.data());
    auto *bih = reinterpret_cast<BITMAPINFOHEADER *>(buffer.data() + sizeof(BITMAPFILEHEADER));
    bfh->bfType = static_cast<WORD>(0x4d42);
    bfh->bfOffBits = static_cast<DWORD>(header_size);
    bfh->bfSize = static_cast<DWORD>(buffer.size());

    bih->biSize = sizeof(BITMAPINFOHEADER);
    bih->biWidth = width;
    bih->biHeight = -height;
    bih->biPlanes = 1;
    bih->biBitCount = 32;
    bih->biCompression = BI_RGB;
    bih->biSizeImage = static_cast<DWORD>(pixel_bytes);

    memcpy(buffer.data() + header_size, bgra_pixels.data(), pixel_bytes);
    return buffer;
}

struct SendStringWindow {
    HWND parent = nullptr;
    HWND edit = nullptr;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    bool Create() {
        static const wchar_t *kClassName = L"OpSendStringTestWindow";
        static bool class_registered = false;
        HINSTANCE hinst = GetModuleHandleW(nullptr);

        if (!class_registered) {
            WNDCLASSW wc = {0};
            wc.lpfnWndProc = SendStringWindow::WndProc;
            wc.hInstance = hinst;
            wc.lpszClassName = kClassName;
            if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
                return false;
            class_registered = true;
        }

        parent = CreateWindowExW(0, kClassName, L"op-sendstring-test", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                 CW_USEDEFAULT, 320, 120, nullptr, nullptr, hinst, nullptr);
        if (!parent)
            return false;

        edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_LEFT | WS_TABSTOP, 8, 8, 280,
                               24, parent, nullptr, hinst, nullptr);
        if (!edit) {
            DestroyWindow(parent);
            parent = nullptr;
            return false;
        }

        ShowWindow(parent, SW_SHOW);
        UpdateWindow(parent);
        SetActiveWindow(parent);
        SetFocus(edit);
        return IsWindow(parent) && IsWindow(edit);
    }

    wstring GetEditText() const {
        wchar_t buffer[256] = {0};
        GetWindowTextW(edit, buffer, static_cast<int>(sizeof(buffer) / sizeof(buffer[0])));
        return buffer;
    }

    ~SendStringWindow() {
        if (parent)
            DestroyWindow(parent);
    }
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

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        MouseEventWindow *self = reinterpret_cast<MouseEventWindow *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            auto *cs = reinterpret_cast<CREATESTRUCTW *>(lparam);
            self = reinterpret_cast<MouseEventWindow *>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }

        if (self) {
            switch (msg) {
            case WM_LBUTTONDOWN:
                self->left_down++;
                self->last_x = GET_X_LPARAM(lparam);
                self->last_y = GET_Y_LPARAM(lparam);
                return 0;
            case WM_LBUTTONUP:
                self->left_up++;
                self->last_x = GET_X_LPARAM(lparam);
                self->last_y = GET_Y_LPARAM(lparam);
                return 0;
            case WM_RBUTTONDOWN:
                self->right_down++;
                self->last_x = GET_X_LPARAM(lparam);
                self->last_y = GET_Y_LPARAM(lparam);
                return 0;
            case WM_RBUTTONUP:
                self->right_up++;
                self->last_x = GET_X_LPARAM(lparam);
                self->last_y = GET_Y_LPARAM(lparam);
                return 0;
            case WM_MOUSEWHEEL:
                self->wheel_count++;
                self->wheel_delta_sum += GET_WHEEL_DELTA_WPARAM(wparam);
                return 0;
            case OP_WM_LBUTTONDOWN:
                self->op_left_down++;
                return 0;
            case OP_WM_LBUTTONUP:
                self->op_left_up++;
                return 0;
            case OP_WM_RBUTTONDOWN:
                self->op_right_down++;
                return 0;
            case OP_WM_RBUTTONUP:
                self->op_right_up++;
                return 0;
            case OP_WM_MOUSEWHEEL:
                self->op_wheel_count++;
                self->op_wheel_delta_sum += static_cast<short>(HIWORD(wparam));
                return 0;
            default:
                break;
            }
        }
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    bool Create() {
        static const wchar_t *kClassName = L"OpMouseEventTestWindow";
        static bool class_registered = false;
        HINSTANCE hinst = GetModuleHandleW(nullptr);

        if (!class_registered) {
            WNDCLASSW wc = {0};
            wc.lpfnWndProc = MouseEventWindow::WndProc;
            wc.hInstance = hinst;
            wc.lpszClassName = kClassName;
            if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
                return false;
            class_registered = true;
        }

        hwnd = CreateWindowExW(0, kClassName, L"op-mouseevent-test", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                               400, 260, nullptr, nullptr, hinst, this);
        if (!hwnd)
            return false;

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        return IsWindow(hwnd);
    }

    ~MouseEventWindow() {
        if (hwnd)
            DestroyWindow(hwnd);
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

TEST(WinApiTest, GetCmdStrEcho) {
    libop op;
    wstring out;
    op.GetCmdStr(L"cmd /c echo op_test_ok", 2000, out);
    EXPECT_NE(out.find(L"op_test_ok"), wstring::npos) << "GetCmdStr should return command output";
}

TEST(WinApiTest, GetCmdStrLongCommandLine) {
    libop op;
    wstring payload(400, L'a');
    wstring cmd = L"cmd /c echo " + payload;
    wstring out;
    op.GetCmdStr(cmd.c_str(), 2000, out);
    EXPECT_NE(out.find(payload.substr(0, 64)), wstring::npos) << "Long command line should not truncate or hang";
}

TEST(WinApiTest, SendStringToFocusedChildEdit) {
    libop op;
    SendStringWindow wnd;
    ASSERT_TRUE(wnd.Create()) << "failed to create test windows";

    long ret = 0;
    op.SendString((long)(intptr_t)wnd.parent, L"abc123", &ret);
    EXPECT_EQ(ret, 1);

    EXPECT_EQ(wnd.GetEditText(), L"abc123");
}

TEST(WinApiTest, SendStringImeToFocusedChildEdit) {
    libop op;
    SendStringWindow wnd;
    ASSERT_TRUE(wnd.Create()) << "failed to create test windows";

    long ret = 0;
    op.SendStringIme((long)(intptr_t)wnd.parent, L"中文A", &ret);
    EXPECT_EQ(ret, 1);

    EXPECT_EQ(wnd.GetEditText(), L"中文A");
}

TEST(IntegrationTest, BindUnbindFurMarkIfPresent) {
    libop op;
    const wchar_t *capture_file = L"D:\\code\\op\\build\\furmark_bound_capture.bmp";

    auto parse_hwnds = [&](const wstring &list) -> vector<long> {
        vector<long> hwnds;
        wstringstream ss(list);
        wstring token;
        while (getline(ss, token, L',')) {
            if (token.empty())
                continue;
            try {
                hwnds.push_back(stol(token));
            } catch (...) {
            }
        }
        return hwnds;
    };

    auto pick_best_furmark_hwnd = [&]() -> long {
        wstring list;
        op.EnumWindowByProcess(L"FurMark_GUI.exe", L"", L"", 8 + 16 + 32, list);
        auto hwnds = parse_hwnds(list);
        long best_hwnd = 0;
        long best_area = -1;
        for (long h : hwnds) {
            long w = 0, hgt = 0, ok = 0;
            op.GetClientSize(h, &w, &hgt, &ok);
            if (ok != 1 || w <= 0 || hgt <= 0)
                continue;

            wstring title;
            wstring cls;
            op.GetWindowTitle(h, title);
            op.GetWindowClass(h, cls);

            bool looks_like_launcher = (cls.find(L"WindowsForms10") != wstring::npos);
            if (looks_like_launcher)
                continue;

            long area = w * hgt;
            if (area > best_area) {
                best_area = area;
                best_hwnd = h;
            }
        }
        return best_hwnd;
    };

    long hwnd = pick_best_furmark_hwnd();
    if (hwnd == 0) {
        GTEST_SKIP() << "FurMark render window not found";
    }

    const vector<wstring> displays = {L"opengl", L"normal", L"dx"};
    bool any_bind_ok = false;
    bool captured = false;
    for (const auto &display : displays) {
        for (int i = 0; i < 3; ++i) {
            long ret = 0;
            op.BindWindow(hwnd, display.c_str(), L"windows", L"windows", 0, &ret);
            if (ret == 1) {
                any_bind_ok = true;
                wstring color;
                op.GetColor(10, 10, color);
                EXPECT_EQ(color.length(), 6u) << "GetColor should return 6 hex chars when bound";

                if (!captured) {
                    long width = 0, height = 0, size_ret = 0;
                    op.GetClientSize(hwnd, &width, &height, &size_ret);
                    ASSERT_EQ(size_ret, 1);
                    ASSERT_GT(width, 0);
                    ASSERT_GT(height, 0);
                    long cap_ret = 0;
                    op.Capture(0, 0, width, height, capture_file, &cap_ret);
                    EXPECT_EQ(cap_ret, 1) << "Capture should succeed after FurMark bind";
                    captured = (cap_ret == 1);
                }
            }
            long unbind_ret = 0;
            op.UnBindWindow(&unbind_ret);
            EXPECT_EQ(unbind_ret, 1);
        }
    }

    EXPECT_TRUE(any_bind_ok) << "No bind mode succeeded for the detected FurMark window";
    EXPECT_TRUE(captured) << "No screenshot captured from bound FurMark window";
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

TEST(MouseKeyTest, WindowsModeMouseReturnAndWheel) {
    libop op;
    MouseEventWindow window;
    ASSERT_TRUE(window.Create());

    long ret = 0;
    op.BindWindow(reinterpret_cast<long>(window.hwnd), L"normal", L"windows", L"windows", 0, &ret);
    ASSERT_EQ(ret, 1) << "BindWindow windows mode should succeed";

    op.MoveTo(30, 40, &ret);
    EXPECT_EQ(ret, 1);
    op.LeftClick(&ret);
    EXPECT_EQ(ret, 1);
    op.RightClick(&ret);
    EXPECT_EQ(ret, 1);
    op.WheelDown(&ret);
    EXPECT_EQ(ret, 1);
    op.WheelUp(&ret);
    EXPECT_EQ(ret, 1);

    EXPECT_GE(window.left_down, 1);
    EXPECT_GE(window.left_up, 1);
    EXPECT_GE(window.right_down, 1);
    EXPECT_GE(window.right_up, 1);
    EXPECT_GE(window.wheel_count, 2);
    EXPECT_EQ(window.wheel_delta_sum, 0);

    long unbind_ret = 0;
    op.UnBindWindow(&unbind_ret);
    EXPECT_EQ(unbind_ret, 1);
}

TEST(MouseKeyTest, DxModeWheelAndCustomMessages) {
    libop op;
    MouseEventWindow window;
    ASSERT_TRUE(window.Create());

    long ret = 0;
    op.BindWindow(reinterpret_cast<long>(window.hwnd), L"normal", L"dx", L"windows", 0, &ret);
    if (ret != 1) {
        GTEST_SKIP() << "DX mouse bind unavailable on current environment";
    }

    op.MoveTo(18, 26, &ret);
    EXPECT_EQ(ret, 1);
    op.LeftClick(&ret);
    EXPECT_EQ(ret, 1);
    op.RightClick(&ret);
    EXPECT_EQ(ret, 1);
    op.WheelDown(&ret);
    EXPECT_EQ(ret, 1);
    op.WheelUp(&ret);
    EXPECT_EQ(ret, 1);

    EXPECT_GE(window.op_left_down, 1);
    EXPECT_GE(window.op_left_up, 1);
    EXPECT_GE(window.op_right_down, 1);
    EXPECT_GE(window.op_right_up, 1);
    EXPECT_GE(window.op_wheel_count, 2);
    EXPECT_EQ(window.op_wheel_delta_sum, 0);

    long unbind_ret = 0;
    op.UnBindWindow(&unbind_ret);
    EXPECT_EQ(unbind_ret, 1);
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

TEST(ImageColorTest, SetDisplayInputMemBmpPointer) {
    libop op;
    long ret = 0;
    const int width = 32;
    const int height = 32;
    vector<uchar> pixels(static_cast<size_t>(width) * height * 4, 0xff);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            auto idx = static_cast<size_t>(y * width + x) * 4;
            pixels[idx + 0] = 0x01;
            pixels[idx + 1] = 0x02;
            pixels[idx + 2] = 0x03;
            pixels[idx + 3] = 0xff;
        }
    }
    auto bmp = BuildBmp32TopDown(width, height, pixels);
    wstring mode = L"mem:" + PtrToWString(bmp.data());

    op.SetDisplayInput(mode.c_str(), &ret);
    ASSERT_EQ(ret, 1);

    wstring color;
    op.GetColor(10, 10, color);
    EXPECT_EQ(color, L"030201");
}

TEST(ImageColorTest, SetDisplayInputMemRawBgr) {
    libop op;
    long ret = 0;
    const int width = 32;
    const int height = 32;
    vector<uchar> raw_bgr(static_cast<size_t>(width) * height * 3, 0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            auto idx = static_cast<size_t>(y * width + x) * 3;
            raw_bgr[idx + 0] = 0x28;
            raw_bgr[idx + 1] = 0x32;
            raw_bgr[idx + 2] = 0x3c;
        }
    }
    wstring mode = L"mem:" + PtrToWString(raw_bgr.data()) + L",32,32,bgr";

    op.SetDisplayInput(mode.c_str(), &ret);
    ASSERT_EQ(ret, 1);

    wstring color;
    op.GetColor(10, 10, color);
    EXPECT_EQ(color, L"3C3228");
}

TEST(ImageColorTest, SetDisplayInputMemRawHexPointer) {
    libop op;
    long ret = 0;
    const int width = 32;
    const int height = 32;
    vector<uchar> raw_bgra(static_cast<size_t>(width) * height * 4, 0xff);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            auto idx = static_cast<size_t>(y * width + x) * 4;
            raw_bgra[idx + 0] = 0x11;
            raw_bgra[idx + 1] = 0x22;
            raw_bgra[idx + 2] = 0x33;
            raw_bgra[idx + 3] = 0xff;
        }
    }
    wstring mode = L"mem:" + PtrToWString(raw_bgra.data(), true) + L",32,32,bgra";

    op.SetDisplayInput(mode.c_str(), &ret);
    ASSERT_EQ(ret, 1);

    wstring color;
    op.GetColor(10, 10, color);
    EXPECT_EQ(color, L"332211");
}

// ============================================================
// 6. OCR Module
// ============================================================
class OcrTest : public ::testing::Test {
  protected:
    libop op;
    long ret = 0;

    void SetUp() override {
        ASSERT_TRUE(IsOcrServerHealthy()) << "ocr_server is required for OcrTest. Start it first: "
                                          << "ocr_server.exe --datapath ./tessdata --lang chi_sim --port 8080";
    }
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
    long engine_ret = op.SetOcrEngine(L"http://127.0.0.1:8080/api/v1/ocr", L"", L"--timeout=5000");
    cout << "SetOcrEngine: " << engine_ret << endl;
    EXPECT_EQ(engine_ret, 1);
}

TEST_F(OcrTest, SetOcrEngineBaseUrl) {
    long engine_ret = op.SetOcrEngine(L"http://127.0.0.1:8080", L"", L"");
    cout << "SetOcrEngine(base url): " << engine_ret << endl;
    EXPECT_EQ(engine_ret, 1);
}

TEST_F(OcrTest, SetOcrEngineInvalidUrl) {
    long engine_ret = op.SetOcrEngine(L"http://", L"", L"--timeout=5000");
    cout << "SetOcrEngine(invalid): " << engine_ret << endl;
    EXPECT_EQ(engine_ret, 0);
}

TEST_F(OcrTest, SetOcrEngineViaDllNameArg) {
    long engine_ret = op.SetOcrEngine(L"", L"http://127.0.0.1:8080/api/v1/ocr", L"--timeout=2000");
    cout << "SetOcrEngine(via dll_name): " << engine_ret << endl;
    EXPECT_EQ(engine_ret, 1);
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
