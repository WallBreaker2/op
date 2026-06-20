// OpAutomation.cpp: OpAutomation 的实现

#include "OpAutomation.h"
#include "stdafx.h"

#include "../runtime/AutomationModes.h"
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

STDMETHODIMP OpAutomation::RuntimeVer(BSTR *ret) {

    // Tool::setlog("address=%d,str=%s", ver, ver);
    wstring s = obj.Ver();
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::RuntimeSetPath(BSTR path, LONG *ret) {

    obj.SetPath(path, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::RuntimeGetPath(BSTR *path) {
    wstring s;
    obj.GetPath(s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(path);
    return S_OK;
}

STDMETHODIMP OpAutomation::RuntimeGetBasePath(BSTR *path) {

    wstring s;
    obj.GetBasePath(s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(path);
    return S_OK;
}

STDMETHODIMP OpAutomation::RuntimeGetID(LONG *ret) {
    obj.GetID(ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::RuntimeGetLastError(LONG *ret) {
    obj.GetLastError(ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::RuntimeSetShowErrorMsg(LONG show_type, LONG *ret) {
    obj.SetShowErrorMsg(show_type, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::RuntimeSleep(LONG millseconds, LONG *ret) {

    obj.Sleep(millseconds, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::WindowInjectDll(BSTR process_name, BSTR dll_name, LONG *ret) {
    // auto proc = _wsto_string(process_name);
    // auto dll = _wsto_string(dll_name);
    // DllInjector::EnablePrivilege(TRUE);
    // auto h = DllInjector::InjectDll(process_name, dll_name);
    obj.InjectDll(process_name, dll_name, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::ImageEnablePicCache(LONG enable, LONG *ret) {

    obj.EnablePicCache(enable, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::ImageCapturePre(BSTR file, LONG *ret) {

    obj.CapturePre(file, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::AlgorithmAStarFindPath(LONG mapWidth, LONG mapHeight, BSTR disable_points, LONG beginX, LONG beginY,
                                        LONG endX, LONG endY, BSTR *path) {
    wstring s;
    obj.AStarFindPath(mapWidth, mapHeight, disable_points, beginX, beginY, endX, endY, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(path);
    return S_OK;
}

// 根据部分Ex接口的返回值，然后在所有坐标里找出距离指定坐标最近的那个坐标.
STDMETHODIMP OpAutomation::AlgorithmFindNearestPos(BSTR all_pos, LONG type, LONG x, LONG y, BSTR *retstr) {
    std::wstring s;
    obj.FindNearestPos(all_pos, type, x, y, s);
    CComBSTR newbstr;
    auto hr = newbstr.Append(s.data());
    hr = newbstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpAutomation::WindowEnumWindow(LONGLONG parent, BSTR title, BSTR class_name, LONG filter, BSTR *retstr) {
    wstring s;
    obj.EnumWindow(static_cast<LONG_PTR>(parent), title, class_name, filter, s);

    CComBSTR newbstr;
    auto hr = newbstr.Append(s.data());
    hr = newbstr.CopyTo(retstr);
    return hr;
}

STDMETHODIMP OpAutomation::WindowEnumWindowByProcess(BSTR process_name, BSTR title, BSTR class_name, LONG filter,
                                              BSTR *retstring) {
    wstring s;
    obj.EnumWindowByProcess(process_name, title, class_name, filter, s);

    CComBSTR newbstr;
    newbstr.Append(s.data());
    newbstr.CopyTo(retstring);
    return S_OK;
}

STDMETHODIMP OpAutomation::WindowEnumProcess(BSTR name, BSTR *retstring) {
    wstring s;
    obj.EnumProcess(name, s);

    CComBSTR newbstr;
    newbstr.Append(s.data());
    newbstr.CopyTo(retstring);
    return S_OK;
}

STDMETHODIMP OpAutomation::WindowClientToScreen(LONGLONG hwnd, VARIANT *x, VARIANT *y, LONG *bret) {
    x->vt = VT_I4;
    y->vt = VT_I4;
    long lx = x->lVal;
    long ly = y->lVal;
    obj.ClientToScreen(static_cast<LONG_PTR>(hwnd), &lx, &ly, bret);
    x->lVal = lx;
    y->lVal = ly;
    return S_OK;
}

STDMETHODIMP OpAutomation::WindowFindWindow(BSTR class_name, BSTR title, LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.FindWindow(class_name, title, &hwnd);
    return SetOutValue(rethwnd, hwnd);
}

STDMETHODIMP OpAutomation::WindowFindWindowByProcess(BSTR process_name, BSTR class_name, BSTR title, LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.FindWindowByProcess(process_name, class_name, title, &hwnd);
    return SetOutValue(rethwnd, hwnd);
}

STDMETHODIMP OpAutomation::WindowFindWindowByProcessId(LONG process_id, BSTR class_name, BSTR title, LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.FindWindowByProcessId(process_id, class_name, title, &hwnd);
    return SetOutValue(rethwnd, hwnd);
}

STDMETHODIMP OpAutomation::WindowFindWindowEx(LONGLONG parent, BSTR class_name, BSTR title, LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.FindWindowEx(static_cast<LONG_PTR>(parent), class_name, title, &hwnd);
    return SetOutValue(rethwnd, hwnd);
}

STDMETHODIMP OpAutomation::WindowGetClientRect(LONGLONG hwnd, VARIANT *x1, VARIANT *y1, VARIANT *x2, VARIANT *y2, LONG *nret) {
    x1->vt = VT_I4;
    y1->vt = VT_I4;
    x2->vt = VT_I4;
    y2->vt = VT_I4;
    obj.GetClientRect(static_cast<LONG_PTR>(hwnd), &x1->lVal, &y1->lVal, &x2->lVal, &y2->lVal, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::WindowGetClientSize(LONGLONG hwnd, VARIANT *width, VARIANT *height, LONG *nret) {
    width->vt = VT_I4;
    height->vt = VT_I4;
    obj.GetClientSize(static_cast<LONG_PTR>(hwnd), &width->lVal, &height->lVal, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::WindowGetForegroundFocus(LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.GetForegroundFocus(&hwnd);
    return SetOutValue(rethwnd, hwnd);
}

STDMETHODIMP OpAutomation::WindowGetForegroundWindow(LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.GetForegroundWindow(&hwnd);
    return SetOutValue(rethwnd, hwnd);
}

STDMETHODIMP OpAutomation::WindowGetMousePointWindow(LONGLONG *rethwnd) {
    //::Sleep(2000);
    LONG_PTR hwnd = 0;
    obj.GetMousePointWindow(&hwnd);
    return SetOutValue(rethwnd, hwnd);
}

STDMETHODIMP OpAutomation::WindowGetPointWindow(LONG x, LONG y, LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.GetPointWindow(x, y, &hwnd);
    return SetOutValue(rethwnd, hwnd);
}

STDMETHODIMP OpAutomation::WindowGetProcessInfo(LONG pid, BSTR *retstring) {
    wstring s;
    obj.GetProcessInfo(pid, s);

    CComBSTR newbstr;
    newbstr.Append(s.data());
    newbstr.CopyTo(retstring);
    return S_OK;
}

STDMETHODIMP OpAutomation::WindowGetSpecialWindow(LONG flag, LONGLONG *rethwnd) {
    LONG_PTR hwnd = 0;
    obj.GetSpecialWindow(flag, &hwnd);
    return SetOutValue(rethwnd, hwnd);
}

STDMETHODIMP OpAutomation::WindowGetWindow(LONGLONG hwnd, LONG flag, LONGLONG *nret) {
    LONG_PTR target = 0;
    obj.GetWindow(static_cast<LONG_PTR>(hwnd), flag, &target);
    return SetOutValue(nret, target);
}

STDMETHODIMP OpAutomation::WindowGetWindowClass(LONGLONG hwnd, BSTR *retstring) {
    wstring s;
    obj.GetWindowClass(static_cast<LONG_PTR>(hwnd), s);

    CComBSTR newbstr;
    newbstr.Append(s.data());
    newbstr.CopyTo(retstring);
    return S_OK;
}

STDMETHODIMP OpAutomation::WindowGetWindowProcessId(LONGLONG hwnd, LONG *nretpid) {
    obj.GetWindowProcessId(static_cast<LONG_PTR>(hwnd), nretpid);

    return S_OK;
}

STDMETHODIMP OpAutomation::WindowGetWindowProcessPath(LONGLONG hwnd, BSTR *retstring) {
    wstring s;
    obj.GetWindowProcessPath(static_cast<LONG_PTR>(hwnd), s);

    CComBSTR newbstr;
    newbstr.Append(s.data());
    newbstr.CopyTo(retstring);
    return S_OK;
}

STDMETHODIMP OpAutomation::WindowGetWindowRect(LONGLONG hwnd, VARIANT *x1, VARIANT *y1, VARIANT *x2, VARIANT *y2, LONG *nret) {
    x1->vt = VT_I4;
    x2->vt = VT_I4;
    y1->vt = VT_I4;
    y2->vt = VT_I4;

    obj.GetWindowRect(static_cast<LONG_PTR>(hwnd), &x1->lVal, &y1->lVal, &x2->lVal, &y2->lVal, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::WindowGetWindowState(LONGLONG hwnd, LONG flag, LONG *rethwnd) {
    obj.GetWindowState(static_cast<LONG_PTR>(hwnd), flag, rethwnd);

    return S_OK;
}

STDMETHODIMP OpAutomation::WindowGetWindowTitle(LONGLONG hwnd, BSTR *rettitle) {
    wstring s;
    obj.GetWindowTitle(static_cast<LONG_PTR>(hwnd), s);

    CComBSTR newbstr;
    newbstr.Append(s.data());
    newbstr.CopyTo(rettitle);
    return S_OK;
}

STDMETHODIMP OpAutomation::WindowMoveWindow(LONGLONG hwnd, LONG x, LONG y, LONG *nret) {
    obj.MoveWindow(static_cast<LONG_PTR>(hwnd), x, y, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::WindowScreenToClient(LONGLONG hwnd, VARIANT *x, VARIANT *y, LONG *nret) {
    x->vt = VT_I4;
    y->vt = VT_I4;
    obj.ScreenToClient(static_cast<LONG_PTR>(hwnd), &x->lVal, &y->lVal, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::WindowSendPaste(LONGLONG hwnd, LONG *nret) {
    obj.SendPaste(static_cast<LONG_PTR>(hwnd), nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::WindowSetClientSize(LONGLONG hwnd, LONG width, LONG hight, LONG *nret) {
    obj.SetClientSize(static_cast<LONG_PTR>(hwnd), width, hight, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::WindowSetWindowState(LONGLONG hwnd, LONG flag, LONG *nret) {
    obj.SetWindowState(static_cast<LONG_PTR>(hwnd), flag, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::WindowSetWindowSize(LONGLONG hwnd, LONG width, LONG height, LONG *nret) {
    obj.SetWindowSize(static_cast<LONG_PTR>(hwnd), width, height, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::WindowSetWindowText(LONGLONG hwnd, BSTR title, LONG *nret) {
    obj.SetWindowText(static_cast<LONG_PTR>(hwnd), title, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::WindowSetWindowTransparent(LONGLONG hwnd, LONG trans, LONG *nret) {
    obj.SetWindowTransparent(static_cast<LONG_PTR>(hwnd), trans, nret);

    return S_OK;
}

STDMETHODIMP OpAutomation::WindowSendString(LONGLONG hwnd, BSTR str, LONG *ret) {
    obj.SendString(static_cast<LONG_PTR>(hwnd), str, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::WindowSendStringIme(LONGLONG hwnd, BSTR str, LONG *ret) {
    obj.SendStringIme(static_cast<LONG_PTR>(hwnd), str, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::WindowRunApp(BSTR cmdline, LONG mode, ULONG *pid, LONG *ret) {
    obj.RunApp(cmdline, mode, pid, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::WindowLayoutWindows(BSTR hwnds, LONG layout_type, LONG columns, LONG start_x, LONG start_y, LONG gap_x,
                                        LONG gap_y, LONG size_mode, LONG window_width, LONG window_height, LONG anchor_mode,
                                        LONG *ret) {
    obj.LayoutWindows(hwnds, layout_type, columns, start_x, start_y, gap_x, gap_y, size_mode, window_width,
                      window_height, anchor_mode, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::WindowWinExec(BSTR cmdline, LONG cmdshow, LONG *ret) {
    obj.WinExec(cmdline, cmdshow, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::WindowGetCmdStr(BSTR cmd, LONG millseconds, BSTR *retstr) {
    wstring s;
    obj.GetCmdStr(cmd, millseconds, s);

    CComBSTR newstr;

    auto hr = newstr.Append(s.data());
    hr = newstr.CopyTo(retstr);
    return hr;
}

STDMETHODIMP OpAutomation::WindowSetClipboard(BSTR str, LONG *ret) {
    obj.SetClipboard(str, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::WindowGetClipboard(BSTR *ret) {
    wstring s;
    obj.GetClipboard(s);

    CComBSTR newstr;
    auto hr = newstr.Append(s.data());
    hr = newstr.CopyTo(ret);
    return hr;
}

STDMETHODIMP OpAutomation::RuntimeDelay(LONG mis, LONG *ret) {
    obj.Delay(mis, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::RuntimeDelays(LONG mis_min, LONG mis_max, LONG *ret) {
    obj.Delays(mis_min, mis_max, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::BindingBindWindow(LONGLONG hwnd, BSTR display, BSTR mouse, BSTR keypad, LONG mode, LONG *ret) {
    obj.BindWindow(static_cast<LONG_PTR>(hwnd), display, mouse, keypad, mode, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::BindingBindWindowEx(LONGLONG display_hwnd, LONGLONG input_hwnd, BSTR display, BSTR mouse, BSTR keypad,
                                       LONG mode, LONG *ret) {
    obj.BindWindowEx(static_cast<LONG_PTR>(display_hwnd), static_cast<LONG_PTR>(input_hwnd), display, mouse, keypad, mode, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::BindingUnBindWindow(LONG *ret) {
    obj.UnBindWindow(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::BindingGetBindWindow(LONGLONG *ret) {
    LONG_PTR hwnd = 0;
    obj.GetBindWindow(&hwnd);
    return SetOutValue(ret, hwnd);
}

STDMETHODIMP OpAutomation::BindingIsBind(LONG *ret) {
    obj.IsBind(ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::MouseGetCursorPos(VARIANT *x, VARIANT *y, LONG *ret) {
    x->vt = y->vt = VT_I4;
    obj.GetCursorPos(&x->lVal, &y->lVal, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MouseGetCursorShape(BSTR *ret) {
    if (!ret)
        return E_POINTER;

    std::wstring value;
    obj.GetCursorShape(value);
    return CopyOutBstr(ret, value);
}

STDMETHODIMP OpAutomation::MouseMoveR(LONG x, LONG y, LONG *ret) {
    obj.MoveR(x, y, ret);

    return S_OK;
}
// 把鼠标移动到目的点(x,y)
STDMETHODIMP OpAutomation::MouseMoveTo(LONG x, LONG y, LONG *ret) {
    obj.MoveTo(x, y, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MouseMoveToEx(LONG x, LONG y, LONG w, LONG h, BSTR *ret) {
    if (!ret)
        return E_POINTER;

    std::wstring s;
    obj.MoveToEx(x, y, w, h, s);
    return CopyOutBstr(ret, s);
}

STDMETHODIMP OpAutomation::MouseLeftClick(LONG *ret) {
    obj.LeftClick(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MouseLeftDoubleClick(LONG *ret) {
    obj.LeftDoubleClick(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MouseLeftDown(LONG *ret) {
    obj.LeftDown(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MouseLeftUp(LONG *ret) {
    obj.LeftUp(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MouseMiddleClick(LONG *ret) {
    obj.MiddleClick(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MouseMiddleDown(LONG *ret) {
    obj.MiddleDown(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MouseMiddleUp(LONG *ret) {
    obj.MiddleUp(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MouseRightClick(LONG *ret) {
    obj.RightClick(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MouseRightDown(LONG *ret) {
    obj.RightDown(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MouseRightUp(LONG *ret) {
    obj.RightUp(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MouseWheelDown(LONG *ret) {
    obj.WheelDown(ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MouseWheelUp(LONG *ret) {
    obj.WheelUp(ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::MouseSetMouseDelay(BSTR type, LONG delay, LONG *ret) {
    obj.SetMouseDelay(type, delay, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::KeyboardGetKeyState(LONG vk_code, LONG *ret) {
    obj.GetKeyState(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::KeyboardKeyDown(LONG vk_code, LONG *ret) {
    obj.KeyDown(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::KeyboardKeyDownChar(BSTR vk_code, LONG *ret) {
    obj.KeyDownChar(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::KeyboardKeyUp(LONG vk_code, LONG *ret) {
    obj.KeyUp(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::KeyboardKeyUpChar(BSTR vk_code, LONG *ret) {
    obj.KeyUpChar(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::KeyboardWaitKey(LONG vk_code, LONG time_out, LONG *ret) {
    obj.WaitKey(vk_code, time_out, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::KeyboardKeyPress(LONG vk_code, LONG *ret) {

    obj.KeyPress(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::KeyboardKeyPressChar(BSTR vk_code, LONG *ret) {
    obj.KeyPressChar(vk_code, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::KeyboardSetKeypadDelay(BSTR type, LONG delay, LONG *ret) {
    obj.SetKeypadDelay(type, delay, ret);
    return S_OK;
}
STDMETHODIMP OpAutomation::KeyboardKeyPressStr(BSTR key_str, LONG delay, LONG *ret) {
    obj.KeyPressStr(key_str, delay, ret);
    return S_OK;
}

// 抓取指定区域(x1, y1, x2, y2)的图像, 保存为file
STDMETHODIMP OpAutomation::ImageCapture(LONG x1, LONG y1, LONG x2, LONG y2, BSTR file_name, LONG *ret) {

    obj.Capture(x1, y1, x2, y2, file_name, ret);

    return S_OK;
}
// 比较指定坐标点(x,y)的颜色
STDMETHODIMP OpAutomation::ImageCmpColor(LONG x, LONG y, BSTR color, DOUBLE sim, LONG *ret) {
    obj.CmpColor(x, y, color, sim, ret);

    return S_OK;
}
// 查找指定区域内的颜色
STDMETHODIMP OpAutomation::ImageFindColor(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG dir, VARIANT *x,
                                    VARIANT *y, LONG *ret) {

    x->vt = y->vt = VT_I4;

    obj.FindColor(x1, y1, x2, y2, color, sim, dir, &x->lVal, &y->lVal, ret);

    return S_OK;
}
// 查找指定区域内的所有颜色
STDMETHODIMP OpAutomation::ImageFindColorEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG dir,
                                      BSTR *retstr) {
    wstring s;
    obj.FindColorEx(x1, y1, x2, y2, color, sim, dir, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}
// 查找指定区域内的颜色数量
STDMETHODIMP OpAutomation::ImageGetColorNum(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG *ret) {
    wstring s;
    obj.GetColorNum(x1, y1, x2, y2, color, sim, ret);

    return S_OK;
}
// 根据指定的多点查找颜色坐标
STDMETHODIMP OpAutomation::ImageFindMultiColor(LONG x1, LONG y1, LONG x2, LONG y2, BSTR first_color, BSTR offset_color,
                                         DOUBLE sim, LONG dir, VARIANT *x, VARIANT *y, LONG *ret) {
    if (!x || !y || !ret)
        return E_POINTER;

    SetOutValue(ret, 0L);
    x->vt = y->vt = VT_I4;
    obj.FindMultiColor(x1, y1, x2, y2, first_color, offset_color, sim, dir, &x->lVal, &y->lVal, ret);

    return S_OK;
}
// 根据指定的多点查找所有颜色坐标
STDMETHODIMP OpAutomation::ImageFindMultiColorEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR first_color, BSTR offset_color,
                                           DOUBLE sim, LONG dir, BSTR *retstr) {
    wstring s;
    obj.FindMultiColorEx(x1, y1, x2, y2, first_color, offset_color, sim, dir, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}
// 查找指定区域内的图片
STDMETHODIMP OpAutomation::ImageFindPic(LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim,
                                  LONG dir, VARIANT *x, VARIANT *y, LONG *ret) {

    x->vt = y->vt = VT_I4;
    obj.FindPic(x1, y1, x2, y2, files, delta_color, sim, dir, &x->lVal, &y->lVal, ret);

    return S_OK;
}
// 查找多个图片
STDMETHODIMP OpAutomation::ImageFindPicEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim,
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
STDMETHODIMP OpAutomation::ImageFindPicExS(LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim,
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
STDMETHODIMP OpAutomation::ImageFindColorBlock(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG count,
                                         LONG height, LONG width, VARIANT *x, VARIANT *y, LONG *ret) {
    x->vt = y->vt = VT_I4;
    obj.FindColorBlock(x1, y1, x2, y2, color, sim, count, height, width, &x->lVal, &y->lVal, ret);
    return S_OK;
}
// 查找指定区域内的所有颜色块, 颜色格式"RRGGBB-DRDGDB", 注意, 和按键的颜色格式相反
STDMETHODIMP OpAutomation::ImageFindColorBlockEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG count,
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
STDMETHODIMP OpAutomation::ImageGetColor(LONG x, LONG y, BSTR *ret) {
    wstring s;
    obj.GetColor(x, y, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::ImageSetDisplayInput(BSTR mode, LONG *ret) {
    obj.SetDisplayInput(mode, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::ImageLoadPic(BSTR pic_name, LONG *ret) {
    // to do;
    obj.LoadPic(pic_name, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::ImageFreePic(BSTR pic_name, LONG *ret) {
    obj.FreePic(pic_name, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::ImageLoadMemPic(BSTR pic_name, long long data, LONG size, LONG *ret) {
    obj.LoadMemPic(pic_name, (void *)data, size, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::ImageGetPicSize(BSTR pic_name, VARIANT *width, VARIANT *height, LONG *ret) {
    width->vt = height->vt = VT_I4;
    obj.GetPicSize(pic_name, &width->lVal, &height->lVal, ret);
    return S_OK;
}

// 获取指定区域的图像,用二进制数据的方式返回
STDMETHODIMP OpAutomation::ImageGetScreenData(LONG x1, LONG y1, LONG x2, LONG y2, LONG *ret) {
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

STDMETHODIMP OpAutomation::ImageGetScreenDataBmp(LONG x1, LONG y1, LONG x2, LONG y2, VARIANT *data, VARIANT *size,
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
STDMETHODIMP OpAutomation::ImageMatchPicName(BSTR pic_name, BSTR *ret) {
    wstring s;
    obj.MatchPicName(pic_name, s);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(ret);
    return S_OK;
}

// 设置字库文件
STDMETHODIMP OpAutomation::OcrSetDict(LONG idx, BSTR file_name, LONG *ret) {
    obj.SetDict(idx, file_name, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::OcrGetDict(LONG idx, LONG font_index, BSTR *retstr) {
    std::wstring s;
    obj.GetDict(idx, font_index, s);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

// 设置字库文件
STDMETHODIMP OpAutomation::OcrSetMemDict(LONG idx, BSTR data, LONG size, LONG *ret) {
    obj.SetMemDict(idx, data, size, ret);

    return S_OK;
}

// 使用哪个字库文件进行识别
STDMETHODIMP OpAutomation::OcrUseDict(LONG idx, LONG *ret) {
    obj.UseDict(idx, ret);

    return S_OK;
}

// 给指定的字库中添加一条字库信息
STDMETHODIMP OpAutomation::OcrAddDict(LONG idx, BSTR dict_info, LONG *ret) {
    obj.AddDict(idx, dict_info, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::OcrSaveDict(LONG idx, BSTR file_name, LONG *ret) {
    obj.SaveDict(idx, file_name, ret);
    return S_OK;
}

// 清空指定的字库
STDMETHODIMP OpAutomation::OcrClearDict(LONG idx, LONG *ret) {
    obj.ClearDict(idx, ret);
    return S_OK;
}

// 获取指定的字库中的字符数量
STDMETHODIMP OpAutomation::OcrGetDictCount(LONG idx, LONG *ret) {
    obj.GetDictCount(idx, ret);
    return S_OK;
}

// 获取当前使用的字库序号
STDMETHODIMP OpAutomation::OcrGetNowDict(LONG *ret) {
    obj.GetNowDict(ret);
    return S_OK;
}

// 根据指定的范围,以及指定的颜色描述，提取点阵信息，类似于大漠工具里的单独提取
STDMETHODIMP OpAutomation::OcrFetchWord(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, BSTR word, BSTR *ret_str) {
    wstring s;
    obj.FetchWord(x1, y1, x2, y2, color, word, s);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(ret_str);
    return S_OK;
}
// 识别这个范围内所有满足条件的词组，这个识别函数不会用到字库. 只是识别大概形状的位置
STDMETHODIMP OpAutomation::OcrGetWordsNoDict(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, BSTR *ret_str) {
    if (!ret_str)
        return E_POINTER;

    wstring s;
    obj.GetWordsNoDict(x1, y1, x2, y2, color, s);
    return CopyOutBstr(ret_str, s);
}
// 在使用GetWords进行词组识别以后,可以用此接口进行识别词组数量的计算
STDMETHODIMP OpAutomation::OcrGetWordResultCount(BSTR result, LONG *ret) {
    if (!ret)
        return E_POINTER;

    SetOutValue(ret, 0L);
    obj.GetWordResultCount(result, ret);
    return S_OK;
}
// 在使用GetWords进行词组识别以后,可以用此接口进行识别各个词组的坐标
STDMETHODIMP OpAutomation::OcrGetWordResultPos(BSTR result, LONG index, VARIANT *x, VARIANT *y, LONG *ret) {
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
STDMETHODIMP OpAutomation::OcrGetWordResultStr(BSTR result, LONG index, BSTR *ret_str) {
    if (!ret_str)
        return E_POINTER;

    wstring s;
    obj.GetWordResultStr(result, index, s);
    return CopyOutBstr(ret_str, s);
}

// 识别屏幕范围(x1,y1,x2,y2)内符合color_format的字符串,并且相似度为sim,sim取值范围(0.1-1.0),
STDMETHODIMP OpAutomation::OcrRecognize(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR *ret_str) {
    wstring s;
    obj.Ocr(x1, y1, x2, y2, color, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(ret_str);
    return S_OK;
}
// 回识别到的字符串，以及每个字符的坐标.
STDMETHODIMP OpAutomation::OcrRecognizeEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR *ret_str) {
    wstring s;
    obj.OcrEx(x1, y1, x2, y2, color, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(ret_str);
    return S_OK;
}
// 在屏幕范围(x1,y1,x2,y2)内,查找string(可以是任意个字符串的组合),并返回符合color_format的坐标位置
STDMETHODIMP OpAutomation::OcrFindStr(LONG x1, LONG y1, LONG x2, LONG y2, BSTR strs, BSTR color, DOUBLE sim, VARIANT *retx,
                                  VARIANT *rety, LONG *ret) {

    retx->vt = rety->vt = VT_INT;
    obj.FindStr(x1, y1, x2, y2, strs, color, sim, &retx->lVal, &rety->lVal, ret);

    return S_OK;
}
// 返回符合color_format的所有坐标位置
STDMETHODIMP OpAutomation::OcrFindStrEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR strs, BSTR color, DOUBLE sim,
                                    BSTR *retstr) {
    wstring s;
    obj.FindStrEx(x1, y1, x2, y2, strs, color, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpAutomation::OcrRecognizeAuto(LONG x1, LONG y1, LONG x2, LONG y2, DOUBLE sim, BSTR *retstr) {
    wstring s;
    obj.OcrAuto(x1, y1, x2, y2, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

// 从文件中识别图片
STDMETHODIMP OpAutomation::OcrRecognizeFromFile(BSTR file_name, BSTR color_format, DOUBLE sim, BSTR *retstr) {
    wstring s;
    obj.OcrFromFile(file_name, color_format, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}
// 从文件中识别图片,无需指定颜色
STDMETHODIMP OpAutomation::OcrRecognizeAutoFromFile(BSTR file_name, DOUBLE sim, BSTR *retstr) {
    wstring s;
    obj.OcrAutoFromFile(file_name, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpAutomation::OcrFindLine(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR *retstr) {
    wstring s;
    obj.FindLine(x1, y1, x2, y2, color, sim, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpAutomation::OcrSetEngine(BSTR path_of_engine, BSTR dll_name, BSTR argv, LONG *ret) {
    return SetOutValue(ret, obj.SetOcrEngine(path_of_engine, dll_name, argv));
}

STDMETHODIMP OpAutomation::YoloSetEngine(BSTR path_of_engine, BSTR dll_name, BSTR argv, LONG *ret) {
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

STDMETHODIMP OpAutomation::MemoryWriteData(LONGLONG hwnd, BSTR address, BSTR data, LONG size, LONG *ret) {
    obj.WriteData(static_cast<LONG_PTR>(hwnd), address, data, size, ret);

    return S_OK;
}

STDMETHODIMP OpAutomation::MemoryReadData(LONGLONG hwnd, BSTR address, LONG size, BSTR *retstr) {
    wstring s;
    obj.ReadData(static_cast<LONG_PTR>(hwnd), address, size, s);

    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpAutomation::MemoryReadInt(LONGLONG hwnd, BSTR address, LONG type, LONGLONG *ret) {
    if (!ret)
        return E_POINTER;

    int64_t value = 0;
    obj.ReadInt(static_cast<LONG_PTR>(hwnd), address, type, &value);
    return SetOutValue(ret, value);
}

STDMETHODIMP OpAutomation::MemoryWriteInt(LONGLONG hwnd, BSTR address, LONG type, LONGLONG value, LONG *ret) {
    obj.WriteInt(static_cast<LONG_PTR>(hwnd), address, type, static_cast<int64_t>(value), ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::MemoryReadFloat(LONGLONG hwnd, BSTR address, DOUBLE *ret) {
    if (!ret)
        return E_POINTER;

    float value = 0.0f;
    obj.ReadFloat(static_cast<LONG_PTR>(hwnd), address, &value);
    return SetOutValue(ret, value);
}

STDMETHODIMP OpAutomation::MemoryWriteFloat(LONGLONG hwnd, BSTR address, DOUBLE value, LONG *ret) {
    obj.WriteFloat(static_cast<LONG_PTR>(hwnd), address, static_cast<float>(value), ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::MemoryReadDouble(LONGLONG hwnd, BSTR address, DOUBLE *ret) {
    if (!ret)
        return E_POINTER;

    double value = 0.0;
    obj.ReadDouble(static_cast<LONG_PTR>(hwnd), address, &value);
    return SetOutValue(ret, value);
}

STDMETHODIMP OpAutomation::MemoryWriteDouble(LONGLONG hwnd, BSTR address, DOUBLE value, LONG *ret) {
    obj.WriteDouble(static_cast<LONG_PTR>(hwnd), address, static_cast<double>(value), ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::MemoryReadString(LONGLONG hwnd, BSTR address, LONG type, LONG len, BSTR *retstr) {
    if (!retstr)
        return E_POINTER;

    wstring s;
    obj.ReadString(static_cast<LONG_PTR>(hwnd), address, type, len, s);
    CComBSTR newstr;
    newstr.Append(s.data());
    return newstr.CopyTo(retstr);
}

STDMETHODIMP OpAutomation::MemoryWriteString(LONGLONG hwnd, BSTR address, LONG type, BSTR value, LONG *ret) {
    obj.WriteString(static_cast<LONG_PTR>(hwnd), address, type, value, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::OpenCvLoadTemplate(BSTR name, BSTR file_path, LONG *ret) {
    obj.CvLoadTemplate(name, file_path, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::OpenCvLoadMaskedTemplate(BSTR name, BSTR template_path, BSTR mask_path, LONG *ret) {
    obj.CvLoadMaskedTemplate(name, template_path, mask_path, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::OpenCvRemoveTemplate(BSTR name, LONG *ret) {
    obj.CvRemoveTemplate(name, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::OpenCvRemoveAllTemplates(LONG *ret) {
    obj.CvRemoveAllTemplates(ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::OpenCvHasTemplate(BSTR name, LONG *ret) {
    obj.CvHasTemplate(name, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::OpenCvGetTemplateCount(LONG *ret) {
    obj.CvGetTemplateCount(ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::OpenCvGetAllTemplateNames(BSTR *retstr) {
    wstring s;
    obj.CvGetAllTemplateNames(s);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpAutomation::OpenCvGetOpenCvVersion(BSTR *retstr) {
    wstring s;
    obj.CvGetOpenCvVersion(s);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retstr);
    return S_OK;
}

STDMETHODIMP OpAutomation::OpenCvLoadTemplateList(BSTR template_list, LONG *ret) {
    obj.CvLoadTemplateList(template_list, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::OpenCvToGray(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvToGray(src_file, dst_file, out); });
}

STDMETHODIMP OpAutomation::OpenCvToBinary(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvToBinary(src_file, dst_file, out); });
}

STDMETHODIMP OpAutomation::OpenCvToEdge(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvToEdge(src_file, dst_file, out); });
}

STDMETHODIMP OpAutomation::OpenCvToOutline(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvToOutline(src_file, dst_file, out); });
}

STDMETHODIMP OpAutomation::OpenCvDenoise(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvDenoise(src_file, dst_file, out); });
}

STDMETHODIMP OpAutomation::OpenCvEqualize(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvEqualize(src_file, dst_file, out); });
}

STDMETHODIMP OpAutomation::OpenCvCLAHE(BSTR src_file, BSTR dst_file, DOUBLE clip_limit, LONG tile_grid_size, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvCLAHE(src_file, dst_file, clip_limit, tile_grid_size, out); });
}

STDMETHODIMP OpAutomation::OpenCvBlur(BSTR src_file, BSTR dst_file, BSTR mode, LONG kernel_size, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvBlur(src_file, dst_file, mode, kernel_size, out); });
}

STDMETHODIMP OpAutomation::OpenCvSharpen(BSTR src_file, BSTR dst_file, DOUBLE strength, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvSharpen(src_file, dst_file, strength, out); });
}

STDMETHODIMP OpAutomation::OpenCvCropValid(BSTR src_file, BSTR dst_file, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvCropValid(src_file, dst_file, out); });
}

STDMETHODIMP OpAutomation::OpenCvConnectedComponents(BSTR src_file, DOUBLE min_area, BSTR *retjson, LONG *ret) {
    if (!retjson || !ret) {
        return E_POINTER;
    }

    SetOutValue(ret, 0L);

    std::wstring s;
    obj.CvConnectedComponents(src_file, min_area, s, ret);
    return CopyOutBstr(retjson, s);
}

STDMETHODIMP OpAutomation::OpenCvFindContours(BSTR src_file, DOUBLE min_area, BSTR *retjson, LONG *ret) {
    if (!retjson || !ret) {
        return E_POINTER;
    }

    SetOutValue(ret, 0L);

    std::wstring s;
    obj.CvFindContours(src_file, min_area, s, ret);
    return CopyOutBstr(retjson, s);
}

STDMETHODIMP OpAutomation::OpenCvPreprocessPipeline(BSTR src_file, BSTR dst_file, BSTR pipeline, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvPreprocessPipeline(src_file, dst_file, pipeline, out); });
}

STDMETHODIMP OpAutomation::OpenCvCrop(BSTR src_file, LONG x, LONG y, LONG width, LONG height, BSTR dst_file, LONG *ret) {
    obj.CvCrop(src_file, x, y, width, height, dst_file, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::OpenCvResize(BSTR src_file, LONG width, LONG height, BSTR dst_file, LONG *ret) {
    obj.CvResize(src_file, width, height, dst_file, ret);
    return S_OK;
}

STDMETHODIMP OpAutomation::OpenCvThreshold(BSTR src_file, BSTR dst_file, DOUBLE threshold, DOUBLE max_value, BSTR mode,
                                      LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvThreshold(src_file, dst_file, threshold, max_value, mode, out); });
}

STDMETHODIMP OpAutomation::OpenCvInRange(BSTR src_file, BSTR dst_file, BSTR color_space, BSTR lower, BSTR upper, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvInRange(src_file, dst_file, color_space, lower, upper, out); });
}

STDMETHODIMP OpAutomation::OpenCvMorphology(BSTR src_file, BSTR dst_file, BSTR mode, LONG kernel_size, LONG iterations,
                                       LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvMorphology(src_file, dst_file, mode, kernel_size, iterations, out); });
}

STDMETHODIMP OpAutomation::OpenCvThin(BSTR src_file, BSTR dst_file, BSTR mode, LONG *ret) {
    return RunCvRetOnly(ret, [&](LONG *out) { obj.CvThin(src_file, dst_file, mode, out); });
}

STDMETHODIMP OpAutomation::OpenCvMatchTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                          DOUBLE threshold, LONG dir, LONG strip_mode, LONG method, LONG color_mode,
                                          BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvMatchTemplate(x, y, width, height, template_name, threshold, dir, strip_mode, method, color_mode, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

STDMETHODIMP OpAutomation::OpenCvMatchTemplateScale(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                               BSTR scales, DOUBLE threshold, LONG method, LONG color_mode,
                                               BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvMatchTemplateScale(x, y, width, height, template_name, scales, threshold, method, color_mode, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

STDMETHODIMP OpAutomation::OpenCvMatchAnyTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_names,
                                             DOUBLE threshold, LONG dir, LONG strip_mode, LONG method, LONG color_mode,
                                             BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvMatchAnyTemplate(x, y, width, height, template_names, threshold, dir, strip_mode, method, color_mode, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

STDMETHODIMP OpAutomation::OpenCvMatchAllTemplates(LONG x, LONG y, LONG width, LONG height, BSTR template_names,
                                              DOUBLE threshold, LONG dir, LONG strip_mode, LONG method, LONG color_mode,
                                              BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvMatchAllTemplates(x, y, width, height, template_names, threshold, dir, strip_mode, method, color_mode, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

STDMETHODIMP OpAutomation::OpenCvFeatureMatchTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                                 DOUBLE threshold, BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvFeatureMatchTemplate(x, y, width, height, template_name, threshold, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

STDMETHODIMP OpAutomation::OpenCvEdgeMatchTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                              DOUBLE threshold, BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvEdgeMatchTemplate(x, y, width, height, template_name, threshold, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

STDMETHODIMP OpAutomation::OpenCvShapeMatchTemplate(LONG x, LONG y, LONG width, LONG height, BSTR template_name,
                                               DOUBLE threshold, BSTR *retjson, LONG *ret) {
    wstring s;
    obj.CvShapeMatchTemplate(x, y, width, height, template_name, threshold, s, ret);
    CComBSTR newstr;
    newstr.Append(s.data());
    newstr.CopyTo(retjson);
    return S_OK;
}

