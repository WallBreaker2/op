// 64 位注入助手。
//
// 为什么需要它：32 位宿主（op_x86.dll）不能直接往原生 64 位目标里注入 op_x64.dll。
// BlackBone 的远程调用按 `barrier().type == wow_32_64` 选汇编器，而那个条件只在
// "64 位宿主 + WOW64 目标"时成立；32 位宿主 attach 原生 64 位目标时会选到错位宽的
// stub，表现是"注入报成功、远端一行代码都没执行、偶发挂上百秒"
// （证据链见 doc/defect_fixes.md §72.13）。
//
// 所以跨位宽改由这个 64 位进程代劳：它自己是 64 位，注入与远程调用都是同构的，
// 不依赖任何跨位宽 RPC。宿主（32 位）只需 CreateProcess 它、读退出码。
//
// 用法（宿主自己拼命令行，路径都传绝对路径）：
//   op_inject64.exe --pid=<pid> --dll="<abs op_x64.dll>" --hwnd=<0x...>
//                   --render-type=<int> [--timeout-ms=5000]
//   op_inject64.exe --pid=<pid> --dll="<abs op_x64.dll>" --release
//
// 退出码：0 = 调用返回 1（成功）；2 = 调用返回 0（目标侧拒绝/失败）；
//         3 = 注入失败；4 = 参数错误；5 = 超时（远端线程已强制结束）。
// stdout 每行 `HELPER_*`，供宿主日志转贴。

#if !defined(_WIN64)
#error "op_inject64 must be built for x64 only"
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

const wchar_t *kKernel32 = L"kernel32.dll";
constexpr int kDefaultTimeoutMs = 5000;

enum ExitCode {
    kExitOk = 0,
    kExitCallFailed = 2,
    kExitInjectFailed = 3,
    kExitBadArgs = 4,
    kExitTimeout = 5,
};

struct Options {
    DWORD pid = 0;
    std::wstring dll;
    HWND hwnd = nullptr;
    int render_type = 0;
    bool release = false;
    int timeout_ms = kDefaultTimeoutMs;
    std::string export_name; // 覆盖默认导出，排查用（--export=SetInputLock）
    std::wstring module;     // 排查用：直接调某个已加载模块的导出（--module=<system dll>），不注入
    bool valid = false;
};

void print(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stdout, fmt, ap);
    va_end(ap);
    std::fputc('\n', stdout);
    std::fflush(stdout);
}

bool parse_wide(const std::wstring &text, unsigned long long *out) {
    if (!out || text.empty())
        return false;
    wchar_t *end = nullptr;
    const unsigned long long value = std::wcstoull(text.c_str(), &end, 0);
    if (!end || *end != L'\0')
        return false;
    *out = value;
    return true;
}

