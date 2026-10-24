
// #include "stdafx.h"

#include "HookCapture.h"
#include "../../base/AutomationModes.h"
#include "../../base/Environment.h"
#include "../../base/Utils.h"
#include "../../hook/HookModule.h"
#include "../../hook/SharedFrame.h"
#include "../../hook/SharedFrameExchange.h"
#include "BlackBone/Process/Process.h"
#include "BlackBone/Process/RPC/RemoteFunction.hpp"
#include <cstddef>
#include <exception>
#include <memory>
#include <span>

#include "../../image/Image.h"

#include <cstdio>
#include <sstream>

using op::capture::FrameInfo;

namespace {

constexpr DWORD kHookFrameReadyTimeoutMs = 200;
constexpr DWORD kHookFramePollIntervalMs = 10;
// 请求的有效期：注入侧超过这个时间没收到新请求就退回整窗搬运（见 SharedFrameExchange.h）。
constexpr DWORD kFrameRequestTtlMs = 250;

// 解绑失败也要看得见：重试前必须先摘干净，release 静默失败会让重试必然再失败，
// 日志里只剩一句 "retry failed"，看不出真因。
// 注意**不要**用返回值判成败：目标侧本来就没绑时 ReleaseDisplayHook 返回 0 是正常路径。
void release_remote_display_hook(blackbone::Process &proc, const std::wstring &dllname) {
    using release_display_hook_t = long(__stdcall *)(void);
    auto release = blackbone::MakeRemoteFunction<release_display_hook_t>(proc, dllname, "ReleaseDisplayHook");
    if (!release) {
        setlog_persistent(L"BindEx: remote function 'ReleaseDisplayHook' not found in %s", dllname.c_str());
        return;
    }
    auto cret = release();
    if (!cret.success()) {
        setlog_persistent(L"BindEx: ReleaseDisplayHook remote call failed status=0x%X dll=%s", cret.status,
                          dllname.c_str());
    }
}

// ---------------------------------------------------------------------------
// 跨位宽：32 位宿主 -> 原生 64 位目标
//
// 宿主自己注入不了（BlackBone 的远程调用按 "目标是否 WOW64" 选汇编器，32 位宿主对
// 原生 64 位目标会选到错位宽的 stub：注入报成功、远端一行代码都没跑，偶发挂上百秒，
// 见 doc/defect_fixes.md §72.13）。所以改由 64 位助手 op_inject64.exe 代劳：
// 它自己就是 64 位，注入与远程调用都是同构的。
//
// 找不到助手时返回 -1，调用方据此报 NotSupportedError（保持"明确拒绝"而不是静默失败）。
// ---------------------------------------------------------------------------

constexpr wchar_t kInject64Exe[] = L"op_inject64.exe";
constexpr wchar_t kHookDll64[] = L"op_x64.dll";
constexpr DWORD kInject64TimeoutMs = 15000;

bool find_first_existing(const std::wstring &base, const wchar_t *name, std::wstring *out) {
    const std::wstring candidates[] = {
        base + L"\\" + name,
        base + L"\\..\\x64\\" + name,
    };
    for (const auto &path : candidates) {
        if (::PathFileExistsW(path.c_str())) {
            *out = path;
            return true;
        }
    }
    return false;
}

// 启动 op_inject64.exe 并把它的 stdout 收进 out_log。
// 返回助手退出码；启动失败返回 -1。
int run_inject64(const std::wstring &base, const std::wstring &cmdline_tail, std::string *out_log) {
    std::wstring exe;
    if (!find_first_existing(base, kInject64Exe, &exe)) {
        setlog(L"BindEx: 64-bit inject helper not found next to %s (looked in <%s>)",
               kInject64Exe, base.c_str());
        return -1;
    }
    std::wstring cmdline = L"\"" + exe + L"\" " + cmdline_tail;

    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE read_pipe = nullptr;
    HANDLE write_pipe = nullptr;
    if (!::CreatePipe(&read_pipe, &write_pipe, &sa, 0)) {
        setlog("BindEx: CreatePipe failed err=%lu", ::GetLastError());
        return -1;
    }
    ::SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = write_pipe;
    si.hStdError = write_pipe;
    si.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION pi = {};
    const BOOL started = ::CreateProcessW(nullptr, cmdline.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                                          nullptr, base.c_str(), &si, &pi);
    ::CloseHandle(write_pipe);
    if (!started) {
        setlog(L"BindEx: CreateProcess <%s> failed err=%lu", exe.c_str(), ::GetLastError());
        ::CloseHandle(read_pipe);
        return -1;
    }

    char buf[512];
    DWORD got = 0;
    while (::ReadFile(read_pipe, buf, sizeof(buf), &got, nullptr) && got > 0) {
        out_log->append(buf, got);
    }
    ::CloseHandle(read_pipe);

    const DWORD wait = ::WaitForSingleObject(pi.hProcess, kInject64TimeoutMs);
    DWORD code = (DWORD)-1;
    if (wait == WAIT_OBJECT_0) {
        ::GetExitCodeProcess(pi.hProcess, &code);
    } else {
        setlog("BindEx: inject helper did not finish in %lu ms, terminating", (unsigned long)kInject64TimeoutMs);
        ::TerminateProcess(pi.hProcess, (UINT)-1);
    }
    ::CloseHandle(pi.hThread);
    ::CloseHandle(pi.hProcess);
    return (int)code;
}

// 把助手的输出逐行转进 setlog，便于用户直接看到目标侧结论。
void log_helper_output(const std::string &log) {
    size_t start = 0;
    while (start < log.size()) {
        size_t end = log.find('\n', start);
        if (end == std::string::npos)
            end = log.size();
        std::string line = log.substr(start, end - start);
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
            line.pop_back();
        if (!line.empty())
            setlog("inject64: %s", line.c_str());
        start = end + 1;
    }
}

struct HookFrameView {
    FrameInfo *info = nullptr;
    std::span<std::byte> pixels;
};

bool isHookFrameReady(const FrameInfo &info, HWND hwnd) {
    FrameInfo expected = info;
    const auto chk = expected.chk;
    expected.fmtChk();
    return chk == expected.chk && info.hwnd == reinterpret_cast<unsigned __int64>(hwnd) && info.width > 0 &&
           info.height > 0;
}

HookFrameView makeHookFrameView(op::SharedMemory &sharedMemory, int width, int height) {
    auto *base = sharedMemory.data<std::byte>();
    auto *info = reinterpret_cast<FrameInfo *>(base);
    const auto pixelBytes = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;

    // 使用 span 表达像素区，后续行拷贝不再直接散落裸指针偏移。
    return {info, {base + sizeof(FrameInfo), pixelBytes}};
}

std::span<const std::byte> hookFrameRow(std::span<const std::byte> pixels, int srcWidth, int row, int x, int width) {
    const auto offset = (static_cast<size_t>(row) * static_cast<size_t>(srcWidth) + static_cast<size_t>(x)) * 4;
    const auto bytes = static_cast<size_t>(width) * 4;
    return pixels.subspan(offset, bytes);
}

} // namespace

