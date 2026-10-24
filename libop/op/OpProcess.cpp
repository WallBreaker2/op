#include "OpContext.h"
#include "OpResult.h"

#include "ipc/CommandRunner.h"
#include "base/Utils.h"
#include "window/DllInjector.h"

#include <libop.h>

#include <string>
#include <vector>

namespace {

std::wstring decode_command_output(const std::string &text) {
    if (text.empty())
        return L"";

    const int utf8_len =
        ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (utf8_len > 0) {
        std::wstring out(utf8_len, L'\0');
        ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), out.data(),
                              utf8_len);
        return out;
    }

    return _s2wstring(text);
}

// §72.51：提升的特权要在离开作用域时还回去，包括提前 return / 抛异常。
struct privilege_guard {
    ~privilege_guard() {
        // 匿名 namespace 在全局作用域，DllInjector 在 op:: 里，要写全名。
        op::DllInjector::EnablePrivilege(FALSE);
    }
};

} // namespace

void op::Op::InjectDll(const wchar_t *process_name, const wchar_t *dll_name, long *ret) {
    LONG_PTR hwnd = 0;
    FindWindowByProcess(process_name, L"", L"", &hwnd);
    long pid = 0;
    GetWindowProcessId(hwnd, &pid);
    internal::set_result(ret, 0L);
    if (DllInjector::EnablePrivilege(TRUE)) {
        // §72.51：提升完必须还回去。SeDebugPrivilege 是"能读写任意进程内存"，
        // 注入结束还留着等于让整个进程（含之后加载的第三方模块）长期持有它。
        // 用 guard 而不是在末尾手动调：中间任何一条路径抛异常也能恢复。
        privilege_guard guard;
        long error_code = 0;
        internal::set_result(ret, DllInjector::InjectDll(pid, internal::wstr(dll_name).c_str(), error_code));
    } else {
        setlog("EnablePrivilege false erro_code=%08X ", ::GetLastError());
    }
}

void op::Op::EnumProcess(const wchar_t *name, std::wstring &retstring) {
    m_context->window_service.EnumProcess(name, retstring);
}

void op::Op::GetProcessInfo(long pid, std::wstring &retstring) {
    m_context->window_service.GetProcessInfo(pid, retstring);
}

void op::Op::GetWindowProcessId(LONG_PTR hwnd, long *nretpid) {
    DWORD pid = 0;
    ::GetWindowThreadProcessId(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), &pid);
    internal::set_result(nretpid, pid);
}

void op::Op::GetWindowProcessPath(LONG_PTR hwnd, std::wstring &retstring) {
    DWORD pid = 0;
    ::GetWindowThreadProcessId(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), &pid);
    m_context->window_service.GetProcesspath(pid, retstring);
}

void op::Op::RunApp(const wchar_t *cmdline, long mode, unsigned long *pid, long *ret) {
    // 成功时返回新进程 pid，失败时返回 0。
    internal::set_result(ret, m_context->window_service.RunApp(internal::wstr(cmdline), mode, pid));
}

void op::Op::WinExec(const wchar_t *cmdline, long cmdshow, long *ret) {
    auto str = _ws2string(internal::wstr(cmdline));
    internal::set_result(ret, ::WinExec(str.c_str(), cmdshow) > 31 ? 1 : 0);
}

void op::Op::GetCmdStr(const wchar_t *cmd, long millseconds, std::wstring &retstr) {
    // 原来 millseconds <= 0 会退化成 5ms：命令行还没来得及输出就被 close() 杀掉进程树，
    // retstr 基本恒为空，与声明里的"运行命令行并返回结果"矛盾（cmd.exe 自身启动就要十几毫秒）。
    // 改成有限的 5 秒默认值；显式传值的调用方行为不变。
    constexpr DWORD kDefaultCmdTimeoutMs = 5000;
    CommandRunner command_runner;
    auto str = command_runner.GetCmdStr(cmd ? std::wstring(cmd) : std::wstring(),
                                        millseconds <= 0 ? kDefaultCmdTimeoutMs
                                                         : static_cast<DWORD>(millseconds));
    retstr = decode_command_output(str);
}
