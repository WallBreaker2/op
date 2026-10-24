#include "test_support.h"

#include "../libop/base/AutomationModes.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <windows.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

using namespace std;
using test_support::SendStringWindow;

namespace {

vector<pair<int, int>> ParsePath(const wstring &path) {
    vector<pair<int, int>> points;
    wstringstream stream(path);
    wstring token;
    while (getline(stream, token, L'|')) {
        int x = 0;
        int y = 0;
        if (swscanf_s(token.c_str(), L"%d,%d", &x, &y) == 2) {
            points.emplace_back(x, y);
        }
    }
    return points;
}

struct ScopedHandle {
    HANDLE handle = INVALID_HANDLE_VALUE;

    ~ScopedHandle() {
        if (handle != INVALID_HANDLE_VALUE) {
            ::CloseHandle(handle);
        }
    }
};

struct TempCommandFile {
    wstring dir;
    wstring script;

    ~TempCommandFile() {
        if (!script.empty()) {
            ::DeleteFileW(script.c_str());
        }
        if (!dir.empty()) {
            ::RemoveDirectoryW(dir.c_str());
        }
    }
};

} // namespace

TEST(OpTest, VersionIsNotEmpty) {
    op::Op op;
    wstring ver = op.Ver();
    EXPECT_FALSE(ver.empty()) << "Version string should not be empty";
    wcout << L"Version: " << ver << endl;
}

TEST(OpTest, GetBasePath) {
    op::Op op;
    wstring path;
    op.GetBasePath(path);
    wcout << L"Base Path: " << path << endl;
    // Just verify it doesn't crash; path may be empty in test env
}

TEST(OpTest, GetID) {
    op::Op op;
    long id = 0;
    op.GetID(&id);
    cout << "Object ID: " << id << endl;
}

// 门面里凡是把 const wchar_t* 直接喂给 std::wstring 形参的，传 nullptr 会在
// _Traits::length(nullptr) 上直接 AV（COM 侧 BSTR 为 NULL 就是这个场景）。
// 修复后 nullptr 一律当空串处理：调用应该返回"失败"，而不是崩进程。
TEST(OpTest, NullStringInputsAreHandledWithoutCrash) {
    op::Op op;

    long ret = -1;
    op.SetPath(nullptr, &ret);
    // 空路径按既有语义会回落到"当前路径"，因此这里期望 1；关键是不要崩。
    EXPECT_EQ(ret, 1) << "空路径应回落到当前路径";

    ret = -1;
    op.CapturePre(nullptr, &ret);
    EXPECT_EQ(ret, 0);

    ret = -1;
    op.LoadPic(nullptr, &ret);
    EXPECT_EQ(ret, 0);

    ret = -1;
    op.FreePic(nullptr, &ret);
    EXPECT_EQ(ret, 0);

    ret = -1;
    op.SetDict(0, nullptr, &ret);
    EXPECT_EQ(ret, 0);

    ret = -1;
    op.SaveDict(0, nullptr, &ret);
    EXPECT_EQ(ret, 0);

    ret = -1;
    op.AddDict(0, nullptr, &ret);
    EXPECT_EQ(ret, 0);

    wstring matched;
    op.MatchPicName(nullptr, matched);
    EXPECT_TRUE(matched.empty());
}

// ============================================================
// 2. Algorithm Module
// ============================================================
TEST(AlgorithmTest, AStarFindPath) {
    op::Op op;
    wstring path;
    op.AStarFindPath(100, 100, L"50,50", 0, 0, 99, 99, path);

    const auto points = ParsePath(path);
    ASSERT_FALSE(points.empty());
    EXPECT_EQ(points.front(), make_pair(0, 0));
    EXPECT_EQ(points.back(), make_pair(99, 99));

    for (size_t i = 1; i < points.size(); ++i) {
        const int dx = std::abs(points[i].first - points[i - 1].first);
        const int dy = std::abs(points[i].second - points[i - 1].second);
        EXPECT_LE(dx, 1);
        EXPECT_LE(dy, 1);
        EXPECT_GT(dx + dy, 0);
    }

    EXPECT_EQ(find(points.begin(), points.end(), make_pair(50, 50)), points.end());
}

TEST(AlgorithmTest, AStarRejectsOutOfBoundsTarget) {
    op::Op op;
    wstring path;
    op.AStarFindPath(3, 3, L"", 0, 0, 3, 2, path);
    EXPECT_TRUE(path.empty());
}

TEST(AlgorithmTest, FindNearestPos) {
    op::Op op;
    wstring pos;
    op.FindNearestPos(L"10,10|20,20|30,30", 1, 15, 15, pos);
    wcout << L"Nearest Pos to (15,15): " << pos << endl;
    EXPECT_TRUE(pos == L"10,10" || pos == L"20,20") << "Unexpected nearest position";
}

// AStarFindPath 的 disable_points 和 FindNearestPos 的 all_pos 都会隐式构造 std::wstring：
// 传 nullptr 时 std::wstring(nullptr) 直接崩。C API/旧 COM 路径都可能给 NULL。
TEST(AlgorithmTest, NullStringInputsAreHandledWithoutCrash) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    wstring path;
    op.AStarFindPath(3, 3, nullptr, 0, 0, 2, 2, path);
    EXPECT_FALSE(path.empty()) << "无障碍时 nullptr 应等价于空列表，仍能算出路径";

    wstring nearest;
    op.FindNearestPos(nullptr, 1, 0, 0, nearest);
    EXPECT_TRUE(nearest.empty()) << "空的候选列表应返回空结果";
}

// ============================================================
// 3. WindowService Module
// ============================================================
TEST(WindowServiceTest, GetForegroundWindow) {
    op::Op op;
    LONG_PTR hwnd = 0;
    op.GetForegroundWindow(&hwnd);
    cout << "Foreground Window: " << hex << hwnd << dec << endl;
}

TEST(WindowServiceTest, ClientToScreenDesktopOriginIsStable) {
    op::Op op;
    const LONG_PTR hwnd = reinterpret_cast<LONG_PTR>(::GetDesktopWindow());
    long x = 10;
    long y = 20;
    long ret = 0;

    op.ClientToScreen(hwnd, &x, &y, &ret);

    EXPECT_EQ(ret, 1);
    EXPECT_EQ(x, 10);
    EXPECT_EQ(y, 20);
}

namespace {

// FindWindowByProcess 的子窗口搜索需要把 title 透传下去（WindowService.cpp 里原来写成
// `titles = titles;`，自赋值等于永远传 NULL，标题过滤失效、会命中错误的子窗口）。
struct ProcessWindowWithTwoChildren {
    HWND parent = nullptr;
    HWND child_alpha = nullptr;
    HWND child_bravo = nullptr;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        return ::DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    bool Create() {
        const HINSTANCE instance = ::GetModuleHandleW(nullptr);
        WNDCLASSW wc = {};
        wc.lpfnWndProc = ProcessWindowWithTwoChildren::WndProc;
        wc.hInstance = instance;
        wc.lpszClassName = L"OpFindWindowByProcessParent";
        if (!::RegisterClassW(&wc) && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
        parent = ::CreateWindowExW(0, wc.lpszClassName, L"op-find-by-process-parent", WS_OVERLAPPEDWINDOW,
                                   CW_USEDEFAULT, CW_USEDEFAULT, 320, 200, nullptr, nullptr, instance, nullptr);
        if (!parent) {
            return false;
        }
        child_alpha = ::CreateWindowExW(0, L"STATIC", L"op-child-alpha", WS_CHILD | WS_VISIBLE, 4, 4, 120, 24, parent,
                                        nullptr, instance, nullptr);
        child_bravo = ::CreateWindowExW(0, L"STATIC", L"op-child-bravo", WS_CHILD | WS_VISIBLE, 4, 40, 120, 24, parent,
                                        nullptr, instance, nullptr);
        ::ShowWindow(parent, SW_SHOW);
        ::UpdateWindow(parent);
        return child_alpha && child_bravo;
    }

    ~ProcessWindowWithTwoChildren() {
        if (parent) {
            ::DestroyWindow(parent);
        }
    }
};

std::wstring CurrentProcessFileName() {
    wchar_t path[MAX_PATH] = {};
    const DWORD len = ::GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return {};
    }
    const wchar_t *name = wcsrchr(path, L'\\');
    return name ? std::wstring(name + 1) : std::wstring(path);
}

} // namespace

TEST(WindowServiceTest, FindWindowByProcessFiltersChildByTitle) {
    ProcessWindowWithTwoChildren window;
    ASSERT_TRUE(window.Create()) << "测试窗口创建失败";

    // 让窗口真正出现在桌面上供枚举
    const auto deadline = ::GetTickCount64() + 120;
    MSG msg = {};
    while (::GetTickCount64() < deadline) {
        while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
        ::Sleep(10);
    }

    const std::wstring process_name = CurrentProcessFileName();
    ASSERT_FALSE(process_name.empty());

    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    LONG_PTR found = 0;
    op.FindWindowByProcess(process_name.c_str(), L"", L"bravo", &found);

    EXPECT_EQ(reinterpret_cast<HWND>(found), window.child_bravo)
        << "标题过滤没有透传到子窗口搜索：命中的不是 title 匹配的子窗口";
}

