// OpAutomation.cpp: OpAutomation 的实现

#include "OpAutomation.h"
#include "stdafx.h"

#include "../base/AutomationModes.h"
#include <cstdint>
#include <filesystem>
#include <string>
// OpAutomation
using std::wstring;

namespace {

template <typename Target, typename Value>
HRESULT SetOutValue(Target *target, Value value) {
    if (!target)
        return E_POINTER;
    *target = static_cast<Target>(value);
    return S_OK;
}

HRESULT CopyOutBstr(BSTR *target, const std::wstring &value) {
    if (!target)
        return E_POINTER;
    *target = nullptr;
    CComBSTR out(value.c_str());
    return out.CopyTo(target);
}

template <typename Callback>
HRESULT RunCvRetOnly(LONG *ret, Callback &&callback) {
    if (!ret)
        return E_POINTER;

    SetOutValue(ret, 0L);
    callback(ret);
    return S_OK;
}

} // namespace

OpAutomation::OpAutomation() {
}

STDMETHODIMP OpAutomation::Ver(BSTR *ret) {

    // Tool::setlog("address=%d,str=%s", ver, ver);
    wstring s = obj.Ver();
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::SetPath(BSTR path, LONG *ret) {

    obj.SetPath(path, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::GetPath(BSTR *path) {
    wstring s;
    obj.GetPath(s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(path);
    return S_OK;
}

STDMETHODIMP OpAutomation::GetBasePath(BSTR *path) {

    wstring s;
    obj.GetBasePath(s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(path);
    return S_OK;
}

STDMETHODIMP OpAutomation::GetID(LONG *ret) {
    obj.GetID(ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::GetLastError(LONG *ret) {
    obj.GetLastError(ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::SetShowErrorMsg(LONG show_type, LONG *ret) {
    obj.SetShowErrorMsg(show_type, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::Sleep(LONG millseconds, LONG *ret) {

    obj.Sleep(millseconds, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::InjectDll(BSTR process_name, BSTR dll_name, LONG *ret) {
    // auto proc = _wsto_string(process_name);
    // auto dll = _wsto_string(dll_name);
    // DllInjector::EnablePrivilege(TRUE);
    // auto h = DllInjector::InjectDll(process_name, dll_name);
    obj.InjectDll(process_name, dll_name, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::EnablePicCache(LONG enable, LONG *ret) {

    obj.EnablePicCache(enable, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::CapturePre(BSTR file, LONG *ret) {

    obj.CapturePre(file, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::SetScreenDataMode(LONG mode, LONG *ret) {
    obj.SetScreenDataMode(mode, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::AStarFindPath(LONG mapWidth, LONG mapHeight, BSTR disable_points, LONG beginX, LONG beginY,
                                        LONG endX, LONG endY, BSTR *path) {
    wstring s;
    obj.AStarFindPath(mapWidth, mapHeight, disable_points, beginX, beginY, endX, endY, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(path);
    return S_OK;
}

// 根据部分Ex接口的返回值，然后在所有坐标里找出距离指定坐标最近的那个坐标.
STDMETHODIMP OpAutomation::FindNearestPos(BSTR all_pos, LONG type, LONG x, LONG y, BSTR *retstr) {
    std::wstring s;
    obj.FindNearestPos(all_pos, type, x, y, s);
    CComBSTR newbstr;
    auto hr = newbstr.Append(s.data());
    hr = newbstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpAutomation::EnumWindow(LONGLONG parent, BSTR title, BSTR class_name, LONG filter, BSTR *retstr) {
    wstring s;
    obj.EnumWindow(static_cast<LONG_PTR>(parent), title, class_name, filter, s);

    CComBSTR newbstr;
    auto hr = newbstr.Append(s.data());
    hr = newbstr.CopyTo(retstr);
    return hr;
}

STDMETHODIMP OpAutomation::EnumWindowByProcess(BSTR process_name, BSTR title, BSTR class_name, LONG filter,
                                              BSTR *retstring) {
    wstring s;
    obj.EnumWindowByProcess(process_name, title, class_name, filter, s);

    CComBSTR newbstr;
    newbstr.Append(s.data());
    newbstr.CopyTo(retstring);
    return S_OK;
}

STDMETHODIMP OpAutomation::EnumProcess(BSTR name, BSTR *retstring) {
    wstring s;
    obj.EnumProcess(name, s);

    CComBSTR newbstr;
    newbstr.Append(s.data());
    newbstr.CopyTo(retstring);
    return S_OK;
}

STDMETHODIMP OpAutomation::ClientToScreen(LONGLONG hwnd, VARIANT *x, VARIANT *y, LONG *bret) {
    x->vt = VT_I4;
    y->vt = VT_I4;
    long lx = x->lVal;
    long ly = y->lVal;
    obj.ClientToScreen(static_cast<LONG_PTR>(hwnd), &lx, &ly, bret);
    x->lVal = lx;
    y->lVal = ly;
    return S_OK;
}

STDMETHODIMP OpAutomation::FindWindow(BSTR class_name, BSTR title, LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.FindWindow(class_name, title, &hwnd);
    return SetOutValue(rethwnd, hwnd);
}

STDMETHODIMP OpAutomation::FindWindowByProcess(BSTR process_name, BSTR class_name, BSTR title, LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.FindWindowByProcess(process_name, class_name, title, &hwnd);
    return SetOutValue(rethwnd, hwnd);
}

STDMETHODIMP OpAutomation::FindWindowByProcessId(LONG process_id, BSTR class_name, BSTR title, LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.FindWindowByProcessId(process_id, class_name, title, &hwnd);
    return SetOutValue(rethwnd, hwnd);
}

STDMETHODIMP OpAutomation::FindWindowEx(LONGLONG parent, BSTR class_name, BSTR title, LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.FindWindowEx(static_cast<LONG_PTR>(parent), class_name, title, &hwnd);
    return SetOutValue(rethwnd, hwnd);
}

STDMETHODIMP OpAutomation::GetClientRect(LONGLONG hwnd, VARIANT *x1, VARIANT *y1, VARIANT *x2, VARIANT *y2, LONG *nret) {
    x1->vt = VT_I4;
    y1->vt = VT_I4;
    x2->vt = VT_I4;
    y2->vt = VT_I4;
    obj.GetClientRect(static_cast<LONG_PTR>(hwnd), &x1->lVal, &y1->lVal, &x2->lVal, &y2->lVal, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::GetClientSize(LONGLONG hwnd, VARIANT *width, VARIANT *height, LONG *nret) {
    width->vt = VT_I4;
    height->vt = VT_I4;
    obj.GetClientSize(static_cast<LONG_PTR>(hwnd), &width->lVal, &height->lVal, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::GetForegroundFocus(LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.GetForegroundFocus(&hwnd);
    return SetOutValue(rethwnd, hwnd);
}

STDMETHODIMP OpAutomation::GetForegroundWindow(LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.GetForegroundWindow(&hwnd);
    return SetOutValue(rethwnd, hwnd);
}

STDMETHODIMP OpAutomation::GetMousePointWindow(LONGLONG *rethwnd) {
    //::Sleep(2000);
    LONG_PTR hwnd = 0;
    obj.GetMousePointWindow(&hwnd);
    return SetOutValue(rethwnd, hwnd);
}

STDMETHODIMP OpAutomation::GetPointWindow(LONG x, LONG y, LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.GetPointWindow(x, y, &hwnd);
    return SetOutValue(rethwnd, hwnd);
}

STDMETHODIMP OpAutomation::GetProcessInfo(LONG pid, BSTR *retstring) {
    wstring s;
    obj.GetProcessInfo(pid, s);

    CComBSTR newbstr;
    newbstr.Append(s.data());
    newbstr.CopyTo(retstring);
    return S_OK;
}

STDMETHODIMP OpAutomation::GetSpecialWindow(LONG flag, LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.GetSpecialWindow(flag, &hwnd);
    return SetOutValue(rethwnd, hwnd);
}

STDMETHODIMP OpAutomation::GetWindow(LONGLONG hwnd, LONG flag, LONGLONG *nret) {
    LONG_PTR target = 0;
    obj.GetWindow(static_cast<LONG_PTR>(hwnd), flag, &target);
    return SetOutValue(nret, target);
}

STDMETHODIMP OpAutomation::GetWindowClass(LONGLONG hwnd, BSTR *retstring) {
    wstring s;
    obj.GetWindowClass(static_cast<LONG_PTR>(hwnd), s);

    CComBSTR newbstr;
    newbstr.Append(s.data());
    newbstr.CopyTo(retstring);
    return S_OK;
}

STDMETHODIMP OpAutomation::GetWindowProcessId(LONGLONG hwnd, LONG *nretpid) {
    obj.GetWindowProcessId(static_cast<LONG_PTR>(hwnd), nretpid);

    return S_OK;
}

STDMETHODIMP OpAutomation::GetWindowProcessPath(LONGLONG hwnd, BSTR *retstring) {
    wstring s;
    obj.GetWindowProcessPath(static_cast<LONG_PTR>(hwnd), s);

    CComBSTR newbstr;
    newbstr.Append(s.data());
    newbstr.CopyTo(retstring);
    return S_OK;
}

STDMETHODIMP OpAutomation::GetWindowRect(LONGLONG hwnd, VARIANT *x1, VARIANT *y1, VARIANT *x2, VARIANT *y2, LONG *nret) {
    x1->vt = VT_I4;
    x2->vt = VT_I4;
    y1->vt = VT_I4;
    y2->vt = VT_I4;

    obj.GetWindowRect(static_cast<LONG_PTR>(hwnd), &x1->lVal, &y1->lVal, &x2->lVal, &y2->lVal, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::GetWindowState(LONGLONG hwnd, LONG flag, LONG *rethwnd) {
    obj.GetWindowState(static_cast<LONG_PTR>(hwnd), flag, rethwnd);

    return S_OK;
}

STDMETHODIMP OpAutomation::GetWindowTitle(LONGLONG hwnd, BSTR *rettitle) {
    wstring s;
    obj.GetWindowTitle(static_cast<LONG_PTR>(hwnd), s);

    CComBSTR newbstr;
    newbstr.Append(s.data());
    newbstr.CopyTo(rettitle);
    return S_OK;
}

STDMETHODIMP OpAutomation::MoveWindow(LONGLONG hwnd, LONG x, LONG y, LONG *nret) {
    obj.MoveWindow(static_cast<LONG_PTR>(hwnd), x, y, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::ScreenToClient(LONGLONG hwnd, VARIANT *x, VARIANT *y, LONG *nret) {
    x->vt = VT_I4;
    y->vt = VT_I4;
    obj.ScreenToClient(static_cast<LONG_PTR>(hwnd), &x->lVal, &y->lVal, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::SendPaste(LONGLONG hwnd, LONG *nret) {
    obj.SendPaste(static_cast<LONG_PTR>(hwnd), nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::SetClientSize(LONGLONG hwnd, LONG width, LONG hight, LONG *nret) {
    obj.SetClientSize(static_cast<LONG_PTR>(hwnd), width, hight, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::SetWindowState(LONGLONG hwnd, LONG flag, LONG *nret) {
    obj.SetWindowState(static_cast<LONG_PTR>(hwnd), flag, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::SetWindowSize(LONGLONG hwnd, LONG width, LONG height, LONG *nret) {
    obj.SetWindowSize(static_cast<LONG_PTR>(hwnd), width, height, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::SetWindowText(LONGLONG hwnd, BSTR title, LONG *nret) {
    obj.SetWindowText(static_cast<LONG_PTR>(hwnd), title, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::SetWindowTransparent(LONGLONG hwnd, LONG trans, LONG *nret) {
    obj.SetWindowTransparent(static_cast<LONG_PTR>(hwnd), trans, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::SendString(LONGLONG hwnd, BSTR str, LONG *ret) {
    obj.SendString(static_cast<LONG_PTR>(hwnd), str, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::SendStringIme(LONGLONG hwnd, BSTR str, LONG *ret) {
    obj.SendStringIme(static_cast<LONG_PTR>(hwnd), str, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::RunApp(BSTR cmdline, LONG mode, ULONG *pid, LONG *ret) {
    obj.RunApp(cmdline, mode, pid, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::LayoutWindows(BSTR hwnds, LONG layout_type, LONG columns, LONG start_x, LONG start_y, LONG gap_x,
                                        LONG gap_y, LONG size_mode, LONG window_width, LONG window_height, LONG anchor_mode,
                                        LONG *ret) {
    obj.LayoutWindows(hwnds, layout_type, columns, start_x, start_y, gap_x, gap_y, size_mode, window_width,
                      window_height, anchor_mode, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::WinExec(BSTR cmdline, LONG cmdshow, LONG *ret) {
    obj.WinExec(cmdline, cmdshow, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::GetCmdStr(BSTR cmd, LONG millseconds, BSTR *retstr) {
    wstring s;
    obj.GetCmdStr(cmd, millseconds, s);

    CComBSTR newstr;

    auto hr = newstr.Append(s.data());
    hr = newstr.CopyTo(retstr);
    return hr;
}

STDMETHODIMP OpAutomation::SetClipboard(BSTR str, LONG *ret) {
    obj.SetClipboard(str, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::GetClipboard(BSTR *ret) {
    wstring s;
    obj.GetClipboard(s);

    CComBSTR newstr;
    auto hr = newstr.Append(s.data());
    hr = newstr.CopyTo(ret);
    return hr;
}

STDMETHODIMP OpAutomation::Delay(LONG mis, LONG *ret) {
    obj.Delay(mis, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::Delays(LONG mis_min, LONG mis_max, LONG *ret) {
    obj.Delays(mis_min, mis_max, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::BindWindow(LONGLONG hwnd, BSTR display, BSTR mouse, BSTR keypad, LONG mode, LONG *ret) {
    obj.BindWindow(static_cast<LONG_PTR>(hwnd), display, mouse, keypad, mode, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::BindWindowEx(LONGLONG display_hwnd, LONGLONG input_hwnd, BSTR display, BSTR mouse, BSTR keypad,
                                       LONG mode, LONG *ret) {
    obj.BindWindowEx(static_cast<LONG_PTR>(display_hwnd), static_cast<LONG_PTR>(input_hwnd), display, mouse, keypad, mode, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::UnBindWindow(LONG *ret) {
    obj.UnBindWindow(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::LockInput(LONG lock, LONG *ret) {
    obj.LockInput(lock, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::GetBindWindow(LONGLONG *ret) {
    LONG_PTR hwnd = 0;
    obj.GetBindWindow(&hwnd);
    return SetOutValue(ret, hwnd);
}

STDMETHODIMP OpAutomation::IsBind(LONG *ret) {
    obj.IsBind(ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::GetCursorPos(VARIANT *x, VARIANT *y, LONG *ret) {
    x->vt = y->vt = VT_I4;
    obj.GetCursorPos(&x->lVal, &y->lVal, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::GetCursorShape(BSTR *ret) {
    if (!ret)
        return E_POINTER;

    std::wstring value;
    obj.GetCursorShape(value);
    return CopyOutBstr(ret, value);
}

STDMETHODIMP OpAutomation::MoveR(LONG x, LONG y, LONG *ret) {
    obj.MoveR(x, y, ret);

    return S_OK;
}
// 把鼠标移动到目的点(x,y)
STDMETHODIMP OpAutomation::MoveTo(LONG x, LONG y, LONG *ret) {
    obj.MoveTo(x, y, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MoveToEx(LONG x, LONG y, LONG w, LONG h, BSTR *ret) {
    if (!ret)
        return E_POINTER;

    std::wstring s;
    obj.MoveToEx(x, y, w, h, s);
    return CopyOutBstr(ret, s);
}

STDMETHODIMP OpAutomation::MoveToSmooth(LONG x, LONG y, LONG duration, LONG *ret) {
    obj.MoveToSmooth(x, y, duration, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MoveToExSmooth(LONG x, LONG y, LONG w, LONG h, LONG duration, BSTR *ret) {
    if (!ret)
        return E_POINTER;

    std::wstring s;
    obj.MoveToExSmooth(x, y, w, h, duration, s);
    return CopyOutBstr(ret, s);
}

STDMETHODIMP OpAutomation::MovePath(BSTR path, LONG duration, LONG *ret) {
    obj.MovePath(path, duration, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::DragPath(BSTR path, LONG duration, LONG *ret) {
    obj.DragPath(path, duration, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::SetMouseTrajectory(LONG mode, LONG min_duration, LONG max_duration, LONG jitter,
                                              LONG start_delay, LONG end_delay, LONG *ret) {
    obj.SetMouseTrajectory(mode, min_duration, max_duration, jitter, start_delay, end_delay, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::LeftClick(LONG *ret) {
    obj.LeftClick(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::LeftDoubleClick(LONG *ret) {
    obj.LeftDoubleClick(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::LeftDown(LONG *ret) {
    obj.LeftDown(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::LeftUp(LONG *ret) {
    obj.LeftUp(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MiddleClick(LONG *ret) {
    obj.MiddleClick(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MiddleDoubleClick(LONG *ret) {
    obj.MiddleDoubleClick(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MiddleDown(LONG *ret) {
    obj.MiddleDown(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MiddleUp(LONG *ret) {
    obj.MiddleUp(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::RightClick(LONG *ret) {
    obj.RightClick(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::RightDoubleClick(LONG *ret) {
    obj.RightDoubleClick(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::RightDown(LONG *ret) {
    obj.RightDown(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::RightUp(LONG *ret) {
    obj.RightUp(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::XButton1Click(LONG *ret) {
    obj.XButton1Click(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::XButton1DoubleClick(LONG *ret) {
    obj.XButton1DoubleClick(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::XButton1Down(LONG *ret) {
    obj.XButton1Down(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::XButton1Up(LONG *ret) {
    obj.XButton1Up(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::XButton2Click(LONG *ret) {
    obj.XButton2Click(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::XButton2DoubleClick(LONG *ret) {
    obj.XButton2DoubleClick(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::XButton2Down(LONG *ret) {
    obj.XButton2Down(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::XButton2Up(LONG *ret) {
    obj.XButton2Up(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::WheelDown(LONG *ret) {
    obj.WheelDown(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::WheelUp(LONG *ret) {
    obj.WheelUp(ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::Wheel(LONG delta, LONG *ret) {
    obj.Wheel(delta, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::HWheel(LONG delta, LONG *ret) {
    obj.HWheel(delta, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::SetMouseDelay(BSTR type, LONG delay, LONG *ret) {
    obj.SetMouseDelay(type, delay, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::GetKeyState(LONG vk_code, LONG *ret) {
    obj.GetKeyState(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::KeyDown(LONG vk_code, LONG *ret) {
    obj.KeyDown(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::KeyDownChar(BSTR vk_code, LONG *ret) {
    obj.KeyDownChar(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::KeyUp(LONG vk_code, LONG *ret) {
    obj.KeyUp(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::KeyUpChar(BSTR vk_code, LONG *ret) {
    obj.KeyUpChar(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::WaitKey(LONG vk_code, LONG time_out, LONG *ret) {
    obj.WaitKey(vk_code, time_out, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::KeyPress(LONG vk_code, LONG *ret) {

    obj.KeyPress(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::KeyPressChar(BSTR vk_code, LONG *ret) {
    obj.KeyPressChar(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::SetKeypadDelay(BSTR type, LONG delay, LONG *ret) {
    obj.SetKeypadDelay(type, delay, ret);
    return S_OK;
}
STDMETHODIMP OpAutomation::KeyPressStr(BSTR key_str, LONG delay, LONG *ret) {
    obj.KeyPressStr(key_str, delay, ret);
    return S_OK;
}

// 抓取指定区域(x1, y1, x2, y2)的图像, 保存为file
STDMETHODIMP OpAutomation::Capture(LONG x1, LONG y1, LONG x2, LONG y2, BSTR file_name, LONG *ret) {

    obj.Capture(x1, y1, x2, y2, file_name, ret);

    return S_OK;
}
// 比较指定坐标点(x,y)的颜色
STDMETHODIMP OpAutomation::CmpColor(LONG x, LONG y, BSTR color, DOUBLE sim, LONG *ret) {
    obj.CmpColor(x, y, color, sim, ret);

    return S_OK;
}
// 查找指定区域内的颜色
STDMETHODIMP OpAutomation::FindColor(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG dir, VARIANT *x,
                                    VARIANT *y, LONG *ret) {

    x->vt = y->vt = VT_I4;

    obj.FindColor(x1, y1, x2, y2, color, sim, dir, &x->lVal, &y->lVal, ret);

    return S_OK;
}
// 查找指定区域内的所有颜色
STDMETHODIMP OpAutomation::FindColorEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG dir,
                                      BSTR *retstr) {
    wstring s;
    obj.FindColorEx(x1, y1, x2, y2, color, sim, dir, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}
// 查找指定区域内的颜色数量
STDMETHODIMP OpAutomation::GetColorNum(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG *ret) {
    wstring s;
    obj.GetColorNum(x1, y1, x2, y2, color, sim, ret);

    return S_OK;
}
// 根据指定的多点查找颜色坐标
STDMETHODIMP OpAutomation::FindMultiColor(LONG x1, LONG y1, LONG x2, LONG y2, BSTR first_color, BSTR offset_color,
                                         DOUBLE sim, LONG dir, VARIANT *x, VARIANT *y, LONG *ret) {
    if (!x || !y || !ret)
        return E_POINTER;

    SetOutValue(ret, 0L);
    x->vt = y->vt = VT_I4;
    obj.FindMultiColor(x1, y1, x2, y2, first_color, offset_color, sim, dir, &x->lVal, &y->lVal, ret);

    return S_OK;
}
// 根据指定的多点查找所有颜色坐标
STDMETHODIMP OpAutomation::FindMultiColorEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR first_color, BSTR offset_color,
                                           DOUBLE sim, LONG dir, BSTR *retstr) {
    wstring s;
    obj.FindMultiColorEx(x1, y1, x2, y2, first_color, offset_color, sim, dir, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}
// 查找指定区域内的图片
STDMETHODIMP OpAutomation::FindPic(LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim,
                                  LONG dir, VARIANT *x, VARIANT *y, LONG *ret) {

    x->vt = y->vt = VT_I4;
    obj.FindPic(x1, y1, x2, y2, files, delta_color, sim, dir, &x->lVal, &y->lVal, ret);

    return S_OK;
}
// 查找多个图片
STDMETHODIMP OpAutomation::FindPicEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim,
                                    LONG dir, BSTR *retstr) {
    wstring s;
    obj.FindPicEx(x1, y1, x2, y2, files, delta_color, sim, dir, s);

    CComBSTR newstr;
    HRESULT hr;
    newstr.Append(s.data());
    hr = newstr.CopyTo(retstr);
    return hr;
}
// 这个函数可以查找多个图片, 并且返回所有找到的图像的坐标.此函数同FindPicEx.只是返回值不同.(file1, x, y | file2, x, y |
// ...)
STDMETHODIMP OpAutomation::FindPicExS(LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim,
                                     LONG dir, BSTR *retstr) {

    wstring s;
    obj.FindPicExS(x1, y1, x2, y2, files, delta_color, sim, dir, s);
    CComBSTR newstr;
    HRESULT hr;
    newstr.Append(s.data());
    hr = newstr.CopyTo(retstr);

    return S_OK;
}
// 查找指定区域内的颜色块,颜色格式"RRGGBB-DRDGDB",注意,和按键的颜色格式相反
STDMETHODIMP OpAutomation::FindColorBlock(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG count,
                                         LONG height, LONG width, VARIANT *x, VARIANT *y, LONG *ret) {
    x->vt = y->vt = VT_I4;
    obj.FindColorBlock(x1, y1, x2, y2, color, sim, count, height, width, &x->lVal, &y->lVal, ret);
    return S_OK;
}
// 查找指定区域内的所有颜色块, 颜色格式"RRGGBB-DRDGDB", 注意, 和按键的颜色格式相反
STDMETHODIMP OpAutomation::FindColorBlockEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG count,
                                           LONG height, LONG width, BSTR *retstr) {
    std::wstring s;
    obj.FindColorBlockEx(x1, y1, x2, y2, color, sim, count, height, width, s);
    CComBSTR newstr;
    HRESULT hr;
    newstr.Append(s.data());
    hr = newstr.CopyTo(retstr);

    return S_OK;
}
// 获取(x,y)的颜色
STDMETHODIMP OpAutomation::GetColor(LONG x, LONG y, BSTR *ret) {
    wstring s;
    obj.GetColor(x, y, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::SetDisplayInput(BSTR mode, LONG *ret) {
    obj.SetDisplayInput(mode, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::LoadPic(BSTR pic_name, LONG *ret) {
    // to do;
    obj.LoadPic(pic_name, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::FreePic(BSTR pic_name, LONG *ret) {
    obj.FreePic(pic_name, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::LoadMemPic(BSTR pic_name, long long data, LONG size, LONG *ret) {
    obj.LoadMemPic(pic_name, (void *)data, size, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::GetPicSize(BSTR pic_name, VARIANT *width, VARIANT *height, LONG *ret) {
    width->vt = height->vt = VT_I4;
    obj.GetPicSize(pic_name, &width->lVal, &height->lVal, ret);
    return S_OK;
}

// 获取指定区域的图像,用二进制数据的方式返回
STDMETHODIMP OpAutomation::GetScreenData(LONG x1, LONG y1, LONG x2, LONG y2, LONG *ret) {
    // #if OP64
    //	data->vt = VT_I8;
    //	data->llVal = 0;
    // #else
    //	data->vt = VT_I4;
    //	data->lVal = 0;
    // #endif
    //	* ret = 0;
    //	void* data_ = nullptr;
    //	obj.GetScreenData(x1, y1, x2, y2, &data_, ret);
    //
    // #if OP64
    //	data->llVal = (long long)data_;
    // #else
    //	data->lVal = (long)data_;
    // #endif
    //	* ret = 1;
    if (!ret)
        return E_POINTER;
    SetOutValue(ret, 0L);
    size_t data_ = 0;
    long capture_ret = 0;
    obj.GetScreenData(x1, y1, x2, y2, &data_, &capture_ret);
    return SetOutValue(ret, static_cast<long>(data_));
}

STDMETHODIMP OpAutomation::GetScreenDataBmp(LONG x1, LONG y1, LONG x2, LONG y2, VARIANT *data, VARIANT *size,
                                           LONG *ret) {
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
}

STDMETHODIMP OpAutomation::GetScreenFrameInfo(LONG *frame_id, LONG *time) {
    obj.GetScreenFrameInfo(frame_id, time);
    return S_OK;
}

// 根据通配符获取文件集合. 方便用于FindPic和FindPicEx
STDMETHODIMP OpAutomation::MatchPicName(BSTR pic_name, BSTR *ret) {
    wstring s;
    obj.MatchPicName(pic_name, s);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(ret);
    return S_OK;
}

// 设置字库文件
STDMETHODIMP OpAutomation::SetDict(LONG idx, BSTR file_name, LONG *ret) {
    obj.SetDict(idx, file_name, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::GetDict(LONG idx, LONG font_index, BSTR *retstr) {
    std::wstring s;
    obj.GetDict(idx, font_index, s);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

// 从内存字节设置全局字库槽，支持 OP 二进制 .dict 和文本字库内容。
STDMETHODIMP OpAutomation::SetMemDict(LONG idx, SAFEARRAY *data, LONG *ret) {
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
}

// 使用哪个字库文件进行识别
STDMETHODIMP OpAutomation::UseDict(LONG idx, LONG *ret) {
    obj.UseDict(idx, ret);

    return S_OK;
}

// 给指定的字库中添加一条字库信息
STDMETHODIMP OpAutomation::AddDict(LONG idx, BSTR dict_info, LONG *ret) {
    obj.AddDict(idx, dict_info, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::SaveDict(LONG idx, BSTR file_name, LONG *ret) {
    obj.SaveDict(idx, file_name, ret);
    return S_OK;
}

// 清空指定的字库
STDMETHODIMP OpAutomation::ClearDict(LONG idx, LONG *ret) {
    obj.ClearDict(idx, ret);
    return S_OK;
}

// 获取指定的字库中的字符数量
STDMETHODIMP OpAutomation::GetDictCount(LONG idx, LONG *ret) {
    obj.GetDictCount(idx, ret);
    return S_OK;
}

// 获取当前使用的字库序号
STDMETHODIMP OpAutomation::GetNowDict(LONG *ret) {
    obj.GetNowDict(ret);
    return S_OK;
}

// 根据指定的范围,以及指定的颜色描述，提取点阵信息，类似于大漠工具里的单独提取
STDMETHODIMP OpAutomation::FetchWord(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, BSTR word, BSTR *ret_str) {
    wstring s;
    obj.FetchWord(x1, y1, x2, y2, color, word, s);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(ret_str);
    return S_OK;
}

STDMETHODIMP OpAutomation::FetchWordEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR word,
                                       BSTR *ret_str) {
    if (!ret_str)
        return E_POINTER;

    wstring s;
    obj.FetchWordEx(x1, y1, x2, y2, color, sim, word, s);
    return CopyOutBstr(ret_str, s);
}

STDMETHODIMP OpAutomation::ExtractWordRects(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim,
                                            LONG min_word_h, BSTR *ret_str) {
    if (!ret_str)
        return E_POINTER;

    wstring s;
    obj.ExtractWordRects(x1, y1, x2, y2, color, sim, min_word_h, s);
    return CopyOutBstr(ret_str, s);
}

STDMETHODIMP OpAutomation::ExtractWordRectsEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim,
                                              LONG min_word_w, LONG min_word_h, LONG padding, BSTR *ret_str) {
    if (!ret_str)
        return E_POINTER;

    wstring s;
    obj.ExtractWordRectsEx(x1, y1, x2, y2, color, sim, min_word_w, min_word_h, padding, s);
    return CopyOutBstr(ret_str, s);
}

STDMETHODIMP OpAutomation::FetchWords(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR words,
                                      LONG min_word_h, BSTR *ret_str) {
    if (!ret_str)
        return E_POINTER;

    wstring s;
    obj.FetchWords(x1, y1, x2, y2, color, sim, words, min_word_h, s);
    return CopyOutBstr(ret_str, s);
}

STDMETHODIMP OpAutomation::FetchWordsEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR words,
                                        LONG min_word_w, LONG min_word_h, LONG padding, BSTR *ret_str) {
    if (!ret_str)
        return E_POINTER;

    wstring s;
    obj.FetchWordsEx(x1, y1, x2, y2, color, sim, words, min_word_w, min_word_h, padding, s);
    return CopyOutBstr(ret_str, s);
}

STDMETHODIMP OpAutomation::FetchWordsByRects(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR words,
                                             BSTR rects, BSTR *ret_str) {
    if (!ret_str)
        return E_POINTER;

    wstring s;
    obj.FetchWordsByRects(x1, y1, x2, y2, color, sim, words, rects, s);
    return CopyOutBstr(ret_str, s);
}

STDMETHODIMP OpAutomation::GetBinaryPreview(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim,
                                            BSTR *ret_str, LONG *ret) {
    if (!ret_str || !ret)
        return E_POINTER;

    wstring s;
    obj.GetBinaryPreview(x1, y1, x2, y2, color, sim, s, ret);
    return CopyOutBstr(ret_str, s);
}

STDMETHODIMP OpAutomation::GetWordPreview(BSTR dict_info, BSTR *ret_str, LONG *ret) {
    if (!ret_str || !ret)
        return E_POINTER;

    wstring s;
    obj.GetWordPreview(dict_info, s, ret);
    return CopyOutBstr(ret_str, s);
}

STDMETHODIMP OpAutomation::CheckWordDict(BSTR dict_info, BSTR *ret_str, LONG *ret) {
    if (!ret_str || !ret)
        return E_POINTER;

    wstring s;
    obj.CheckWordDict(dict_info, s, ret);
    return CopyOutBstr(ret_str, s);
}

STDMETHODIMP OpAutomation::NormalizeWordDict(BSTR dict_info, BSTR *ret_str, LONG *ret) {
    if (!ret_str || !ret)
        return E_POINTER;

    wstring s;
    obj.NormalizeWordDict(dict_info, s, ret);
    return CopyOutBstr(ret_str, s);
}

STDMETHODIMP OpAutomation::RenameWordDict(BSTR dict_info, BSTR words, BSTR *ret_str, LONG *ret) {
    if (!ret_str || !ret)
        return E_POINTER;

    wstring s;
    obj.RenameWordDict(dict_info, words, s, ret);
    return CopyOutBstr(ret_str, s);
}

STDMETHODIMP OpAutomation::SetBinaryPreprocess(LONG mode, LONG isolated_threshold, LONG min_component_area,
                                               LONG bridge_gap, LONG *ret) {
    if (!ret)
        return E_POINTER;

    obj.SetBinaryPreprocess(mode, isolated_threshold, min_component_area, bridge_gap, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::GetBinaryPreprocess(LONG *mode, LONG *isolated_threshold, LONG *min_component_area,
                                               LONG *bridge_gap, LONG *ret) {
    if (!mode || !isolated_threshold || !min_component_area || !bridge_gap || !ret)
        return E_POINTER;

    obj.GetBinaryPreprocess(mode, isolated_threshold, min_component_area, bridge_gap, ret);
    return S_OK;
}

// 识别这个范围内所有满足条件的词组，这个识别函数不会用到字库. 只是识别大概形状的位置
STDMETHODIMP OpAutomation::GetWordsNoDict(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, BSTR *ret_str) {
    if (!ret_str)
        return E_POINTER;

    wstring s;
    obj.GetWordsNoDict(x1, y1, x2, y2, color, s);
    return CopyOutBstr(ret_str, s);
}
// 在使用GetWords进行词组识别以后,可以用此接口进行识别词组数量的计算
STDMETHODIMP OpAutomation::GetWordResultCount(BSTR result, LONG *ret) {
    if (!ret)
        return E_POINTER;

    SetOutValue(ret, 0L);
    obj.GetWordResultCount(result, ret);
    return S_OK;
}
// 在使用GetWords进行词组识别以后,可以用此接口进行识别各个词组的坐标
STDMETHODIMP OpAutomation::GetWordResultPos(BSTR result, LONG index, VARIANT *x, VARIANT *y, LONG *ret) {
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
}
// 在使用GetWords进行词组识别以后,可以用此接口进行识别各个词组的内容
STDMETHODIMP OpAutomation::GetWordResultStr(BSTR result, LONG index, BSTR *ret_str) {
    if (!ret_str)
        return E_POINTER;

    wstring s;
    obj.GetWordResultStr(result, index, s);
    return CopyOutBstr(ret_str, s);
}

// 识别屏幕范围(x1,y1,x2,y2)内符合color_format的字符串,并且相似度为sim,sim取值范围(0.1-1.0),
STDMETHODIMP OpAutomation::Ocr(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR *ret_str) {
    wstring s;
    obj.Ocr(x1, y1, x2, y2, color, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(ret_str);
    return S_OK;
}
// 回识别到的字符串，以及每个字符的坐标.
STDMETHODIMP OpAutomation::OcrEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR *ret_str) {
    wstring s;
    obj.OcrEx(x1, y1, x2, y2, color, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(ret_str);
    return S_OK;
}
// 在屏幕范围(x1,y1,x2,y2)内,查找string(可以是任意个字符串的组合),并返回符合color_format的坐标位置
STDMETHODIMP OpAutomation::FindStr(LONG x1, LONG y1, LONG x2, LONG y2, BSTR strs, BSTR color, DOUBLE sim, VARIANT *retx,
                                  VARIANT *rety, LONG *ret) {

    retx->vt = rety->vt = VT_INT;
    obj.FindStr(x1, y1, x2, y2, strs, color, sim, &retx->lVal, &rety->lVal, ret);

    return S_OK;
}
// 返回符合color_format的所有坐标位置
STDMETHODIMP OpAutomation::FindStrEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR strs, BSTR color, DOUBLE sim,
                                    BSTR *retstr) {
    wstring s;
    obj.FindStrEx(x1, y1, x2, y2, strs, color, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpAutomation::OcrAuto(LONG x1, LONG y1, LONG x2, LONG y2, DOUBLE sim, BSTR *retstr) {
    wstring s;
    obj.OcrAuto(x1, y1, x2, y2, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

// 从文件中识别图片
STDMETHODIMP OpAutomation::OcrFromFile(BSTR file_name, BSTR color_format, DOUBLE sim, BSTR *retstr) {
    wstring s;
    obj.OcrFromFile(file_name, color_format, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}
// 从文件中识别图片,无需指定颜色
STDMETHODIMP OpAutomation::OcrAutoFromFile(BSTR file_name, DOUBLE sim, BSTR *retstr) {
    wstring s;
    obj.OcrAutoFromFile(file_name, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpAutomation::FindLine(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR *retstr) {
    wstring s;
    obj.FindLine(x1, y1, x2, y2, color, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpAutomation::SetOcrEngine(BSTR path_of_engine, BSTR dll_name, BSTR argv, LONG *ret) {
    return SetOutValue(ret, obj.SetOcrEngine(path_of_engine, dll_name, argv));
}

STDMETHODIMP OpAutomation::SetYoloEngine(BSTR path_of_engine, BSTR dll_name, BSTR argv, LONG *ret) {
    return SetOutValue(ret, obj.SetYoloEngine(path_of_engine, dll_name, argv));
}

STDMETHODIMP OpAutomation::YoloDetect(LONG x1, LONG y1, LONG x2, LONG y2, DOUBLE conf, DOUBLE iou, BSTR *retjson,
                                     LONG *ret) {
    if (!ret)
        return E_POINTER;
    std::wstring s;
    obj.YoloDetect(x1, y1, x2, y2, conf, iou, s, ret);
    return CopyOutBstr(retjson, s);
}

STDMETHODIMP OpAutomation::YoloDetectFromFile(BSTR file_name, DOUBLE conf, DOUBLE iou, BSTR *retjson, LONG *ret) {
    if (!ret)
        return E_POINTER;
    std::wstring s;
    obj.YoloDetectFromFile(file_name, conf, iou, s, ret);
    return CopyOutBstr(retjson, s);
}

STDMETHODIMP OpAutomation::WriteData(LONGLONG hwnd, BSTR address, BSTR data, LONG size, LONG *ret) {
    obj.WriteData(static_cast<LONG_PTR>(hwnd), address, data, size, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::ReadData(LONGLONG hwnd, BSTR address, LONG size, BSTR *retstr) {
    wstring s;
    obj.ReadData(static_cast<LONG_PTR>(hwnd), address, size, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpAutomation::ReadInt(LONGLONG hwnd, BSTR address, LONG type, LONGLONG *ret) {
    if (!ret)
        return E_POINTER;

    int64_t value = 0;
    obj.ReadInt(static_cast<LONG_PTR>(hwnd), address, type, &value);
    return SetOutValue(ret, value);
}

STDMETHODIMP OpAutomation::WriteInt(LONGLONG hwnd, BSTR address, LONG type, LONGLONG value, LONG *ret) {
    obj.WriteInt(static_cast<LONG_PTR>(hwnd), address, type, static_cast<int64_t>(value), ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::ReadFloat(LONGLONG hwnd, BSTR address, DOUBLE *ret) {
    if (!ret)
        return E_POINTER;

    float value = 0.0f;
    obj.ReadFloat(static_cast<LONG_PTR>(hwnd), address, &value);
    return SetOutValue(ret, value);
}

STDMETHODIMP OpAutomation::WriteFloat(LONGLONG hwnd, BSTR address, DOUBLE value, LONG *ret) {
    obj.WriteFloat(static_cast<LONG_PTR>(hwnd), address, static_cast<float>(value), ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::ReadDouble(LONGLONG hwnd, BSTR address, DOUBLE *ret) {
    if (!ret)
        return E_POINTER;

    double value = 0.0;
    obj.ReadDouble(static_cast<LONG_PTR>(hwnd), address, &value);
    return SetOutValue(ret, value);
}

STDMETHODIMP OpAutomation::WriteDouble(LONGLONG hwnd, BSTR address, DOUBLE value, LONG *ret) {
    obj.WriteDouble(static_cast<LONG_PTR>(hwnd), address, static_cast<double>(value), ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::ReadString(LONGLONG hwnd, BSTR address, LONG type, LONG len, BSTR *retstr) {
    if (!retstr)
        return E_POINTER;

    wstring s;
    obj.ReadString(static_cast<LONG_PTR>(hwnd), address, type, len, s);
    CComBSTR newstr;
    newstr.Append(s.data());
    return newstr.CopyTo(retstr);
}

STDMETHODIMP OpAutomation::WriteString(LONGLONG hwnd, BSTR address, LONG type, BSTR value, LONG *ret) {
    obj.WriteString(static_cast<LONG_PTR>(hwnd), address, type, value, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::CvLoadTemplate(BSTR name, BSTR file_path, LONG *ret) {
    obj.CvLoadTemplate(name, file_path, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::CvLoadMaskedTemplate(BSTR name, BSTR template_path, BSTR mask_path, LONG *ret) {
    obj.CvLoadMaskedTemplate(name, template_path, mask_path, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::CvRemoveTemplate(BSTR name, LONG *ret) {
    obj.CvRemoveTemplate(name, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::CvRemoveAllTemplates(LONG *ret) {
    obj.CvRemoveAllTemplates(ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::CvHasTemplate(BSTR name, LONG *ret) {
    obj.CvHasTemplate(name, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::CvGetTemplateCount(LONG *ret) {
    obj.CvGetTemplateCount(ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::CvGetAllTemplateNames(BSTR *retstr) {
    wstring s;
    obj.CvGetAllTemplateNames(s);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpAutomation::CvGetOpenCvVersion(BSTR *retstr) {
    wstring s;
    obj.CvGetOpenCvVersion(s);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpAutomation::CvLoadTemplateList(BSTR template_list, LONG *ret) {
    obj.CvLoadTemplateList(template_list, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::CvToGray(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvToGray(src_file, dst_file, out); });
}

STDMETHODIMP OpAutomation::CvToBinary(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvToBinary(src_file, dst_file, out); });
}

STDMETHODIMP OpAutomation::CvToEdge(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvToEdge(src_file, dst_file, out); });
}

STDMETHODIMP OpAutomation::CvToOutline(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvToOutline(src_file, dst_file, out); });
}

STDMETHODIMP OpAutomation::CvDenoise(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvDenoise(src_file, dst_file, out); });
}

STDMETHODIMP OpAutomation::CvEqualize(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvEqualize(src_file, dst_file, out); });
}

STDMETHODIMP OpAutomation::CvCLAHE(BSTR src_file, BSTR dst_file, DOUBLE clip_limit, LONG tile_grid_size, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvCLAHE(src_file, dst_file, clip_limit, tile_grid_size, out); });
}

STDMETHODIMP OpAutomation::CvBlur(BSTR src_file, BSTR dst_file, BSTR mode, LONG kernel_size, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvBlur(src_file, dst_file, mode, kernel_size, out); });
}

STDMETHODIMP OpAutomation::CvSharpen(BSTR src_file, BSTR dst_file, DOUBLE strength, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvSharpen(src_file, dst_file, strength, out); });
}

STDMETHODIMP OpAutomation::CvCropValid(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvCropValid(src_file, dst_file, out); });
}

STDMETHODIMP OpAutomation::CvConnectedComponents(BSTR src_file, DOUBLE min_area, BSTR *retjson, LONG *ret) {
    if (!retjson || !ret) {
        return E_POINTER;
    }

    SetOutValue(ret, 0L);

    std::wstring s;
    obj.CvConnectedComponents(src_file, min_area, s, ret);
    return CopyOutBstr(retjson, s);
}

STDMETHODIMP OpAutomation::CvFindContours(BSTR src_file, DOUBLE min_area, BSTR *retjson, LONG *ret) {
    if (!retjson || !ret) {
        return E_POINTER;
    }

    SetOutValue(ret, 0L);

    std::wstring s;
    obj.CvFindContours(src_file, min_area, s, ret);
    return CopyOutBstr(retjson, s);
}

STDMETHODIMP OpAutomation::CvPreprocessPipeline(BSTR src_file, BSTR dst_file, BSTR pipeline, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvPreprocessPipeline(src_file, dst_file, pipeline, out); });
}

STDMETHODIMP OpAutomation::CvCrop(BSTR src_file, LONG x, LONG y, LONG width, LONG height, BSTR dst_file, LONG *ret) {
    obj.CvCrop(src_file, x, y, width, height, dst_file, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::CvResize(BSTR src_file, LONG width, LONG height, BSTR dst_file, LONG *ret) {
    obj.CvResize(src_file, width, height, dst_file, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::CvThreshold(BSTR src_file, BSTR dst_file, DOUBLE threshold, DOUBLE max_value, BSTR mode,
                                      LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvThreshold(src_file, dst_file, threshold, max_value, mode, out); });
}

STDMETHODIMP OpAutomation::CvInRange(BSTR src_file, BSTR dst_file, BSTR color_space, BSTR lower, BSTR upper, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvInRange(src_file, dst_file, color_space, lower, upper, out); });
}

STDMETHODIMP OpAutomation::CvMorphology(BSTR src_file, BSTR dst_file, BSTR mode, LONG kernel_size, LONG iterations,
                                       LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvMorphology(src_file, dst_file, mode, kernel_size, iterations, out); });
}

STDMETHODIMP OpAutomation::CvThin(BSTR src_file, BSTR dst_file, BSTR mode, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvThin(src_file, dst_file, mode, out); });
}

STDMETHODIMP OpAutomation::CvMatchTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                          DOUBLE threshold, LONG dir, LONG strip_mode, LONG method, LONG color_mode,
                                          BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvMatchTemplate(x, y, width, height, template_name, threshold, dir, strip_mode, method, color_mode, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

STDMETHODIMP OpAutomation::CvMatchTemplateScale(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                               BSTR scales, DOUBLE threshold, LONG method, LONG color_mode,
                                               BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvMatchTemplateScale(x, y, width, height, template_name, scales, threshold, method, color_mode, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

STDMETHODIMP OpAutomation::CvMatchAnyTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_names,
                                             DOUBLE threshold, LONG dir, LONG strip_mode, LONG method, LONG color_mode,
                                             BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvMatchAnyTemplate(x, y, width, height, template_names, threshold, dir, strip_mode, method, color_mode, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

STDMETHODIMP OpAutomation::CvMatchAllTemplates(LONG x, LONG y, LONG width, LONG height, BSTR template_names,
                                              DOUBLE threshold, LONG dir, LONG strip_mode, LONG method, LONG color_mode,
                                              BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvMatchAllTemplates(x, y, width, height, template_names, threshold, dir, strip_mode, method, color_mode, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

STDMETHODIMP OpAutomation::CvFeatureMatchTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                                 DOUBLE threshold, BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvFeatureMatchTemplate(x, y, width, height, template_name, threshold, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

STDMETHODIMP OpAutomation::CvEdgeMatchTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                              DOUBLE threshold, BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvEdgeMatchTemplate(x, y, width, height, template_name, threshold, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

STDMETHODIMP OpAutomation::CvShapeMatchTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                               DOUBLE threshold, BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvShapeMatchTemplate(x, y, width, height, template_name, threshold, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