Options parse_args(int argc, wchar_t **argv) {
    Options opt;
    bool saw_pid = false;
    bool saw_dll = false;
    for (int i = 1; i < argc; ++i) {
        const std::wstring arg = argv[i];
        const std::wstring prefix_pid = L"--pid=";
        const std::wstring prefix_dll = L"--dll=";
        const std::wstring prefix_hwnd = L"--hwnd=";
        const std::wstring prefix_rt = L"--render-type=";
        const std::wstring prefix_timeout = L"--timeout-ms=";
        const std::wstring prefix_export = L"--export=";
        const std::wstring prefix_module = L"--module=";
        unsigned long long value = 0;
        if (arg == L"--release") {
            opt.release = true;
        } else if (arg.rfind(prefix_module, 0) == 0) {
            opt.module = arg.substr(prefix_module.size());
        } else if (arg.rfind(prefix_export, 0) == 0) {
            const std::wstring name = arg.substr(prefix_export.size());
            // 导出名只有 ASCII：逐字符窄化，别用 assign(wchar_t 迭代器) —— 那会触发 C4244。
            opt.export_name.clear();
            for (wchar_t ch : name)
                opt.export_name.push_back(ch >= 0 && ch < 0x80 ? static_cast<char>(ch) : '?');
        } else if (arg.rfind(prefix_pid, 0) == 0 && parse_wide(arg.substr(prefix_pid.size()), &value)) {
            opt.pid = (DWORD)value;
            saw_pid = true;
        } else if (arg.rfind(prefix_dll, 0) == 0) {
            opt.dll = arg.substr(prefix_dll.size());
            saw_dll = !opt.dll.empty();
        } else if (arg.rfind(prefix_hwnd, 0) == 0 && parse_wide(arg.substr(prefix_hwnd.size()), &value)) {
            opt.hwnd = (HWND)(ULONG_PTR)value;
        } else if (arg.rfind(prefix_rt, 0) == 0 && parse_wide(arg.substr(prefix_rt.size()), &value)) {
            opt.render_type = (int)value;
        } else if (arg.rfind(prefix_timeout, 0) == 0 && parse_wide(arg.substr(prefix_timeout.size()), &value)) {
            if (value > 0 && value <= 60000)
                opt.timeout_ms = (int)value;
        } else {
            print("HELPER_ARG_UNKNOWN %ls", arg.c_str());
        }
    }
    opt.valid = saw_pid && saw_dll && opt.pid != 0;
    if (!opt.valid)
        print("HELPER_ARG_ERROR need --pid=<pid> and --dll=<abs path>");
    return opt;
}

std::wstring base_name(const std::wstring &path) {
    const size_t slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? path : path.substr(slash + 1);
}

// 在目标进程的模块表里找同名模块，返回基址；找不到返回 0。
// 注意用 ULONG_PTR 而不是 DWORD：模块基址是 64 位，截断成 32 位会让远程调用跳到
// 一个非法地址（表现为目标进程崩或线程挂死，极难看出是截断）。
// Toolhelp 枚举目标模块表。这里**必须重试**：`CreateToolhelp32Snapshot(TH32CS_SNAPMODULE)`
// 在目标进程模块表正在变化时会失败（典型是 `ERROR_BAD_LENGTH`），而 `Module32NextW` 走到一半
// 也可能因为同样的原因提前结束、把后面的模块漏掉。机器有负载时这个概率明显上升 ——
// 曾观察到 x86 宿主注入 64 位目标时偶发 "op_x64.dll not found in target module list
// after LoadLibrary"（LoadLibrary 其实成功了，是枚举没看见），见 defect_fixes.md §72.32。
bool toolhelp_find_module(DWORD pid, const std::wstring &module_name, ULONG_PTR *out_base) {
    // 已知的 Toolhelp 抖动窗口：连续几次 10 ms 的重试足够跨过模块表变化。
    constexpr int kSnapshotAttempts = 20;
    constexpr DWORD kRetrySleepMs = 10;
    for (int attempt = 0; attempt < kSnapshotAttempts; ++attempt) {
        HANDLE snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snap == INVALID_HANDLE_VALUE) {
            ::Sleep(kRetrySleepMs);
            continue;
        }
        MODULEENTRY32W entry = {};
        entry.dwSize = sizeof(entry);
        bool complete = false;
        if (::Module32FirstW(snap, &entry)) {
            do {
                if (_wcsicmp(entry.szModule, module_name.c_str()) == 0) {
                    *out_base = (ULONG_PTR)entry.modBaseAddr;
                    ::CloseHandle(snap);
                    return true;
                }
            } while (::Module32NextW(snap, &entry));
            // 枚举自然走完才算"确实没有这个模块"；中途被打断（ERROR_BAD_LENGTH）就重来。
            complete = ::GetLastError() == ERROR_NO_MORE_FILES;
        }
        ::CloseHandle(snap);
        if (complete)
            return false;
        ::Sleep(kRetrySleepMs);
    }
    return false;
}