// SetWindowTransparent 曾经用 `SetWindowLong(GWL_EXSTYLE, 0x80001)` 整体覆盖扩展样式：
// 0x80001 = WS_EX_LAYERED | WS_EX_DLGMODALFRAME，会把既有的工具窗/不激活等位一起抹掉。
// 注意不能用 WS_EX_TOPMOST 做断言——置顶状态由窗口 band 维护，覆盖 EXSTYLE 后系统会把它同步回来，
// 拿它当探针会得到"假通过"。这里用 WS_EX_TOOLWINDOW（真实存在 EXSTYLE 里的位）。
TEST(WindowServiceTest, SetWindowTransparentPreservesExistingExtendedStyles) {
    ProcessWindowWithTwoChildren window;
    ASSERT_TRUE(window.Create()) << "测试窗口创建失败";

    const LONG ex_before = ::GetWindowLongW(window.parent, GWL_EXSTYLE);
    ::SetWindowLongW(window.parent, GWL_EXSTYLE, ex_before | WS_EX_TOOLWINDOW);
    ASSERT_TRUE((::GetWindowLongW(window.parent, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) != 0)
        << "前置条件：窗口应带 WS_EX_TOOLWINDOW";

    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);
    op.SetWindowTransparent(reinterpret_cast<LONG_PTR>(window.parent), 128, &ret);

    const LONG after = ::GetWindowLongW(window.parent, GWL_EXSTYLE);
    EXPECT_TRUE((after & WS_EX_LAYERED) != 0) << "应补上 WS_EX_LAYERED";
    EXPECT_TRUE((after & WS_EX_TOOLWINDOW) != 0) << "既有的 WS_EX_TOOLWINDOW 被抹掉了";
}

// GetForegroundFocus 的契约是"顶层活动窗口中具有输入焦点的窗口"。
// ::GetFocus() 只认调用线程的队列，插件在工作线程里被调时恒为 NULL（§72.37）。
TEST(WindowServiceTest, GetForegroundFocusMatchesForegroundThread) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    GUITHREADINFO gti = {};
    gti.cbSize = sizeof(gti);
    const bool have_gti = ::GetGUIThreadInfo(0, &gti) != FALSE;

    LONG_PTR focus = 0;
    op.GetForegroundFocus(&focus);

    if (have_gti && gti.hwndFocus != nullptr) {
        EXPECT_EQ(focus, reinterpret_cast<LONG_PTR>(gti.hwndFocus))
            << "应返回前台线程的焦点窗口，而不是调用线程的 GetFocus()";
    } else {
        // 前台线程没有焦点窗口（锁屏、无交互桌面）时退回本线程语义。
        EXPECT_EQ(focus, reinterpret_cast<LONG_PTR>(::GetFocus()));
    }
}

TEST(WindowServiceTest, GetWindowTitleAndRect) {
    op::Op op;
    LONG_PTR hwnd = 0;
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

TEST(WindowServiceTest, EnumWindow) {
    op::Op op;
    wstring list;
    op.EnumWindow(0, L"", L"", 1 + 2, list);
    wcout << L"EnumWindow (top 50 chars): " << list.substr(0, 50) << L"..." << endl;
}

TEST(WindowServiceTest, GetCmdStrEcho) {
    op::Op op;
    wstring out;
    op.GetCmdStr(L"cmd /c echo op_test_ok", 2000, out);
    EXPECT_NE(out.find(L"op_test_ok"), wstring::npos) << "GetCmdStr should return command output";
}

// millseconds <= 0 表示"用默认超时"。修复前默认值是 5ms：cmd.exe 还没输出就被 close() 杀掉进程树，
// 结果恒为空（与"运行命令行并返回结果"的声明矛盾）。这里用一个必然超过 5ms 的命令把它钉住。
TEST(WindowServiceTest, GetCmdStrUsesUsableDefaultTimeout) {
    op::Op op;
    wstring out;
    const auto start = std::chrono::steady_clock::now();
    op.GetCmdStr(L"cmd /c \"ping -n 2 127.0.0.1 >nul & echo op_default_ok\"", 0, out);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    EXPECT_NE(out.find(L"op_default_ok"), wstring::npos)
        << "默认超时太短：命令行输出被截断（现状输出长度=" << out.size() << "）";
    EXPECT_LT(elapsed.count(), 5000) << "默认超时不应无界等待";
}

TEST(WindowServiceTest, GetCmdStrLongCommandLine) {
    op::Op op;
    wstring payload(400, L'a');
    wstring cmd = L"cmd /c echo " + payload;
    wstring out;
    op.GetCmdStr(cmd.c_str(), 2000, out);
    EXPECT_NE(out.find(payload.substr(0, 64)), wstring::npos) << "Long command line should not truncate or hang";
}

TEST(WindowServiceTest, GetCmdStrStartsUnicodeCommandLine) {
    wchar_t temp_path[MAX_PATH] = {};
    ASSERT_GT(::GetTempPathW(MAX_PATH, temp_path), 0u);

    TempCommandFile temp;
    temp.dir = wstring(temp_path) + L"op_cmd_unicode_\u0100_" + to_wstring(::GetCurrentProcessId());
    temp.script = temp.dir + L"\\run.cmd";

    ASSERT_NE(::CreateDirectoryW(temp.dir.c_str(), nullptr), 0) << ::GetLastError();

    {
        ScopedHandle file{::CreateFileW(temp.script.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL, nullptr)};
        ASSERT_NE(file.handle, INVALID_HANDLE_VALUE) << ::GetLastError();

        const char body[] = "@echo off\r\necho unicode_cmd_ok\r\n";
        DWORD written = 0;
        ASSERT_NE(::WriteFile(file.handle, body, static_cast<DWORD>(sizeof(body) - 1), &written, nullptr), 0)
            << ::GetLastError();
        EXPECT_EQ(written, sizeof(body) - 1);
    }

    op::Op op;
    wstring out;
    const wstring cmd = L"cmd /d /c call \"" + temp.script + L"\"";
    op.GetCmdStr(cmd.c_str(), 3000, out);
    EXPECT_NE(out.find(L"unicode_cmd_ok"), wstring::npos) << out;
}

TEST(WindowServiceTest, GetCmdStrReturnsPartialOutputOnTimeout) {
    if (!test_support::GetEnvString(L"GITHUB_ACTIONS").empty()) {
        GTEST_SKIP() << "Skip flaky timeout command test on GitHub Actions";
    }

    wchar_t temp_path[MAX_PATH] = {};
    ASSERT_GT(::GetTempPathW(MAX_PATH, temp_path), 0u);

    TempCommandFile temp;
    temp.dir = wstring(temp_path) + L"op_cmd_timeout_" + to_wstring(::GetCurrentProcessId());
    temp.script = temp.dir + L"\\run.cmd";

    ASSERT_NE(::CreateDirectoryW(temp.dir.c_str(), nullptr), 0) << ::GetLastError();

    {
        ScopedHandle file{::CreateFileW(temp.script.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL, nullptr)};
        ASSERT_NE(file.handle, INVALID_HANDLE_VALUE) << ::GetLastError();

        const char body[] =
            "@echo off\r\n"
            "echo before\r\n"
            "%SystemRoot%\\System32\\ping.exe -n 6 127.0.0.1 > nul\r\n"
            "echo after\r\n";
        DWORD written = 0;
        ASSERT_NE(::WriteFile(file.handle, body, static_cast<DWORD>(sizeof(body) - 1), &written, nullptr), 0)
            << ::GetLastError();
        EXPECT_EQ(written, sizeof(body) - 1);
    }

    op::Op op;
    wstring out;
    const auto start = std::chrono::steady_clock::now();
    const wstring cmd = L"cmd /d /c call \"" + temp.script + L"\"";
    op.GetCmdStr(cmd.c_str(), 1500, out);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

    EXPECT_NE(out.find(L"before"), wstring::npos) << "Expected early output before timeout";
    EXPECT_EQ(out.find(L"after"), wstring::npos) << "Timed out command should not wait for trailing output";
    EXPECT_LT(elapsed.count(), 4000) << "GetCmdStr should return promptly after timeout";
}

TEST(WindowServiceTest, RunAppReturnsPid) {
    op::Op op;
    unsigned long pid = 0;
    long ret = 0;

    op.RunApp(L"notepad.exe", 0, &pid, &ret);

    EXPECT_EQ(ret, 1);
    EXPECT_NE(pid, 0u);

    // 测试结束后主动关闭进程，避免残留。
    HANDLE process = ::OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    ASSERT_TRUE(process != nullptr);
    EXPECT_NE(::TerminateProcess(process, 0), 0);
    ::CloseHandle(process);
}

TEST(WindowServiceTest, GetCmdStrHandlesLargeOutputWithoutHanging) {
    op::Op op;
    wstring out;
    op.GetCmdStr(L"cmd /c for /L %i in (1,1,256) do @echo 0123456789abcdefghijklmnopqrstuvwxyz", 2000, out);
    EXPECT_NE(out.find(L"0123456789abcdefghijklmnopqrstuvwxyz"), wstring::npos);
    EXPECT_GT(out.size(), 1024u) << "Expected a sizable chunk of output from the loop command";
}

TEST(WindowServiceTest, SendStringToFocusedChildEdit) {
    op::Op op;
    SendStringWindow wnd;
    ASSERT_TRUE(wnd.Create()) << "failed to create test windows";

    long ret = 0;
    op.SendString((long)(intptr_t)wnd.parent, L"abc123", &ret);
    EXPECT_EQ(ret, 1);

    EXPECT_EQ(wnd.GetEditText(), L"abc123");
}

TEST(WindowServiceTest, SendStringImeToFocusedChildEdit) {
    op::Op op;
    SendStringWindow wnd;
    ASSERT_TRUE(wnd.Create()) << "failed to create test windows";

    long ret = 0;
    op.SendStringIme((long)(intptr_t)wnd.parent, L"中文A", &ret);
    EXPECT_EQ(ret, 1);

    EXPECT_EQ(wnd.GetEditText(), L"中文A");
}

TEST(IntegrationTest, BindUnbindFurMarkIfPresent) {
    op::Op op;
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
// 4. Display input: mem:<ptr>[,w,h[,fmt]]
// ============================================================
namespace {
wstring FormatMemMethod(const void *ptr, const wchar_t *rest) {
    wchar_t buffer[96] = {};
    swprintf_s(buffer, L"mem:0x%llX%s", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(ptr)), rest);
    return buffer;
}

vector<unsigned char> MakeBmpWithInfoHeader(int width, int height, WORD header_size, int bit_count) {
    vector<unsigned char> bytes(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 64, 0);
    auto *bfh = reinterpret_cast<BITMAPFILEHEADER *>(bytes.data());
    auto *bih = reinterpret_cast<BITMAPINFOHEADER *>(bytes.data() + sizeof(BITMAPFILEHEADER));
    bfh->bfType = 0x4D42;
    bfh->bfOffBits = static_cast<DWORD>(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
    bih->biSize = header_size;
    bih->biWidth = width;
    bih->biHeight = height;
    bih->biPlanes = 1;
    bih->biBitCount = static_cast<WORD>(bit_count);
    bih->biCompression = BI_RGB;
    return bytes;
}
} // namespace

// 合法的外部缓冲区必须还能用（防止收紧校验时把正常路径一起挡掉）。
TEST(DisplayInputTest, MemModeAcceptsValidRawBuffer) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    vector<unsigned char> pixels(4 * 4 * 4, 0);
    for (size_t i = 0; i < pixels.size(); i += 4) {
        pixels[i + 0] = 0x00; // B
        pixels[i + 1] = 0x00; // G
        pixels[i + 2] = 0xFF; // R
        pixels[i + 3] = 0xFF; // A
    }

    ret = 0;
    op.SetDisplayInput(FormatMemMethod(pixels.data(), L",4,4,bgra").c_str(), &ret);
    EXPECT_EQ(ret, 1) << "合法的 mem:<ptr>,4,4,bgra 应被接受";
}

// 指针文本带负号：wcstoull(L"-1") 会回绕成 ULLONG_MAX 且不置 ERANGE，
// 修复前等于放一个野指针进来，随后按 w*h*4 读取必 AV。
TEST(DisplayInputTest, MemModeRejectsNegativePointer) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);
    ret = 0;
    op.SetDisplayInput(L"mem:-1,4,4,bgra", &ret);
    EXPECT_EQ(ret, 0) << "负号指针必须被拒绝，而不是回绕成野指针";
}

