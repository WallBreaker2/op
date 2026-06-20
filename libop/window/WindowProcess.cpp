#include "WindowService.h"

#include "runtime/WindowsHandle.h"

#include <Tlhelp32.h>
#include <cwchar>
#include <cstring>
#include <memory>
#include <psapi.h>
#include <string>
#include <vector>

#pragma comment(lib, "psapi.lib")

namespace op {

namespace {

template <typename Target, typename Value> void set_out(Target *target, Value value) {
    if (target)
        *target = static_cast<Target>(value);
}

void append_process_id(std::wstring &result, DWORD pid) {
    if (!result.empty())
        result.push_back(L',');
    result += std::to_wstring(pid);
}

std::wstring get_process_module_path(HANDLE process) {
    HMODULE module = nullptr;
    DWORD bytes_needed = 0;
    if (!::EnumProcessModules(process, &module, sizeof(module), &bytes_needed))
        return L"";

    std::vector<wchar_t> buffer(1024, L'\0');
    for (;;) {
        const DWORD copied = ::GetModuleFileNameExW(process, module, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (copied == 0)
            return L"";
        if (copied < buffer.size() - 1)
            return std::wstring(buffer.data(), copied);
        buffer.assign(buffer.size() * 2, L'\0');
    }
}

std::wstring find_process_name(DWORD pid) {
    PROCESSENTRY32 pe32 = {sizeof(PROCESSENTRY32)};
    op::win32::unique_handle process_snapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!process_snapshot)
        return L"";

    if (::Process32First(process_snapshot.get(), &pe32)) {
        do {
            if (pe32.th32ProcessID == pid)
                return pe32.szExeFile;
        } while (::Process32Next(process_snapshot.get(), &pe32));
    }

    return L"";
}

} // namespace

BOOL WindowService::EnumProcessbyName(DWORD dwPID, LPCWSTR ExeName, LONG type) {
    if (enum_process_success_count == 0) {
        npid.clear();
        PROCESSENTRY32 pe32 = {sizeof(PROCESSENTRY32)};
        op::win32::unique_handle process_snapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
        if (!process_snapshot)
            return FALSE;
        if (::Process32First(process_snapshot.get(), &pe32)) {
            do {
                if (type == 1) {
                    if (wcsstr(pe32.szExeFile, ExeName) != NULL) // 模糊匹配
                    {
                        npid.push_back(pe32.th32ProcessID);
                    }
                } else {
                    if (!_wcsicmp(pe32.szExeFile, ExeName)) {
                        npid.push_back(pe32.th32ProcessID);
                    }
                }

            } while (::Process32Next(process_snapshot.get(), &pe32));
        }
        enum_process_success_count = static_cast<int>(npid.size());
        if (enum_process_success_count > 0)
            return TRUE;
    } else {
        for (const DWORD pid : npid) {
            if (dwPID == pid)
                return TRUE;
        }
    }

    return FALSE;
}

bool WindowService::EnumProcess(const wchar_t *name, std::wstring &retstring) {
    retstring.clear();
    retstringlen = 0;
    if (!name || wcslen(name) < 1)
        return false;

    enum_process_success_count = 0;
    npid.clear();
    if (!EnumProcessbyName(0, name))
        return false;

    for (const DWORD pid : npid)
        append_process_id(retstring, pid);

    return true;
}

int WindowService::GetProcessNumber() // 获取CPU个数
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (int)info.dwNumberOfProcessors;
}

// 时间格式转换
__int64 WindowService::FileTimeToInt64(const FILETIME &time) {
    ULARGE_INTEGER tt;
    tt.LowPart = time.dwLowDateTime;
    tt.HighPart = time.dwHighDateTime;
    return (tt.QuadPart);
}