namespace op::capture {

HookCapture::HookCapture() : ICaptureBackend(), m_opPath(RuntimeEnvironment::getBasePath()) {
}

HookCapture::~HookCapture() {
    // ICaptureBackend::UnBind() 已经调过 UnBindEx()，并把 _hwnd 清成 NULL。
    // 只有"还处于绑定状态就被销毁"这一种情况才需要在这里补一次远端脱钩；
    // 否则会拿着空 hwnd 再去 Attach 一次（见 UnBindEx 开头的说明，那里曾经
    // 用未初始化的 pid 去附加进程，命中就是 0xc0000005）。
    if (_hwnd) {
        UnBindEx();
    }
}

long HookCapture::BindEx(HWND hwnd, long render_type) {
    // setlog("BindEx");
    _hwnd = hwnd;
    long bind_ret = 0;
    if (render_type == RDT_GL_NOX) {
        bind_ret = BindNox(hwnd, render_type);
    } else {
        _render_type = render_type;
        RECT rc;
        // 获取客户区大小
        ::GetClientRect(hwnd, &rc);
        _width = rc.right - rc.left;
        _height = rc.bottom - rc.top;
        // bind_init();
        if (render_type == RDT_GL_NOX) {
        }
        DWORD id = 0;
        ::GetWindowThreadProcessId(_hwnd, &id);

        // attach 进程
        blackbone::Process proc;
        NTSTATUS hr;

        hr = proc.Attach(id);

        if (NT_SUCCESS(hr)) {
            // GetMainModule() 在目标进程刚起来/已经退出时可能返回空，
            // 原代码直接 ->type 就是空指针 AV。
            auto main_module = proc.modules().GetMainModule();
            if (!main_module) {
                setlog("BindEx: main module not found pid=%u hwnd=%p", id, _hwnd);
                proc.Detach();
                return 0;
            }
            BOOL is64 = main_module->type == blackbone::eModType::mt_mod64;
            wstring dllname = op::hook::ResolveHookModuleName(is64 != FALSE);

            // 32 位宿主 + 原生 64 位目标：宿主自己注入不了，交给 64 位助手 op_inject64.exe。
            //
            // 实测（doc/defect_fixes.md §72.13）：32 位宿主注入 op_x64.dll 后
            // modules().Inject() 报成功、MakeRemoteFunction 也能解析到导出，但目标进程里
            // 一行代码都没跑 —— 连 DllMain 写的 __op.log 都不出现；偶发还会卡 111 秒才返回。
            // 根因在 BlackBone 的 RemoteExec：它按"目标进程是否 WOW64"选执行模式，
            // 从 32 位宿主给原生 64 位目标建线程走的是 32 位 ntdll，64 位代码不会被执行。
            // 助手自己是 64 位，注入与远程调用同构，不受这个限制影响。
            if (sizeof(void *) == 4 && is64) {
                std::wstring dll64;
                if (!find_first_existing(m_opPath, kHookDll64, &dll64)) {
                    setlog(L"BindEx: 32-bit host cannot inject into native 64-bit target pid=%u hwnd=%p "
                           L"(no %s next to <%s>, needs a 64-bit inject helper)",
                           id, _hwnd, kHookDll64, m_opPath.c_str());
                    proc.Detach();
                    setlog("NotSupportedError");
                    return -2;
                }
                wchar_t tail[600] = {};
                swprintf_s(tail, L"--pid=%lu --dll=\"%s\" --hwnd=0x%llX --render-type=%d --timeout-ms=5000",
                           (unsigned long)id, dll64.c_str(), (unsigned long long)(ULONG_PTR)hwnd, render_type);
                std::string helper_log;
                const int helper_code = run_inject64(m_opPath, tail, &helper_log);
                proc.Detach();
                log_helper_output(helper_log);
                if (helper_code == 0) {
                    setlog(L"BindEx: 64-bit helper bound pid=%u hwnd=%p dll=%s", id, _hwnd, dll64.c_str());
                    return 1;
                }
                if (helper_code == 2 || helper_code == -1) {
                    // 2 = 目标侧 SetDisplayHook 返回 0（比如已被别的窗口占用）；-1 = 助手没起来。
                    setlog("NotSupportedError");
                    return -2;
                }
                setlog("UnknownError");
                return -1;
            }

            bool injected = false;
            // 判断是否已经注入
            auto _dllptr = proc.modules().GetModule(dllname);
            if (_dllptr) {
                injected = true;
            } else {
                wstring opFile = m_opPath + L"\\" + dllname;
                if (::PathFileExistsW(opFile.data())) {
                    auto iret = proc.modules().Inject(opFile);
                    injected = (iret ? true : false);
                    if (!injected) {
                        setlog(L"Inject failed. pid=%d hwnd=%d status=0x%X dll=%s", id, _hwnd, iret.status,
                               opFile.c_str());
                    }
                } else {
                    setlog(L"file:<%s> not exists!", opFile.data());
                }
            }
            if (injected) {
                // setlog("before MakeRemoteFunction");
                using my_func_t = long(__stdcall *)(HWND, int);
                auto pSetXHook = blackbone::MakeRemoteFunction<my_func_t>(proc, dllname, "SetDisplayHook");
                if (pSetXHook) {
                    // setlog("after MakeRemoteFunction");
                    auto cret = pSetXHook(hwnd, render_type);
                    // setlog("after pSetXHook");
                    // result(0)：远端调用失败时 result_data 是空 optional，直接 result() 会抛
                    // bad_optional_access；失败原因看 status，别把它混成"钩子返回 0"。
                    bind_ret = cret.result(0);
                    if (!cret.success()) {
                        setlog_persistent(L"BindEx: SetDisplayHook remote call failed status=0x%X pid=%u hwnd=%p",
                                          cret.status, id, _hwnd);
                        release_remote_display_hook(proc, dllname);
                    } else if (bind_ret != 1) {
                        // 这条路径原来是**静默**的：注入成功、远程调用也报成功，但目标侧
                        // SetDisplayHook 返回 0。目标侧每条拒绝/失败分支都会自己写 __op.log，
                        // 所以"一行目标日志都没有"基本可判定为远程代码没执行 —— BlackBone 的
                        // ExecInNewThread 在远程线程没跑完时仍返回 STATUS_SUCCESS，读回的
                        // callResult 就是 0（§72.13 记录过同类现象）。这种假失败重试一次即可
                        // 恢复，不该甩给上层调用者。
                        setlog_persistent(L"BindEx: SetDisplayHook returned %ld, retrying once pid=%u hwnd=%p dll=%s",
                                          bind_ret, id, _hwnd, dllname.c_str());
                        release_remote_display_hook(proc, dllname);
                        auto retry = pSetXHook(hwnd, render_type);
                        const long retry_ret = retry.result(0);
                        if (retry.success() && retry_ret == 1) {
                            bind_ret = 1;
                            setlog_persistent(L"BindEx: SetDisplayHook recovered on retry pid=%u hwnd=%p", id, _hwnd);
                        } else {
                            bind_ret = retry_ret;
                            setlog_persistent(L"BindEx: SetDisplayHook retry failed ret=%ld status=0x%X pid=%u "
                                              L"hwnd=%p dll=%s",
                                              retry_ret, retry.status, id, _hwnd, dllname.c_str());
                            release_remote_display_hook(proc, dllname);
                        }
                    }
                    // setlog("after result");
                } else {
                    setlog(L"remote function 'SetDisplayHook' not found in %s.", dllname.c_str());
                }
            } else {
                setlog(L"Inject false.");
            }
        } else {
            setlog(L"attach false. pid=%d hwnd=%d hr=0x%X", id, _hwnd, hr);
        }

        proc.Detach();
        // setlog("after Detach");
    }

    if (bind_ret == -1) {
        setlog("UnknownError");
    } else if (bind_ret == -2) {
        setlog("NotSupportedError");
    } else if (bind_ret == -3) {
        setlog("ModuleNotFoundError");
    }
    return bind_ret;
}

long HookCapture::UnBindEx() {
    // setlog("bkdo::UnBindEx()");
    // 【必须早退】ICaptureBackend::UnBind() 会先调 UnBindEx() 再 bind_release()，
    // 而 bind_release() 把 _hwnd 清成 NULL；析构/_capture.reset() 又可能再进来一次。
    // 空 hwnd 时 ::GetWindowThreadProcessId(NULL, &id) 返回 0 且**不写 id**，
    // 原代码 `DWORD id;` 未初始化，于是 proc.Attach(栈垃圾) 随机附加到别的进程，
    // 拿到空 main module 后 ->type 就是 0xc0000005。
    if (!_hwnd) {
        return 1;
    }
    if (_render_type == RDT_GL_NOX)
        return UnBindNox();
    DWORD id = 0;
    ::GetWindowThreadProcessId(_hwnd, &id);

    // 跨位宽解绑：本进程是 WOW64（32 位宿主跑在 64 位系统上）、而目标不是 WOW64
    // （原生 64 位）时，BlackBone 的远程调用同样不能用，改走 64 位助手。
    if (sizeof(void *) == 4 && id != 0) {
        BOOL self_wow64 = FALSE;
        ::IsWow64Process(::GetCurrentProcess(), &self_wow64);
        HANDLE target = self_wow64 ? ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, id) : nullptr;
        BOOL target_wow64 = TRUE;
        if (target) {
            ::IsWow64Process(target, &target_wow64);
            ::CloseHandle(target);
        }
        if (self_wow64 && !target_wow64) {
            std::wstring dll64;
            if (find_first_existing(m_opPath, kHookDll64, &dll64)) {
                wchar_t tail[600] = {};
                swprintf_s(tail, L"--pid=%lu --dll=\"%s\" --release --timeout-ms=5000", (unsigned long)id,
                           dll64.c_str());
                std::string helper_log;
                const int helper_code = run_inject64(m_opPath, tail, &helper_log);
                log_helper_output(helper_log);
                if (helper_code != 0) {
                    setlog(L"UnBindEx: 64-bit helper release failed code=%d pid=%u hwnd=%p", helper_code, id,
                           _hwnd);
                }
            } else {
                setlog(L"UnBindEx: %s not found next to <%s>, skip cross-bitness release", kHookDll64,
                       m_opPath.c_str());
            }
            return 1;
        }
    }