// x64 的 long 是 64 位：99999999999 能通过 wcstol，static_cast<int> 后变成一个完全不同的尺寸。
TEST(DisplayInputTest, MemModeRejectsOversizedDimension) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    vector<unsigned char> pixels(16 * 16 * 4, 0);
    ret = 0;
    op.SetDisplayInput(FormatMemMethod(pixels.data(), L",99999999999,4,bgra").c_str(), &ret);
    EXPECT_EQ(ret, 0) << "超出 int 范围的宽高必须被拒绝，不能截断后当成别的尺寸";

    ret = 0;
    op.SetDisplayInput(FormatMemMethod(pixels.data(), L",4,-2,bgra").c_str(), &ret);
    EXPECT_EQ(ret, 0) << "负高度必须被拒绝";
}

// BITMAPCOREHEADER(12 字节) 只给 biSize=12：按 40 字节解释会把后面的数据当成垃圾宽高。
TEST(DisplayInputTest, MemModeRejectsBmpWithCoreHeader) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    auto bytes = MakeBmpWithInfoHeader(2, 2, 12, 32);
    ret = 0;
    op.SetDisplayInput(FormatMemMethod(bytes.data(), L"").c_str(), &ret);
    EXPECT_EQ(ret, 0) << "biSize 不是 BITMAPINFOHEADER 时必须拒绝";
}

// biHeight == INT_MIN 时 -bih.biHeight 是 UB。
TEST(DisplayInputTest, MemModeRejectsBmpWithIntMinHeight) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    auto bytes = MakeBmpWithInfoHeader(2, INT_MIN, static_cast<WORD>(sizeof(BITMAPINFOHEADER)), 32);
    ret = 0;
    op.SetDisplayInput(FormatMemMethod(bytes.data(), L"").c_str(), &ret);
    EXPECT_EQ(ret, 0) << "biHeight=INT_MIN 必须被拒绝，而不是走 -INT_MIN 的 UB";
}


// 像素级验证：解析成功不代表搬运正确。bgr 分支要 3→4 通道扩展，BMP 分支要处理
// 自底向上/自顶向下的行序和 24bpp 的行填充，这两处最容易被写反。
// 用 3x2 的已知色块逐点比对，R/B 颠倒或上下翻转都会立刻暴露。
namespace {
struct Rgb {
    unsigned char r, g, b;
};

wstring HexOf(const Rgb &c) {
    wchar_t buffer[7] = {};
    swprintf_s(buffer, L"%02X%02X%02X", c.r, c.g, c.b);
    return buffer;
}

// 3x2 图案：第 0 行是红/绿/蓝，第 1 行是洋红/青/黄（每个通道组合都覆盖到）。
std::vector<Rgb> MakeReferencePattern() {
    return {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 0, 255}, {0, 255, 255}, {255, 255, 0}};
}
} // namespace

TEST(DisplayInputTest, MemModeBgrRawKeepsChannelOrder) {
    const int width = 3;
    const int height = 2;
    const auto pattern = MakeReferencePattern();

    std::vector<unsigned char> pixels;
    pixels.reserve(pattern.size() * 3);
    for (const auto &c : pattern) {
        pixels.push_back(c.b);
        pixels.push_back(c.g);
        pixels.push_back(c.r);
    }

    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);
    op.SetDisplayInput(FormatMemMethod(pixels.data(), L",3,2,bgr").c_str(), &ret);
    ASSERT_EQ(ret, 1) << "mem bgr 应被接受";

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            wstring color;
            op.GetColor(x, y, color);
            EXPECT_EQ(color, HexOf(pattern[y * width + x])) << "bgr 第 (" << x << "," << y << ") 点颜色不对";
        }
    }
}

namespace {
std::vector<unsigned char> MakeBmp(int width, int height, int bit_count, bool top_down,
                                   const std::vector<Rgb> &pattern) {
    const int stride = ((width * bit_count + 31) / 32) * 4;
    const size_t header_bytes = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    std::vector<unsigned char> bytes(header_bytes + static_cast<size_t>(stride) * height, 0);

    auto *bfh = reinterpret_cast<BITMAPFILEHEADER *>(bytes.data());
    auto *bih = reinterpret_cast<BITMAPINFOHEADER *>(bytes.data() + sizeof(BITMAPFILEHEADER));
    bfh->bfType = 0x4D42;
    bfh->bfSize = static_cast<DWORD>(bytes.size());
    bfh->bfOffBits = static_cast<DWORD>(header_bytes);
    bih->biSize = sizeof(BITMAPINFOHEADER);
    bih->biWidth = width;
    bih->biHeight = top_down ? -height : height;
    bih->biPlanes = 1;
    bih->biBitCount = static_cast<WORD>(bit_count);
    bih->biCompression = BI_RGB;
    bih->biSizeImage = static_cast<DWORD>(static_cast<size_t>(stride) * height);

    unsigned char *pixel_base = bytes.data() + header_bytes;
    for (int y = 0; y < height; ++y) {
        // 自顶向下：内存第 0 行就是图片第 0 行；自底向上：内存最后一行才是图片第 0 行。
        const int stored_row = top_down ? y : (height - 1 - y);
        unsigned char *row = pixel_base + static_cast<size_t>(stored_row) * stride;
        for (int x = 0; x < width; ++x) {
            const auto &c = pattern[y * width + x];
            if (bit_count == 32) {
                row[x * 4 + 0] = c.b;
                row[x * 4 + 1] = c.g;
                row[x * 4 + 2] = c.r;
                row[x * 4 + 3] = 0xFF;
            } else {
                row[x * 3 + 0] = c.b;
                row[x * 3 + 1] = c.g;
                row[x * 3 + 2] = c.r;
            }
        }
    }
    return bytes;
}
} // namespace

TEST(DisplayInputTest, MemModeBmp32TopDownKeepsRowOrder) {
    const auto pattern = MakeReferencePattern();
    auto bytes = MakeBmp(3, 2, 32, true, pattern);

    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);
    op.SetDisplayInput(FormatMemMethod(bytes.data(), L"").c_str(), &ret);
    ASSERT_EQ(ret, 1) << "32bpp 自顶向下 BMP 应被接受";

    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 3; ++x) {
            wstring color;
            op.GetColor(x, y, color);
            EXPECT_EQ(color, HexOf(pattern[y * 3 + x])) << "32bpp 自顶向下 BMP 第 (" << x << "," << y << ") 点颜色不对";
        }
    }
}

TEST(DisplayInputTest, MemModeBmp32BottomUpFlipsRows) {
    const auto pattern = MakeReferencePattern();
    auto bytes = MakeBmp(3, 2, 32, false, pattern);

    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);
    op.SetDisplayInput(FormatMemMethod(bytes.data(), L"").c_str(), &ret);
    ASSERT_EQ(ret, 1) << "32bpp 自底向上 BMP 应被接受";

    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 3; ++x) {
            wstring color;
            op.GetColor(x, y, color);
            EXPECT_EQ(color, HexOf(pattern[y * 3 + x]))
                << "32bpp 自底向上 BMP 第 (" << x << "," << y << ") 点颜色不对（行序没有翻转？）";
        }
    }
}

// 24bpp + 宽度 3 时行距是 12 字节、每行有 3 字节填充，最容易把 stride 和行宽搞混。
TEST(DisplayInputTest, MemModeBmp24UsesPaddedStride) {
    const auto pattern = MakeReferencePattern();
    auto bytes = MakeBmp(3, 2, 24, true, pattern);

    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);
    op.SetDisplayInput(FormatMemMethod(bytes.data(), L"").c_str(), &ret);
    ASSERT_EQ(ret, 1) << "24bpp 自顶向下 BMP 应被接受";

    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 3; ++x) {
            wstring color;
            op.GetColor(x, y, color);
            EXPECT_EQ(color, HexOf(pattern[y * 3 + x]))
                << "24bpp BMP 第 (" << x << "," << y << ") 点颜色不对（行填充处理错了？）";
        }
    }
}