double WindowService::get_cpu_usage(DWORD ProcessID) // 获取指定进程CPU使用率
{
    // cpu数量
    static int processor_count_ = -1;
    // 上一次的时间
    static __int64 last_time_ = 0;
    static __int64 last_system_time_ = 0;

    FILETIME now;
    FILETIME creation_time;
    FILETIME exit_time;
    FILETIME kernel_time;
    FILETIME user_time;
    __int64 system_time;
    __int64 time;
    // 	__int64 system_time_delta;
    // 	__int64 time_delta;

    double cpu = -1;

    if (processor_count_ == -1) {
        processor_count_ = GetProcessNumber();
    }

    GetSystemTimeAsFileTime(&now);

    // HANDLE hProcess =
    // OpenProcess(PROCESS_QUERY_INFORMATION/*PROCESS_ALL_ACCESS*/, false,
    // ProcessID);
    op::win32::unique_handle process(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessID));

    if (!process) {
        return -1;
    }
    if (!GetProcessTimes(process.get(), &creation_time, &exit_time, &kernel_time, &user_time)) {
        return -1;
    }
    system_time = (FileTimeToInt64(kernel_time) + FileTimeToInt64(user_time)) / processor_count_; // CPU使用时间
    time = FileTimeToInt64(now);                                                                  // 现在的时间

    last_system_time_ = system_time;
    last_time_ = time;

    Sleep(1000);

    // hProcess = OpenProcess(PROCESS_QUERY_INFORMATION/*PROCESS_ALL_ACCESS*/,
    // false, ProcessID);

    process.reset(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessID));

    if (!process) {
        return -1;
    }
    if (!GetProcessTimes(process.get(), &creation_time, &exit_time, &kernel_time, &user_time)) {
        return -1;
    }
    GetSystemTimeAsFileTime(&now);
    system_time = (FileTimeToInt64(kernel_time) + FileTimeToInt64(user_time)) / processor_count_; // CPU使用时间
    time = FileTimeToInt64(now);                                                                  // 现在的时间

    cpu = ((double)(system_time - last_system_time_) / (double)(time - last_time_)) * 100;
    return cpu;
}

// 或者指定进程内存使用率
DWORD WindowService::GetMemoryInfo(DWORD ProcessID) {
    PROCESS_MEMORY_COUNTERS pmc;
    DWORD memoryInK = 0;
    op::win32::unique_handle process(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessID));

    if (process && GetProcessMemoryInfo(process.get(), &pmc, sizeof(pmc))) {
        // memoryInK = pmc.WorkingSetSize/1024;		//单位为k
        memoryInK = static_cast<DWORD>(pmc.WorkingSetSize);
    }

    return memoryInK;
}

bool WindowService::GetProcessInfo(LONG pid, std::wstring &retstring) {
    retstring.clear();

    const std::wstring process_name = find_process_name(static_cast<DWORD>(pid));
    if (process_name.empty())
        return false;

    std::wstring process_path;
    GetProcesspath(static_cast<DWORD>(pid), process_path);
    const auto cpu = static_cast<DWORD>(get_cpu_usage(static_cast<DWORD>(pid)));
    const auto meminfo = GetMemoryInfo(static_cast<DWORD>(pid));

    retstring = process_name + L"|" + process_path + L"|" + std::to_wstring(cpu) + L"|" + std::to_wstring(meminfo);
    return true;
}

bool WindowService::GetProcesspath(DWORD ProcessID, std::wstring &process_path) {
    process_path.clear();

    op::win32::unique_handle process(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessID));
    if (!process)
        return false;

    process_path = get_process_module_path(process.get());
    return !process_path.empty();
}

long WindowService::RunApp(const std::wstring &cmd, long mode, DWORD *pid) {
    auto cmdptr = std::make_unique<wchar_t[]>(cmd.length() + 1);
    memcpy(cmdptr.get(), cmd.data(), cmd.length() * sizeof(wchar_t));
    cmdptr.get()[cmd.length()] = 0; // C字符串需要末尾有0
    /*SECURITY_ATTRIBUTES SA;
    SA.bInheritHandle = NULL;
    SA.*/
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    set_out(pid, 0);
    int bret;
    std::wstring curr_dir;
    if (mode == 1) {
        // find
        size_t pos;
        pos = cmd.find(L".exe");
        if (pos != std::wstring::npos && pos != 0) {
            for (int i = static_cast<int>(pos) - 1; i >= 1; --i) {
                if (cmd[i] == L'\\' || cmd[i] == L'/') {
                    pos = i;
                    break;
                }
            }
            if (pos > 0) {
                curr_dir = cmd.substr(0, pos);
            }
        }
        // setlog(curr_dir.c_str());
    }
    bret = ::CreateProcessW(nullptr,      //// 应用程序名称
                            cmdptr.get(), // 命令行字符串
                            NULL,         // 进程的安全属性
                            NULL,         // 进程的安全属性
                            false,        // 是否继承父进程的属性
                            0,            // 进程的安全属性
                            nullptr,      // 进程的安全属性
                            mode == 1 && !curr_dir.empty() ? curr_dir.c_str() : nullptr, // 指向当前目录名的指针
                            &si,                                                         // 传递给新进程的信息
                            &pi                                                          // 新进程返回的信息
    );
    if (bret) {
        set_out(pid, pi.dwProcessId);
        op::win32::unique_handle process(pi.hProcess);
        op::win32::unique_handle thread(pi.hThread);
    }

    return bret;
}

} // namespace op
