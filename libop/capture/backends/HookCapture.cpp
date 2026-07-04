
// #include "stdafx.h"

#include "HookCapture.h"
#include "../../base/AutomationModes.h"
#include "../../base/Environment.h"
#include "../../base/Utils.h"
#include "../../hook/HookModule.h"
#include "BlackBone/Process/Process.h"
#include "BlackBone/Process/RPC/RemoteFunction.hpp"
#include <cstddef>
#include <exception>
#include <span>

#include "../../image/Image.h"

#include <sstream>

using op::capture::FrameInfo;

namespace {

constexpr DWORD kHookFrameReadyTimeoutMs = 200;
constexpr DWORD kHookFramePollIntervalMs = 10;

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
    // do clear
    UnBindEx();
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
        DWORD id;
        ::GetWindowThreadProcessId(_hwnd, &id);

        // attach 进程
        blackbone::Process proc;
        NTSTATUS hr;

        hr = proc.Attach(id);

        if (NT_SUCCESS(hr)) {
            BOOL is64 = proc.modules().GetMainModule()->type == blackbone::eModType::mt_mod64;
            wstring dllname = op::hook::ResolveHookModuleName(is64 != FALSE);

            bool injected = false;
            // 判断是否已经注入
            auto _dllptr = proc.modules().GetModule(dllname);
            auto mods = proc.modules().GetAllModules();
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
                    bind_ret = cret.result();
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
    if (_render_type == RDT_GL_NOX)
        return UnBindNox();
    DWORD id;
    ::GetWindowThreadProcessId(_hwnd, &id);

    // attach 进程s
    blackbone::Process proc;
    NTSTATUS hr;
    // setlog("bkdo::Attach");
    hr = proc.Attach(id);

    if (NT_SUCCESS(hr)) {
        BOOL is64 = proc.modules().GetMainModule()->type == blackbone::eModType::mt_mod64;
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
    } else {
        setlog("blackbone::MakeRemoteFunction false,errcode:%X,pid=%d,hwnd=%d", hr, id, _hwnd);
    }
    // setlog(L"bkdo::Detach");
    proc.Detach();
    // bind_release();
    return 1;
}

void HookCapture::waitForBindReady() {
    if (!_pmutex || !_shmem) {
        return;
    }

    // 远端 hook 成功后，要等目标进程下一帧写入共享内存。
    const auto start = ::GetTickCount64();
    do {
        FrameInfo info = {};
        getFrameInfo(info);
        if (isHookFrameReady(info, _hwnd)) {
            return;
        }
        ::Sleep(kHookFramePollIntervalMs);
    } while (::GetTickCount64() - start < kHookFrameReadyTimeoutMs);
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
        auto mods = proc.modules().GetAllModules();
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
    img.create(w, h);
    _pmutex->lock();
    FrameInfo *pInfo = reinterpret_cast<FrameInfo *>(_shmem->data<std::byte>());

    const int src_width = pInfo->width > 0 ? (int)pInfo->width : (int)_width;
    const int src_height = pInfo->height > 0 ? (int)pInfo->height : (int)_height;

    if (pInfo->width > 0 && pInfo->height > 0 &&
        ((long)pInfo->width != _width || (long)pInfo->height != _height)) {
        _width = (long)pInfo->width;
        _height = (long)pInfo->height;
    }

    if (src_width <= 0 || src_height <= 0 || x1 < 0 || y1 < 0 || w <= 0 || h <= 0 || x1 + w > src_width ||
        y1 + h > src_height) {
        _pmutex->unlock();
        return false;
    }

    auto frame = makeHookFrameView(*_shmem, src_width, src_height);

    if (GET_RENDER_TYPE(_render_type) == RENDER_TYPE::DX) { // NORMAL

        for (int i = 0; i < h; i++) {
            const auto row = hookFrameRow(frame.pixels, src_width, i + y1, x1, w);
            memcpy(img.ptr<uchar>(i), row.data(), row.size());
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

} // namespace op::capture