// pic: 模式 = Path2GlobalPath + ATL::CImage::Load + 逐行取块。
// 这条路径此前没有任何用例，本批覆盖：正常取块、图片边缘裁剪、缺文件、非图片文件。
namespace {
std::wstring WriteTempFile(const wchar_t *file_name, const std::vector<unsigned char> &bytes) {
    const std::wstring path = test_support::GetTempBmpPath(file_name);
    std::ofstream out(std::filesystem::path(path), std::ios::binary | std::ios::trunc);
    if (!out) {
        return std::wstring();
    }
    out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return out ? path : std::wstring();
}

wstring PicMethod(const std::wstring &path) {
    return wstring(L"pic:") + path;
}
} // namespace

TEST(DisplayInputTest, PicModeReadsBmpFileAndServesPixels) {
    const auto pattern = MakeReferencePattern();
    auto bytes = MakeBmp(3, 2, 32, true, pattern);
    const std::wstring path = WriteTempFile(L"op_pic_pattern.bmp", bytes);
    ASSERT_FALSE(path.empty()) << "临时 BMP 写入失败";

    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);
    op.SetDisplayInput(PicMethod(path).c_str(), &ret);
    ASSERT_EQ(ret, 1) << "pic: 应接受刚写出的 BMP 文件";

    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 3; ++x) {
            wstring color;
            op.GetColor(x, y, color);
            EXPECT_EQ(color, HexOf(pattern[y * 3 + x])) << "pic 第 (" << x << "," << y << ") 点颜色不对";
        }
    }
}

// 右下角取色会走到"请求块超出图片右/下边界"的裁剪分支，不能读到相邻行或越界内存。
TEST(DisplayInputTest, PicModeClipsRequestAtImageEdge) {
    const auto pattern = MakeReferencePattern();
    auto bytes = MakeBmp(3, 2, 32, true, pattern);
    const std::wstring path = WriteTempFile(L"op_pic_edge.bmp", bytes);
    ASSERT_FALSE(path.empty());

    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);
    op.SetDisplayInput(PicMethod(path).c_str(), &ret);
    ASSERT_EQ(ret, 1);

    wstring corner;
    op.GetColor(2, 1, corner);
    EXPECT_EQ(corner, HexOf(pattern[1 * 3 + 2])) << "右下角取色应命中图片最后一个像素";
}

TEST(DisplayInputTest, PicModeRejectsMissingFile) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    const std::wstring path = test_support::GetTempBmpPath(L"op_pic_does_not_exist.bmp");
    ::DeleteFileW(path.c_str());
    ret = 0;
    op.SetDisplayInput(PicMethod(path).c_str(), &ret);
    EXPECT_EQ(ret, 0) << "不存在的图片文件必须被拒绝";
}

TEST(DisplayInputTest, PicModeRejectsNonImageFile) {
    std::vector<unsigned char> text{'n', 'o', 't', ' ', 'a', 'n', ' ', 'i', 'm', 'a', 'g', 'e'};
    const std::wstring path = WriteTempFile(L"op_pic_fake.bmp", text);
    ASSERT_FALSE(path.empty());

    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);
    ret = 0;
    op.SetDisplayInput(PicMethod(path).c_str(), &ret);
    EXPECT_EQ(ret, 0) << "内容不是图片的文件必须被拒绝";
}


// 共享段/互斥体的名字是主控 DLL 与注入 DLL 之间的"线格式"。
// 历史上这两个前缀是反的（段叫 op_mutex_*、互斥体叫 op_shared_mem_*），第 72 批按操作者要求
// 改回正名（defect_fixes.md §72.20）。这条用例把新格式钉死 —— 谁再"顺手"改前缀，
// 混装版本的两侧就会各自等不到对方的帧（表现是后台截图超时，不是报错）。
TEST(DisplayInputTest, SharedResourceNamesAreLockedWireFormat) {
    const HWND a = reinterpret_cast<HWND>(static_cast<uintptr_t>(0x1234));
    const HWND b = reinterpret_cast<HWND>(static_cast<uintptr_t>(0x5678));

    EXPECT_EQ(MakeOpSharedResourceName(a), L"op_shared_mem_4660") << "共享段的名字格式被改了（注入 DLL 会失联）";
    EXPECT_EQ(MakeOpMutexName(a), L"op_mutex_4660") << "互斥体的名字格式被改了（注入 DLL 会失联）";
    EXPECT_NE(MakeOpSharedResourceName(a), MakeOpMutexName(a)) << "段和互斥体不能重名";
    EXPECT_NE(MakeOpSharedResourceName(a), MakeOpSharedResourceName(b)) << "不同窗口必须拿到不同名字";
    // 名字不能带反：段用 op_shared_mem_ 前缀、互斥体用 op_mutex_ 前缀。
    EXPECT_EQ(MakeOpSharedResourceName(a).rfind(L"op_shared_mem_", 0), 0u) << "段名字前缀不对";
    EXPECT_EQ(MakeOpMutexName(a).rfind(L"op_mutex_", 0), 0u) << "互斥体名字前缀不对";

    // 老的 "op_xxx_%d" 宏在 x64 上会把 HWND 截断成 32 位；helper 必须保留完整值。
    // 32 位构建里 HWND 本身就是 32 位，"高 32 位非零"的场景构造不出来（值在入参处就被截断），
    // 所以这段只在 x64 上有意义 —— 以前它无条件跑，x86 上 EXPECT_NE 必然失败。
#if defined(_WIN64)
    const auto wide_value = static_cast<uintptr_t>(0x1234567890ULL);
    const HWND wide = reinterpret_cast<HWND>(wide_value);
    EXPECT_EQ(MakeOpSharedResourceName(wide), L"op_shared_mem_" + std::to_wstring(wide_value));
    const auto truncated = static_cast<uint32_t>(wide_value);
    EXPECT_NE(MakeOpSharedResourceName(wide), L"op_shared_mem_" + std::to_wstring(truncated))
        << "64 位句柄被截断成了 32 位名字";
#endif
}


// MatchPicName 把用户输入当正则用（只转义了 .()[]{}，* 和 ? 被转成通配），
// 但 std::wregex 的**构造**没在 try 里：反斜杠、多余的 + 等会让它抛 std::regex_error，
// 异常直接穿出 facade（COM/C API 调用方拿到的是一次崩溃，而不是"没有匹配"）。
TEST(DisplayInputTest, MatchPicNameRejectsInvalidPatternWithoutCrash) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    std::wstring matched;
    op.MatchPicName(L"a\\", matched);
    EXPECT_TRUE(matched.empty()) << "以反斜杠结尾的模式非法，应返回空结果而不是抛异常";

    matched.clear();
    op.MatchPicName(L"a++", matched);
    EXPECT_TRUE(matched.empty()) << "重复量词非法，应返回空结果而不是抛异常";

    matched.clear();
    op.MatchPicName(L"ok*", matched); // 合法模式仍应正常工作（不要求匹配到文件）
    EXPECT_NO_THROW(op.MatchPicName(L"ok*", matched));
}


// ============================================================
// 5. Memory facade: 非法参数必须是"干净失败"，不是崩溃
// ============================================================
// 这些入口的防御检查是**门面可达**的（上层没有裁剪参数），所以必须有覆盖：
// 非法 hwnd / 非法地址串 / 负长度 都必须返回失败值，且不能把进程带崩。
TEST(MemoryFacadeTest, InvalidArgumentsFailCleanly) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    const LONG_PTR bogus_hwnd = static_cast<LONG_PTR>(0x7FFFFFF0);

    // 非法地址串：解析不出地址 → 失败，不崩。
    int64_t int_value = -1;
    op.ReadInt(bogus_hwnd, L"not-an-address", 1, &int_value);
    EXPECT_EQ(int_value, 0) << "非法地址应返回 0";

    std::wstring data;
    op.ReadData(bogus_hwnd, L"zzzz", 16, data);
    EXPECT_TRUE(data.empty()) << "非法地址读数据应返回空串";

    ret = -1;
    op.WriteInt(bogus_hwnd, L"zzzz", 1, 123, &ret);
    EXPECT_EQ(ret, 0) << "非法地址写整数应失败";

    ret = -1;
    op.WriteData(bogus_hwnd, L"zzzz", L"00", 1, &ret);
    EXPECT_EQ(ret, 0) << "非法地址写数据应失败";

    // 空指针地址：直接判定失败。
    int_value = -1;
    op.ReadInt(bogus_hwnd, nullptr, 1, &int_value);
    EXPECT_EQ(int_value, 0) << "空地址应返回 0";

    // 负长度：不能被当成超大 size_t 去分配/读取。
    std::wstring negative;
    op.ReadData(bogus_hwnd, L"0", -1, negative);
    EXPECT_TRUE(negative.empty()) << "负长度应返回空串";
}

// 正例：对自己进程的模块基址读一个整数，应当成功（证明上面那些失败不是"全都失败"）。
TEST(MemoryFacadeTest, ReadsIntFromOwnProcess) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    // 用测试窗口的 hwnd 让 facade 解析出目标进程（本进程）。
    test_support::ColorPulseWindow window;
    ASSERT_TRUE(window.Create(false));
    const LONG_PTR hwnd = reinterpret_cast<LONG_PTR>(window.hwnd);

    const auto *module_base = reinterpret_cast<const wchar_t *>(::GetModuleHandleW(nullptr));
    ASSERT_NE(module_base, nullptr);

    wchar_t address[32] = {};
    swprintf_s(address, L"%llX", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(module_base)));

    // 'MZ' 头：type=1 是 int16（type=6 是 uint8，只会读到 'M' 一个字节 —— 一开始我就写错了）。
    int64_t value = 0;
    op.ReadInt(hwnd, address, 1, &value);
    EXPECT_EQ(value & 0xFFFF, 0x5A4D) << "应能读到自己进程模块头的 MZ 标记（实际=" << value << "）";
}


