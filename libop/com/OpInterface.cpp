// OpInterface.cpp: OpInterface 的实现

#include "OpInterface.h"
#include "stdafx.h"

#include "../core/globalVar.h"
#include <filesystem>
#include <string>
// OpInterface
using std::wstring;

namespace {

template <typename Callback>
HRESULT RunCvRetOnly(LONG *ret, Callback &&callback) {
    if (!ret)
        return E_POINTER;

    *ret = 0;
    callback(ret);
    return S_OK;
}

} // namespace

OpInterface::OpInterface() {
}

STDMETHODIMP OpInterface::Ver(BSTR *ret) {

    // Tool::setlog("address=%d,str=%s", ver, ver);
    wstring s = obj.Ver();
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(ret);

    return S_OK;
}

STDMETHODIMP OpInterface::SetPath(BSTR path, LONG *ret) {

    obj.SetPath(path, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::GetPath(BSTR *path) {
    wstring s;
    obj.GetPath(s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(path);
    return S_OK;
}

STDMETHODIMP OpInterface::GetBasePath(BSTR *path) {

    wstring s;
    obj.GetBasePath(s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(path);
    return S_OK;
}

STDMETHODIMP OpInterface::GetID(LONG *ret) {
    obj.GetID(ret);
    return S_OK;
}

STDMETHODIMP::OpInterface::GetLastError(LONG *ret) {
    obj.GetLastError(ret);
    return S_OK;
}

STDMETHODIMP OpInterface::SetShowErrorMsg(LONG show_type, LONG *ret) {
    obj.SetShowErrorMsg(show_type, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::Sleep(LONG millseconds, LONG *ret) {

    obj.Sleep(millseconds, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::InjectDll(BSTR process_name, BSTR dll_name, LONG *ret) {
    // auto proc = _wsto_string(process_name);
    // auto dll = _wsto_string(dll_name);
    // Injecter::EnablePrivilege(TRUE);
    // auto h = Injecter::InjectDll(process_name, dll_name);
    obj.InjectDll(process_name, dll_name, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::EnablePicCache(LONG enable, LONG *ret) {

    obj.EnablePicCache(enable, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::CapturePre(BSTR file, LONG *ret) {

    obj.CapturePre(file, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::AStarFindPath(LONG mapWidth, LONG mapHeight, BSTR disable_points, LONG beginX, LONG beginY,
                                        LONG endX, LONG endY, BSTR *path) {
    wstring s;
    obj.AStarFindPath(mapWidth, mapHeight, disable_points, beginX, beginY, endX, endY, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(path);
    return S_OK;
}

// 根据部分Ex接口的返回值，然后在所有坐标里找出距离指定坐标最近的那个坐标.
STDMETHODIMP OpInterface::FindNearestPos(BSTR all_pos, LONG type, LONG x, LONG y, BSTR *retstr) {
    std::wstring s;
    obj.FindNearestPos(all_pos, type, x, y, s);
    CComBSTR newbstr;
    auto hr = newbstr.Append(s.data());
    hr = newbstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpInterface::EnumWindow(LONGLONG parent, BSTR title, BSTR class_name, LONG filter, BSTR *retstr) {
    wstring s;
    obj.EnumWindow(static_cast<LONG_PTR>(parent), title, class_name, filter, s);

    CComBSTR newbstr;
    auto hr = newbstr.Append(s.data());
    hr = newbstr.CopyTo(retstr);
    return hr;
}

STDMETHODIMP OpInterface::EnumWindowByProcess(BSTR process_name, BSTR title, BSTR class_name, LONG filter,
                                              BSTR *retstring) {
    wstring s;
    obj.EnumWindowByProcess(process_name, title, class_name, filter, s);

    CComBSTR newbstr;
    newbstr.Append(s.data());
    newbstr.CopyTo(retstring);
    return S_OK;
}

STDMETHODIMP OpInterface::EnumProcess(BSTR name, BSTR *retstring) {
    wstring s;
    obj.EnumProcess(name, s);

    CComBSTR newbstr;
    newbstr.Append(s.data());
    newbstr.CopyTo(retstring);
    return S_OK;
}

STDMETHODIMP OpInterface::ClientToScreen(LONGLONG hwnd, VARIANT *x, VARIANT *y, LONG *bret) {
    x->vt = VT_I4;
    y->vt = VT_I4;
    long lx = x->lVal;
    long ly = y->lVal;
    obj.ClientToScreen(static_cast<LONG_PTR>(hwnd), &lx, &ly, bret);
    x->lVal = lx;
    y->lVal = ly;
    return S_OK;
}

STDMETHODIMP OpInterface::FindWindow(BSTR class_name, BSTR title, LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.FindWindow(class_name, title, &hwnd);
    *rethwnd = static_cast<LONGLONG>(hwnd);

    return S_OK;
}

STDMETHODIMP OpInterface::FindWindowByProcess(BSTR process_name, BSTR class_name, BSTR title, LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.FindWindowByProcess(process_name, class_name, title, &hwnd);
    *rethwnd = static_cast<LONGLONG>(hwnd);

    return S_OK;
}

STDMETHODIMP OpInterface::FindWindowByProcessId(LONG process_id, BSTR class_name, BSTR title, LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.FindWindowByProcessId(process_id, class_name, title, &hwnd);
    *rethwnd = static_cast<LONGLONG>(hwnd);

    return S_OK;
}

STDMETHODIMP OpInterface::FindWindowEx(LONGLONG parent, BSTR class_name, BSTR title, LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.FindWindowEx(static_cast<LONG_PTR>(parent), class_name, title, &hwnd);
    *rethwnd = static_cast<LONGLONG>(hwnd);

    return S_OK;
}

STDMETHODIMP OpInterface::GetClientRect(LONGLONG hwnd, VARIANT *x1, VARIANT *y1, VARIANT *x2, VARIANT *y2, LONG *nret) {
    x1->vt = VT_I4;
    y1->vt = VT_I4;
    x2->vt = VT_I4;
    y2->vt = VT_I4;
    obj.GetClientRect(static_cast<LONG_PTR>(hwnd), &x1->lVal, &y1->lVal, &x2->lVal, &y2->lVal, nret);

    return S_OK;
}

STDMETHODIMP OpInterface::GetClientSize(LONGLONG hwnd, VARIANT *width, VARIANT *height, LONG *nret) {
    width->vt = VT_I4;
    height->vt = VT_I4;
    obj.GetClientSize(static_cast<LONG_PTR>(hwnd), &width->lVal, &height->lVal, nret);

    return S_OK;
}

STDMETHODIMP OpInterface::GetForegroundFocus(LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.GetForegroundFocus(&hwnd);
    *rethwnd = static_cast<LONGLONG>(hwnd);

    return S_OK;
}

STDMETHODIMP OpInterface::GetForegroundWindow(LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.GetForegroundWindow(&hwnd);
    *rethwnd = static_cast<LONGLONG>(hwnd);

    return S_OK;
}

STDMETHODIMP OpInterface::GetMousePointWindow(LONGLONG *rethwnd) {
    //::Sleep(2000);
    LONG_PTR hwnd = 0;
    obj.GetMousePointWindow(&hwnd);
    *rethwnd = static_cast<LONGLONG>(hwnd);

    return S_OK;
}

STDMETHODIMP OpInterface::GetPointWindow(LONG x, LONG y, LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.GetPointWindow(x, y, &hwnd);
    *rethwnd = static_cast<LONGLONG>(hwnd);

    return S_OK;
}

STDMETHODIMP OpInterface::GetProcessInfo(LONG pid, BSTR *retstring) {
    wstring s;
    obj.GetProcessInfo(pid, s);

    CComBSTR newbstr;
    newbstr.Append(s.data());
    newbstr.CopyTo(retstring);
    return S_OK;
}

STDMETHODIMP OpInterface::GetSpecialWindow(LONG flag, LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.GetSpecialWindow(flag, &hwnd);
    *rethwnd = static_cast<LONGLONG>(hwnd);

    return S_OK;
}

STDMETHODIMP OpInterface::GetWindow(LONGLONG hwnd, LONG flag, LONGLONG *nret) {
    LONG_PTR target = 0;
    obj.GetWindow(static_cast<LONG_PTR>(hwnd), flag, &target);
    *nret = static_cast<LONGLONG>(target);

    return S_OK;
}

STDMETHODIMP OpInterface::GetWindowClass(LONGLONG hwnd, BSTR *retstring) {
    wstring s;
    obj.GetWindowClass(static_cast<LONG_PTR>(hwnd), s);

    CComBSTR newbstr;
    newbstr.Append(s.data());
    newbstr.CopyTo(retstring);
    return S_OK;
}

STDMETHODIMP OpInterface::GetWindowProcessId(LONGLONG hwnd, LONG *nretpid) {
    obj.GetWindowProcessId(static_cast<LONG_PTR>(hwnd), nretpid);

    return S_OK;
}

STDMETHODIMP OpInterface::GetWindowProcessPath(LONGLONG hwnd, BSTR *retstring) {
    wstring s;
    obj.GetWindowProcessPath(static_cast<LONG_PTR>(hwnd), s);

    CComBSTR newbstr;
    newbstr.Append(s.data());
    newbstr.CopyTo(retstring);
    return S_OK;
}

STDMETHODIMP OpInterface::GetWindowRect(LONGLONG hwnd, VARIANT *x1, VARIANT *y1, VARIANT *x2, VARIANT *y2, LONG *nret) {
    x1->vt = VT_I4;
    x2->vt = VT_I4;
    y1->vt = VT_I4;
    y2->vt = VT_I4;

    obj.GetWindowRect(static_cast<LONG_PTR>(hwnd), &x1->lVal, &y1->lVal, &x2->lVal, &y2->lVal, nret);

    return S_OK;
}

STDMETHODIMP OpInterface::GetWindowState(LONGLONG hwnd, LONG flag, LONG *rethwnd) {
    obj.GetWindowState(static_cast<LONG_PTR>(hwnd), flag, rethwnd);

    return S_OK;
}

STDMETHODIMP OpInterface::GetWindowTitle(LONGLONG hwnd, BSTR *rettitle) {
    wstring s;
    obj.GetWindowTitle(static_cast<LONG_PTR>(hwnd), s);

    CComBSTR newbstr;
    newbstr.Append(s.data());
    newbstr.CopyTo(rettitle);
    return S_OK;
}

STDMETHODIMP OpInterface::MoveWindow(LONGLONG hwnd, LONG x, LONG y, LONG *nret) {
    obj.MoveWindow(static_cast<LONG_PTR>(hwnd), x, y, nret);

    return S_OK;
}

STDMETHODIMP OpInterface::ScreenToClient(LONGLONG hwnd, VARIANT *x, VARIANT *y, LONG *nret) {
    x->vt = VT_I4;
    y->vt = VT_I4;
    obj.ScreenToClient(static_cast<LONG_PTR>(hwnd), &x->lVal, &y->lVal, nret);

    return S_OK;
}

STDMETHODIMP OpInterface::SendPaste(LONGLONG hwnd, LONG *nret) {
    obj.SendPaste(static_cast<LONG_PTR>(hwnd), nret);

    return S_OK;
}

STDMETHODIMP OpInterface::SetClientSize(LONGLONG hwnd, LONG width, LONG hight, LONG *nret) {
    obj.SetClientSize(static_cast<LONG_PTR>(hwnd), width, hight, nret);

    return S_OK;
}

STDMETHODIMP OpInterface::SetWindowState(LONGLONG hwnd, LONG flag, LONG *nret) {
    obj.SetWindowState(static_cast<LONG_PTR>(hwnd), flag, nret);

    return S_OK;
}

STDMETHODIMP OpInterface::SetWindowSize(LONGLONG hwnd, LONG width, LONG height, LONG *nret) {
    obj.SetWindowSize(static_cast<LONG_PTR>(hwnd), width, height, nret);

    return S_OK;
}

STDMETHODIMP OpInterface::SetWindowText(LONGLONG hwnd, BSTR title, LONG *nret) {
    //*nret=gWindowObj.TSSetWindowState(hwnd,flag);
    obj.SetWindowText(static_cast<LONG_PTR>(hwnd), title, nret);

    return S_OK;
}

STDMETHODIMP OpInterface::SetWindowTransparent(LONGLONG hwnd, LONG trans, LONG *nret) {
    obj.SetWindowTransparent(static_cast<LONG_PTR>(hwnd), trans, nret);

    return S_OK;
}

STDMETHODIMP OpInterface::SendString(LONGLONG hwnd, BSTR str, LONG *ret) {
    obj.SendString(static_cast<LONG_PTR>(hwnd), str, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::SendStringIme(LONGLONG hwnd, BSTR str, LONG *ret) {
    obj.SendStringIme(static_cast<LONG_PTR>(hwnd), str, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::RunApp(BSTR cmdline, LONG mode, ULONG *pid, LONG *ret) {
    obj.RunApp(cmdline, mode, pid, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::LayoutWindows(BSTR hwnds, LONG layout_type, LONG columns, LONG start_x, LONG start_y, LONG gap_x,
                                        LONG gap_y, LONG size_mode, LONG window_width, LONG window_height, LONG anchor_mode,
                                        LONG *ret) {
    obj.LayoutWindows(hwnds, layout_type, columns, start_x, start_y, gap_x, gap_y, size_mode, window_width,
                      window_height, anchor_mode, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::WinExec(BSTR cmdline, LONG cmdshow, LONG *ret) {
    obj.WinExec(cmdline, cmdshow, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::GetCmdStr(BSTR cmd, LONG millseconds, BSTR *retstr) {
    wstring s;
    obj.GetCmdStr(cmd, millseconds, s);

    CComBSTR newstr;

    auto hr = newstr.Append(s.data());
    hr = newstr.CopyTo(retstr);
    return hr;
}

STDMETHODIMP OpInterface::SetClipboard(BSTR str, LONG *ret) {
    obj.SetClipboard(str, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::GetClipboard(BSTR *ret) {
    wstring s;
    obj.GetClipboard(s);

    CComBSTR newstr;
    auto hr = newstr.Append(s.data());
    hr = newstr.CopyTo(ret);
    return hr;
}

STDMETHODIMP OpInterface::Delay(LONG mis, LONG *ret) {
    obj.Delay(mis, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::Delays(LONG mis_min, LONG mis_max, LONG *ret) {
    obj.Delays(mis_min, mis_max, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::BindWindow(LONGLONG hwnd, BSTR display, BSTR mouse, BSTR keypad, LONG mode, LONG *ret) {
    obj.BindWindow(static_cast<LONG_PTR>(hwnd), display, mouse, keypad, mode, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::BindWindowEx(LONGLONG display_hwnd, LONGLONG input_hwnd, BSTR display, BSTR mouse, BSTR keypad,
                                       LONG mode, LONG *ret) {
    obj.BindWindowEx(static_cast<LONG_PTR>(display_hwnd), static_cast<LONG_PTR>(input_hwnd), display, mouse, keypad, mode, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::UnBindWindow(LONG *ret) {
    obj.UnBindWindow(ret);

    return S_OK;
}

STDMETHODIMP OpInterface::GetBindWindow(LONGLONG *ret) {
    LONG_PTR hwnd = 0;
    obj.GetBindWindow(&hwnd);
    *ret = static_cast<LONGLONG>(hwnd);
    return S_OK;
}

STDMETHODIMP OpInterface::IsBind(LONG *ret) {
    obj.IsBind(ret);
    return S_OK;
}

STDMETHODIMP OpInterface::GetCursorPos(VARIANT *x, VARIANT *y, LONG *ret) {
    x->vt = y->vt = VT_I4;
    obj.GetCursorPos(&x->lVal, &y->lVal, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::GetCursorShape(BSTR *ret) {
    if (!ret)
        return E_POINTER;

    std::wstring value;
    obj.GetCursorShape(value);
    *ret = ::SysAllocString(value.c_str());
    return S_OK;
}

STDMETHODIMP OpInterface::MoveR(LONG x, LONG y, LONG *ret) {
    obj.MoveR(x, y, ret);

    return S_OK;
}
// 把鼠标移动到目的点(x,y)
STDMETHODIMP OpInterface::MoveTo(LONG x, LONG y, LONG *ret) {
    obj.MoveTo(x, y, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::MoveToEx(LONG x, LONG y, LONG w, LONG h, BSTR *ret) {
    if (!ret)
        return E_POINTER;

    std::wstring s;
    obj.MoveToEx(x, y, w, h, s);
    *ret = ::SysAllocString(s.c_str());

    return S_OK;
}

STDMETHODIMP OpInterface::LeftClick(LONG *ret) {
    obj.LeftClick(ret);

    return S_OK;
}

STDMETHODIMP OpInterface::LeftDoubleClick(LONG *ret) {
    obj.LeftDoubleClick(ret);

    return S_OK;
}

STDMETHODIMP OpInterface::LeftDown(LONG *ret) {
    obj.LeftDown(ret);

    return S_OK;
}

STDMETHODIMP OpInterface::LeftUp(LONG *ret) {
    obj.LeftUp(ret);

    return S_OK;
}

STDMETHODIMP OpInterface::MiddleClick(LONG *ret) {
    obj.MiddleClick(ret);

    return S_OK;
}

STDMETHODIMP OpInterface::MiddleDown(LONG *ret) {
    obj.MiddleDown(ret);

    return S_OK;
}

STDMETHODIMP OpInterface::MiddleUp(LONG *ret) {
    obj.MiddleUp(ret);

    return S_OK;
}

STDMETHODIMP OpInterface::RightClick(LONG *ret) {
    obj.RightClick(ret);

    return S_OK;
}

STDMETHODIMP OpInterface::RightDown(LONG *ret) {
    obj.RightDown(ret);

    return S_OK;
}

STDMETHODIMP OpInterface::RightUp(LONG *ret) {
    obj.RightUp(ret);

    return S_OK;
}

STDMETHODIMP OpInterface::WheelDown(LONG *ret) {
    obj.WheelDown(ret);

    return S_OK;
}

STDMETHODIMP OpInterface::WheelUp(LONG *ret) {
    obj.WheelUp(ret);
    return S_OK;
}

STDMETHODIMP OpInterface::SetMouseDelay(BSTR type, LONG delay, LONG *ret) {
    obj.SetMouseDelay(type, delay, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::GetKeyState(LONG vk_code, LONG *ret) {
    obj.GetKeyState(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::KeyDown(LONG vk_code, LONG *ret) {
    obj.KeyDown(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::KeyDownChar(BSTR vk_code, LONG *ret) {
    obj.KeyDownChar(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::KeyUp(LONG vk_code, LONG *ret) {
    obj.KeyUp(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::KeyUpChar(BSTR vk_code, LONG *ret) {
    obj.KeyUpChar(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::WaitKey(LONG vk_code, LONG time_out, LONG *ret) {
    obj.WaitKey(vk_code, time_out, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::KeyPress(LONG vk_code, LONG *ret) {

    obj.KeyPress(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::KeyPressChar(BSTR vk_code, LONG *ret) {
    obj.KeyPressChar(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::SetKeypadDelay(BSTR type, LONG delay, LONG *ret) {
    obj.SetKeypadDelay(type, delay, ret);
    return S_OK;
}
STDMETHODIMP OpInterface::KeyPressStr(BSTR key_str, LONG delay, LONG *ret) {
    obj.KeyPressStr(key_str, delay, ret);
    return S_OK;
}

// 抓取指定区域(x1, y1, x2, y2)的图像, 保存为file
STDMETHODIMP OpInterface::Capture(LONG x1, LONG y1, LONG x2, LONG y2, BSTR file_name, LONG *ret) {

    obj.Capture(x1, y1, x2, y2, file_name, ret);

    return S_OK;
}
// 比较指定坐标点(x,y)的颜色
STDMETHODIMP OpInterface::CmpColor(LONG x, LONG y, BSTR color, DOUBLE sim, LONG *ret) {
    obj.CmpColor(x, y, color, sim, ret);

    return S_OK;
}
// 查找指定区域内的颜色
STDMETHODIMP OpInterface::FindColor(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG dir, VARIANT *x,
                                    VARIANT *y, LONG *ret) {

    x->vt = y->vt = VT_I4;

    obj.FindColor(x1, y1, x2, y2, color, sim, dir, &x->lVal, &y->lVal, ret);

    return S_OK;
}
// 查找指定区域内的所有颜色
STDMETHODIMP OpInterface::FindColorEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG dir,
                                      BSTR *retstr) {
    wstring s;
    obj.FindColorEx(x1, y1, x2, y2, color, sim, dir, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}
// 查找指定区域内的颜色数量
STDMETHODIMP OpInterface::GetColorNum(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG *ret) {
    wstring s;
    obj.GetColorNum(x1, y1, x2, y2, color, sim, ret);

    return S_OK;
}
// 根据指定的多点查找颜色坐标
STDMETHODIMP OpInterface::FindMultiColor(LONG x1, LONG y1, LONG x2, LONG y2, BSTR first_color, BSTR offset_color,
                                         DOUBLE sim, LONG dir, VARIANT *x, VARIANT *y, LONG *ret) {
    LONG rx = -1, ry = -1;
    *ret = 0;
    x->vt = y->vt = VT_I4;
    obj.FindMultiColor(x1, y1, x2, y2, first_color, offset_color, sim, dir, &x->lVal, &y->lVal, ret);

    return S_OK;
}
// 根据指定的多点查找所有颜色坐标
STDMETHODIMP OpInterface::FindMultiColorEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR first_color, BSTR offset_color,
                                           DOUBLE sim, LONG dir, BSTR *retstr) {
    wstring s;
    obj.FindMultiColorEx(x1, y1, x2, y2, first_color, offset_color, sim, dir, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}
// 查找指定区域内的图片
STDMETHODIMP OpInterface::FindPic(LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim,
                                  LONG dir, VARIANT *x, VARIANT *y, LONG *ret) {

    x->vt = y->vt = VT_I4;
    obj.FindPic(x1, y1, x2, y2, files, delta_color, sim, dir, &x->lVal, &y->lVal, ret);

    return S_OK;
}
// 查找多个图片
STDMETHODIMP OpInterface::FindPicEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim,
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
STDMETHODIMP OpInterface::FindPicExS(LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim,
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
STDMETHODIMP OpInterface::FindColorBlock(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG count,
                                         LONG height, LONG width, VARIANT *x, VARIANT *y, LONG *ret) {
    x->vt = y->vt = VT_I4;
    obj.FindColorBlock(x1, y1, x2, y2, color, sim, count, height, width, &x->lVal, &y->lVal, ret);
    return S_OK;
}
// 查找指定区域内的所有颜色块, 颜色格式"RRGGBB-DRDGDB", 注意, 和按键的颜色格式相反
STDMETHODIMP OpInterface::FindColorBlockEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG count,
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
STDMETHODIMP OpInterface::GetColor(LONG x, LONG y, BSTR *ret) {
    wstring s;
    obj.GetColor(x, y, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(ret);
    return S_OK;
}

STDMETHODIMP OpInterface::SetDisplayInput(BSTR mode, LONG *ret) {
    obj.SetDisplayInput(mode, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::LoadPic(BSTR pic_name, LONG *ret) {
    // to do;
    obj.LoadPic(pic_name, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::FreePic(BSTR pic_name, LONG *ret) {
    obj.FreePic(pic_name, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::LoadMemPic(BSTR pic_name, long long data, LONG size, LONG *ret) {
    obj.LoadMemPic(pic_name, (void *)data, size, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::GetPicSize(BSTR pic_name, VARIANT *width, VARIANT *height, LONG *ret) {
    width->vt = height->vt = VT_I4;
    obj.GetPicSize(pic_name, &width->lVal, &height->lVal, ret);
    return S_OK;
}

// 获取指定区域的图像,用二进制数据的方式返回
STDMETHODIMP OpInterface::GetScreenData(LONG x1, LONG y1, LONG x2, LONG y2, LONG *ret) {
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
    *ret = 0;
    size_t data_ = 0;
    obj.GetScreenData(x1, y1, x2, y2, &data_, ret);
    *ret = (long)data_;
    return S_OK;
}

STDMETHODIMP OpInterface::GetScreenDataBmp(LONG x1, LONG y1, LONG x2, LONG y2, VARIANT *data, VARIANT *size,
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

// 根据通配符获取文件集合. 方便用于FindPic和FindPicEx
STDMETHODIMP OpInterface::MatchPicName(BSTR pic_name, BSTR *ret) {
    wstring s;
    obj.MatchPicName(pic_name, s);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(ret);
    return S_OK;
}

// 设置字库文件
STDMETHODIMP OpInterface::SetDict(LONG idx, BSTR file_name, LONG *ret) {
    obj.SetDict(idx, file_name, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::GetDict(LONG idx, LONG font_index, BSTR *retstr) {
    std::wstring s;
    obj.GetDict(idx, font_index, s);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

// 设置字库文件
STDMETHODIMP OpInterface::SetMemDict(LONG idx, BSTR data, LONG size, LONG *ret) {
    obj.SetMemDict(idx, data, size, ret);

    return S_OK;
}

// 使用哪个字库文件进行识别
STDMETHODIMP OpInterface::UseDict(LONG idx, LONG *ret) {
    obj.UseDict(idx, ret);

    return S_OK;
}

// 给指定的字库中添加一条字库信息
STDMETHODIMP OpInterface::AddDict(LONG idx, BSTR dict_info, LONG *ret) {
    obj.AddDict(idx, dict_info, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::SaveDict(LONG idx, BSTR file_name, LONG *ret) {
    obj.SaveDict(idx, file_name, ret);
    return S_OK;
}

// 清空指定的字库
STDMETHODIMP OpInterface::ClearDict(LONG idx, LONG *ret) {
    obj.ClearDict(idx, ret);
    return S_OK;
}

// 获取指定的字库中的字符数量
STDMETHODIMP OpInterface::GetDictCount(LONG idx, LONG *ret) {
    obj.GetDictCount(idx, ret);
    return S_OK;
}

// 获取当前使用的字库序号
STDMETHODIMP OpInterface::GetNowDict(LONG *ret) {
    obj.GetNowDict(ret);
    return S_OK;
}

// 根据指定的范围,以及指定的颜色描述，提取点阵信息，类似于大漠工具里的单独提取
STDMETHODIMP OpInterface::FetchWord(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, BSTR word, BSTR *ret_str) {
    wstring s;
    obj.FetchWord(x1, y1, x2, y2, color, word, s);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(ret_str);
    return S_OK;
}
// 识别这个范围内所有满足条件的词组，这个识别函数不会用到字库. 只是识别大概形状的位置
STDMETHODIMP OpInterface::GetWordsNoDict(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, BSTR *ret_str) {
    if (!ret_str)
        return E_POINTER;

    *ret_str = nullptr;
    wstring s;
    obj.GetWordsNoDict(x1, y1, x2, y2, color, s);
    CComBSTR newstr;
    newstr.Append(s.data());
    return newstr.CopyTo(ret_str);
}
// 在使用GetWords进行词组识别以后,可以用此接口进行识别词组数量的计算
STDMETHODIMP OpInterface::GetWordResultCount(BSTR result, LONG *ret) {
    if (!ret)
        return E_POINTER;

    *ret = 0;
    obj.GetWordResultCount(result, ret);
    return S_OK;
}
// 在使用GetWords进行词组识别以后,可以用此接口进行识别各个词组的坐标
STDMETHODIMP OpInterface::GetWordResultPos(BSTR result, LONG index, VARIANT *x, VARIANT *y, LONG *ret) {
    if (!x || !y || !ret)
        return E_POINTER;

    ::VariantInit(x);
    ::VariantInit(y);
    x->vt = y->vt = VT_I4;
    x->lVal = 0;
    y->lVal = 0;
    *ret = 0;
    obj.GetWordResultPos(result, index, &x->lVal, &y->lVal, ret);
    return S_OK;
}
// 在使用GetWords进行词组识别以后,可以用此接口进行识别各个词组的内容
STDMETHODIMP OpInterface::GetWordResultStr(BSTR result, LONG index, BSTR *ret_str) {
    if (!ret_str)
        return E_POINTER;

    *ret_str = nullptr;
    wstring s;
    obj.GetWordResultStr(result, index, s);
    CComBSTR newstr;
    newstr.Append(s.data());
    return newstr.CopyTo(ret_str);
}

// 识别屏幕范围(x1,y1,x2,y2)内符合color_format的字符串,并且相似度为sim,sim取值范围(0.1-1.0),
STDMETHODIMP OpInterface::Ocr(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR *ret_str) {
    wstring s;
    obj.Ocr(x1, y1, x2, y2, color, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(ret_str);
    return S_OK;
}
// 回识别到的字符串，以及每个字符的坐标.
STDMETHODIMP OpInterface::OcrEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR *ret_str) {
    wstring s;
    obj.OcrEx(x1, y1, x2, y2, color, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(ret_str);
    return S_OK;
}
// 在屏幕范围(x1,y1,x2,y2)内,查找string(可以是任意个字符串的组合),并返回符合color_format的坐标位置
STDMETHODIMP OpInterface::FindStr(LONG x1, LONG y1, LONG x2, LONG y2, BSTR strs, BSTR color, DOUBLE sim, VARIANT *retx,
                                  VARIANT *rety, LONG *ret) {

    retx->vt = rety->vt = VT_INT;
    obj.FindStr(x1, y1, x2, y2, strs, color, sim, &retx->lVal, &rety->lVal, ret);

    return S_OK;
}
// 返回符合color_format的所有坐标位置
STDMETHODIMP OpInterface::FindStrEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR strs, BSTR color, DOUBLE sim,
                                    BSTR *retstr) {
    wstring s;
    obj.FindStrEx(x1, y1, x2, y2, strs, color, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpInterface::OcrAuto(LONG x1, LONG y1, LONG x2, LONG y2, DOUBLE sim, BSTR *retstr) {
    wstring s;
    obj.OcrAuto(x1, y1, x2, y2, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

// 从文件中识别图片
STDMETHODIMP OpInterface::OcrFromFile(BSTR file_name, BSTR color_format, DOUBLE sim, BSTR *retstr) {
    wstring s;
    obj.OcrFromFile(file_name, color_format, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}
// 从文件中识别图片,无需指定颜色
STDMETHODIMP OpInterface::OcrAutoFromFile(BSTR file_name, DOUBLE sim, BSTR *retstr) {
    wstring s;
    obj.OcrAutoFromFile(file_name, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpInterface::FindLine(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR *retstr) {
    wstring s;
    obj.FindLine(x1, y1, x2, y2, color, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpInterface::SetOcrEngine(BSTR path_of_engine, BSTR dll_name, BSTR argv, LONG *ret) {
    *ret = obj.SetOcrEngine(path_of_engine, dll_name, argv);
    return S_OK;
}

STDMETHODIMP OpInterface::WriteData(LONGLONG hwnd, BSTR address, BSTR data, LONG size, LONG *ret) {
    obj.WriteData(static_cast<LONG_PTR>(hwnd), address, data, size, ret);

    return S_OK;
}

STDMETHODIMP OpInterface::ReadData(LONGLONG hwnd, BSTR address, LONG size, BSTR *retstr) {
    wstring s;
    obj.ReadData(static_cast<LONG_PTR>(hwnd), address, size, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpInterface::CvLoadTemplate(BSTR name, BSTR file_path, LONG *ret) {
    obj.CvLoadTemplate(name, file_path, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::CvLoadMaskedTemplate(BSTR name, BSTR template_path, BSTR mask_path, LONG *ret) {
    obj.CvLoadMaskedTemplate(name, template_path, mask_path, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::CvRemoveTemplate(BSTR name, LONG *ret) {
    obj.CvRemoveTemplate(name, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::CvRemoveAllTemplates(LONG *ret) {
    obj.CvRemoveAllTemplates(ret);
    return S_OK;
}

STDMETHODIMP OpInterface::CvHasTemplate(BSTR name, LONG *ret) {
    obj.CvHasTemplate(name, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::CvGetTemplateCount(LONG *ret) {
    obj.CvGetTemplateCount(ret);
    return S_OK;
}

STDMETHODIMP OpInterface::CvGetAllTemplateNames(BSTR *retstr) {
    wstring s;
    obj.CvGetAllTemplateNames(s);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpInterface::CvGetOpenCvVersion(BSTR *retstr) {
    wstring s;
    obj.CvGetOpenCvVersion(s);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpInterface::CvLoadTemplateList(BSTR template_list, LONG *ret) {
    obj.CvLoadTemplateList(template_list, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::CvToGray(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvToGray(src_file, dst_file, out); });
}

STDMETHODIMP OpInterface::CvToBinary(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvToBinary(src_file, dst_file, out); });
}

STDMETHODIMP OpInterface::CvToEdge(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvToEdge(src_file, dst_file, out); });
}

STDMETHODIMP OpInterface::CvToOutline(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvToOutline(src_file, dst_file, out); });
}

STDMETHODIMP OpInterface::CvDenoise(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvDenoise(src_file, dst_file, out); });
}

STDMETHODIMP OpInterface::CvEqualize(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvEqualize(src_file, dst_file, out); });
}

STDMETHODIMP OpInterface::CvCLAHE(BSTR src_file, BSTR dst_file, DOUBLE clip_limit, LONG tile_grid_size, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvCLAHE(src_file, dst_file, clip_limit, tile_grid_size, out); });
}

STDMETHODIMP OpInterface::CvBlur(BSTR src_file, BSTR dst_file, BSTR mode, LONG kernel_size, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvBlur(src_file, dst_file, mode, kernel_size, out); });
}

STDMETHODIMP OpInterface::CvSharpen(BSTR src_file, BSTR dst_file, DOUBLE strength, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvSharpen(src_file, dst_file, strength, out); });
}

STDMETHODIMP OpInterface::CvCropValid(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvCropValid(src_file, dst_file, out); });
}

STDMETHODIMP OpInterface::CvConnectedComponents(BSTR src_file, DOUBLE min_area, BSTR *retjson, LONG *ret) {
    if (!retjson || !ret) {
        return E_POINTER;
    }

    *retjson = nullptr;
    *ret = 0;

    std::wstring s;
    obj.CvConnectedComponents(src_file, min_area, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    return newstr.CopyTo(retjson);
}

STDMETHODIMP OpInterface::CvFindContours(BSTR src_file, DOUBLE min_area, BSTR *retjson, LONG *ret) {
    if (!retjson || !ret) {
        return E_POINTER;
    }

    *retjson = nullptr;
    *ret = 0;

    std::wstring s;
    obj.CvFindContours(src_file, min_area, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    return newstr.CopyTo(retjson);
}

STDMETHODIMP OpInterface::CvPreprocessPipeline(BSTR src_file, BSTR dst_file, BSTR pipeline, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvPreprocessPipeline(src_file, dst_file, pipeline, out); });
}

STDMETHODIMP OpInterface::CvCrop(BSTR src_file, LONG x, LONG y, LONG width, LONG height, BSTR dst_file, LONG *ret) {
    obj.CvCrop(src_file, x, y, width, height, dst_file, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::CvResize(BSTR src_file, LONG width, LONG height, BSTR dst_file, LONG *ret) {
    obj.CvResize(src_file, width, height, dst_file, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::CvThreshold(BSTR src_file, BSTR dst_file, DOUBLE threshold, DOUBLE max_value, BSTR mode,
                                      LONG *ret) {
    if (!ret)
        return E_POINTER;

    *ret = 0;
    obj.CvThreshold(src_file, dst_file, threshold, max_value, mode, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::CvInRange(BSTR src_file, BSTR dst_file, BSTR color_space, BSTR lower, BSTR upper, LONG *ret) {
    if (!ret)
        return E_POINTER;

    *ret = 0;
    obj.CvInRange(src_file, dst_file, color_space, lower, upper, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::CvMorphology(BSTR src_file, BSTR dst_file, BSTR mode, LONG kernel_size, LONG iterations,
                                       LONG *ret) {
    if (!ret)
        return E_POINTER;

    *ret = 0;
    obj.CvMorphology(src_file, dst_file, mode, kernel_size, iterations, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::CvThin(BSTR src_file, BSTR dst_file, BSTR mode, LONG *ret) {
    if (!ret)
        return E_POINTER;

    *ret = 0;
    obj.CvThin(src_file, dst_file, mode, ret);
    return S_OK;
}

STDMETHODIMP OpInterface::CvMatchTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                          DOUBLE threshold, LONG dir, LONG strip_mode, LONG method, LONG color_mode,
                                          BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvMatchTemplate(x, y, width, height, template_name, threshold, dir, strip_mode, method, color_mode, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

STDMETHODIMP OpInterface::CvMatchTemplateScale(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                               BSTR scales, DOUBLE threshold, LONG method, LONG color_mode,
                                               BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvMatchTemplateScale(x, y, width, height, template_name, scales, threshold, method, color_mode, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

STDMETHODIMP OpInterface::CvMatchAnyTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_names,
                                             DOUBLE threshold, LONG dir, LONG strip_mode, LONG method, LONG color_mode,
                                             BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvMatchAnyTemplate(x, y, width, height, template_names, threshold, dir, strip_mode, method, color_mode, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

STDMETHODIMP OpInterface::CvMatchAllTemplates(LONG x, LONG y, LONG width, LONG height, BSTR template_names,
                                              DOUBLE threshold, LONG dir, LONG strip_mode, LONG method, LONG color_mode,
                                              BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvMatchAllTemplates(x, y, width, height, template_names, threshold, dir, strip_mode, method, color_mode, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

STDMETHODIMP OpInterface::CvFeatureMatchTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                                 DOUBLE threshold, BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvFeatureMatchTemplate(x, y, width, height, template_name, threshold, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

STDMETHODIMP OpInterface::CvEdgeMatchTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                              DOUBLE threshold, BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvEdgeMatchTemplate(x, y, width, height, template_name, threshold, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

STDMETHODIMP OpInterface::CvShapeMatchTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                               DOUBLE threshold, BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvShapeMatchTemplate(x, y, width, height, template_name, threshold, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