// Toolhelp 实在不行时的第二来源：`K32EnumProcessModulesEx(LIST_MODULES_ALL)`。
// 走的是 PEB 的模块链表，和 Toolhelp 的快照机制不同，两者一起用基本不会同时瞎。
bool psapi_find_module(DWORD pid, const std::wstring &module_name, ULONG_PTR *out_base) {
    using EnumModulesExFn = BOOL(WINAPI *)(HANDLE, HMODULE *, DWORD, LPDWORD, DWORD);
    const HMODULE k32 = ::GetModuleHandleW(L"kernel32.dll");
    if (!k32)
        return false;
    auto enum_modules = reinterpret_cast<EnumModulesExFn>(::GetProcAddress(k32, "K32EnumProcessModulesEx"));
    if (!enum_modules)
        return false;

    HANDLE process = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!process)
        return false;

    bool found = false;
    HMODULE modules[1024] = {};
    DWORD needed = 0;
    if (enum_modules(process, modules, sizeof(modules), &needed, 0x03 /* LIST_MODULES_ALL */)) {
        const size_t count = needed / sizeof(HMODULE);
        wchar_t path[MAX_PATH] = {};
        for (size_t i = 0; i < count && i < _countof(modules); ++i) {
            if (!::GetModuleFileNameExW(process, modules[i], path, _countof(path)))
                continue;
            const std::wstring full(path);
            const size_t slash = full.find_last_of(L"\\");
            const std::wstring base = slash == std::wstring::npos ? full : full.substr(slash + 1);
            if (_wcsicmp(base.c_str(), module_name.c_str()) == 0) {
                *out_base = reinterpret_cast<ULONG_PTR>(modules[i]);
                found = true;
                break;
            }
        }
    }
    ::CloseHandle(process);
    return found;
}

ULONG_PTR find_remote_module(DWORD pid, const std::wstring &module_name) {
    ULONG_PTR base = 0;
    if (toolhelp_find_module(pid, module_name, &base))
        return base;
    if (psapi_find_module(pid, module_name, &base))
        return base;
    return 0;
}

// 目标进程里 LoadLibraryW 的地址：用"本进程 kernel32 基址 + 导出 RVA"推算。
// 同一台机器上同一份 kernel32，RVA 一致，所以这个推算是可靠的（不依赖模块基址相同）。
FARPROC remote_load_library(DWORD pid) {
    HMODULE local_kernel32 = ::GetModuleHandleW(kKernel32);
    if (!local_kernel32)
        return nullptr;
    const FARPROC local_load = ::GetProcAddress(local_kernel32, "LoadLibraryW");
    if (!local_load)
        return nullptr;
    const ULONG_PTR remote_kernel32 = find_remote_module(pid, kKernel32);
    if (!remote_kernel32)
        return nullptr;
    const ULONG_PTR rva = (ULONG_PTR)local_load - (ULONG_PTR)local_kernel32;
    return (FARPROC)(remote_kernel32 + rva);
}

