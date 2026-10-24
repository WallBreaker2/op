// OpAutomation.cpp: OpAutomation 的实现

#include "OpAutomation.h"
#include "stdafx.h"

#include "../base/AutomationModes.h"
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
// OpAutomation
using std::wstring;

namespace {

template <typename Target, typename Value> HRESULT SetOutValue(Target *target, Value value) {
    if (!target)
        return E_POINTER;
    *target = static_cast<Target>(value);
    return S_OK;
}

// COM 入口统一把 NULL BSTR 归一成一个"真实的空 BSTR"。
// 脚本传 Null / VT_EMPTY 时 BSTR 就是 NULL，而下游（门面 -> std::wstring / wcslen）
// 会直接把 NULL 当字符串用，表现为宿主进程 AV。这里返回的是进程级常驻对象，
// 调用方不要释放，也不要对它做 SysFreeString。
BSTR SafeBstr(BSTR value) {
    if (value) {
        return value;
    }
    static BSTR empty = ::SysAllocString(L"");
    return empty ? empty : const_cast<BSTR>(L"");
}

// 将 C++ 宽字符串复制到 COM 调用方持有的 BSTR 输出参数。
HRESULT CopyOutBstr(BSTR *target, const std::wstring &value) {
    if (!target)
        return E_POINTER;
    *target = nullptr;
    CComBSTR out(value.c_str());
    return out.CopyTo(target);
}

template <typename Callback> HRESULT RunCvRetOnly(LONG *ret, Callback &&callback) {
    if (!ret)
        return E_POINTER;

    SetOutValue(ret, 0L);
    callback(ret);
    return S_OK;
}

} // namespace

// 形参恰为 (LONG *ret) 的纯转发：宏内 obj.<Name>(ret) 由同一个 Name 推导，
// 方法名写错会直接编译失败，因此不可能悄悄转发到错误的对象上。
// try/catch 写在宏里：COM 边界不能漏异常（/EHa 下连 AV 也会被捕获），
// 否则脚本侧拿到的不是 HRESULT 而是宿主进程崩溃。
#define OP_COM_FORWARD(Name)                                                                                           \
    STDMETHODIMP OpAutomation::Name(LONG *ret) {                                                                       \
        try {                                                                                                          \
            obj.Name(ret);                                                                                             \
            return S_OK;                                                                                               \
        } catch (...) {                                                                                                \
            return E_UNEXPECTED;                                                                                       \
        }                                                                                                              \
    }

OpAutomation::OpAutomation() {
}