// 批量形状：一次调用读一整组地址。判据是"结果和逐条读完全一致"，外加
// ①失败项被单独标出来（ok=false、值写 0）②一批里混入坏地址不影响其它项。
// 这里的意义不只是省调用次数：每条 `ReadInt` 都要过一次进程上下文准备、字符串转换和异常帧，
// 批量把这些固定开销摊到整批上。
TEST(MemoryFacadeTest, ReadIntBatchMatchesSingleReads) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    test_support::ColorPulseWindow window;
    ASSERT_TRUE(window.Create(false));
    const LONG_PTR hwnd = reinterpret_cast<LONG_PTR>(window.hwnd);

    // 造一块已知内容的内存，四个字段分别用 i32 / i32 / i64 / i16 读。
    struct Fields {
        int32_t a;
        int32_t b;
        int64_t c;
        int16_t d;
    };
    static Fields fields;
    fields.a = 0x12345678;
    fields.b = -7;
    fields.c = 0x1122334455667788LL;
    fields.d = -2;

    auto hex_of = [](const void *p) {
        wchar_t buf[32] = {};
        swprintf_s(buf, L"%llX", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(p)));
        return std::wstring(buf);
    };
    const std::wstring a0 = hex_of(&fields.a);
    const std::wstring a1 = hex_of(&fields.b);
    const std::wstring a2 = hex_of(&fields.c);
    const std::wstring a3 = hex_of(&fields.d);

    // 逐条读（老形状）当基准。
    int64_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    op.ReadInt(hwnd, a0.c_str(), 0, &s0);
    op.ReadInt(hwnd, a1.c_str(), 0, &s1);
    op.ReadInt(hwnd, a2.c_str(), 3, &s2);
    op.ReadInt(hwnd, a3.c_str(), 1, &s3);
    EXPECT_EQ(s0, 0x12345678);
    EXPECT_EQ(s1, -7);
    EXPECT_EQ(s2, 0x1122334455667788LL);
    EXPECT_EQ(s3, -2);

    // 批量读（同一批只能一个 type，所以按 type 分组）。
    const wchar_t *addrs32[] = {a0.c_str(), a1.c_str()};
    int64_t vals32[2] = {-1, -1};
    bool ok32[2] = {false, false};
    long done32 = 0;
    op.ReadIntBatch(hwnd, addrs32, 2, 0, vals32, &done32, ok32);
    EXPECT_EQ(done32, 2);
    EXPECT_TRUE(ok32[0]);
    EXPECT_TRUE(ok32[1]);
    EXPECT_EQ(vals32[0], s0);
    EXPECT_EQ(vals32[1], s1);

    const wchar_t *addrs64[] = {a2.c_str()};
    int64_t vals64[1] = {0};
    long done64 = 0;
    op.ReadIntBatch(hwnd, addrs64, 1, 3, vals64, &done64, nullptr);
    EXPECT_EQ(done64, 1);
    EXPECT_EQ(vals64[0], s2);

    // 一批里混一个坏地址：只有它失败，其它照旧。
    const wchar_t *mixed[] = {a0.c_str(), L"0", a1.c_str()};
    int64_t mvals[3] = {-1, -1, -1};
    bool mok[3] = {false, false, false};
    long mdone = 0;
    op.ReadIntBatch(hwnd, mixed, 3, 0, mvals, &mdone, mok);
    EXPECT_EQ(mdone, 2) << "地址 0 应当读失败";
    EXPECT_TRUE(mok[0]);
    EXPECT_FALSE(mok[1]);
    EXPECT_TRUE(mok[2]);
    EXPECT_EQ(mvals[0], s0);
    EXPECT_EQ(mvals[1], 0) << "失败项必须写 0，不能留调用前的值";
    EXPECT_EQ(mvals[2], s1);
}

// 批量写：写进去的值必须逐项生效，坏地址不写、不影响其它项。
// 一批里混入不可读地址时：只那一项失败，其余照常返回，而且失败之后句柄要能重新挂上
// （mem_read 失败会 invalidate_attach，不重新 prepare 的话后面每一项都会跟着失败——§72.36）。
TEST(MemoryFacadeTest, ReadIntBatchIsolatesUnreadableAddress) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    test_support::ColorPulseWindow window;
    ASSERT_TRUE(window.Create(false));
    const LONG_PTR hwnd = reinterpret_cast<LONG_PTR>(window.hwnd);

    static int32_t slots[2] = {0x0B0B0B0B, 0x0C0C0C0C};
    auto hex_of = [](const void *p) {
        wchar_t buf[32] = {};
        swprintf_s(buf, L"%llX", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(p)));
        return std::wstring(buf);
    };

    const std::wstring addrs[] = {hex_of(&slots[0]), L"1", hex_of(&slots[1])};
    const wchar_t *addr_ptrs[] = {addrs[0].c_str(), addrs[1].c_str(), addrs[2].c_str()};
    int64_t values[3] = {1, 1, 1};
    bool ok[3] = {true, true, true};
    long done = -1;
    op.ReadIntBatch(hwnd, addr_ptrs, 3, 0, values, &done, ok);

    EXPECT_EQ(done, 2) << "只有坏地址那一项该失败";
    EXPECT_TRUE(ok[0]);
    EXPECT_FALSE(ok[1]) << "坏地址必须被单独标出来";
    EXPECT_TRUE(ok[2]) << "坏地址后面的项不该被连坐";
    EXPECT_EQ(values[0], 0x0B0B0B0B);
    EXPECT_EQ(values[1], 0) << "失败项的值写 0";
    EXPECT_EQ(values[2], 0x0C0C0C0C);

    // 坏地址之后单条读也要正常（句柄被重新 prepare 过）。
    int64_t again = 0;
    op.ReadInt(hwnd, addrs[2].c_str(), 0, &again);
    EXPECT_EQ(again, 0x0C0C0C0C);
}

TEST(MemoryFacadeTest, WriteIntBatchWritesEveryItem) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    test_support::ColorPulseWindow window;
    ASSERT_TRUE(window.Create(false));
    const LONG_PTR hwnd = reinterpret_cast<LONG_PTR>(window.hwnd);

    static int32_t slots[3] = {0, 0, 0};
    auto hex_of = [](const void *p) {
        wchar_t buf[32] = {};
        swprintf_s(buf, L"%llX", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(p)));
        return std::wstring(buf);
    };
    const std::wstring a0 = hex_of(&slots[0]);
    const std::wstring a1 = hex_of(&slots[1]);
    const std::wstring a2 = hex_of(&slots[2]);

    const wchar_t *addrs[] = {a0.c_str(), a1.c_str(), L"0", a2.c_str()};
    const int64_t values[] = {111, 222, 333, 444};
    bool ok[4] = {false, false, false, false};
    long done = 0;
    op.WriteIntBatch(hwnd, addrs, 4, 0, values, &done, ok);

    EXPECT_EQ(done, 3) << "地址 0 那项应当写失败";
    EXPECT_TRUE(ok[0]);
    EXPECT_TRUE(ok[1]);
    EXPECT_FALSE(ok[2]);
    EXPECT_TRUE(ok[3]);
    EXPECT_EQ(slots[0], 111);
    EXPECT_EQ(slots[1], 222);
    EXPECT_EQ(slots[2], 444);
}

// 打印批量 vs 逐条的每条平均开销（只打印、不断言时间，避免机器差异导致 flaky）。
// 关注点：批量形状是否真的把"每次调用的固定开销"摊掉了。
TEST(MemoryFacadeTest, ReadIntBatchPerItemCostIsLower) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    test_support::ColorPulseWindow window;
    ASSERT_TRUE(window.Create(false));
    const LONG_PTR hwnd = reinterpret_cast<LONG_PTR>(window.hwnd);

    constexpr int kCount = 64;
    static int32_t block[kCount] = {};
    std::vector<std::wstring> texts;
    std::vector<const wchar_t *> addrs;
    texts.reserve(kCount);
    addrs.reserve(kCount);
    for (int i = 0; i < kCount; ++i) {
        wchar_t buf[32] = {};
        swprintf_s(buf, L"%llX", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(&block[i])));
        texts.emplace_back(buf);
    }
    for (const auto &t : texts)
        addrs.push_back(t.c_str());

    std::vector<int64_t> values(kCount, 0);
    int64_t sink = 0;
    op.ReadInt(hwnd, addrs[0], 0, &sink); // 预热：模块列表/地址缓存

    constexpr int kRounds = 50;
    auto single_start = std::chrono::steady_clock::now();
    for (int r = 0; r < kRounds; ++r) {
        for (int i = 0; i < kCount; ++i)
            op.ReadInt(hwnd, addrs[i], 0, &values[i]);
    }
    const auto single_end = std::chrono::steady_clock::now();

    long done = 0;
    auto batch_start = std::chrono::steady_clock::now();
    for (int r = 0; r < kRounds; ++r) {
        op.ReadIntBatch(hwnd, addrs.data(), kCount, 0, values.data(), &done, nullptr);
    }
    const auto batch_end = std::chrono::steady_clock::now();

    const double single_us =
        std::chrono::duration<double, std::micro>(single_end - single_start).count() / (kRounds * kCount);
    const double batch_us =
        std::chrono::duration<double, std::micro>(batch_end - batch_start).count() / (kRounds * kCount);
    std::cout << "[BENCH] 每条 " << kCount << " 地址：逐条 " << single_us << " us/条，批量 " << batch_us
              << " us/条，比值 " << (batch_us > 0 ? single_us / batch_us : 0.0) << "x" << std::endl;
    EXPECT_EQ(done, kCount);
}