// 在目标进程里跑一段 17 字节的 stub：
//   48 B8 <imm64 fn>   mov rax, fn
//   BA <imm32 arg2>    mov edx, arg2
//   FF E0              jmp rax
// CreateRemoteThread 把 arg1 放进 RCX，于是远端函数看到 (arg1, arg2)，
// 返回值经 BaseThreadInitThunk 变成线程退出码。
bool remote_call(HANDLE process, FARPROC fn, ULONG_PTR arg1, DWORD arg2, int timeout_ms,
                 DWORD *exit_code, bool *timed_out) {
    *exit_code = 0;
    *timed_out = false;

    BYTE stub[17] = {0x48, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0, 0xBA, 0, 0, 0, 0, 0xFF, 0xE0};
    const ULONG_PTR fn_addr = (ULONG_PTR)fn;
    std::memcpy(stub + 2, &fn_addr, sizeof(fn_addr));
    std::memcpy(stub + 11, &arg2, sizeof(arg2));

    void *remote_stub = ::VirtualAllocEx(process, nullptr, sizeof(stub), MEM_COMMIT | MEM_RESERVE,
                                        PAGE_EXECUTE_READWRITE);
    if (!remote_stub) {
        print("HELPER_FAIL VirtualAllocEx stub err=%lu", ::GetLastError());
        return false;
    }
    if (!::WriteProcessMemory(process, remote_stub, stub, sizeof(stub), nullptr)) {
        print("HELPER_FAIL WriteProcessMemory stub err=%lu", ::GetLastError());
        ::VirtualFreeEx(process, remote_stub, 0, MEM_RELEASE);
        return false;
    }

    HANDLE thread = ::CreateRemoteThread(process, nullptr, 0,
                                         (LPTHREAD_START_ROUTINE)remote_stub, (LPVOID)arg1, 0, nullptr);
    if (!thread) {
        print("HELPER_FAIL CreateRemoteThread err=%lu", ::GetLastError());
        ::VirtualFreeEx(process, remote_stub, 0, MEM_RELEASE);
        return false;
    }

    const DWORD wait = ::WaitForSingleObject(thread, (DWORD)timeout_ms);
    bool ok = true;
    if (wait == WAIT_OBJECT_0) {
        if (!::GetExitCodeThread(thread, exit_code)) {
            print("HELPER_FAIL GetExitCodeThread err=%lu", ::GetLastError());
            ok = false;
        }
    } else if (wait == WAIT_TIMEOUT) {
        *timed_out = true;
        print("HELPER_TIMEOUT remote call did not return in %d ms, terminating thread", timeout_ms);
        ::TerminateThread(thread, 0);
        ok = false;
    } else {
        print("HELPER_FAIL WaitForSingleObject ret=%lu err=%lu", wait, ::GetLastError());
        ok = false;
    }

    ::CloseHandle(thread);
    ::VirtualFreeEx(process, remote_stub, 0, MEM_RELEASE);
    return ok;
}

// 从磁盘上的 DLL 里取导出 RVA：本地按"不解析引用"方式映射，拿到基址后减掉。
// 这样不会执行 op_x64.dll 的 DllMain，也不会把它的依赖拉进本进程。
bool export_rva(const std::wstring &dll, const char *name, ULONG_PTR *rva) {
    HMODULE local = ::LoadLibraryExW(dll.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES);
    if (!local) {
        print("HELPER_FAIL LoadLibraryEx(DONT_RESOLVE) err=%lu", ::GetLastError());
        return false;
    }
    FARPROC proc = ::GetProcAddress(local, name);
    if (!proc) {
        print("HELPER_FAIL GetProcAddress(%s) err=%lu", name, ::GetLastError());
        ::FreeLibrary(local);
        return false;
    }
    *rva = (ULONG_PTR)proc - (ULONG_PTR)local;
    ::FreeLibrary(local);
    return true;
}