STDMETHODIMP OpAutomation::Ver(BSTR *ret) {
    try {

        // Tool::setlog("address=%d,str=%s", ver, ver);
        wstring s = obj.Ver();
        CopyOutBstr(ret, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::SetPath(BSTR path, LONG *ret) {
    try {
        path = SafeBstr(path);

        obj.SetPath(path, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetPath(BSTR *path) {
    try {
        wstring s;
        obj.GetPath(s);

        CopyOutBstr(path, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetBasePath(BSTR *path) {
    try {

        wstring s;
        obj.GetBasePath(s);

        CopyOutBstr(path, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

OP_COM_FORWARD(GetID)

OP_COM_FORWARD(GetLastError)

STDMETHODIMP OpAutomation::SetShowErrorMsg(LONG show_type, LONG *ret) {
    try {
        obj.SetShowErrorMsg(show_type, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::Sleep(LONG millseconds, LONG *ret) {
    try {

        obj.Sleep(millseconds, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::InjectDll(BSTR process_name, BSTR dll_name, LONG *ret) {
    try {
        dll_name = SafeBstr(dll_name);
        process_name = SafeBstr(process_name);

        // auto proc = _wsto_string(process_name);
        // auto dll = _wsto_string(dll_name);
        // DllInjector::EnablePrivilege(TRUE);
        // auto h = DllInjector::InjectDll(process_name, dll_name);
        obj.InjectDll(process_name, dll_name, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::EnablePicCache(LONG enable, LONG *ret) {
    try {

        obj.EnablePicCache(enable, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CapturePre(BSTR file, LONG *ret) {
    try {
        file = SafeBstr(file);

        obj.CapturePre(file, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::SetScreenDataMode(LONG mode, LONG *ret) {
    try {
        obj.SetScreenDataMode(mode, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::AStarFindPath(LONG mapWidth, LONG mapHeight, BSTR disable_points, LONG beginX, LONG beginY,
                                         LONG endX, LONG endY, BSTR *path) {
    try {
        disable_points = SafeBstr(disable_points);

        wstring s;
        obj.AStarFindPath(mapWidth, mapHeight, disable_points, beginX, beginY, endX, endY, s);

        CopyOutBstr(path, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

// 根据部分Ex接口的返回值，然后在所有坐标里找出距离指定坐标最近的那个坐标.
STDMETHODIMP OpAutomation::FindNearestPos(BSTR all_pos, LONG type, LONG x, LONG y, BSTR *retstr) {
    try {
        all_pos = SafeBstr(all_pos);

        std::wstring s;
        obj.FindNearestPos(all_pos, type, x, y, s);
        CopyOutBstr(retstr, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::EnumWindow(LONGLONG parent, BSTR title, BSTR class_name, LONG filter, BSTR *retstr) {
    try {
        class_name = SafeBstr(class_name);
        title = SafeBstr(title);

        wstring s;
        obj.EnumWindow(static_cast<LONG_PTR>(parent), title, class_name, filter, s);

        return CopyOutBstr(retstr, s);

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::EnumWindowByProcess(BSTR process_name, BSTR title, BSTR class_name, LONG filter,
                                               BSTR *retstring) {
    try {
        class_name = SafeBstr(class_name);
        process_name = SafeBstr(process_name);
        title = SafeBstr(title);

        wstring s;
        obj.EnumWindowByProcess(process_name, title, class_name, filter, s);

        CopyOutBstr(retstring, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::EnumProcess(BSTR name, BSTR *retstring) {
    try {
        name = SafeBstr(name);

        wstring s;
        obj.EnumProcess(name, s);

        CopyOutBstr(retstring, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::ClientToScreen(LONGLONG hwnd, VARIANT *x, VARIANT *y, LONG *bret) {
    try {
        x->vt = VT_I4;
        y->vt = VT_I4;
        long lx = x->lVal;
        long ly = y->lVal;
        obj.ClientToScreen(static_cast<LONG_PTR>(hwnd), &lx, &ly, bret);
        x->lVal = lx;
        y->lVal = ly;
        return S_OK;

    } catch (...) {
        if (bret) {
            *bret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::FindWindow(BSTR class_name, BSTR title, LONGLONG *rethwnd) {
    try {
        class_name = SafeBstr(class_name);
        title = SafeBstr(title);

        LONG_PTR hwnd = 0;
        obj.FindWindow(class_name, title, &hwnd);
        return SetOutValue(rethwnd, hwnd);

    } catch (...) {
        if (rethwnd) {
            *rethwnd = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::FindWindowByProcess(BSTR process_name, BSTR class_name, BSTR title, LONGLONG *rethwnd) {
    try {
        class_name = SafeBstr(class_name);
        process_name = SafeBstr(process_name);
        title = SafeBstr(title);

        LONG_PTR hwnd = 0;
        obj.FindWindowByProcess(process_name, class_name, title, &hwnd);
        return SetOutValue(rethwnd, hwnd);

    } catch (...) {
        if (rethwnd) {
            *rethwnd = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::FindWindowByProcessId(LONG process_id, BSTR class_name, BSTR title, LONGLONG *rethwnd) {
    try {
        class_name = SafeBstr(class_name);
        title = SafeBstr(title);

        LONG_PTR hwnd = 0;
        obj.FindWindowByProcessId(process_id, class_name, title, &hwnd);
        return SetOutValue(rethwnd, hwnd);

    } catch (...) {
        if (rethwnd) {
            *rethwnd = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::FindWindowEx(LONGLONG parent, BSTR class_name, BSTR title, LONGLONG *rethwnd) {
    try {
        class_name = SafeBstr(class_name);
        title = SafeBstr(title);

        LONG_PTR hwnd = 0;
        obj.FindWindowEx(static_cast<LONG_PTR>(parent), class_name, title, &hwnd);
        return SetOutValue(rethwnd, hwnd);

    } catch (...) {
        if (rethwnd) {
            *rethwnd = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetClientRect(LONGLONG hwnd, VARIANT *x1, VARIANT *y1, VARIANT *x2, VARIANT *y2,
                                         LONG *nret) {
    try {
        x1->vt = VT_I4;
        y1->vt = VT_I4;
        x2->vt = VT_I4;
        y2->vt = VT_I4;
        obj.GetClientRect(static_cast<LONG_PTR>(hwnd), &x1->lVal, &y1->lVal, &x2->lVal, &y2->lVal, nret);

        return S_OK;

    } catch (...) {
        if (nret) {
            *nret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetClientSize(LONGLONG hwnd, VARIANT *width, VARIANT *height, LONG *nret) {
    try {
        width->vt = VT_I4;
        height->vt = VT_I4;
        obj.GetClientSize(static_cast<LONG_PTR>(hwnd), &width->lVal, &height->lVal, nret);

        return S_OK;

    } catch (...) {
        if (nret) {
            *nret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetForegroundFocus(LONGLONG *rethwnd) {
    try {
        LONG_PTR hwnd = 0;
        obj.GetForegroundFocus(&hwnd);
        return SetOutValue(rethwnd, hwnd);

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetForegroundWindow(LONGLONG *rethwnd) {
    try {
        LONG_PTR hwnd = 0;
        obj.GetForegroundWindow(&hwnd);
        return SetOutValue(rethwnd, hwnd);

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetMousePointWindow(LONGLONG *rethwnd) {
    try {
        //::Sleep(2000);
        LONG_PTR hwnd = 0;
        obj.GetMousePointWindow(&hwnd);
        return SetOutValue(rethwnd, hwnd);

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetPointWindow(LONG x, LONG y, LONGLONG *rethwnd) {
    try {
        LONG_PTR hwnd = 0;
        obj.GetPointWindow(x, y, &hwnd);
        return SetOutValue(rethwnd, hwnd);

    } catch (...) {
        if (rethwnd) {
            *rethwnd = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetProcessInfo(LONG pid, BSTR *retstring) {
    try {
        wstring s;
        obj.GetProcessInfo(pid, s);

        CopyOutBstr(retstring, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetSpecialWindow(LONG flag, LONGLONG *rethwnd) {
    try {
        LONG_PTR hwnd = 0;
        obj.GetSpecialWindow(flag, &hwnd);
        return SetOutValue(rethwnd, hwnd);

    } catch (...) {
        if (rethwnd) {
            *rethwnd = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetWindow(LONGLONG hwnd, LONG flag, LONGLONG *nret) {
    try {
        LONG_PTR target = 0;
        obj.GetWindow(static_cast<LONG_PTR>(hwnd), flag, &target);
        return SetOutValue(nret, target);

    } catch (...) {
        if (nret) {
            *nret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetWindowClass(LONGLONG hwnd, BSTR *retstring) {
    try {
        wstring s;
        obj.GetWindowClass(static_cast<LONG_PTR>(hwnd), s);

        CopyOutBstr(retstring, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetWindowProcessId(LONGLONG hwnd, LONG *nretpid) {
    try {
        obj.GetWindowProcessId(static_cast<LONG_PTR>(hwnd), nretpid);

        return S_OK;

    } catch (...) {
        if (nretpid) {
            *nretpid = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetWindowProcessPath(LONGLONG hwnd, BSTR *retstring) {
    try {
        wstring s;
        obj.GetWindowProcessPath(static_cast<LONG_PTR>(hwnd), s);

        CopyOutBstr(retstring, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetWindowRect(LONGLONG hwnd, VARIANT *x1, VARIANT *y1, VARIANT *x2, VARIANT *y2,
                                         LONG *nret) {
    try {
        x1->vt = VT_I4;
        x2->vt = VT_I4;
        y1->vt = VT_I4;
        y2->vt = VT_I4;

        obj.GetWindowRect(static_cast<LONG_PTR>(hwnd), &x1->lVal, &y1->lVal, &x2->lVal, &y2->lVal, nret);

        return S_OK;

    } catch (...) {
        if (nret) {
            *nret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetWindowState(LONGLONG hwnd, LONG flag, LONG *rethwnd) {
    try {
        obj.GetWindowState(static_cast<LONG_PTR>(hwnd), flag, rethwnd);

        return S_OK;

    } catch (...) {
        if (rethwnd) {
            *rethwnd = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetWindowTitle(LONGLONG hwnd, BSTR *rettitle) {
    try {
        wstring s;
        obj.GetWindowTitle(static_cast<LONG_PTR>(hwnd), s);

        CopyOutBstr(rettitle, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::MoveWindow(LONGLONG hwnd, LONG x, LONG y, LONG *nret) {
    try {
        obj.MoveWindow(static_cast<LONG_PTR>(hwnd), x, y, nret);

        return S_OK;

    } catch (...) {
        if (nret) {
            *nret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::ScreenToClient(LONGLONG hwnd, VARIANT *x, VARIANT *y, LONG *nret) {
    try {
        x->vt = VT_I4;
        y->vt = VT_I4;
        obj.ScreenToClient(static_cast<LONG_PTR>(hwnd), &x->lVal, &y->lVal, nret);

        return S_OK;

    } catch (...) {
        if (nret) {
            *nret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::SendPaste(LONGLONG hwnd, LONG *nret) {
    try {
        obj.SendPaste(static_cast<LONG_PTR>(hwnd), nret);

        return S_OK;

    } catch (...) {
        if (nret) {
            *nret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::SetClientSize(LONGLONG hwnd, LONG width, LONG hight, LONG *nret) {
    try {
        obj.SetClientSize(static_cast<LONG_PTR>(hwnd), width, hight, nret);

        return S_OK;

    } catch (...) {
        if (nret) {
            *nret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::SetWindowState(LONGLONG hwnd, LONG flag, LONG *nret) {
    try {
        obj.SetWindowState(static_cast<LONG_PTR>(hwnd), flag, nret);

        return S_OK;

    } catch (...) {
        if (nret) {
            *nret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::SetWindowSize(LONGLONG hwnd, LONG width, LONG height, LONG *nret) {
    try {
        obj.SetWindowSize(static_cast<LONG_PTR>(hwnd), width, height, nret);

        return S_OK;

    } catch (...) {
        if (nret) {
            *nret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::SetWindowText(LONGLONG hwnd, BSTR title, LONG *nret) {
    try {
        title = SafeBstr(title);

        obj.SetWindowText(static_cast<LONG_PTR>(hwnd), title, nret);

        return S_OK;

    } catch (...) {
        if (nret) {
            *nret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::SetWindowTransparent(LONGLONG hwnd, LONG trans, LONG *nret) {
    try {
        obj.SetWindowTransparent(static_cast<LONG_PTR>(hwnd), trans, nret);

        return S_OK;

    } catch (...) {
        if (nret) {
            *nret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::SendString(LONGLONG hwnd, BSTR str, LONG *ret) {
    try {
        str = SafeBstr(str);

        obj.SendString(static_cast<LONG_PTR>(hwnd), str, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::SendStringIme(LONGLONG hwnd, BSTR str, LONG *ret) {
    try {
        str = SafeBstr(str);

        obj.SendStringIme(static_cast<LONG_PTR>(hwnd), str, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::RunApp(BSTR cmdline, LONG mode, ULONG *pid, LONG *ret) {
    try {
        cmdline = SafeBstr(cmdline);

        obj.RunApp(cmdline, mode, pid, ret);

        return S_OK;

    } catch (...) {
        if (pid) {
            *pid = 0;
        }
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::LayoutWindows(BSTR hwnds, LONG layout_type, LONG columns, LONG start_x, LONG start_y,
                                         LONG gap_x, LONG gap_y, LONG size_mode, LONG window_width, LONG window_height,
                                         LONG anchor_mode, LONG *ret) {
    try {
        hwnds = SafeBstr(hwnds);

        obj.LayoutWindows(hwnds, layout_type, columns, start_x, start_y, gap_x, gap_y, size_mode, window_width,
                          window_height, anchor_mode, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::WinExec(BSTR cmdline, LONG cmdshow, LONG *ret) {
    try {
        cmdline = SafeBstr(cmdline);

        obj.WinExec(cmdline, cmdshow, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetCmdStr(BSTR cmd, LONG millseconds, BSTR *retstr) {
    try {
        cmd = SafeBstr(cmd);

        wstring s;
        obj.GetCmdStr(cmd, millseconds, s);

        return CopyOutBstr(retstr, s);

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::SetClipboard(BSTR str, LONG *ret) {
    try {
        str = SafeBstr(str);

        obj.SetClipboard(str, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetClipboard(BSTR *ret) {
    try {
        wstring s;
        obj.GetClipboard(s);

        return CopyOutBstr(ret, s);

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::Delay(LONG mis, LONG *ret) {
    try {
        obj.Delay(mis, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::Delays(LONG mis_min, LONG mis_max, LONG *ret) {
    try {
        obj.Delays(mis_min, mis_max, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::BindWindow(LONGLONG hwnd, BSTR display, BSTR mouse, BSTR keypad, LONG mode, LONG *ret) {
    try {
        display = SafeBstr(display);
        keypad = SafeBstr(keypad);
        mouse = SafeBstr(mouse);

        obj.BindWindow(static_cast<LONG_PTR>(hwnd), display, mouse, keypad, mode, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::BindWindowEx(LONGLONG display_hwnd, LONGLONG input_hwnd, BSTR display, BSTR mouse,
                                        BSTR keypad, LONG mode, LONG *ret) {
    try {
        display = SafeBstr(display);
        keypad = SafeBstr(keypad);
        mouse = SafeBstr(mouse);

        obj.BindWindowEx(static_cast<LONG_PTR>(display_hwnd), static_cast<LONG_PTR>(input_hwnd), display, mouse, keypad,
                         mode, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

OP_COM_FORWARD(UnBindWindow)

STDMETHODIMP OpAutomation::LockInput(LONG lock, LONG *ret) {
    try {
        obj.LockInput(lock, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetBindWindow(LONGLONG *ret) {
    try {
        LONG_PTR hwnd = 0;
        obj.GetBindWindow(&hwnd);
        return SetOutValue(ret, hwnd);

    } catch (...) {
        return E_UNEXPECTED;
    }
}

OP_COM_FORWARD(IsBind)

STDMETHODIMP OpAutomation::GetCursorPos(VARIANT *x, VARIANT *y, LONG *ret) {
    try {
        x->vt = y->vt = VT_I4;
        obj.GetCursorPos(&x->lVal, &y->lVal, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetCursorShape(BSTR *ret) {
    try {
        if (!ret)
            return E_POINTER;

        std::wstring value;
        obj.GetCursorShape(value);
        return CopyOutBstr(ret, value);

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::MoveR(LONG x, LONG y, LONG *ret) {
    try {
        obj.MoveR(x, y, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}
// 把鼠标移动到目的点(x,y)
STDMETHODIMP OpAutomation::MoveTo(LONG x, LONG y, LONG *ret) {
    try {
        obj.MoveTo(x, y, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::MoveToEx(LONG x, LONG y, LONG w, LONG h, BSTR *ret) {
    try {
        if (!ret)
            return E_POINTER;

        std::wstring s;
        obj.MoveToEx(x, y, w, h, s);
        return CopyOutBstr(ret, s);

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::MoveToSmooth(LONG x, LONG y, LONG duration, LONG *ret) {
    try {
        obj.MoveToSmooth(x, y, duration, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::MoveToExSmooth(LONG x, LONG y, LONG w, LONG h, LONG duration, BSTR *ret) {
    try {
        if (!ret)
            return E_POINTER;

        std::wstring s;
        obj.MoveToExSmooth(x, y, w, h, duration, s);
        return CopyOutBstr(ret, s);

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::MovePath(BSTR path, LONG duration, LONG *ret) {
    try {
        path = SafeBstr(path);

        obj.MovePath(path, duration, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::DragPath(BSTR path, LONG duration, LONG *ret) {
    try {
        path = SafeBstr(path);

        obj.DragPath(path, duration, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::SetMouseTrajectory(LONG mode, LONG min_duration, LONG max_duration, LONG jitter,
                                              LONG start_delay, LONG end_delay, LONG *ret) {
    try {
        obj.SetMouseTrajectory(mode, min_duration, max_duration, jitter, start_delay, end_delay, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

OP_COM_FORWARD(LeftClick)

OP_COM_FORWARD(LeftDoubleClick)

OP_COM_FORWARD(LeftDown)

OP_COM_FORWARD(LeftUp)

OP_COM_FORWARD(MiddleClick)

OP_COM_FORWARD(MiddleDoubleClick)

OP_COM_FORWARD(MiddleDown)

OP_COM_FORWARD(MiddleUp)

OP_COM_FORWARD(RightClick)

OP_COM_FORWARD(RightDoubleClick)

OP_COM_FORWARD(RightDown)

OP_COM_FORWARD(RightUp)

OP_COM_FORWARD(XButton1Click)

OP_COM_FORWARD(XButton1DoubleClick)

OP_COM_FORWARD(XButton1Down)

OP_COM_FORWARD(XButton1Up)

OP_COM_FORWARD(XButton2Click)

OP_COM_FORWARD(XButton2DoubleClick)

OP_COM_FORWARD(XButton2Down)

OP_COM_FORWARD(XButton2Up)

OP_COM_FORWARD(WheelDown)

OP_COM_FORWARD(WheelUp)

STDMETHODIMP OpAutomation::Wheel(LONG delta, LONG *ret) {
    try {
        obj.Wheel(delta, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::HWheel(LONG delta, LONG *ret) {
    try {
        obj.HWheel(delta, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::SetMouseDelay(BSTR type, LONG delay, LONG *ret) {
    try {
        type = SafeBstr(type);

        obj.SetMouseDelay(type, delay, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetKeyState(LONG vk_code, LONG *ret) {
    try {
        obj.GetKeyState(vk_code, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::KeyDown(LONG vk_code, LONG *ret) {
    try {
        obj.KeyDown(vk_code, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::KeyDownChar(BSTR vk_code, LONG *ret) {
    try {
        vk_code = SafeBstr(vk_code);

        obj.KeyDownChar(vk_code, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::KeyUp(LONG vk_code, LONG *ret) {
    try {
        obj.KeyUp(vk_code, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::KeyUpChar(BSTR vk_code, LONG *ret) {
    try {
        vk_code = SafeBstr(vk_code);

        obj.KeyUpChar(vk_code, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::WaitKey(LONG vk_code, LONG time_out, LONG *ret) {
    try {
        obj.WaitKey(vk_code, time_out, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::KeyPress(LONG vk_code, LONG *ret) {
    try {

        obj.KeyPress(vk_code, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::KeyPressChar(BSTR vk_code, LONG *ret) {
    try {
        vk_code = SafeBstr(vk_code);

        obj.KeyPressChar(vk_code, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::SetKeypadDelay(BSTR type, LONG delay, LONG *ret) {
    try {
        type = SafeBstr(type);

        obj.SetKeypadDelay(type, delay, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}
STDMETHODIMP OpAutomation::KeyPressStr(BSTR key_str, LONG delay, LONG *ret) {
    try {
        key_str = SafeBstr(key_str);

        obj.KeyPressStr(key_str, delay, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

// 抓取指定区域(x1, y1, x2, y2)的图像, 保存为file
STDMETHODIMP OpAutomation::Capture(LONG x1, LONG y1, LONG x2, LONG y2, BSTR file_name, LONG *ret) {
    try {
        file_name = SafeBstr(file_name);

        obj.Capture(x1, y1, x2, y2, file_name, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}
// 比较指定坐标点(x,y)的颜色
STDMETHODIMP OpAutomation::CmpColor(LONG x, LONG y, BSTR color, DOUBLE sim, LONG *ret) {
    try {
        color = SafeBstr(color);

        obj.CmpColor(x, y, color, sim, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}
// 查找指定区域内的颜色
STDMETHODIMP OpAutomation::FindColor(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG dir, VARIANT *x,
                                     VARIANT *y, LONG *ret) {
    color = SafeBstr(color);

    if (!x || !y)
        return E_POINTER;
    try {
        x->vt = y->vt = VT_I4;

        obj.FindColor(x1, y1, x2, y2, color, sim, dir, &x->lVal, &y->lVal, ret);

        return S_OK;
    } catch (...) {
        return E_UNEXPECTED;
    }
}
// 查找指定区域内的所有颜色
STDMETHODIMP OpAutomation::FindColorEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG dir,
                                       BSTR *retstr) {
    try {
        color = SafeBstr(color);

        wstring s;
        obj.FindColorEx(x1, y1, x2, y2, color, sim, dir, s);

        CopyOutBstr(retstr, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}
// 查找指定区域内的颜色数量
STDMETHODIMP OpAutomation::GetColorNum(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG *ret) {
    try {
        color = SafeBstr(color);

        wstring s;
        obj.GetColorNum(x1, y1, x2, y2, color, sim, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}
// 根据指定的多点查找颜色坐标
STDMETHODIMP OpAutomation::FindMultiColor(LONG x1, LONG y1, LONG x2, LONG y2, BSTR first_color, BSTR offset_color,
                                          DOUBLE sim, LONG dir, VARIANT *x, VARIANT *y, LONG *ret) {
    try {
        first_color = SafeBstr(first_color);
        offset_color = SafeBstr(offset_color);

        if (!x || !y || !ret)
            return E_POINTER;

        SetOutValue(ret, 0L);
        x->vt = y->vt = VT_I4;
        obj.FindMultiColor(x1, y1, x2, y2, first_color, offset_color, sim, dir, &x->lVal, &y->lVal, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}
// 根据指定的多点查找所有颜色坐标
STDMETHODIMP OpAutomation::FindMultiColorEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR first_color, BSTR offset_color,
                                            DOUBLE sim, LONG dir, BSTR *retstr) {
    try {
        first_color = SafeBstr(first_color);
        offset_color = SafeBstr(offset_color);

        wstring s;
        obj.FindMultiColorEx(x1, y1, x2, y2, first_color, offset_color, sim, dir, s);

        CopyOutBstr(retstr, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}
// 查找指定区域内的图片
STDMETHODIMP OpAutomation::FindPic(LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim,
                                   LONG dir, VARIANT *x, VARIANT *y, LONG *ret) {
    delta_color = SafeBstr(delta_color);
    files = SafeBstr(files);

    if (!x || !y)
        return E_POINTER;
    try {
        x->vt = y->vt = VT_I4;
        obj.FindPic(x1, y1, x2, y2, files, delta_color, sim, dir, &x->lVal, &y->lVal, ret);

        return S_OK;
    } catch (...) {
        return E_UNEXPECTED;
    }
}
// 查找多个图片
STDMETHODIMP OpAutomation::FindPicEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim,
                                     LONG dir, BSTR *retstr) {
    delta_color = SafeBstr(delta_color);
    files = SafeBstr(files);

    try {
        wstring s;
        obj.FindPicEx(x1, y1, x2, y2, files, delta_color, sim, dir, s);

        return CopyOutBstr(retstr, s);
    } catch (...) {
        return E_UNEXPECTED;
    }
}
// 这个函数可以查找多个图片, 并且返回所有找到的图像的坐标.此函数同FindPicEx.只是返回值不同.(file1, x, y | file2, x, y |
// ...)
STDMETHODIMP OpAutomation::FindPicExS(LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim,
                                      LONG dir, BSTR *retstr) {
    try {
        delta_color = SafeBstr(delta_color);
        files = SafeBstr(files);

        wstring s;
        obj.FindPicExS(x1, y1, x2, y2, files, delta_color, sim, dir, s);
        CopyOutBstr(retstr, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}
// 查找指定区域内的颜色块,颜色格式"RRGGBB-DRDGDB",注意,和按键的颜色格式相反
STDMETHODIMP OpAutomation::FindColorBlock(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG count,
                                          LONG height, LONG width, VARIANT *x, VARIANT *y, LONG *ret) {
    try {
        color = SafeBstr(color);

        x->vt = y->vt = VT_I4;
        obj.FindColorBlock(x1, y1, x2, y2, color, sim, count, height, width, &x->lVal, &y->lVal, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}
// 查找指定区域内的所有颜色块, 颜色格式"RRGGBB-DRDGDB", 注意, 和按键的颜色格式相反
STDMETHODIMP OpAutomation::FindColorBlockEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG count,
                                            LONG height, LONG width, BSTR *retstr) {
    try {
        color = SafeBstr(color);

        std::wstring s;
        obj.FindColorBlockEx(x1, y1, x2, y2, color, sim, count, height, width, s);
        CopyOutBstr(retstr, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}
// 获取(x,y)的颜色
STDMETHODIMP OpAutomation::GetColor(LONG x, LONG y, BSTR *ret) {
    // GetColor -> requestCapture -> Image::create：这条链中间没有别的 catch，
    // 是本仓库少数"异常真能穿到 COM 边界"的路径之一。
    try {
        wstring s;
        obj.GetColor(x, y, s);

        CopyOutBstr(ret, s);
        return S_OK;
    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::SetDisplayInput(BSTR mode, LONG *ret) {
    try {
        mode = SafeBstr(mode);

        obj.SetDisplayInput(mode, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::LoadPic(BSTR pic_name, LONG *ret) {
    pic_name = SafeBstr(pic_name);

    // LoadPic 会走 Image::create：这条路径是少数会抛 C++ 异常的地方，
    // 异常绝不允许穿过 COM ABI（否则宿主进程直接终止）。
    try {
        obj.LoadPic(pic_name, ret);
        return S_OK;
    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::FreePic(BSTR pic_name, LONG *ret) {
    try {
        pic_name = SafeBstr(pic_name);

        obj.FreePic(pic_name, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::LoadMemPic(BSTR pic_name, long long data, LONG size, LONG *ret) {
    pic_name = SafeBstr(pic_name);

    // 同 LoadPic：解析内存图会走到 Image::create，异常必须在 COM 边界兜住。
    try {
        obj.LoadMemPic(pic_name, (void *)data, size, ret);
        return S_OK;
    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetPicSize(BSTR pic_name, VARIANT *width, VARIANT *height, LONG *ret) {
    try {
        pic_name = SafeBstr(pic_name);

        if (!width || !height)
            return E_POINTER;
        width->vt = height->vt = VT_I4;
        obj.GetPicSize(pic_name, &width->lVal, &height->lVal, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

// 返回截图内存地址，x64 下不能用 LONG，否则高位会被截断。
STDMETHODIMP OpAutomation::GetScreenData(LONG x1, LONG y1, LONG x2, LONG y2, LONGLONG *ret) {
    if (!ret)
        return E_POINTER;
    SetOutValue(ret, 0LL);
    // 与 GetScreenDataBmp 同族：这条路径会整块分配/拷贝屏幕像素，是少数可能抛
    // （std::bad_alloc）的地方之一，异常不许穿过 COM ABI。
    try {
        size_t data_ = 0;
        long capture_ret = 0;
        obj.GetScreenData(x1, y1, x2, y2, &data_, &capture_ret);
        if (!capture_ret)
            return S_OK;
        return SetOutValue(ret, static_cast<LONGLONG>(data_));
    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetScreenDataBmp(LONG x1, LONG y1, LONG x2, LONG y2, VARIANT *data, VARIANT *size,
                                            LONG *ret) {
    // 输出槽原来是直接 data->vt = …，宿主不传某个槽时会空指针解引用。
    if (!data || !size)
        return E_POINTER;
#if OP64
    data->vt = VT_I8;
    size->vt = VT_I8;
    data->llVal = 0;
    size->llVal = 0;
#else
    data->vt = VT_I4;
    size->vt = VT_I4;
    data->lVal = 0;
    size->lVal = 0;
#endif
    try {
        size_t data_ = 0;
        long size_ = 0;

        obj.GetScreenDataBmp(x1, y1, x2, y2, &data_, &size_, ret);
#if OP64
        data->llVal = (long long)data_;
        size->llVal = size_;
#else
        data->lVal = (long)data_;
        size->lVal = size_;
#endif
        // size->lVal = bfh.bfSize;

        return S_OK;
    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetScreenFrameInfo(LONG *frame_id, LONG *time) {
    try {
        obj.GetScreenFrameInfo(frame_id, time);
        return S_OK;

    } catch (...) {
        if (time) {
            *time = 0;
        }
        return E_UNEXPECTED;
    }
}

// 根据通配符获取文件集合. 方便用于FindPic和FindPicEx
STDMETHODIMP OpAutomation::MatchPicName(BSTR pic_name, BSTR *ret) {
    try {
        pic_name = SafeBstr(pic_name);

        wstring s;
        obj.MatchPicName(pic_name, s);
        CopyOutBstr(ret, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

// 设置字库文件
STDMETHODIMP OpAutomation::SetDict(LONG idx, BSTR file_name, LONG *ret) {
    try {
        file_name = SafeBstr(file_name);

        obj.SetDict(idx, file_name, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetDict(LONG idx, LONG font_index, BSTR *retstr) {
    try {
        std::wstring s;
        obj.GetDict(idx, font_index, s);
        CopyOutBstr(retstr, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

// 从内存字节设置全局字库槽，支持 OP 二进制 .dict 和文本字库内容。
STDMETHODIMP OpAutomation::SetMemDict(LONG idx, SAFEARRAY *data, LONG *ret) {
    try {
        if (!ret)
            return E_POINTER;

        *ret = 0;
        if (!data)
            return S_OK;

        if (SafeArrayGetDim(data) != 1)
            return S_OK;

        VARTYPE vt = VT_EMPTY;
        if (FAILED(SafeArrayGetVartype(data, &vt)) || vt != VT_UI1)
            return S_OK;

        LONG lower = 0;
        LONG upper = -1;
        if (FAILED(SafeArrayGetLBound(data, 1, &lower)) || FAILED(SafeArrayGetUBound(data, 1, &upper)) || upper < lower)
            return S_OK;

        void *bytes = nullptr;
        const HRESULT hr = SafeArrayAccessData(data, &bytes);
        if (FAILED(hr))
            return hr;

        const LONG byte_count = upper - lower + 1;
        obj.SetMemDict(idx, bytes, byte_count, ret);
        SafeArrayUnaccessData(data);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

// 使用哪个字库文件进行识别
STDMETHODIMP OpAutomation::UseDict(LONG idx, LONG *ret) {
    try {
        obj.UseDict(idx, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

// 给指定的字库中添加一条字库信息
STDMETHODIMP OpAutomation::AddDict(LONG idx, BSTR dict_info, LONG *ret) {
    try {
        dict_info = SafeBstr(dict_info);

        obj.AddDict(idx, dict_info, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::SaveDict(LONG idx, BSTR file_name, LONG *ret) {
    try {
        file_name = SafeBstr(file_name);

        obj.SaveDict(idx, file_name, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

// 清空指定的字库
STDMETHODIMP OpAutomation::ClearDict(LONG idx, LONG *ret) {
    try {
        obj.ClearDict(idx, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

// 获取指定的字库中的字符数量
STDMETHODIMP OpAutomation::GetDictCount(LONG idx, LONG *ret) {
    try {
        obj.GetDictCount(idx, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

// 获取当前使用的字库序号
OP_COM_FORWARD(GetNowDict)

// 根据指定的范围,以及指定的颜色描述，提取点阵信息，类似于大漠工具里的单独提取
STDMETHODIMP OpAutomation::FetchWord(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, BSTR word, BSTR *ret_str) {
    try {
        color = SafeBstr(color);
        word = SafeBstr(word);

        wstring s;
        obj.FetchWord(x1, y1, x2, y2, color, word, s);
        CopyOutBstr(ret_str, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::FetchWordEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR word,
                                       BSTR *ret_str) {
    try {
        color = SafeBstr(color);
        word = SafeBstr(word);

        if (!ret_str)
            return E_POINTER;

        wstring s;
        obj.FetchWordEx(x1, y1, x2, y2, color, sim, word, s);
        return CopyOutBstr(ret_str, s);

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::ExtractWordRects(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG min_word_h,
                                            BSTR *ret_str) {
    try {
        color = SafeBstr(color);

        if (!ret_str)
            return E_POINTER;

        wstring s;
        obj.ExtractWordRects(x1, y1, x2, y2, color, sim, min_word_h, s);
        return CopyOutBstr(ret_str, s);

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::ExtractWordRectsEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim,
                                              LONG min_word_w, LONG min_word_h, LONG padding, BSTR *ret_str) {
    try {
        color = SafeBstr(color);

        if (!ret_str)
            return E_POINTER;

        wstring s;
        obj.ExtractWordRectsEx(x1, y1, x2, y2, color, sim, min_word_w, min_word_h, padding, s);
        return CopyOutBstr(ret_str, s);

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::FetchWords(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR words,
                                      LONG min_word_h, BSTR *ret_str) {
    try {
        color = SafeBstr(color);
        words = SafeBstr(words);

        if (!ret_str)
            return E_POINTER;

        wstring s;
        obj.FetchWords(x1, y1, x2, y2, color, sim, words, min_word_h, s);
        return CopyOutBstr(ret_str, s);

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::FetchWordsEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR words,
                                        LONG min_word_w, LONG min_word_h, LONG padding, BSTR *ret_str) {
    try {
        color = SafeBstr(color);
        words = SafeBstr(words);

        if (!ret_str)
            return E_POINTER;

        wstring s;
        obj.FetchWordsEx(x1, y1, x2, y2, color, sim, words, min_word_w, min_word_h, padding, s);
        return CopyOutBstr(ret_str, s);

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::FetchWordsByRects(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR words,
                                             BSTR rects, BSTR *ret_str) {
    try {
        color = SafeBstr(color);
        rects = SafeBstr(rects);
        words = SafeBstr(words);

        if (!ret_str)
            return E_POINTER;

        wstring s;
        obj.FetchWordsByRects(x1, y1, x2, y2, color, sim, words, rects, s);
        return CopyOutBstr(ret_str, s);

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetBinaryPreview(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR *ret_str,
                                            LONG *ret) {
    try {
        color = SafeBstr(color);

        if (!ret_str || !ret)
            return E_POINTER;

        wstring s;
        obj.GetBinaryPreview(x1, y1, x2, y2, color, sim, s, ret);
        return CopyOutBstr(ret_str, s);

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetWordPreview(BSTR dict_info, BSTR *ret_str, LONG *ret) {
    try {
        dict_info = SafeBstr(dict_info);

        if (!ret_str || !ret)
            return E_POINTER;

        wstring s;
        obj.GetWordPreview(dict_info, s, ret);
        return CopyOutBstr(ret_str, s);

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CheckWordDict(BSTR dict_info, BSTR *ret_str, LONG *ret) {
    try {
        dict_info = SafeBstr(dict_info);

        if (!ret_str || !ret)
            return E_POINTER;

        wstring s;
        obj.CheckWordDict(dict_info, s, ret);
        return CopyOutBstr(ret_str, s);

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::NormalizeWordDict(BSTR dict_info, BSTR *ret_str, LONG *ret) {
    try {
        dict_info = SafeBstr(dict_info);

        if (!ret_str || !ret)
            return E_POINTER;

        wstring s;
        obj.NormalizeWordDict(dict_info, s, ret);
        return CopyOutBstr(ret_str, s);

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::RenameWordDict(BSTR dict_info, BSTR words, BSTR *ret_str, LONG *ret) {
    try {
        dict_info = SafeBstr(dict_info);
        words = SafeBstr(words);

        if (!ret_str || !ret)
            return E_POINTER;

        wstring s;
        obj.RenameWordDict(dict_info, words, s, ret);
        return CopyOutBstr(ret_str, s);

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::SetBinaryPreprocess(LONG mode, LONG isolated_threshold, LONG min_component_area,
                                               LONG bridge_gap, LONG *ret) {
    try {
        if (!ret)
            return E_POINTER;

        obj.SetBinaryPreprocess(mode, isolated_threshold, min_component_area, bridge_gap, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::GetBinaryPreprocess(LONG *mode, LONG *isolated_threshold, LONG *min_component_area,
                                               LONG *bridge_gap, LONG *ret) {
    try {
        if (!mode || !isolated_threshold || !min_component_area || !bridge_gap || !ret)
            return E_POINTER;

        obj.GetBinaryPreprocess(mode, isolated_threshold, min_component_area, bridge_gap, ret);
        return S_OK;

    } catch (...) {
        if (isolated_threshold) {
            *isolated_threshold = 0;
        }
        if (min_component_area) {
            *min_component_area = 0;
        }
        if (bridge_gap) {
            *bridge_gap = 0;
        }
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

// 识别这个范围内所有满足条件的词组，这个识别函数不会用到字库. 只是识别大概形状的位置
STDMETHODIMP OpAutomation::GetWordsNoDict(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, BSTR *ret_str) {
    try {
        color = SafeBstr(color);

        if (!ret_str)
            return E_POINTER;

        wstring s;
        obj.GetWordsNoDict(x1, y1, x2, y2, color, s);
        return CopyOutBstr(ret_str, s);

    } catch (...) {
        return E_UNEXPECTED;
    }
}
// 在使用GetWords进行词组识别以后,可以用此接口进行识别词组数量的计算
STDMETHODIMP OpAutomation::GetWordResultCount(BSTR result, LONG *ret) {
    try {
        result = SafeBstr(result);

        if (!ret)
            return E_POINTER;

        SetOutValue(ret, 0L);
        obj.GetWordResultCount(result, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}
// 在使用GetWords进行词组识别以后,可以用此接口进行识别各个词组的坐标
STDMETHODIMP OpAutomation::GetWordResultPos(BSTR result, LONG index, VARIANT *x, VARIANT *y, LONG *ret) {
    try {
        result = SafeBstr(result);

        if (!x || !y || !ret)
            return E_POINTER;

        ::VariantInit(x);
        ::VariantInit(y);
        x->vt = y->vt = VT_I4;
        x->lVal = 0;
        y->lVal = 0;
        SetOutValue(ret, 0L);
        obj.GetWordResultPos(result, index, &x->lVal, &y->lVal, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}
// 在使用GetWords进行词组识别以后,可以用此接口进行识别各个词组的内容
STDMETHODIMP OpAutomation::GetWordResultStr(BSTR result, LONG index, BSTR *ret_str) {
    try {
        result = SafeBstr(result);

        if (!ret_str)
            return E_POINTER;

        wstring s;
        obj.GetWordResultStr(result, index, s);
        return CopyOutBstr(ret_str, s);

    } catch (...) {
        return E_UNEXPECTED;
    }
}

// 识别屏幕范围(x1,y1,x2,y2)内符合color_format的字符串,并且相似度为sim,sim取值范围(0.1-1.0),
STDMETHODIMP OpAutomation::Ocr(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR *ret_str) {
    try {
        color = SafeBstr(color);

        wstring s;
        obj.Ocr(x1, y1, x2, y2, color, sim, s);

        CopyOutBstr(ret_str, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}
// 回识别到的字符串，以及每个字符的坐标.
STDMETHODIMP OpAutomation::OcrEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR *ret_str) {
    try {
        color = SafeBstr(color);

        wstring s;
        obj.OcrEx(x1, y1, x2, y2, color, sim, s);

        CopyOutBstr(ret_str, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}
// 在屏幕范围(x1,y1,x2,y2)内,查找string(可以是任意个字符串的组合),并返回符合color_format的坐标位置
STDMETHODIMP OpAutomation::FindStr(LONG x1, LONG y1, LONG x2, LONG y2, BSTR strs, BSTR color, DOUBLE sim, VARIANT *retx,
                                   VARIANT *rety, LONG *ret) {
    try {
        color = SafeBstr(color);
        strs = SafeBstr(strs);

        retx->vt = rety->vt = VT_INT;
        obj.FindStr(x1, y1, x2, y2, strs, color, sim, &retx->lVal, &rety->lVal, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}
// 返回符合color_format的所有坐标位置
STDMETHODIMP OpAutomation::FindStrEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR strs, BSTR color, DOUBLE sim,
                                     BSTR *retstr) {
    try {
        color = SafeBstr(color);
        strs = SafeBstr(strs);

        wstring s;
        obj.FindStrEx(x1, y1, x2, y2, strs, color, sim, s);

        CopyOutBstr(retstr, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::OcrAuto(LONG x1, LONG y1, LONG x2, LONG y2, DOUBLE sim, BSTR *retstr) {
    try {
        wstring s;
        obj.OcrAuto(x1, y1, x2, y2, sim, s);

        CopyOutBstr(retstr, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

// 从文件中识别图片
STDMETHODIMP OpAutomation::OcrFromFile(BSTR file_name, BSTR color_format, DOUBLE sim, BSTR *retstr) {
    try {
        color_format = SafeBstr(color_format);
        file_name = SafeBstr(file_name);

        wstring s;
        obj.OcrFromFile(file_name, color_format, sim, s);

        CopyOutBstr(retstr, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}
// 从文件中识别图片,无需指定颜色
STDMETHODIMP OpAutomation::OcrAutoFromFile(BSTR file_name, DOUBLE sim, BSTR *retstr) {
    try {
        file_name = SafeBstr(file_name);

        wstring s;
        obj.OcrAutoFromFile(file_name, sim, s);

        CopyOutBstr(retstr, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::FindLine(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR *retstr) {
    try {
        color = SafeBstr(color);

        wstring s;
        obj.FindLine(x1, y1, x2, y2, color, sim, s);

        CopyOutBstr(retstr, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::SetOcrEngine(BSTR path_of_engine, BSTR dll_name, BSTR argv, LONG *ret) {
    try {
        argv = SafeBstr(argv);
        dll_name = SafeBstr(dll_name);
        path_of_engine = SafeBstr(path_of_engine);

        return SetOutValue(ret, obj.SetOcrEngine(path_of_engine, dll_name, argv));

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::SetYoloEngine(BSTR path_of_engine, BSTR dll_name, BSTR argv, LONG *ret) {
    try {
        argv = SafeBstr(argv);
        dll_name = SafeBstr(dll_name);
        path_of_engine = SafeBstr(path_of_engine);

        return SetOutValue(ret, obj.SetYoloEngine(path_of_engine, dll_name, argv));

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::YoloDetect(LONG x1, LONG y1, LONG x2, LONG y2, DOUBLE conf, DOUBLE iou, BSTR *retjson,
                                      LONG *ret) {
    try {
        if (!ret)
            return E_POINTER;
        std::wstring s;
        obj.YoloDetect(x1, y1, x2, y2, conf, iou, s, ret);
        return CopyOutBstr(retjson, s);

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::YoloDetectFromFile(BSTR file_name, DOUBLE conf, DOUBLE iou, BSTR *retjson, LONG *ret) {
    try {
        file_name = SafeBstr(file_name);

        if (!ret)
            return E_POINTER;
        std::wstring s;
        obj.YoloDetectFromFile(file_name, conf, iou, s, ret);
        return CopyOutBstr(retjson, s);

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::WriteData(LONGLONG hwnd, BSTR address, BSTR data, LONG size, LONG *ret) {
    try {
        address = SafeBstr(address);
        data = SafeBstr(data);

        obj.WriteData(static_cast<LONG_PTR>(hwnd), address, data, size, ret);

        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::ReadData(LONGLONG hwnd, BSTR address, LONG size, BSTR *retstr) {
    try {
        address = SafeBstr(address);

        wstring s;
        obj.ReadData(static_cast<LONG_PTR>(hwnd), address, size, s);

        CopyOutBstr(retstr, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::ReadInt(LONGLONG hwnd, BSTR address, LONG type, LONGLONG *ret) {
    try {
        address = SafeBstr(address);

        if (!ret)
            return E_POINTER;

        int64_t value = 0;
        obj.ReadInt(static_cast<LONG_PTR>(hwnd), address, type, &value);
        return SetOutValue(ret, value);

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::WriteInt(LONGLONG hwnd, BSTR address, LONG type, LONGLONG value, LONG *ret) {
    try {
        address = SafeBstr(address);

        obj.WriteInt(static_cast<LONG_PTR>(hwnd), address, type, static_cast<int64_t>(value), ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

namespace {

// 批量接口的分隔符：'|' 不在地址表达式（十六进制、四则运算、<module>、[addr+off]）里出现。
void split_pipe(const std::wstring &text, std::vector<std::wstring> &out) {
    out.clear();
    std::wstring item;
    for (const wchar_t ch : text) {
        if (ch == L'|') {
            out.push_back(item);
            item.clear();
        } else if (ch != L' ' && ch != L'\t' && ch != L'\r' && ch != L'\n') {
            item.push_back(ch);
        }
    }
    out.push_back(item);
}

} // namespace

STDMETHODIMP OpAutomation::ReadIntBatch(LONGLONG hwnd, BSTR addresses, LONG type, BSTR *ret) {
    try {
        if (!ret)
            return E_POINTER;
        *ret = nullptr;

        addresses = SafeBstr(addresses);
        std::vector<std::wstring> list;
        split_pipe(addresses, list);
        if (list.empty()) {
            *ret = ::SysAllocString(L"");
            return S_OK;
        }

        std::vector<const wchar_t *> raw;
        raw.reserve(list.size());
        for (const auto &item : list)
            raw.push_back(item.c_str());

        std::vector<int64_t> values(list.size(), 0);
        // std::vector<bool> 没有 data()，这里用裸数组拿 bool* 交给下层。
        std::unique_ptr<bool[]> ok(new bool[list.size()]());
        long done = 0;
        obj.ReadIntBatch(static_cast<LONG_PTR>(hwnd), raw.data(), static_cast<long>(raw.size()), type, values.data(),
                         &done, ok.get());

        std::wstring joined;
        for (size_t i = 0; i < values.size(); ++i) {
            if (i)
                joined += L'|';
            if (ok[i]) {
                wchar_t buf[32] = {};
                swprintf_s(buf, L"%lld", static_cast<long long>(values[i]));
                joined += buf;
            }
        }
        *ret = ::SysAllocString(joined.c_str());
        return *ret ? S_OK : E_OUTOFMEMORY;
    } catch (...) {
        if (ret)
            *ret = nullptr;
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::WriteIntBatch(LONGLONG hwnd, BSTR addresses, LONG type, BSTR values, LONG *ret) {
    try {
        if (ret)
            *ret = 0;

        addresses = SafeBstr(addresses);
        values = SafeBstr(values);
        std::vector<std::wstring> list;
        std::vector<std::wstring> vlist;
        split_pipe(addresses, list);
        split_pipe(values, vlist);

        const size_t count = list.size() < vlist.size() ? list.size() : vlist.size();
        if (count == 0)
            return S_OK;

        std::vector<const wchar_t *> raw;
        std::vector<int64_t> nums;
        raw.reserve(count);
        nums.reserve(count);
        // §72.51：wcstoll 不校验 endptr 的话，"abc" 会静默变成 0、"12abc" 截成 12 ——
        // 一个拼错的字符就能把任意值写进目标进程，而且返回值还是"成功"。
        // 现在要求整串都被消费掉，否则按"这一项不写"跳过（和空字段同语义，
        // 不搞坏地址连坐，见 §72.36：一项坏不连累整批）。
        static const wchar_t kSkipField[] = L"";
        for (size_t i = 0; i < count; ++i) {
            const std::wstring &text = vlist[i];
            if (text.empty()) {
                // 空字段表示"这一项不写"。
                raw.push_back(kSkipField);
                nums.push_back(0);
                continue;
            }
            const wchar_t *begin = text.c_str();
            wchar_t *end = nullptr;
            errno = 0;
            const long long parsed = ::wcstoll(begin, &end, 10);
            const bool whole = end != begin && end && *end == L'\0';
            if (!whole || errno == ERANGE) {
                raw.push_back(kSkipField);
                nums.push_back(0);
                continue;
            }
            raw.push_back(list[i].c_str());
            nums.push_back(static_cast<int64_t>(parsed));
        }

        long done = 0;
        obj.WriteIntBatch(static_cast<LONG_PTR>(hwnd), raw.data(), static_cast<long>(count), type, nums.data(), &done,
                          nullptr);
        if (ret)
            *ret = done;
        return S_OK;
    } catch (...) {
        if (ret)
            *ret = 0;
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::ReadFloat(LONGLONG hwnd, BSTR address, DOUBLE *ret) {
    try {
        address = SafeBstr(address);

        if (!ret)
            return E_POINTER;

        float value = 0.0f;
        obj.ReadFloat(static_cast<LONG_PTR>(hwnd), address, &value);
        return SetOutValue(ret, value);

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::WriteFloat(LONGLONG hwnd, BSTR address, DOUBLE value, LONG *ret) {
    try {
        address = SafeBstr(address);

        obj.WriteFloat(static_cast<LONG_PTR>(hwnd), address, static_cast<float>(value), ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::ReadDouble(LONGLONG hwnd, BSTR address, DOUBLE *ret) {
    try {
        address = SafeBstr(address);

        if (!ret)
            return E_POINTER;

        double value = 0.0;
        obj.ReadDouble(static_cast<LONG_PTR>(hwnd), address, &value);
        return SetOutValue(ret, value);

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::WriteDouble(LONGLONG hwnd, BSTR address, DOUBLE value, LONG *ret) {
    try {
        address = SafeBstr(address);

        obj.WriteDouble(static_cast<LONG_PTR>(hwnd), address, static_cast<double>(value), ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::ReadString(LONGLONG hwnd, BSTR address, LONG type, LONG len, BSTR *retstr) {
    try {
        address = SafeBstr(address);

        if (!retstr)
            return E_POINTER;

        wstring s;
        obj.ReadString(static_cast<LONG_PTR>(hwnd), address, type, len, s);
        return CopyOutBstr(retstr, s);

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::WriteString(LONGLONG hwnd, BSTR address, LONG type, BSTR value, LONG *ret) {
    try {
        address = SafeBstr(address);
        value = SafeBstr(value);

        obj.WriteString(static_cast<LONG_PTR>(hwnd), address, type, value, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvLoadTemplate(BSTR name, BSTR file_path, LONG *ret) {
    try {
        file_path = SafeBstr(file_path);
        name = SafeBstr(name);

        obj.CvLoadTemplate(name, file_path, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvLoadMaskedTemplate(BSTR name, BSTR template_path, BSTR mask_path, LONG *ret) {
    try {
        mask_path = SafeBstr(mask_path);
        name = SafeBstr(name);
        template_path = SafeBstr(template_path);

        obj.CvLoadMaskedTemplate(name, template_path, mask_path, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvRemoveTemplate(BSTR name, LONG *ret) {
    try {
        name = SafeBstr(name);

        obj.CvRemoveTemplate(name, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

OP_COM_FORWARD(CvRemoveAllTemplates)

STDMETHODIMP OpAutomation::CvHasTemplate(BSTR name, LONG *ret) {
    try {
        name = SafeBstr(name);

        obj.CvHasTemplate(name, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

OP_COM_FORWARD(CvGetTemplateCount)

STDMETHODIMP OpAutomation::CvGetAllTemplateNames(BSTR *retstr) {
    try {
        wstring s;
        obj.CvGetAllTemplateNames(s);
        CopyOutBstr(retstr, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvGetOpenCvVersion(BSTR *retstr) {
    try {
        wstring s;
        obj.CvGetOpenCvVersion(s);
        CopyOutBstr(retstr, s);
        return S_OK;

    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvLoadTemplateList(BSTR template_list, LONG *ret) {
    try {
        template_list = SafeBstr(template_list);

        obj.CvLoadTemplateList(template_list, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvToGray(BSTR src_file, BSTR dst_file, LONG *ret) {
    try {
        dst_file = SafeBstr(dst_file);
        src_file = SafeBstr(src_file);

        return RunCvRetOnly(ret, [&](LONG *out) { obj.CvToGray(src_file, dst_file, out); });

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvToBinary(BSTR src_file, BSTR dst_file, LONG *ret) {
    try {
        dst_file = SafeBstr(dst_file);
        src_file = SafeBstr(src_file);

        return RunCvRetOnly(ret, [&](LONG *out) { obj.CvToBinary(src_file, dst_file, out); });

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvToEdge(BSTR src_file, BSTR dst_file, LONG *ret) {
    try {
        dst_file = SafeBstr(dst_file);
        src_file = SafeBstr(src_file);

        return RunCvRetOnly(ret, [&](LONG *out) { obj.CvToEdge(src_file, dst_file, out); });

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvToOutline(BSTR src_file, BSTR dst_file, LONG *ret) {
    try {
        dst_file = SafeBstr(dst_file);
        src_file = SafeBstr(src_file);

        return RunCvRetOnly(ret, [&](LONG *out) { obj.CvToOutline(src_file, dst_file, out); });

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvDenoise(BSTR src_file, BSTR dst_file, LONG *ret) {
    try {
        dst_file = SafeBstr(dst_file);
        src_file = SafeBstr(src_file);

        return RunCvRetOnly(ret, [&](LONG *out) { obj.CvDenoise(src_file, dst_file, out); });

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvEqualize(BSTR src_file, BSTR dst_file, LONG *ret) {
    try {
        dst_file = SafeBstr(dst_file);
        src_file = SafeBstr(src_file);

        return RunCvRetOnly(ret, [&](LONG *out) { obj.CvEqualize(src_file, dst_file, out); });

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvCLAHE(BSTR src_file, BSTR dst_file, DOUBLE clip_limit, LONG tile_grid_size, LONG *ret) {
    try {
        dst_file = SafeBstr(dst_file);
        src_file = SafeBstr(src_file);

        return RunCvRetOnly(ret, [&](LONG *out) { obj.CvCLAHE(src_file, dst_file, clip_limit, tile_grid_size, out); });

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvBlur(BSTR src_file, BSTR dst_file, BSTR mode, LONG kernel_size, LONG *ret) {
    try {
        dst_file = SafeBstr(dst_file);
        mode = SafeBstr(mode);
        src_file = SafeBstr(src_file);

        return RunCvRetOnly(ret, [&](LONG *out) { obj.CvBlur(src_file, dst_file, mode, kernel_size, out); });

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvSharpen(BSTR src_file, BSTR dst_file, DOUBLE strength, LONG *ret) {
    try {
        dst_file = SafeBstr(dst_file);
        src_file = SafeBstr(src_file);

        return RunCvRetOnly(ret, [&](LONG *out) { obj.CvSharpen(src_file, dst_file, strength, out); });

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvCropValid(BSTR src_file, BSTR dst_file, LONG *ret) {
    try {
        dst_file = SafeBstr(dst_file);
        src_file = SafeBstr(src_file);

        return RunCvRetOnly(ret, [&](LONG *out) { obj.CvCropValid(src_file, dst_file, out); });

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvConnectedComponents(BSTR src_file, DOUBLE min_area, BSTR *retjson, LONG *ret) {
    try {
        src_file = SafeBstr(src_file);

        if (!retjson || !ret) {
            return E_POINTER;
        }

        SetOutValue(ret, 0L);

        std::wstring s;
        obj.CvConnectedComponents(src_file, min_area, s, ret);
        return CopyOutBstr(retjson, s);

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvFindContours(BSTR src_file, DOUBLE min_area, BSTR *retjson, LONG *ret) {
    try {
        src_file = SafeBstr(src_file);

        if (!retjson || !ret) {
            return E_POINTER;
        }

        SetOutValue(ret, 0L);

        std::wstring s;
        obj.CvFindContours(src_file, min_area, s, ret);
        return CopyOutBstr(retjson, s);

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvPreprocessPipeline(BSTR src_file, BSTR dst_file, BSTR pipeline, LONG *ret) {
    try {
        dst_file = SafeBstr(dst_file);
        pipeline = SafeBstr(pipeline);
        src_file = SafeBstr(src_file);

        return RunCvRetOnly(ret, [&](LONG *out) { obj.CvPreprocessPipeline(src_file, dst_file, pipeline, out); });

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvCrop(BSTR src_file, LONG x, LONG y, LONG width, LONG height, BSTR dst_file, LONG *ret) {
    try {
        dst_file = SafeBstr(dst_file);
        src_file = SafeBstr(src_file);

        obj.CvCrop(src_file, x, y, width, height, dst_file, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvResize(BSTR src_file, LONG width, LONG height, BSTR dst_file, LONG *ret) {
    try {
        dst_file = SafeBstr(dst_file);
        src_file = SafeBstr(src_file);

        obj.CvResize(src_file, width, height, dst_file, ret);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvThreshold(BSTR src_file, BSTR dst_file, DOUBLE threshold, DOUBLE max_value, BSTR mode,
                                       LONG *ret) {
    try {
        dst_file = SafeBstr(dst_file);
        mode = SafeBstr(mode);
        src_file = SafeBstr(src_file);

        return RunCvRetOnly(ret,
                            [&](LONG *out) { obj.CvThreshold(src_file, dst_file, threshold, max_value, mode, out); });

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvInRange(BSTR src_file, BSTR dst_file, BSTR color_space, BSTR lower, BSTR upper,
                                     LONG *ret) {
    try {
        color_space = SafeBstr(color_space);
        dst_file = SafeBstr(dst_file);
        lower = SafeBstr(lower);
        src_file = SafeBstr(src_file);
        upper = SafeBstr(upper);

        return RunCvRetOnly(ret, [&](LONG *out) { obj.CvInRange(src_file, dst_file, color_space, lower, upper, out); });

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvMorphology(BSTR src_file, BSTR dst_file, BSTR mode, LONG kernel_size, LONG iterations,
                                        LONG *ret) {
    try {
        dst_file = SafeBstr(dst_file);
        mode = SafeBstr(mode);
        src_file = SafeBstr(src_file);

        return RunCvRetOnly(
            ret, [&](LONG *out) { obj.CvMorphology(src_file, dst_file, mode, kernel_size, iterations, out); });

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvThin(BSTR src_file, BSTR dst_file, BSTR mode, LONG *ret) {
    try {
        dst_file = SafeBstr(dst_file);
        mode = SafeBstr(mode);
        src_file = SafeBstr(src_file);

        return RunCvRetOnly(ret, [&](LONG *out) { obj.CvThin(src_file, dst_file, mode, out); });

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvMatchTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                           DOUBLE threshold, LONG dir, LONG strip_mode, LONG method, LONG color_mode,
                                           BSTR *retjson, LONG *ret) {
    try {
        template_name = SafeBstr(template_name);

        wstring s;
        obj.CvMatchTemplate(x, y, width, height, template_name, threshold, dir, strip_mode, method, color_mode, s, ret);
        CopyOutBstr(retjson, s);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvMatchTemplateScale(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                                BSTR scales, DOUBLE threshold, LONG method, LONG color_mode,
                                                BSTR *retjson, LONG *ret) {
    try {
        scales = SafeBstr(scales);
        template_name = SafeBstr(template_name);

        wstring s;
        obj.CvMatchTemplateScale(x, y, width, height, template_name, scales, threshold, method, color_mode, s, ret);
        CopyOutBstr(retjson, s);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvMatchAnyTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_names,
                                              DOUBLE threshold, LONG dir, LONG strip_mode, LONG method, LONG color_mode,
                                              BSTR *retjson, LONG *ret) {
    try {
        template_names = SafeBstr(template_names);

        wstring s;
        obj.CvMatchAnyTemplate(x, y, width, height, template_names, threshold, dir, strip_mode, method, color_mode, s,
                               ret);
        CopyOutBstr(retjson, s);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvMatchAllTemplates(LONG x, LONG y, LONG width, LONG height, BSTR template_names,
                                               DOUBLE threshold, LONG dir, LONG strip_mode, LONG method,
                                               LONG color_mode, BSTR *retjson, LONG *ret) {
    try {
        template_names = SafeBstr(template_names);

        wstring s;
        obj.CvMatchAllTemplates(x, y, width, height, template_names, threshold, dir, strip_mode, method, color_mode, s,
                                ret);
        CopyOutBstr(retjson, s);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvFeatureMatchTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                                  DOUBLE threshold, BSTR *retjson, LONG *ret) {
    try {
        template_name = SafeBstr(template_name);

        wstring s;
        obj.CvFeatureMatchTemplate(x, y, width, height, template_name, threshold, s, ret);
        CopyOutBstr(retjson, s);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvEdgeMatchTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                               DOUBLE threshold, BSTR *retjson, LONG *ret) {
    try {
        template_name = SafeBstr(template_name);

        wstring s;
        obj.CvEdgeMatchTemplate(x, y, width, height, template_name, threshold, s, ret);
        CopyOutBstr(retjson, s);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}

STDMETHODIMP OpAutomation::CvShapeMatchTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                                DOUBLE threshold, BSTR *retjson, LONG *ret) {
    try {
        template_name = SafeBstr(template_name);

        wstring s;
        obj.CvShapeMatchTemplate(x, y, width, height, template_name, threshold, s, ret);
        CopyOutBstr(retjson, s);
        return S_OK;

    } catch (...) {
        if (ret) {
            *ret = 0;
        }
        return E_UNEXPECTED;
    }
}
