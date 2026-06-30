#include "InputHookClient.h"
#include "../runtime/AutomationModes.h"
#include "../runtime/RuntimeUtils.h"
#include "../runtime/RuntimeEnvironment.h"
#include "HookModule.h"
#include "BlackBone/Process/Process.h"
#include "BlackBone/Process/RPC/RemoteFunction.hpp"
#include <mutex>
#include <unordered_map>

namespace {

std::mutex g_mutex;
std::unordered_map<HWND, long> g_bind_refs;

std::wstring resolve_hook_dll(blackbone::Process &proc) {
    const BOOL target_is64 = proc.modules().GetMainModule()->type == blackbone::eModType::mt_mod64;
    return op::hook::ResolveHookModuleName(target_is64 != FALSE);
}

long call_set_input_hook(HWND hwnd, int mode) {
    DWORD pid = 0;
    ::GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0)
        return 0;

    blackbone::Process proc;
    const NTSTATUS status = proc.Attach(pid);
    if (!NT_SUCCESS(status)) {
        setlog(L"input hook attach failed. pid=%d hwnd=%p status=0x%X", pid, hwnd, status);
        return 0;
    }

    long ret = 0;
    const std::wstring dll_name = resolve_hook_dll(proc);
    bool injected = proc.modules().GetModule(dll_name) != nullptr;
    if (!injected) {
        const std::wstring dll_path = RuntimeEnvironment::getBasePath() + L"\\" + dll_name;
        if (::PathFileExistsW(dll_path.c_str())) {
            auto inject_ret = proc.modules().Inject(dll_path);
            injected = inject_ret ? true : false;
            if (!injected) {
                setlog(L"input hook inject failed. pid=%d hwnd=%p status=0x%X dll=%s", pid, hwnd, inject_ret.status,
                       dll_path.c_str());
            }
        } else {
            setlog(L"input hook dll not exists: %s", dll_path.c_str());
        }
    }

    if (injected) {
        using set_input_hook_t = long(__stdcall *)(HWND, int);
        auto remote = blackbone::MakeRemoteFunction<set_input_hook_t>(proc, dll_name, "SetInputHook");
        if (remote) {
            auto call_ret = remote(hwnd, mode);
            ret = call_ret.result();
        } else {
            setlog(L"remote function 'SetInputHook' not found in %s.", dll_name.c_str());
        }
    }

    proc.Detach();
    return ret;
}

long call_release_input_hook(HWND hwnd) {
    DWORD pid = 0;
    ::GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0)
        return 0;

    blackbone::Process proc;
    const NTSTATUS status = proc.Attach(pid);
    if (!NT_SUCCESS(status)) {
        setlog(L"input hook release attach failed. pid=%d hwnd=%p status=0x%X", pid, hwnd, status);
        return 0;
    }

    long ret = 0;
    const std::wstring dll_name = resolve_hook_dll(proc);
    using release_input_hook_t = long(__stdcall *)();
    auto remote = blackbone::MakeRemoteFunction<release_input_hook_t>(proc, dll_name, "ReleaseInputHook");
    if (remote) {
        auto call_ret = remote();
        ret = call_ret.result();
    } else {
        setlog(L"remote function 'ReleaseInputHook' not found in %s.", dll_name.c_str());
    }

    proc.Detach();
    return ret;
}

bool call_cursor_shape(HWND hwnd, unsigned long long &hash, unsigned long long &meta) {
    DWORD pid = 0;
    ::GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0)
        return false;

    blackbone::Process proc;
    const NTSTATUS status = proc.Attach(pid);
    if (!NT_SUCCESS(status)) {
        setlog(L"input hook cursor attach failed. pid=%d hwnd=%p status=0x%X", pid, hwnd, status);
        return false;
    }

    const std::wstring dll_name = resolve_hook_dll(proc);
    using cursor_part_t = unsigned long(__stdcall *)();
    auto hash_low = blackbone::MakeRemoteFunction<cursor_part_t>(proc, dll_name, "GetInputCursorShapeHashLow");
    auto hash_high = blackbone::MakeRemoteFunction<cursor_part_t>(proc, dll_name, "GetInputCursorShapeHashHigh");
    auto meta_low = blackbone::MakeRemoteFunction<cursor_part_t>(proc, dll_name, "GetInputCursorShapeMetaLow");
    auto meta_high = blackbone::MakeRemoteFunction<cursor_part_t>(proc, dll_name, "GetInputCursorShapeMetaHigh");
    if (!hash_low || !hash_high || !meta_low || !meta_high) {
        proc.Detach();
        return false;
    }

    hash = static_cast<unsigned long long>(hash_low().result()) |
           (static_cast<unsigned long long>(hash_high().result()) << 32);
    meta = static_cast<unsigned long long>(meta_low().result()) |
           (static_cast<unsigned long long>(meta_high().result()) << 32);
    proc.Detach();
    return true;
}

} // namespace

namespace op::hook::input_hook_client {

long Bind(HWND hwnd, int mode) {
    if (!::IsWindow(hwnd))
        return 0;

    std::lock_guard<std::mutex> lock(g_mutex);
    auto &refs = g_bind_refs[hwnd];
    // 鼠标和键盘 dx 会共用同一个目标进程 Hook，宿主侧只做一次注入。
    if (refs > 0) {
        ++refs;
        return 1;
    }

    const long ret = call_set_input_hook(hwnd, mode);
    if (ret == 1) {
        refs = 1;
    } else {
        g_bind_refs.erase(hwnd);
    }
    return ret;
}

long UnBind(HWND hwnd) {
    if (!hwnd)
        return 1;

    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = g_bind_refs.find(hwnd);
    if (it == g_bind_refs.end())
        return 1;

    if (--it->second > 0)
        return 1;

    g_bind_refs.erase(it);
    return call_release_input_hook(hwnd);
}

bool GetCursorShape(HWND hwnd, unsigned long long &hash, unsigned long long &meta) {
    if (!hwnd)
        return false;

    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_bind_refs.find(hwnd) == g_bind_refs.end())
        return false;

    return call_cursor_shape(hwnd, hash, meta);
}

} // namespace op::hook::input_hook_client