bool inject_dll(HANDLE process, DWORD pid, const std::wstring &dll, ULONG_PTR *out_base) {
    const std::wstring name = base_name(dll);
    const ULONG_PTR existing = find_remote_module(pid, name);
    if (existing) {
        print("HELPER_DLL_ALREADY_LOADED %ls base=0x%llX", name.c_str(), (unsigned long long)existing);
        *out_base = existing;
        return true;
    }

    FARPROC remote_load = remote_load_library(pid);
    if (!remote_load) {
        print("HELPER_FAIL cannot resolve LoadLibraryW in target pid=%lu", pid);
        return false;
    }

    const size_t bytes = (dll.size() + 1) * sizeof(wchar_t);
    void *remote_path = ::VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote_path) {
        print("HELPER_FAIL VirtualAllocEx path err=%lu", ::GetLastError());
        return false;
    }
    if (!::WriteProcessMemory(process, remote_path, dll.c_str(), bytes, nullptr)) {
        print("HELPER_FAIL WriteProcessMemory path err=%lu", ::GetLastError());
        ::VirtualFreeEx(process, remote_path, 0, MEM_RELEASE);
        return false;
    }

    DWORD load_ret = 0;
    bool timed_out = false;
    const bool called = remote_call(process, remote_load, (ULONG_PTR)remote_path, 0, kDefaultTimeoutMs,
                                    &load_ret, &timed_out);
    ::VirtualFreeEx(process, remote_path, 0, MEM_RELEASE);
    if (!called) {
        print("HELPER_FAIL LoadLibraryW remote call failed (timed_out=%d)", (int)timed_out);
        return false;
    }
    if (load_ret == 0) {
        print("HELPER_FAIL LoadLibraryW returned NULL in target (check bitness/path)");
        return false;
    }
    // 线程退出码只有 32 位，拿不到真实基址 —— 重新快照模块表取 64 位基址。
    const ULONG_PTR module_base = find_remote_module(pid, name);
    if (module_base == 0) {
        print("HELPER_FAIL %ls not found in target module list after LoadLibrary", name.c_str());
        return false;
    }
    print("HELPER_INJECT_OK %ls base=0x%llX", name.c_str(), (unsigned long long)module_base);
    *out_base = module_base;
    return true;
}

} // namespace

int wmain(int argc, wchar_t **argv) {
    const Options opt = parse_args(argc, argv);
    if (!opt.valid)
        return kExitBadArgs;

    const char *export_name =
        !opt.export_name.empty() ? opt.export_name.c_str() : (opt.release ? "ReleaseDisplayHook" : "SetDisplayHook");

    HANDLE process = ::OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
                                       PROCESS_VM_WRITE | PROCESS_VM_READ,
                                   FALSE, opt.pid);
    if (!process) {
        print("HELPER_FAIL OpenProcess pid=%lu err=%lu", opt.pid, ::GetLastError());
        return kExitInjectFailed;
    }

    int exit_code = kExitInjectFailed;
    ULONG_PTR module_base = 0;
    const std::wstring dll_for_rva = opt.module.empty() ? opt.dll : opt.module;
    do {
        if (!opt.module.empty()) {
            // 排查模式：不注入，直接调目标进程里已加载模块的导出。
            module_base = find_remote_module(opt.pid, base_name(opt.module));
            if (module_base == 0) {
                print("HELPER_FAIL module %ls not loaded in target", base_name(opt.module).c_str());
                break;
            }
        } else if (opt.release) {
            // 解绑：DLL 没加载就没什么可释放的，直接当成功。
            module_base = find_remote_module(opt.pid, base_name(opt.dll));
            if (module_base == 0) {
                print("HELPER_RELEASE_SKIPPED %ls not loaded in target", base_name(opt.dll).c_str());
                exit_code = kExitOk;
                break;
            }
        } else if (!inject_dll(process, opt.pid, opt.dll, &module_base)) {
            break;
        }

        ULONG_PTR rva = 0;
        if (!export_rva(dll_for_rva, export_name, &rva))
            break;
        const FARPROC remote_fn = (FARPROC)(module_base + rva);
        print("HELPER_CALL %s at 0x%llX (base=0x%llX rva=0x%llX)", export_name,
              (unsigned long long)(ULONG_PTR)remote_fn, (unsigned long long)module_base,
              (unsigned long long)rva);

        DWORD call_ret = 0;
        bool timed_out = false;
        if (!remote_call(process, remote_fn, (ULONG_PTR)opt.hwnd, (DWORD)opt.render_type, opt.timeout_ms,
                         &call_ret, &timed_out)) {
            exit_code = timed_out ? kExitTimeout : kExitInjectFailed;
            break;
        }
        print("HELPER_CALL_RET %s ret=%lu", export_name, call_ret);
        exit_code = (call_ret == 1) ? kExitOk : kExitCallFailed;
    } while (false);

    ::CloseHandle(process);
    print("HELPER_DONE exit=%d", exit_code);
    return exit_code;
}