    // attach 进程s
    blackbone::Process proc;
    NTSTATUS hr;
    // setlog("bkdo::Attach");
    hr = proc.Attach(id);

    if (NT_SUCCESS(hr)) {
        auto main_module = proc.modules().GetMainModule();
        if (!main_module) {
            setlog("UnBindEx: main module not found pid=%u hwnd=%p", id, _hwnd);
        } else {
            BOOL is64 = main_module->type == blackbone::eModType::mt_mod64;
            wstring dllname = op::hook::ResolveHookModuleName(is64 != FALSE);
            // setlog(L"bkdo::dllname=%s",dllname);
            using my_func_t = long(__stdcall *)(void);
            auto pUnXHook = blackbone::MakeRemoteFunction<my_func_t>(proc, dllname, "ReleaseDisplayHook");
            if (pUnXHook) {
                // setlog(L"bkdo::pUnXHook");
                pUnXHook();
                // BOOL fret = ::FreeLibrary((HMODULE)proc.modules().GetModule(dllname)->baseAddress);
                // if (!fret)setlog("fret=%d", fret);
                /*proc.modules().RemoveManualModule(dllname,
                    is64 ? blackbone::eModType::mt_mod64 : blackbone::eModType::mt_mod32);*/
            } else {
                setlog(L"get unhook ptr false.");
            }
        }
    } else {
        setlog("blackbone::MakeRemoteFunction false,errcode:%X,pid=%u,hwnd=%p", hr, id, _hwnd);
    }
    // setlog(L"bkdo::Detach");
    proc.Detach();
    // bind_release();
    return 1;
}

