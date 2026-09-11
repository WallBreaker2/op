// #include "stdafx.h"
#include "DllInjector.h"

#include "base/WindowsHandle.h"

#include <cwchar>
#include <psapi.h>
#include <vector>

#pragma comment(lib, "psapi.lib")

namespace op {

namespace {

const wchar_t *file_name_of(const wchar_t *path) {
    const wchar_t *slash = std::wcsrchr(path, L'\\');
    return slash ? slash + 1 : path;
}

// 等待远端线程的上限。超过即认定注入未成功，避免调用线程被永久挂住。
constexpr DWORD kRemoteThreadTimeoutMs = 15000;

// 判断目标进程里是否已加载指定 DLL，用于确认远端 LoadLibraryW 的真实结果。
// 不能只看 GetExitCodeThread：它只返回 DWORD，而 LoadLibraryW 返回的是 64 位模块句柄，
// x64 下低位可能恰好为 0，只看退出码会把成功误判成失败。
// 返回 false 代表"枚举失败"或"确实没加载"，调用方按保守策略处理。
bool is_module_loaded(HANDLE process, const wchar_t *dll_path) {
    std::vector<HMODULE> modules(512);
    DWORD needed = 0;
    if (!::EnumProcessModules(process, modules.data(), static_cast<DWORD>(modules.size() * sizeof(HMODULE)), &needed))
        return false;
    if (needed > modules.size() * sizeof(HMODULE))
        modules.resize(needed / sizeof(HMODULE));
    if (!::EnumProcessModules(process, modules.data(), static_cast<DWORD>(modules.size() * sizeof(HMODULE)), &needed))
        return false;

    // 按文件名比对即可满足需求：只需要区分"完全没加载"与"加载了"。
    // 判断刻意偏保守（宁可认定成功），避免把一次成功的注入误报成失败。
    const wchar_t *const want_name = file_name_of(dll_path);
    const size_t count = needed / sizeof(HMODULE);
    std::vector<wchar_t> buffer(1024, L'\0');
    for (size_t i = 0; i < count && i < modules.size(); ++i) {
        if (::GetModuleFileNameExW(process, modules[i], buffer.data(), static_cast<DWORD>(buffer.size())) == 0)
            continue;
        if (_wcsicmp(file_name_of(buffer.data()), want_name) == 0)
            return true;
    }
    return false;
}

class remote_process_memory {
  public:
    remote_process_memory(HANDLE process, void *address) noexcept : process_(process), address_(address) {
    }

    ~remote_process_memory() {
        reset();
    }

    remote_process_memory(const remote_process_memory &) = delete;
    remote_process_memory &operator=(const remote_process_memory &) = delete;

    void *get() const noexcept {
        return address_;
    }

    void reset() noexcept {
        if (process_ && address_) {
            // MEM_RELEASE 要求 dwSize 为 0；原来的 MEM_DECOMMIT 只解提交、不归还保留区。
            ::VirtualFreeEx(process_, address_, 0, MEM_RELEASE);
            address_ = nullptr;
        }
    }

    // 放弃所有权且不释放。远端线程可能仍在读取这块内存（超时场景），
    // 宁可漏一小块地址空间，也不能释放后制造悬垂指针打崩目标进程。
    void detach() noexcept {
        address_ = nullptr;
    }

  private:
    HANDLE process_ = nullptr;
    void *address_ = nullptr;
};

} // namespace

DllInjector::DllInjector() {
}

DllInjector::~DllInjector() {
}

BOOL DllInjector::EnablePrivilege(BOOL enable) {
    // 得到令牌句柄
    HANDLE token_handle = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_READ, &token_handle))
        return FALSE;
    op::win32::unique_handle token(token_handle);

    // 得到特权值
    LUID luid;
    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
        return FALSE;

    // 提升令牌句柄权限
    TOKEN_PRIVILEGES tp = {};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;
    if (!AdjustTokenPrivileges(token.get(), FALSE, &tp, sizeof(tp), NULL, NULL))
        return FALSE;

    return TRUE;
}

long DllInjector::InjectDll(DWORD pid, LPCTSTR dllPath, long &error_code) {

    op::win32::unique_handle process(::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid));
    if (!process) {
        error_code = ::GetLastError();
        return -1;
    }
    DWORD dllPathSize = ((DWORD)wcslen(dllPath) + 1) * sizeof(TCHAR);

    // 申请内存用来存放DLL路径。只是字符串数据，不需要可执行权限。
    remote_process_memory remoteMemory(
        process.get(), VirtualAllocEx(process.get(), NULL, dllPathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (remoteMemory.get() == NULL) {
        // setlog(L"申请内存失败，错误代码：%u\n", GetLastError());
        error_code = ::GetLastError();
        return -2;
    }

    // 写入DLL路径
    if (!WriteProcessMemory(process.get(), remoteMemory.get(), dllPath, dllPathSize, NULL)) {
        // setlog(L"写入内存失败，错误代码：%u\n", GetLastError());
        error_code = ::GetLastError();
        return -3;
    }

    // 创建远线程调用LoadLibrary
    auto lpfn = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
    if (!lpfn) {
        error_code = ::GetLastError();
        return -4;
    }
    op::win32::unique_handle remoteThread(
        CreateRemoteThread(process.get(), NULL, 0, (LPTHREAD_START_ROUTINE)lpfn, remoteMemory.get(), 0, NULL));
    if (!remoteThread) {
        // setlog(L"创建远线程失败，错误代码：%u\n", GetLastError());
        error_code = ::GetLastError();
        return -5;
    }
    // 等待远线程结束
    const DWORD wait_result = ::WaitForSingleObject(remoteThread.get(), kRemoteThreadTimeoutMs);
    if (wait_result == WAIT_TIMEOUT) {
        // 远端线程可能仍在读这块内存，交还所有权（故意不释放）以避免打崩目标进程。
        remoteMemory.detach();
        error_code = ERROR_TIMEOUT;
        return -6;
    }
    if (wait_result != WAIT_OBJECT_0) {
        error_code = ::GetLastError();
        return -7;
    }

    // 取DLL在目标进程的句柄
    DWORD remoteModule = 0;
    if (!::GetExitCodeThread(remoteThread.get(), &remoteModule)) {
        error_code = ::GetLastError();
        return -8;
    }
    // 退出码为 0 时再用模块列表确认一次，避免把加载失败当成功上报。
    if (remoteModule == 0 && !is_module_loaded(process.get(), dllPath)) {
        error_code = ERROR_MOD_NOT_FOUND;
        return -9;
    }

    error_code = 0;
    return 1;
}

} // namespace op
