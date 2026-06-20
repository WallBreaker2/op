#include "WindowsVersion.h"

#include <winternl.h>

namespace {

using RtlGetVersionFn = LONG(WINAPI *)(PRTL_OSVERSIONINFOW);

// 直接调用 ntdll!RtlGetVersion，避免普通 Win32 版本接口受 manifest 影响。
bool QueryWindowsVersion(RTL_OSVERSIONINFOW &info) {
    const HMODULE ntdll = ::GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) {
        return false;
    }

    const auto rtl_get_version =
        reinterpret_cast<RtlGetVersionFn>(::GetProcAddress(ntdll, "RtlGetVersion"));
    if (!rtl_get_version) {
        return false;
    }

    ::ZeroMemory(&info, sizeof(info));
    info.dwOSVersionInfoSize = sizeof(info);
    return rtl_get_version(&info) == 0;
}

} // namespace

bool GetWindowsVersion(WindowsVersion &out) {
    RTL_OSVERSIONINFOW info = {};
    if (!QueryWindowsVersion(info)) {
        return false;
    }

    out.major = info.dwMajorVersion;
    out.minor = info.dwMinorVersion;
    out.build = info.dwBuildNumber;
    return true;
}

DWORD GetWindowsBuildNumber() {
    WindowsVersion version = {};
    return GetWindowsVersion(version) ? version.build : 0;
}

bool IsWindowsVersionAtLeast(DWORD major, DWORD minor, DWORD build) {
    WindowsVersion current = {};
    if (!GetWindowsVersion(current)) {
        return false;
    }

    if (current.major != major) {
        return current.major > major;
    }
    if (current.minor != minor) {
        return current.minor > minor;
    }
    return current.build >= build;
}

bool IsWindows10BuildOrGreater(DWORD build) {
    return IsWindowsVersionAtLeast(10, 0, build);
}