// ============================================================
// 6. Window facade: 非法 hwnd / 正例
// ============================================================
// OpWindow.cpp 里这些入口把 hwnd 直接交给 Win32，没有上层裁剪 —— 属于"门面可达"的防御，
// 之前零用例。要求：非法句柄一律干净失败（返回 0 / 空串），不能崩，也不能返回"成功"。
TEST(WindowFacadeTest, InvalidHwndFailsCleanly) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    const LONG_PTR bogus = static_cast<LONG_PTR>(0x0BADF00D);

    long x1 = -1, y1 = -1, x2 = -1, y2 = -1, nret = -1;
    op.GetClientRect(bogus, &x1, &y1, &x2, &y2, &nret);
    EXPECT_EQ(nret, 0) << "非法句柄取客户区应失败";

    long width = -1, height = -1;
    nret = -1;
    op.GetClientSize(bogus, &width, &height, &nret);
    EXPECT_EQ(nret, 0) << "非法句柄取客户区尺寸应失败";

    nret = -1;
    op.GetWindowRect(bogus, &x1, &y1, &x2, &y2, &nret);
    EXPECT_EQ(nret, 0) << "非法句柄取窗口矩形应失败";

    std::wstring text;
    op.GetWindowClass(bogus, text);
    EXPECT_TRUE(text.empty()) << "非法句柄取类名应为空";

    text.clear();
    op.GetWindowTitle(bogus, text);
    EXPECT_TRUE(text.empty()) << "非法句柄取标题应为空";

    nret = -1;
    op.SetWindowText(bogus, L"nope", &nret);
    EXPECT_EQ(nret, 0) << "非法句柄设置标题应失败";

    nret = -1;
    op.SetWindowSize(bogus, 200, 150, &nret);
    EXPECT_EQ(nret, 0) << "非法句柄设置窗口尺寸应失败";

    nret = -1;
    op.SetClientSize(bogus, 200, 150, &nret);
    EXPECT_EQ(nret, 0) << "非法句柄设置客户区尺寸应失败";

    nret = -1;
    op.MoveWindow(bogus, 10, 10, &nret);
    EXPECT_EQ(nret, 0) << "非法句柄移动窗口应失败";

    long screen_x = -1, screen_y = -1;
    nret = -1;
    op.ClientToScreen(bogus, &screen_x, &screen_y, &nret);
    EXPECT_EQ(nret, 0) << "非法句柄客户区坐标转屏幕坐标应失败";

    // x/y 是可选出参：旧实现无条件解引用，调用方传 nullptr 会崩。
    // 这里锁住“空出参也不崩、且干净返回失败”。
    nret = -1;
    op.ClientToScreen(bogus, nullptr, nullptr, &nret);
    EXPECT_EQ(nret, 0) << "非法句柄 + 空出参也应干净失败";

    nret = -1;
    op.SetWindowState(bogus, 1, &nret);
    EXPECT_EQ(nret, 0) << "非法句柄设置窗口状态应失败";

    nret = -1;
    op.SetWindowTransparent(bogus, 200, &nret);
    EXPECT_EQ(nret, 0) << "非法句柄设置透明度应失败";

    long send_ret = -1;
    op.SendString(bogus, L"x", &send_ret);
    EXPECT_EQ(send_ret, 0) << "非法句柄发送文本应失败";

    LONG_PTR found = -1;
    op.FindWindowEx(bogus, nullptr, nullptr, &found);
    EXPECT_EQ(found, 0) << "非法父窗口查找子窗口应返回 0";
}

TEST(WindowFacadeTest, BasicWindowQueriesAndMutations) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    test_support::ColorPulseWindow window;
    ASSERT_TRUE(window.Create(false));
    const LONG_PTR hwnd = reinterpret_cast<LONG_PTR>(window.hwnd);

    long nret = 0;
    long width = 0, height = 0;
    op.GetClientSize(hwnd, &width, &height, &nret);
    ASSERT_EQ(nret, 1) << "合法句柄取客户区尺寸应成功";
    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);

    std::wstring title;
    op.GetWindowTitle(hwnd, title);
    EXPECT_FALSE(title.empty()) << "合法句柄应能取到标题";

    std::wstring cls;
    op.GetWindowClass(hwnd, cls);
    EXPECT_FALSE(cls.empty()) << "合法句柄应能取到类名";

    // 设置尺寸后客户区尺寸应跟着变。
    nret = 0;
    op.SetWindowSize(hwnd, 300, 220, &nret);
    EXPECT_EQ(nret, 1) << "合法句柄设置窗口尺寸应成功";
    { // 本文件没有 PumpMessagesFor，就地泵 150ms
        MSG msg = {};
        const ULONGLONG deadline = ::GetTickCount64() + 150;
        while (::GetTickCount64() < deadline) {
            while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                ::TranslateMessage(&msg);
                ::DispatchMessageW(&msg);
            }
            ::Sleep(10);
        }
    }
    long new_width = 0, new_height = 0;
    nret = 0;
    op.GetClientSize(hwnd, &new_width, &new_height, &nret);
    ASSERT_EQ(nret, 1);
    // 窗口初始是 220x180（客户区更小），设成 300x220 是**变大**。
    EXPECT_GT(new_width, width) << "设成 300x220 后客户区宽应变大";
    EXPECT_GT(new_width, 250) << "客户区宽应接近 300（差值来自边框）";
    EXPECT_GT(new_height, 0);
}


// ============================================================
// 7. Input facade: 参数校验（不产生真实输入）
// ============================================================
// 只覆盖"必须被拒绝、且不得真的注入输入"的那几条路径 —— 不做全局鼠标/键盘动作，
// 避免干扰当前桌面。
TEST(InputFacadeTest, RejectsInvalidParametersWithoutInjecting) {
    op::Op op;
    long ret = -1;
    op.SetShowErrorMsg(3, &ret);

    // 延迟类型/数值非法：必须返回 0，且不能把既有配置带坏（后续合法调用仍返回 1）。
    ret = -1;
    op.SetMouseDelay(nullptr, 100, &ret);
    EXPECT_EQ(ret, 0) << "空类型应被拒绝";

    ret = -1;
    op.SetMouseDelay(L"bogus", 100, &ret);
    EXPECT_EQ(ret, 0) << "未知鼠标类型应被拒绝";

    ret = -1;
    op.SetMouseDelay(L"normal", -5, &ret);
    EXPECT_EQ(ret, 0) << "负延迟应被拒绝";

    ret = -1;
    op.SetKeypadDelay(nullptr, 10, &ret);
    EXPECT_EQ(ret, 0) << "空类型应被拒绝";

    ret = -1;
    op.SetKeypadDelay(L"bogus", 10, &ret);
    EXPECT_EQ(ret, 0) << "未知键盘类型应被拒绝";

    ret = -1;
    op.SetKeypadDelay(L"normal", -1, &ret);
    EXPECT_EQ(ret, 0) << "负延迟应被拒绝";

    // 合法调用仍应成功（证明上面的拒绝没有把状态带坏）。
    ret = -1;
    op.SetMouseDelay(L"normal", 30, &ret);
    EXPECT_EQ(ret, 1) << "合法鼠标延迟应被接受";

    ret = -1;
    op.SetKeypadDelay(L"normal", 30, &ret);
    EXPECT_EQ(ret, 1) << "合法键盘延迟应被接受";

    // 文本键名非法/为空：不得注入任何按键。
    ret = -1;
    op.KeyPressChar(nullptr, &ret);
    EXPECT_EQ(ret, 0) << "空键名应被拒绝";

    ret = -1;
    op.KeyPressChar(L"", &ret);
    EXPECT_EQ(ret, 0) << "空字符串键名应被拒绝";

    ret = -1;
    op.KeyDownChar(L"no-such-key", &ret);
    EXPECT_EQ(ret, 0) << "未知键名应被拒绝";

    ret = -1;
    op.KeyUpChar(L"no-such-key", &ret);
    EXPECT_EQ(ret, 0) << "未知键名应被拒绝";

    ret = -1;
    op.KeyPressStr(nullptr, 10, &ret);
    EXPECT_EQ(ret, 0) << "空字符串应被拒绝";

    // 零超时的 WaitKey 必须立刻返回，不能卡住。
    ret = -1;
    const auto start = std::chrono::steady_clock::now();
    op.WaitKey(0, 0, &ret);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    EXPECT_LT(elapsed, 500) << "零超时 WaitKey 卡了 " << elapsed << " ms";
}


