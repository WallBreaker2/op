#pragma once

#include <windows.h>

// 这里只关心系统主版本号、次版本号和 build。
struct WindowsVersion {
    DWORD major = 0;
    DWORD minor = 0;
    DWORD build = 0;
};

// 当前WGC关心的几个系统 build 门槛。
constexpr DWORD kWindows10Build1903 = 18362;
constexpr DWORD kWindows10Build2004 = 19041;
constexpr DWORD kWindowsServer2022Build = 20348;

// 从 ntdll 读取真实系统版本。
// 只有查询失败时才返回 false。
bool GetWindowsVersion(WindowsVersion &out);

// 只返回当前系统 build。
// 查询失败时返回 0。
DWORD GetWindowsBuildNumber();

// 判断当前系统版本是否大于等于目标版本。
bool IsWindowsVersionAtLeast(DWORD major, DWORD minor, DWORD build);

// 便捷函数：判断当前系统是否至少为某个 Windows 10/11 build。
bool IsWindows10BuildOrGreater(DWORD build);