bool HookCapture::waitForFrameReady(DWORD timeout_ms) {
    if (!_pmutex || !_shmem) {
        return false;
    }

    // 远端 hook 成功后，要等目标进程下一帧写入共享内存。
    const auto start = ::GetTickCount64();
    do {
        FrameInfo info = {};
        getFrameInfo(info);
        if (isHookFrameReady(info, _hwnd)) {
            return true;
        }
        ::Sleep(kHookFramePollIntervalMs);
    } while (::GetTickCount64() - start < timeout_ms);
    return false;
}

void HookCapture::waitForBindReady() {
    if (!_pmutex || !_shmem) {
        return;
    }

    if (waitForFrameReady(kHookFrameReadyTimeoutMs)) {
        return;
    }

    // 超时只可能是"注入侧没在写"：没出帧（静态/最小化/暂停）、注入失败，或者目标进程里跑的是
    // 改名之前那一版注入 DLL（§72.20 把段/互斥体的前缀改回正名，老 DLL 按老名字找对象，两边
    // 永远等不到对方的帧）。
    // 只说"没写帧"定位不了，所以把帧头原样打出来 —— isHookFrameReady 的三个判据各对应一种原因：
    //   chk 不自洽        → 从没写过（注入失败 / 老 DLL），或者正好撞上一次写入
    //   frame_hwnd 不同   → 写过，但绑的是别的窗口
    //   width/height 为 0 → 写过，但目标最小化了
    // 日志必须是 ASCII，setlog 的 %s 走窄化，写中文会变乱码。
    FrameInfo info = {};
    getFrameInfo(info);
    FrameInfo expected = info;
    const auto chk = expected.chk;
    expected.fmtChk();
    setlog("waitForBindReady timeout %ums hwnd=%p: chk=%08X expect=%08X frame_hwnd=%p %ux%u "
           "(chk mismatch = never written or torn; frame_hwnd mismatch = bound to another window; "
           "0x0 = minimized)",
           static_cast<unsigned>(kHookFrameReadyTimeoutMs), _hwnd, chk, expected.chk,
           reinterpret_cast<void *>(static_cast<unsigned __int64>(info.hwnd)), info.width, info.height);
}

