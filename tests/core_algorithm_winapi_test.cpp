#include "test_support.h"

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using test_support::SendStringWindow;

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
    EXPECT_TRUE(pos == L"10,10" || pos == L"20,20") << "Unexpected nearest position";
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

TEST(WinApiTest, GetCmdStrReturnsPartialOutputOnTimeout) {
    libop op;
    wstring out;
    const auto start = std::chrono::steady_clock::now();
    op.GetCmdStr(L"cmd /c \"echo before & ping 127.0.0.1 -n 3 >nul & echo after\"", 200, out);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

    EXPECT_NE(out.find(L"before"), wstring::npos) << "Expected early output before timeout";
    EXPECT_EQ(out.find(L"after"), wstring::npos) << "Timed out command should not wait for trailing output";
    EXPECT_LT(elapsed.count(), 1500) << "GetCmdStr should return promptly after timeout";
}

TEST(WinApiTest, GetCmdStrHandlesLargeOutputWithoutHanging) {
    libop op;
    wstring out;
    op.GetCmdStr(L"cmd /c for /L %i in (1,1,256) do @echo 0123456789abcdefghijklmnopqrstuvwxyz", 2000, out);
    EXPECT_NE(out.find(L"0123456789abcdefghijklmnopqrstuvwxyz"), wstring::npos);
    EXPECT_GT(out.size(), 1024u) << "Expected a sizable chunk of output from the loop command";
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