// ============================================================
// 8. Dictionary facade: idx 越界 / 空参数
// ============================================================
// 字典索引上限是 100（ImageSearchService::_max_dict）。这些入口都是门面可达的，
// 之前零用例。要求：越界与非法的 idx/文件名一律返回 0 或空串，不能崩，也不能"假装成功"。
TEST(DictFacadeTest, RejectsOutOfRangeIndexesAndBadFiles) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    const long kMaxDict = 100;

    // SetDict：越界 + 不存在的文件
    ret = -1;
    op.SetDict(-1, L"x.dict", &ret);
    EXPECT_EQ(ret, 0) << "负索引应被拒绝";

    ret = -1;
    op.SetDict(kMaxDict, L"x.dict", &ret);
    EXPECT_EQ(ret, 0) << "索引等于上限应被拒绝";

    ret = -1;
    op.SetDict(0, L"no-such-file.dict", &ret);
    EXPECT_EQ(ret, 0) << "不存在的字典文件应被拒绝";

    // UseDict：越界拒绝，合法索引成功
    ret = -1;
    op.UseDict(-1, &ret);
    EXPECT_EQ(ret, 0) << "负索引应被拒绝";

    ret = -1;
    op.UseDict(kMaxDict, &ret);
    EXPECT_EQ(ret, 0) << "索引等于上限应被拒绝";

    ret = -1;
    op.UseDict(0, &ret);
    EXPECT_EQ(ret, 1) << "合法索引应被接受";

    // GetDict：越界 / 空字典 / 字体索引越界 都要返回空串
    std::wstring word;
    op.GetDict(-1, 0, word);
    EXPECT_TRUE(word.empty()) << "负索引取词应为空";

    word.clear();
    op.GetDict(kMaxDict, 0, word);
    EXPECT_TRUE(word.empty()) << "索引等于上限取词应为空";

    word.clear();
    op.GetDict(1, 0, word);
    EXPECT_TRUE(word.empty()) << "空字典取词应为空";

    // SetMemDict：越界 / 空指针 / 非正长度
    const unsigned char payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    ret = -1;
    op.SetMemDict(-1, payload, sizeof(payload), &ret);
    EXPECT_EQ(ret, 0) << "负索引应被拒绝";

    ret = -1;
    op.SetMemDict(0, nullptr, sizeof(payload), &ret);
    EXPECT_EQ(ret, 0) << "空数据指针应被拒绝";

    ret = -1;
    op.SetMemDict(0, payload, 0, &ret);
    EXPECT_EQ(ret, 0) << "零长度应被拒绝";

    // AddDict：越界 / 非法条目格式
    ret = -1;
    op.AddDict(kMaxDict, L"0,0,0,0,31", &ret);
    EXPECT_EQ(ret, 0) << "索引等于上限应被拒绝";

    ret = -1;
    op.AddDict(0, L"not-a-valid-entry", &ret);
    EXPECT_EQ(ret, 0) << "非法条目格式应被拒绝";

    // SaveDict / ClearDict / GetDictCount：越界一律失败
    const std::wstring out_path = test_support::GetTempBmpPath(L"op_dict_out.dict");
    ret = -1;
    op.SaveDict(-1, out_path.c_str(), &ret);
    EXPECT_EQ(ret, 0) << "负索引保存应失败";

    ret = -1;
    op.ClearDict(kMaxDict, &ret);
    EXPECT_EQ(ret, 0) << "索引等于上限清空应失败";

    ret = -1;
    op.GetDictCount(-1, &ret);
    EXPECT_EQ(ret, 0) << "负索引取数量应为 0";

    // 合法索引但空字典：数量应为 0（不能报成功但有值）
    ret = -1;
    op.GetDictCount(2, &ret);
    EXPECT_EQ(ret, 0) << "空字典数量应为 0";
}


// ============================================================
// 9. Process / Clipboard facade
// ============================================================
TEST(ProcessFacadeTest, WinExecRejectsEmptyCommand) {
    op::Op op;
    long ret = -1;
    op.SetShowErrorMsg(3, &ret);

    ret = -1;
    op.WinExec(nullptr, 0, &ret);
    EXPECT_EQ(ret, 0) << "空命令行不能启动任何进程";

    ret = -1;
    op.WinExec(L"", 0, &ret);
    EXPECT_EQ(ret, 0) << "空字符串命令行不能启动任何进程";
}

TEST(ProcessFacadeTest, GetCmdStrHandlesEmptyAndNormalCommands) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    // 空命令：不得崩溃，也不得卡住（内部默认超时 5s）。
    std::wstring empty_out;
    const auto start_empty = std::chrono::steady_clock::now();
    op.GetCmdStr(nullptr, 500, empty_out);
    const auto empty_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now() - start_empty)
                                   .count();
    EXPECT_LT(empty_elapsed, 3000) << "空命令不应卡住（实际 " << empty_elapsed << " ms）";

    // 正常命令：输出要能拿到。
    std::wstring out;
    op.GetCmdStr(L"cmd /c echo op_proc_ok", 3000, out);
    EXPECT_NE(out.find(L"op_proc_ok"), std::wstring::npos) << "应拿到命令输出（实际=" << out << L"）";

    // millseconds<=0 走默认超时：快速命令仍必须有输出（第 16 批修的就是这个 5ms 退化）。
    std::wstring default_out;
    op.GetCmdStr(L"cmd /c echo op_default_ok", 0, default_out);
    EXPECT_NE(default_out.find(L"op_default_ok"), std::wstring::npos)
        << "默认超时下应拿到输出（实际=" << default_out << L"）";

    // 立即退出的命令不应被当成失败。
    std::wstring exit_out;
    const auto start_exit = std::chrono::steady_clock::now();
    op.GetCmdStr(L"cmd /c exit 0", 2000, exit_out);
    const auto exit_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  std::chrono::steady_clock::now() - start_exit)
                                  .count();
    EXPECT_LT(exit_elapsed, 3000) << "立即退出的命令不应长时间等待（实际 " << exit_elapsed << " ms）";
}

// 剪贴板用例会改动用户剪贴板，所以先存原值、跑完再还原。
TEST(ClipboardFacadeTest, SetAndGetRoundTripRestoresOriginal) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    std::wstring original;
    op.GetClipboard(original);

    ret = 0;
    op.SetClipboard(L"op_clipboard_probe_123", &ret);
    EXPECT_EQ(ret, 1) << "设置剪贴板应成功";

    std::wstring readback;
    op.GetClipboard(readback);
    EXPECT_NE(readback.find(L"op_clipboard_probe_123"), std::wstring::npos)
        << "读回的剪贴板应包含刚写入的内容（实际=" << readback << L"）";

    // 空指针不得崩溃（语义等同于写空串）。
    ret = -1;
    EXPECT_NO_FATAL_FAILURE(op.SetClipboard(nullptr, &ret));

    // 还原原始剪贴板内容（空也要写回，避免留下测试痕迹）。
    long restore_ret = 0;
    op.SetClipboard(original.c_str(), &restore_ret);
    EXPECT_EQ(restore_ret, 1) << "还原剪贴板应成功";
}


// ============================================================
// 10. 跨进程内存读写（受控子进程）
// ============================================================
// 门面里的 ReadInt/WriteInt 以 hwnd 定位目标进程。之前只有"读自己进程"一条正例，
// 跨进程路径完全没验证过。这里起一个受控子进程（notepad），
//   1) 读它的模块头 'MZ'（跨进程读必须成功）；
//   2) 往它只读的模块头写（跨进程写必须干净失败，不能崩，也不能真改掉）。
// 全程不碰系统进程，结束时关掉子进程。

namespace {

struct ChildProcess {
    HANDLE process = nullptr;
    DWORD pid = 0;

    bool Start(const wchar_t *command_line) {
        STARTUPINFOW si = {sizeof(si)};
        PROCESS_INFORMATION pi = {};
        std::wstring cmd(command_line);
        if (!::CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
            return false;
        }
        process = pi.hProcess;
        pid = pi.dwProcessId;
        ::CloseHandle(pi.hThread);
        return true;
    }

    ~ChildProcess() {
        if (process) {
            ::TerminateProcess(process, 0);
            ::WaitForSingleObject(process, 3000);
            ::CloseHandle(process);
        }
    }

    // 等子进程出现可见窗口（门面靠 hwnd 定位进程）。
    HWND WaitForWindow(int timeout_ms) {
        const ULONGLONG deadline = ::GetTickCount64() + static_cast<ULONGLONG>(timeout_ms);
        while (::GetTickCount64() < deadline) {
            struct Ctx {
                DWORD pid;
                HWND found;
            } ctx{pid, nullptr};
            ::EnumWindows(
                [](HWND hwnd, LPARAM param) -> BOOL {
                    auto *c = reinterpret_cast<Ctx *>(param);
                    DWORD owner = 0;
                    ::GetWindowThreadProcessId(hwnd, &owner);
                    if (owner == c->pid && ::IsWindowVisible(hwnd)) {
                        c->found = hwnd;
                        return FALSE;
                    }
                    return TRUE;
                },
                reinterpret_cast<LPARAM>(&ctx));
            if (ctx.found) {
                return ctx.found;
            }
            ::Sleep(100);
        }
        return nullptr;
    }

    // 取子进程主模块基址（读 'MZ' 用）。
    void *ModuleBase() const {
        HANDLE query = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (!query) {
            return nullptr;
        }
        HMODULE modules[64] = {};
        DWORD needed = 0;
        void *base = nullptr;
        if (::EnumProcessModules(query, modules, sizeof(modules), &needed) && needed >= sizeof(HMODULE)) {
            base = modules[0];
        }
        ::CloseHandle(query);
        return base;
    }
};

std::wstring HexAddress(const void *ptr) {
    wchar_t buffer[32] = {};
    swprintf_s(buffer, L"%llX", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(ptr)));
    return buffer;
}

} // namespace

TEST(MemoryFacadeTest, CrossProcessReadWorksAndWriteToReadOnlyFailsCleanly) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    ChildProcess child;
    if (!child.Start(L"notepad.exe")) {
        GTEST_SKIP() << "无法启动受控子进程（notepad.exe）";
    }
    HWND child_window = child.WaitForWindow(8000);
    if (!child_window) {
        GTEST_SKIP() << "受控子进程没有出现窗口，跨进程路径无法用 hwnd 定位";
    }
    const void *module_base = child.ModuleBase();
    if (!module_base) {
        GTEST_SKIP() << "拿不到子进程模块基址";
    }

    const LONG_PTR hwnd = reinterpret_cast<LONG_PTR>(child_window);
    const std::wstring address = HexAddress(module_base);

    // 1) 跨进程读：模块头的 'MZ'。
    int64_t value = 0;
    op.ReadInt(hwnd, address.c_str(), 1, &value);
    EXPECT_EQ(value & 0xFFFF, 0x5A4D)
        << "跨进程读模块头失败（地址=" << address << L"，实际=" << value << L"）";

    // 2) 跨进程写：模块头是只读页，必须干净失败。
    ret = -1;
    op.WriteInt(hwnd, address.c_str(), 1, 0x1234, &ret);
    EXPECT_EQ(ret, 0) << "写只读页应失败";

    // 3) 失败之后目标进程仍在（说明没有把它写崩）。
    EXPECT_TRUE(::IsWindow(child_window)) << "跨进程写失败后目标窗口不应消失";

    // 4) 再读一次，确认内容没被改坏。
    int64_t after = 0;
    op.ReadInt(hwnd, address.c_str(), 1, &after);
    EXPECT_EQ(after & 0xFFFF, 0x5A4D) << "跨进程写失败后模块头不应被改动";
}