long HookCapture::BindNox(HWND hwnd, long render_type) {
    _render_type = render_type;
    _hwnd = hwnd;
    RECT rc;
    // 获取客户区大小
    ::GetClientRect(hwnd, &rc);
    _width = rc.right - rc.left;
    _height = rc.bottom - rc.top;
    // bind_init();

    // attach 进程
    blackbone::Process proc;
    NTSTATUS hr = -1;

    wstring dllname = L"op_x64.dll";

    hr = proc.Attach(L"NoxVMHandle.exe");

    long bind_ret = 0;

    if (NT_SUCCESS(hr)) {
        /*_process.Resume();*/
        bool injected = false;
        // 判断是否已经注入
        auto _dllptr = proc.modules().GetModule(dllname);
        if (_dllptr) {
            injected = true;
        } else {
            wstring opFile = m_opPath + L"\\" + dllname;
            if (::PathFileExistsW(opFile.data())) {
                auto iret = proc.modules().Inject(opFile);
                injected = (iret ? true : false);
                if (!injected) {
                    setlog(L"Inject failed. pid=%d hwnd=%d status=0x%X dll=%s", proc.pid(), _hwnd, iret.status,
                           opFile.c_str());
                }
            } else {
                setlog(L"file:<%s> not exists!", opFile.data());
            }
        }
        if (injected) {
            using my_func_t = long(__stdcall *)(HWND, int);
            auto pSetXHook = blackbone::MakeRemoteFunction<my_func_t>(proc, dllname, "SetDisplayHook");
            if (pSetXHook) {
                auto cret = pSetXHook(hwnd, render_type);
                bind_ret = cret.result();
                if (bind_ret != 1) {
                    release_remote_display_hook(proc, dllname);
                }
            } else {
                setlog(L"remote function not found.");
            }
        } else {
            setlog(L"Inject false.");
        }

    } else {
        setlog(L"attach false.");
    }
    proc.Detach();

    return bind_ret;
}

