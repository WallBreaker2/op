// #include "stdafx.h"
#include "Utils.h"
#include "AutomationModes.h"
#include "Environment.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <shlwapi.h>
#include <sstream>
#include <vector>
// #define USE_BOOST_STACK_TRACE
#ifdef USE_BOOST_STACK_TRACE
#include <boost/stacktrace.hpp>
#endif

std::wstring _s2wstring(const std::string &s) {
    if (s.empty())
        return L"";

    const int length = MultiByteToWideChar(CP_ACP, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (length <= 0)
        return L"";

    std::wstring out(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.data(), static_cast<int>(s.size()), out.data(), length);
    return out;
}

std::string _ws2string(const std::wstring &ws) {
    // std::string strLocale = setlocale(LC_ALL, "");
    // const wchar_t* wchSrc = ws.c_str();
    // size_t nDestSize = wcstombs(NULL, wchSrc, 0) + 1;
    // char *chDest = new char[nDestSize];
    // memset(chDest, 0, nDestSize);
    // wcstombs(chDest, wchSrc, nDestSize);
    // std::string strResult = chDest;
    // delete[]chDest;
    // setlocale(LC_ALL, strLocale.c_str());
    // return strResult;
    if (ws.empty())
        return "";

    const int length = WideCharToMultiByte(CP_ACP, 0, ws.data(), static_cast<int>(ws.size()), nullptr, 0, nullptr, nullptr);
    if (length <= 0)
        return "";

    std::string out(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_ACP, 0, ws.data(), static_cast<int>(ws.size()), out.data(), length, nullptr, nullptr);
    return out;
}

std::string utf8_to_ansi(std::string strUTF8) {
    if (strUTF8.empty())
        return "";

    int wide_length = MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, nullptr, 0);
    if (wide_length <= 0)
        return "";

    std::wstring wide(static_cast<size_t>(wide_length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, wide.data(), wide_length);

    int ansi_length = WideCharToMultiByte(936, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (ansi_length <= 0)
        return "";

    std::string ansi(static_cast<size_t>(ansi_length), '\0');
    WideCharToMultiByte(936, 0, wide.c_str(), -1, ansi.data(), ansi_length, nullptr, nullptr);
    if (!ansi.empty() && ansi.back() == '\0')
        ansi.pop_back();
    return ansi;
}

long Path2GlobalPath(const std::wstring &file, const std::wstring &curr_path, std::wstring &out) {
    if (::PathFileExistsW(file.c_str())) {
        out = file;
        return 1;
    }
    out.clear();
    out = curr_path + L"\\" + file;
    if (::PathFileExistsW(out.c_str())) {
        return 1;
    }
    return 0;
}

// §72.48：见 Utils.h 的说明。复用 Win32 的线程 last error 通道，
// 不新增 API、不改任何函数签名，只保证"OP 失败时确实写过"。
void op::op_set_last_error(int code) {
    ::SetLastError(static_cast<DWORD>(code));
}

long setlog(const wchar_t *format, ...) {
    va_list args;
    va_start(args, format);
    const int length = _vscwprintf(format, args);
    va_end(args);

    if (length < 0)
        return 0;

    std::vector<wchar_t> buffer(static_cast<size_t>(length) + 1, L'\0');
    va_start(args, format);
    vswprintf_s(buffer.data(), buffer.size(), format, args);
    va_end(args);

    std::wstring tmpw(buffer.data(), static_cast<size_t>(length));
    std::string tmps = _ws2string(tmpw);

    return setlog(tmps.data());
}

// §72.51：注入到别人进程里时，__op.log 原来用相对路径，落在**目标进程的当前目录**。
// 游戏目录通常不可写（日志就静默丢了），可写的时候又把我们的日志污染进人家的目录。
// 这里按 Temp → Windows\Temp → 当前目录 的顺序挑一个真能用的。
const std::wstring &op_log_file_path() {
    static const std::wstring path = [] {
        wchar_t dir[MAX_PATH] = {0};
        const DWORD len = ::GetTempPathW(MAX_PATH, dir);
        if (len > 0 && len < MAX_PATH) {
            const DWORD attr = ::GetFileAttributesW(dir);
            if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
                std::wstring full(dir);
                if (!full.empty() && full.back() != L'\\')
                    full += L'\\';
                full += L"__op.log";
                return full;
            }
        }
        wchar_t win[MAX_PATH] = {0};
        const UINT win_len = ::GetWindowsDirectoryW(win, MAX_PATH);
        if (win_len > 0 && win_len < MAX_PATH) {
            std::wstring full(win);
            full += L"\\Temp\\__op.log";
            return full;
        }
        // 连系统目录都拿不到（几乎不会发生）：退回相对路径，别把日志彻底关掉。
        return std::wstring(L"__op.log");
    }();
    return path;
}

long setlog(const char *format, ...) {
    std::stringstream ss(std::wstringstream::in | std::wstringstream::out);
    va_list args;
    SYSTEMTIME sys;
    GetLocalTime(&sys);
    char tm[128];
    std::snprintf(tm, sizeof(tm), "[%4d/%02d/%02d %02d:%02d:%02d.%03d]", sys.wYear, sys.wMonth, sys.wDay, sys.wHour,
                  sys.wMinute, sys.wSecond, sys.wMilliseconds);
    va_start(args, format);
    const int length = _vscprintf(format, args);
    va_end(args);

    if (length < 0)
        return 0;

    std::vector<char> buffer(static_cast<size_t>(length) + 1, '\0');
    va_start(args, format);
    vsprintf_s(buffer.data(), buffer.size(), format, args);
    va_end(args);

    ss << tm << (OP64 == 1 ? "x64" : "x32") << "info: " << buffer.data() << std::endl;
#ifdef USE_BOOST_STACK_TRACE
    ss << "<stack>\n" << boost::stacktrace::stacktrace() << std::endl;
#endif // USE_BOOST_STACK_TRACE

    std::string s = ss.str();
    if (RuntimeEnvironment::m_showErrorMsg == 1) {
        MessageBoxA(NULL, s.data(), "error", MB_ICONERROR);
    } else if (RuntimeEnvironment::m_showErrorMsg == 2) {
        std::fstream file;
        file.open(op_log_file_path().c_str(), std::ios::app | std::ios::out);
        if (!file.is_open())
            return 0;
        file << s << std::endl;
        file.close();
    } else if (RuntimeEnvironment::m_showErrorMsg == 3) {
        std::cout << s << std::endl;
    }

    return 1;
}

void split(const std::wstring &s, std::vector<std::wstring> &v, const std::wstring &c) {
    std::wstring::size_type pos1, pos2;
    size_t len = s.length();
    pos2 = s.find(c);
    pos1 = 0;
    v.clear();
    while (std::wstring::npos != pos2) {
        v.emplace_back(s.substr(pos1, pos2 - pos1));

        pos1 = pos2 + c.size();
        pos2 = s.find(c, pos1);
    }
    if (pos1 != len)
        v.emplace_back(s.substr(pos1));
}

void split(const std::string &s, std::vector<std::string> &v, const std::string &c) {
    std::string::size_type pos1, pos2;
    size_t len = s.length();
    pos2 = s.find(c);
    pos1 = 0;
    v.clear();
    while (std::string::npos != pos2) {
        v.emplace_back(s.substr(pos1, pos2 - pos1));

        pos1 = pos2 + c.size();
        pos2 = s.find(c, pos1);
    }
    if (pos1 != len)
        v.emplace_back(s.substr(pos1));
}

void wstring2lower(std::wstring &s) {
    std::transform(s.begin(), s.end(), s.begin(), towlower);
}

void replacew(std::wstring &str, const std::wstring &oldval, const std::wstring &newval) {
    size_t x0 = 0, dx = newval.length() - oldval.length() + 1;
    size_t idx = str.find(oldval, x0);
    while (idx != -1 && x0 >= 0) {
        str.replace(idx, oldval.length(), newval);
        x0 = idx + dx;
        idx = str.find(oldval, x0);
    }
}

namespace op {

std::ostream &operator<<(std::ostream &o, point_t const &rhs) {
    o << rhs.x << "," << rhs.y;
    return o;
}

std::wostream &operator<<(std::wostream &o, point_t const &rhs) {
    o << rhs.x << L"," << rhs.y;
    return o;
}

} // namespace op

// Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetLastErrorAsString() {
    // Get the error message ID, if any.
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0) {
        return std::string(); // No error message has been recorded
    }

    LPSTR messageBuffer = nullptr;

    // Ask Win32 to give us the string version of that message ID.
    // The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet
    // know how long the message string will be).
    size_t size =
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    // Copy the error message into a std::string.
    std::string message(messageBuffer, size);

    // Free the Win32's string's buffer.
    LocalFree(messageBuffer);

    return message;
}

bool Delay(long mis) {
    if (mis <= 0) {
        return true;
    }

    MSG msg = {};
    const ULONGLONG deadline = ::GetTickCount64() + static_cast<ULONGLONG>(mis);
    for (;;) {
        const ULONGLONG now = ::GetTickCount64();
        if (now >= deadline) {
            break;
        }

        // 除收到'WM_QUIT'消息，结果始终都是大于0的
        if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        // 原来这里没有等待，纯忙等会把一整核 CPU 烧满（输入延时默认 30ms/次，
        // 连续输入时很可观）。没有消息时阻塞到"有消息"或"到点"，
        // 语义不变（仍然在此期间派发窗口消息），但不再占用 CPU。
        const DWORD remaining = static_cast<DWORD>(deadline - now);
        ::MsgWaitForMultipleObjectsEx(0, nullptr, remaining, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    }
    return true;
}

bool Delays(long mis_min, long mis_max) {
    if (mis_min <= 0 || mis_max <= 0)
        return false;
    long mis = mis_min + rand() % mis_max;
    return Delay(mis);
}