// 跨进程**可写**内存：上一批只验证了"读得到 + 写只读页会失败"。
// 这里在受控子进程里找一块 PAGE_READWRITE 的私有内存，写入已知值 → 读回验证 → **还原原值**，
// 验证跨进程写路径真的生效，同时不把子进程弄坏。
TEST(MemoryFacadeTest, CrossProcessWriteToWritableRegionTakesEffect) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    ChildProcess child;
    if (!child.Start(L"notepad.exe")) {
        GTEST_SKIP() << "无法启动受控子进程（notepad.exe）";
    }
    HWND child_window = child.WaitForWindow(8000);
    if (!child_window) {
        GTEST_SKIP() << "受控子进程没有出现窗口";
    }
    const LONG_PTR hwnd = reinterpret_cast<LONG_PTR>(child_window);

    // 在子进程里找一块可写私有内存（取区域中部，避开堆头）。
    HANDLE target = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
                                  FALSE, child.pid);
    if (!target) {
        GTEST_SKIP() << "打不开子进程（权限不足）";
    }

    std::uintptr_t writable = 0;
    for (std::uintptr_t address = 0x10000; address < 0x7FFFFFFF0000ULL;) {
        MEMORY_BASIC_INFORMATION info = {};
        if (::VirtualQueryEx(target, reinterpret_cast<LPCVOID>(address), &info, sizeof(info)) != sizeof(info)) {
            break;
        }
        const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
        const std::uintptr_t size = static_cast<std::uintptr_t>(info.RegionSize);
        if (info.State == MEM_COMMIT && info.Type == MEM_PRIVATE && info.Protect == PAGE_READWRITE && size >= 0x10000) {
            writable = base + 0x8000; // 区域中部
            break;
        }
        address = base + (size ? size : 0x1000);
    }
    ::CloseHandle(target);

    if (!writable) {
        GTEST_SKIP() << "子进程里没找到合适的可写区域";
    }

    const std::wstring address = HexAddress(reinterpret_cast<const void *>(writable));

    // 基线值 → 写新值 → 读回 → 还原。
    int64_t baseline = 0;
    op.ReadInt(hwnd, address.c_str(), 1, &baseline);

    const int64_t probe = 0x4F50; // 'OP'
    ret = -1;
    op.WriteInt(hwnd, address.c_str(), 1, probe, &ret);
    ASSERT_EQ(ret, 1) << "跨进程写可写内存应成功（地址=" << address << L"）";

    int64_t readback = 0;
    op.ReadInt(hwnd, address.c_str(), 1, &readback);
    EXPECT_EQ(readback & 0xFFFF, probe & 0xFFFF)
        << "写进去的值应能读回（地址=" << address << L"，读到=" << readback << L"）";

    // 还原，别给子进程留下脏数据。
    op.WriteInt(hwnd, address.c_str(), 1, baseline, &ret);
    int64_t restored = 0;
    op.ReadInt(hwnd, address.c_str(), 1, &restored);
    EXPECT_EQ(restored & 0xFFFF, baseline & 0xFFFF) << "还原原值应成功";
}


// ============================================================
// 11. 输出指针为空时不得崩溃（门面可达）
// ============================================================
// 这些函数的 `_Out_` 参数允许传 nullptr（COM/C API 侧都可能给空指针），
// 门面里用 internal::set_result 做空判断。之前只有 COM 层的异常兜底用例，
// **直接给 C++ 门面传 nullptr** 这条路径没有覆盖。
TEST(OutputPointerTest, NullOutputPointersDoNotCrash) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    // FindColor：坐标指针为空，只取 ret。
    ret = -1;
    EXPECT_NO_FATAL_FAILURE(op.FindColor(0, 0, 32, 32, L"000000", 0.9, 0, nullptr, nullptr, &ret));
    EXPECT_GE(ret, 0) << "FindColor 在坐标指针为空时应正常返回（ret=" << ret << "）";

    // FindColor：ret 指针为空，只取坐标。
    long x = -1, y = -1;
    EXPECT_NO_FATAL_FAILURE(op.FindColor(0, 0, 32, 32, L"000000", 0.9, 0, &x, &y, nullptr));

    // FindColor：三个输出全空。
    EXPECT_NO_FATAL_FAILURE(op.FindColor(0, 0, 32, 32, L"000000", 0.9, 0, nullptr, nullptr, nullptr));

    // GetColorNum：输出为空。
    EXPECT_NO_FATAL_FAILURE(op.GetColorNum(0, 0, 32, 32, L"000000", 0.9, nullptr));

    // FindPic：输出全空（模板名不存在也不应崩溃）。
    EXPECT_NO_FATAL_FAILURE(
        op.FindPic(0, 0, 32, 32, L"no_such_pic_name", L"000000", 0.9, 0, nullptr, nullptr, nullptr));

    // FindMultiColor：输出全空。
    EXPECT_NO_FATAL_FAILURE(
        op.FindMultiColor(0, 0, 32, 32, L"000000", L"0|0|000000", 0.9, 0, nullptr, nullptr, nullptr));

    // GetPicSize：输出全空。
    EXPECT_NO_FATAL_FAILURE(op.GetPicSize(L"no_such_pic_name", nullptr, nullptr, nullptr));

    // GetScreenFrameInfo：输出为空。
    EXPECT_NO_FATAL_FAILURE(op.GetScreenFrameInfo(nullptr, nullptr));
}

// 正例对照：同样的调用给出合法输出指针时，仍应正常工作（证明上面的"不崩"不是因为整条链被短路）。
TEST(OutputPointerTest, ValidOutputPointersStillWork) {
    op::Op op;
    long ret = 0;
    op.SetShowErrorMsg(3, &ret);

    // 找白色（桌面/窗口里通常存在）：拿不到也不算失败，这里只要求"不崩且 ret 合理"。
    long x = -1, y = -1;
    ret = -1;
    op.FindColor(0, 0, 200, 200, L"FFFFFF", 0.9, 0, &x, &y, &ret);
    EXPECT_GE(ret, 0) << "FindColor 正常调用应返回 0/1（ret=" << ret << "）";

    // "未绑定"时 FindPic 不会执行捕获，ret 保持 -1（= 未执行）。
    // 这是有意义的契约：-1=未执行 / 0=未找到 / 1=找到，三者可区分。
    long px = -1, py = -1;
    ret = -12345;
    op.FindPic(0, 0, 64, 64, L"no_such_pic_name", L"000000", 0.9, 0, &px, &py, &ret);
    EXPECT_EQ(ret, -1) << "未绑定时 FindPic 应返回 -1（未执行），实际=" << ret;

    // 绑定真实窗口后：模板名不存在时仍返回 -1 —— 因为"没有可用模板"根本无法执行匹配，
    // 这与"模板可用但画面上没找到(0)"是两件事。三态语义（实测钉住）：
    //   -1 = 未执行 / 无可用模板
    //    0 = 执行了但没找到
    //    1 = 找到
    // 这条语义之前文档里没有，靠本用例固定下来。
    test_support::ColorPulseWindow window;
    ASSERT_TRUE(window.Create(false));
    const auto pump_ms = [](int ms) {
        MSG msg = {};
        const ULONGLONG deadline = ::GetTickCount64() + static_cast<ULONGLONG>(ms);
        while (::GetTickCount64() < deadline) {
            while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                ::TranslateMessage(&msg);
                ::DispatchMessageW(&msg);
            }
            ::Sleep(10);
        }
    };
    pump_ms(150);

    long bind_ret = 0;
    op.BindWindow(reinterpret_cast<LONG_PTR>(window.hwnd), L"normal", L"windows", L"windows", 0, &bind_ret);
    ASSERT_EQ(bind_ret, 1) << "绑定测试窗口应成功";

    px = -1;
    py = -1;
    ret = -12345;
    op.FindPic(0, 0, 64, 64, L"no_such_pic_name", L"000000", 0.9, 0, &px, &py, &ret);
    EXPECT_EQ(ret, -1) << "模板不存在时应返回 -1（无可用模板，无法执行），实际=" << ret;

    // 再用一个**真实存在**的模板验证 0/1 两态：取窗口自身的一小块当模板。
    {
        const std::wstring tpl_path = test_support::GetTempBmpPath(L"op_outptr_template.bmp");
        long cap_ret = 0;
        op.Capture(0, 0, 20, 20, tpl_path.c_str(), &cap_ret);
        ASSERT_EQ(cap_ret, 1) << "先截一小块作为模板";

        long load_ret = 0;
        op.LoadPic(tpl_path.c_str(), &load_ret);
        ASSERT_EQ(load_ret, 1) << "加载模板应成功";

        long hx = -1, hy = -1;
        long found_ret = -12345;
        op.FindPic(0, 0, 64, 64, tpl_path.c_str(), L"000000", 0.9, 0, &hx, &hy, &found_ret);
        EXPECT_GE(found_ret, 0) << "模板可用时应返回 0（未找到）或 1（找到），实际=" << found_ret;
    }

    long unbind_ret = 0;
    op.UnBindWindow(&unbind_ret);
    EXPECT_EQ(unbind_ret, 1);
}