long HookCapture::UnBindNox() {

    // attach 进程
    blackbone::Process proc;
    NTSTATUS hr;

    hr = proc.Attach(L"NoxVMHandle.exe");
    wstring dllname = L"op_x64.dll";

    if (NT_SUCCESS(hr)) {

        using my_func_t = long(__stdcall *)(void);
        auto pUnXHook = blackbone::MakeRemoteFunction<my_func_t>(proc, dllname, "ReleaseDisplayHook");
        if (pUnXHook) {
            pUnXHook();

            /*BOOL fret = ::FreeLibrary((HMODULE)proc.modules().GetModule(dllname)->baseAddress);
            if (!fret)setlog("fret=%d", fret);*/
        } else {
            setlog(L"get unhook ptr false.");
        }
    } else {
        setlog("blackbone::MakeRemoteFunction false,errcode:%Xhwnd=%d", hr, _hwnd);
    }

    proc.Detach();

    return 1;
}

bool HookCapture::requestCapture(int x1, int y1, int w, int h, Image &img) {
    if (!_pmutex || !_shmem) {
        return false;
    }

    for (int attempt = 0; attempt < 2; ++attempt) {
        _pmutex->lock();
        FrameInfo *pInfo = reinterpret_cast<FrameInfo *>(_shmem->data<std::byte>());
        const FrameInfo info = pInfo ? *pInfo : FrameInfo{};
        if (!isHookFrameReady(info, _hwnd)) {
            _pmutex->unlock();
            // 首帧可能还没到：bind 之后 waitForBindReady 只等 200 ms，慢启动的宿主赶不上
            // （§4.9 的 egl_host_app 就是），紧接着第一次 requestCapture 就撞空、外层只留一句
            // "error requestCapture"。只给第一次尝试补等一轮，等不到照旧失败 —— 热路径不能
            // 变成无界阻塞。
            if (attempt == 0 && waitForFrameReady(kHookFrameReadyTimeoutMs)) {
                continue;
            }
            return false;
        }

        const int src_width = info.width > 0 ? (int)info.width : (int)_width;
        const int src_height = info.height > 0 ? (int)info.height : (int)_height;

        if (info.width > 0 && info.height > 0 && ((long)info.width != _width || (long)info.height != _height)) {
            _width = (long)info.width;
            _height = (long)info.height;
        }

        if (!op::hook::SharedFrameHasCapacity(*_shmem, src_width, src_height)) {
            _pmutex->unlock();
            if (!ensureSharedFrameCapacity(src_width, src_height)) {
                return false;
            }
            waitForBindReady();
            continue;
        }

        if (src_width <= 0 || src_height <= 0 || x1 < 0 || y1 < 0 || w <= 0 || h <= 0 || x1 + w > src_width ||
            y1 + h > src_height) {
            _pmutex->unlock();
            return false;
        }

        // 发布本次想要的矩形（持锁状态下写，注入侧无锁读，靠 seqlock 判完整）。
        // 必须放在越界检查之后：注入侧会照单全收，发布一个越界矩形会让它读越界。
        op::hook::PublishFrameRequest(*_shmem, static_cast<std::uint32_t>(src_width),
                                      static_cast<std::uint32_t>(src_height), x1, y1, w, h, kFrameRequestTtlMs);

        // 图像缓冲延后到确定要写像素时再建：放在这里的话，下面任何一条 return false 的
        // 路径都会把调用方的 Image 改成半成品尺寸（内容还是上一帧的）。
        auto frame = makeHookFrameView(*_shmem, src_width, src_height);

        // 远端写入共享帧的行顺序不完全等同于渲染大类。
        // DX 是窗口坐标方向（顶朝下）；传统 OpenGL 与 EGL/GLES 都按左下角原点读回，要逐行翻转。
        // GLES 原来被当成顶朝下，是 §72.25 拿 ANGLE（夸克网盘自带的 libEGL/libGLESv2）做跨进程
        // e2e 时抓到的：画面上下颠倒，顶部标记带跑到了图像底部。
        const bool top_down_frame = GET_RENDER_TYPE(_render_type) == RENDER_TYPE::DX;

        // 注入侧可能只搬了我们请求的那块（§3.2）。响应描述的是**缓冲区里这一帧**的实际布局，
        // 所以按响应读、而不是按我们刚才请求的读：请求和这一帧可能对不上（请求是刚发的）。
        // 缓冲区里这一帧可能是上一帧的矩形（请求刚发布，注入侧还没画新帧），
        // 那就等下一帧再读响应；等不到才失败。没有这个等待，紧跟 GetColor 的
        // 一次性 Capture 会读到 1x1 的旧区域而直接失败。
        int rx = 0;
        int ry = 0;
        int rw = 0;
        int rh = 0;
        bool have_region = false;
        // 协商区在不在，决定"响应不可信"该怎么解释：
        //   不在（旧注入）→ 没有 §3.2，缓冲区必然是整窗帧，直接整窗读；
        //   在 → 响应无效只说明这次读到了撕裂（seqlock 奇数）或者注入侧还没写过，
        //        绝不能当成"整窗帧"去读，否则会把"按矩形搬的那一帧"当整窗解析，
        //        读出来是错位的垃圾（§72.19 的 FF00FF）。
        const bool exchange_present = op::hook::FrameExchangeFits(
            *_shmem, static_cast<std::uint32_t>(src_width), static_cast<std::uint32_t>(src_height));
        const ULONGLONG response_deadline = GetTickCount64() + kHookFrameReadyTimeoutMs;
        while (exchange_present) {
            const op::hook::FrameResponseSnapshot response =
                op::hook::ReadFrameResponse(*_shmem, static_cast<std::uint32_t>(src_width),
                                            static_cast<std::uint32_t>(src_height));
            if (response.valid) {
                if (response.mode == op::hook::kFrameModeRegion && response.w > 0 && response.h > 0) {
                    if (x1 >= response.x && y1 >= response.y && x1 + w <= response.x + response.w &&
                        y1 + h <= response.y + response.h) {
                        rx = response.x;
                        ry = response.y;
                        rw = response.w;
                        rh = response.h;
                        have_region = true;
                        break;
                    }
                    // 这一帧搬的是别的矩形，覆盖不到我们这块 —— 等下一帧。
                } else {
                    break; // 整窗帧
                }
            }
            if (GetTickCount64() >= response_deadline) {
                    // 等到超时说明目标一直没出新帧（窗口静止/最小化/游戏暂停）。把请求改成
                    // 整窗并留着它：目标哪怕只再 Present 一帧，缓冲区里就是一整帧，下一次
                    // Capture 不管要哪块都能直接读出来，不用再空等一轮。
                    op::hook::PublishFrameRequest(*_shmem, static_cast<std::uint32_t>(src_width),
                                                  static_cast<std::uint32_t>(src_height), 0, 0, src_width, src_height,
                                                  kFrameRequestTtlMs);
                _pmutex->unlock();
                setlog("Capture: no frame covers the requested rect (static target?), asked for full window");
                return false;
            }
            _pmutex->unlock();
            Sleep(kHookFramePollIntervalMs);
            _pmutex->lock();
        }

        // 上面等新帧时 unlock 过：这期间别的线程可能把共享段换成更大的（ensureSharedFrameCapacity
        // 会 SAFE_DELETE 旧的再 open_create），frame 就指向已解映射的内存。重开过必须重建视图，
        // 容量不够就照旧走扩容分支。
        if (!op::hook::SharedFrameHasCapacity(*_shmem, src_width, src_height)) {
            _pmutex->unlock();
            if (!ensureSharedFrameCapacity(src_width, src_height)) {
                return false;
            }
            waitForBindReady();
            continue;
        }
        frame = makeHookFrameView(*_shmem, src_width, src_height);

        if (have_region) {
            const int column = x1 - rx;
            const int row_offset = y1 - ry;
            img.create(w, h);
            for (int i = 0; i < h; i++) {
                const int source_row = top_down_frame ? row_offset + i : (rh - 1 - row_offset - i);
                const auto row = hookFrameRow(frame.pixels, rw, source_row, column, w);
                memcpy(img.ptr<uchar>(i), row.data(), row.size());
            }
            _pmutex->unlock();
            return true;
        }

        img.create(w, h);
        if (top_down_frame) {
            const size_t row_bytes = static_cast<size_t>(w) * 4;
            if (x1 == 0 && w == src_width) {
                // 整行拷贝时行距与目标一致，一次 memcpy 代替逐行循环。
                memcpy(img.pdata, frame.pixels.data() + static_cast<size_t>(y1) * row_bytes, row_bytes * h);
            } else {
                for (int i = 0; i < h; i++) {
                    const auto row = hookFrameRow(frame.pixels, src_width, i + y1, x1, w);
                    memcpy(img.ptr<uchar>(i), row.data(), row.size());
                }
            }
        } else {
            for (int i = 0; i < h; i++) {
                const auto row = hookFrameRow(frame.pixels, src_width, src_height - 1 - i - y1, x1, w);
                memcpy(img.ptr<uchar>(i), row.data(), row.size());
            }
        }

        _pmutex->unlock();
        return true;
    }

    return false;
}

bool HookCapture::ensureSharedFrameCapacity(int width, int height) {
    // 多留一个协商区（§3.2）：旧注入不认它，只多占 72 字节；新注入据此按矩形搬运。
    const size_t required = op::hook::RequiredSharedFrameBytesWithExchange(width, height);
    if (required == 0) {
        return false;
    }
    if (_shmem && _shmem->size() >= required) {
        return true;
    }

    if (!_pmutex) {
        auto mutex = std::make_unique<ProcessMutex>();
        if (!mutex->open_create(_mutex_name)) {
            return false;
        }
        _pmutex = mutex.release();
    }

    for (int attempt = 0; attempt < 20; ++attempt) {
        _pmutex->lock();
        SAFE_DELETE(_shmem);

        auto shmem = std::make_unique<SharedMemory>();
        const bool opened = shmem->open_create(_shared_res_name, required);
        if (opened && shmem->size() >= required) {
            _shmem = shmem.release();
            _pmutex->unlock();
            return true;
        }

        _pmutex->unlock();
        ::Sleep(10);
    }

    return false;
}

} // namespace op::capture
