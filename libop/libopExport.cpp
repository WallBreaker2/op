/**
This is an automatically generated class by OpExport. Please do not modify it.
License：https://github.com/WallBreaker2/op/blob/master/LICENSE 
**/

#include "libopExport.h"

libop* OP_CreateOP()
{
    return new libop();
}
void OP_ReleaseOP(libop* _op)
{
    delete _op;
}
int OP_AStarFindPath(libop* _op, long mapWidth, long mapHeight, const wchar_t* disable_points, long beginX, long beginY, long endX, long endY, wchar_t* _pStr, int _nSize)
{
    std::wstring ret;
    _op->AStarFindPath(mapWidth, mapHeight, disable_points, beginX, beginY, endX, endY, ret);
    if (_pStr == nullptr || _nSize <= (int)(ret.length() * sizeof(wchar_t)))
        return (int)(ret.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), ret.c_str());
    return 0;
}
long OP_BindWindow(libop* _op, long hwnd, const wchar_t* display, const wchar_t* mouse, const wchar_t* keypad, long mode)
{
    long ret;
    _op->BindWindow(hwnd, display, mouse, keypad, mode, &ret);
    return ret;
}
long OP_Capture(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* file_name)
{
    long ret;
    _op->Capture(x1, y1, x2, y2, file_name, &ret);
    return ret;
}
long OP_CapturePre(libop* _op, const wchar_t* file_name)
{
    long ret;
    _op->CapturePre(file_name, &ret);
    return ret;
}
long OP_ClientToScreen(libop* _op, long ClientToScreen, long* x, long* y)
{
    long bret;
    _op->ClientToScreen(ClientToScreen, x, y, &bret);
    return bret;
}
long OP_CmpColor(libop* _op, long x, long y, const wchar_t* color, double sim)
{
    long ret;
    _op->CmpColor(x, y, color, sim, &ret);
    return ret;
}
long OP_Delay(libop* _op, long mis)
{
    long ret;
    _op->Delay(mis, &ret);
    return ret;
}
long OP_Delays(libop* _op, long mis_min, long mis_max)
{
    long ret;
    _op->Delays(mis_min, mis_max, &ret);
    return ret;
}
long OP_EnablePicCache(libop* _op, long enable)
{
    long ret;
    _op->EnablePicCache(enable, &ret);
    return ret;
}
int OP_EnumProcess(libop* _op, const wchar_t* name, wchar_t* _pStr, int _nSize)
{
    std::wstring ret;
    _op->EnumProcess(name, ret);
    if (_pStr == nullptr || _nSize <= (int)(ret.length() * sizeof(wchar_t)))
        return (int)(ret.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), ret.c_str());
    return 0;
}
int OP_EnumWindow(libop* _op, long parent, const wchar_t* title, const wchar_t* class_name, long filter, wchar_t* _pStr, int _nSize)
{
    std::wstring ret;
    _op->EnumWindow(parent, title, class_name, filter, ret);
    if (_pStr == nullptr || _nSize <= (int)(ret.length() * sizeof(wchar_t)))
        return (int)(ret.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), ret.c_str());
    return 0;
}
int OP_EnumWindowByProcess(libop* _op, const wchar_t* process_name, const wchar_t* title, const wchar_t* class_name, long filter, wchar_t* _pStr, int _nSize)
{
    std::wstring ret;
    _op->EnumWindowByProcess(process_name, title, class_name, filter, ret);
    if (_pStr == nullptr || _nSize <= (int)(ret.length() * sizeof(wchar_t)))
        return (int)(ret.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), ret.c_str());
    return 0;
}
long OP_FindColor(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* color, double sim, long dir, long* x, long* y)
{
    long ret;
    _op->FindColor(x1, y1, x2, y2, color, sim, dir, x, y, &ret);
    return ret;
}
long OP_FindColorBlock(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* color, double sim, long count, long height, long width, long* x, long* y)
{
    long ret;
    _op->FindColorBlock(x1, y1, x2, y2, color, sim, count, height, width, x, y, &ret);
    return ret;
}
int OP_FindColorBlockEx(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* color, double sim, long count, long height, long width, wchar_t* _pStr, int _nSize)
{
    std::wstring retstr;
    _op->FindColorBlockEx(x1, y1, x2, y2, color, sim, count, height, width, retstr);
    if (_pStr == nullptr || _nSize <= (int)(retstr.length() * sizeof(wchar_t)))
        return (int)(retstr.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), retstr.c_str());
    return 0;
}
int OP_FindColorEx(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* color, double sim, long dir, wchar_t* _pStr, int _nSize)
{
    std::wstring retstr;
    _op->FindColorEx(x1, y1, x2, y2, color, sim, dir, retstr);
    if (_pStr == nullptr || _nSize <= (int)(retstr.length() * sizeof(wchar_t)))
        return (int)(retstr.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), retstr.c_str());
    return 0;
}
int OP_FindLine(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* color, double sim, wchar_t* _pStr, int _nSize)
{
    std::wstring retstr;
    _op->FindLine(x1, y1, x2, y2, color, sim, retstr);
    if (_pStr == nullptr || _nSize <= (int)(retstr.length() * sizeof(wchar_t)))
        return (int)(retstr.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), retstr.c_str());
    return 0;
}
long OP_FindMultiColor(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* first_color, const wchar_t* offset_color, double sim, long dir, long* x, long* y)
{
    long ret;
    _op->FindMultiColor(x1, y1, x2, y2, first_color, offset_color, sim, dir, x, y, &ret);
    return ret;
}
int OP_FindMultiColorEx(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* first_color, const wchar_t* offset_color, double sim, long dir, wchar_t* _pStr, int _nSize)
{
    std::wstring retstr;
    _op->FindMultiColorEx(x1, y1, x2, y2, first_color, offset_color, sim, dir, retstr);
    if (_pStr == nullptr || _nSize <= (int)(retstr.length() * sizeof(wchar_t)))
        return (int)(retstr.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), retstr.c_str());
    return 0;
}
int OP_FindNearestPos(libop* _op, const wchar_t* all_pos, long type, long x, long y, wchar_t* _pStr, int _nSize)
{
    std::wstring ret;
    _op->FindNearestPos(all_pos, type, x, y, ret);
    if (_pStr == nullptr || _nSize <= (int)(ret.length() * sizeof(wchar_t)))
        return (int)(ret.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), ret.c_str());
    return 0;
}
long OP_FindPic(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* files, const wchar_t* delta_color, double sim, long dir, long* x, long* y)
{
    long ret;
    _op->FindPic(x1, y1, x2, y2, files, delta_color, sim, dir, x, y, &ret);
    return ret;
}
int OP_FindPicEx(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* files, const wchar_t* delta_color, double sim, long dir, wchar_t* _pStr, int _nSize)
{
    std::wstring retstr;
    _op->FindPicEx(x1, y1, x2, y2, files, delta_color, sim, dir, retstr);
    if (_pStr == nullptr || _nSize <= (int)(retstr.length() * sizeof(wchar_t)))
        return (int)(retstr.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), retstr.c_str());
    return 0;
}
int OP_FindPicExS(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* files, const wchar_t* delta_color, double sim, long dir, wchar_t* _pStr, int _nSize)
{
    std::wstring retstr;
    _op->FindPicExS(x1, y1, x2, y2, files, delta_color, sim, dir, retstr);
    if (_pStr == nullptr || _nSize <= (int)(retstr.length() * sizeof(wchar_t)))
        return (int)(retstr.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), retstr.c_str());
    return 0;
}
long OP_FindStr(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* strs, const wchar_t* color, double sim, long* retx, long* rety)
{
    long ret;
    _op->FindStr(x1, y1, x2, y2, strs, color, sim, retx, rety, &ret);
    return ret;
}
int OP_FindStrEx(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* strs, const wchar_t* color, double sim, wchar_t* _pStr, int _nSize)
{
    std::wstring retstr;
    _op->FindStrEx(x1, y1, x2, y2, strs, color, sim, retstr);
    if (_pStr == nullptr || _nSize <= (int)(retstr.length() * sizeof(wchar_t)))
        return (int)(retstr.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), retstr.c_str());
    return 0;
}
long OP_FindWindow(libop* _op, const wchar_t* class_name, const wchar_t* title)
{
    long ret;
    _op->FindWindow(class_name, title, &ret);
    return ret;
}
long OP_FindWindowByProcess(libop* _op, const wchar_t* process_name, const wchar_t* class_name, const wchar_t* title)
{
    long ret;
    _op->FindWindowByProcess(process_name, class_name, title, &ret);
    return ret;
}
long OP_FindWindowByProcessId(libop* _op, long process_id, const wchar_t* class_name, const wchar_t* title)
{
    long ret;
    _op->FindWindowByProcessId(process_id, class_name, title, &ret);
    return ret;
}
long OP_FindWindowEx(libop* _op, long parent, const wchar_t* class_name, const wchar_t* title)
{
    long ret;
    _op->FindWindowEx(parent, class_name, title, &ret);
    return ret;
}
long OP_FreePic(libop* _op, const wchar_t* file_name)
{
    long ret;
    _op->FreePic(file_name, &ret);
    return ret;
}
int OP_GetBasePath(libop* _op, wchar_t* _pStr, int _nSize)
{
    std::wstring ret;
    _op->GetBasePath(ret);
    if (_pStr == nullptr || _nSize <= (int)(ret.length() * sizeof(wchar_t)))
        return (int)(ret.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), ret.c_str());
    return 0;
}
long OP_GetBindWindow(libop* _op)
{
    long ret;
    _op->GetBindWindow(&ret);
    return ret;
}
long OP_GetClientRect(libop* _op, long hwnd, long* x1, long* y1, long* x2, long* y2)
{
    long ret;
    _op->GetClientRect(hwnd, x1, y1, x2, y2, &ret);
    return ret;
}
long OP_GetClientSize(libop* _op, long hwnd, long* width, long* height)
{
    long ret;
    _op->GetClientSize(hwnd, width, height, &ret);
    return ret;
}
int OP_GetClipboard(libop* _op, wchar_t* _pStr, int _nSize)
{
    std::wstring ret;
    _op->GetClipboard(ret);
    if (_pStr == nullptr || _nSize <= (int)(ret.length() * sizeof(wchar_t)))
        return (int)(ret.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), ret.c_str());
    return 0;
}
int OP_GetCmdStr(libop* _op, const wchar_t* cmd, long millseconds, wchar_t* _pStr, int _nSize)
{
    std::wstring retstr;
    _op->GetCmdStr(cmd, millseconds, retstr);
    if (_pStr == nullptr || _nSize <= (int)(retstr.length() * sizeof(wchar_t)))
        return (int)(retstr.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), retstr.c_str());
    return 0;
}
int OP_GetColor(libop* _op, long x, long y, wchar_t* _pStr, int _nSize)
{
    std::wstring ret;
    _op->GetColor(x, y, ret);
    if (_pStr == nullptr || _nSize <= (int)(ret.length() * sizeof(wchar_t)))
        return (int)(ret.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), ret.c_str());
    return 0;
}
long OP_GetCursorPos(libop* _op, long* x, long* y)
{
    long ret;
    _op->GetCursorPos(x, y, &ret);
    return ret;
}
long OP_GetForegroundFocus(libop* _op)
{
    long ret;
    _op->GetForegroundFocus(&ret);
    return ret;
}
long OP_GetForegroundWindow(libop* _op)
{
    long ret;
    _op->GetForegroundWindow(&ret);
    return ret;
}
long OP_GetID(libop* _op)
{
    long ret;
    _op->GetID(&ret);
    return ret;
}
long OP_GetKeyState(libop* _op, long vk_code)
{
    long ret;
    _op->GetKeyState(vk_code, &ret);
    return ret;
}
long OP_GetLastError(libop* _op)
{
    long ret;
    _op->GetLastError(&ret);
    return ret;
}
long OP_GetMousePointWindow(libop* _op)
{
    long ret;
    _op->GetMousePointWindow(&ret);
    return ret;
}
int OP_GetPath(libop* _op, wchar_t* _pStr, int _nSize)
{
    std::wstring ret;
    _op->GetPath(ret);
    if (_pStr == nullptr || _nSize <= (int)(ret.length() * sizeof(wchar_t)))
        return (int)(ret.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), ret.c_str());
    return 0;
}
long OP_GetPointWindow(libop* _op, long x, long y)
{
    long ret;
    _op->GetPointWindow(x, y, &ret);
    return ret;
}
int OP_GetProcessInfo(libop* _op, long pid, wchar_t* _pStr, int _nSize)
{
    std::wstring ret;
    _op->GetProcessInfo(pid, ret);
    if (_pStr == nullptr || _nSize <= (int)(ret.length() * sizeof(wchar_t)))
        return (int)(ret.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), ret.c_str());
    return 0;
}
long OP_GetScreenData(libop* _op, long x1, long y1, long x2, long y2, size_t* data)
{
    long ret;
    _op->GetScreenData(x1, y1, x2, y2, data, &ret);
    return ret;
}
long OP_GetScreenDataBmp(libop* _op, long x1, long y1, long x2, long y2, size_t* data, long* size)
{
    long ret;
    _op->GetScreenDataBmp(x1, y1, x2, y2, data, size, &ret);
    return ret;
}
long OP_GetScreenFrameInfo(libop* _op, long* frame_id)
{
    long time;
    _op->GetScreenFrameInfo(frame_id, &time);
    return time;
}
long OP_GetSpecialWindow(libop* _op, long flag)
{
    long ret;
    _op->GetSpecialWindow(flag, &ret);
    return ret;
}
long OP_GetWindow(libop* _op, long hwnd, long flag)
{
    long ret;
    _op->GetWindow(hwnd, flag, &ret);
    return ret;
}
int OP_GetWindowClass(libop* _op, long hwnd, wchar_t* _pStr, int _nSize)
{
    std::wstring ret;
    _op->GetWindowClass(hwnd, ret);
    if (_pStr == nullptr || _nSize <= (int)(ret.length() * sizeof(wchar_t)))
        return (int)(ret.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), ret.c_str());
    return 0;
}
long OP_GetWindowProcessId(libop* _op, long hwnd)
{
    long ret;
    _op->GetWindowProcessId(hwnd, &ret);
    return ret;
}
int OP_GetWindowProcessPath(libop* _op, long hwnd, wchar_t* _pStr, int _nSize)
{
    std::wstring ret;
    _op->GetWindowProcessPath(hwnd, ret);
    if (_pStr == nullptr || _nSize <= (int)(ret.length() * sizeof(wchar_t)))
        return (int)(ret.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), ret.c_str());
    return 0;
}
long OP_GetWindowRect(libop* _op, long hwnd, long* x1, long* y1, long* x2, long* y2)
{
    long ret;
    _op->GetWindowRect(hwnd, x1, y1, x2, y2, &ret);
    return ret;
}
long OP_GetWindowState(libop* _op, long hwnd, long flag)
{
    long ret;
    _op->GetWindowState(hwnd, flag, &ret);
    return ret;
}
int OP_GetWindowTitle(libop* _op, long hwnd, wchar_t* _pStr, int _nSize)
{
    std::wstring rettitle;
    _op->GetWindowTitle(hwnd, rettitle);
    if (_pStr == nullptr || _nSize <= (int)(rettitle.length() * sizeof(wchar_t)))
        return (int)(rettitle.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), rettitle.c_str());
    return 0;
}
long OP_InjectDll(libop* _op, const wchar_t* process_name, const wchar_t* dll_name)
{
    long ret;
    _op->InjectDll(process_name, dll_name, &ret);
    return ret;
}
long OP_IsBind(libop* _op)
{
    long ret;
    _op->IsBind(&ret);
    return ret;
}
long OP_KeyDown(libop* _op, long vk_code)
{
    long ret;
    _op->KeyDown(vk_code, &ret);
    return ret;
}
long OP_KeyDownChar(libop* _op, const wchar_t* vk_code)
{
    long ret;
    _op->KeyDownChar(vk_code, &ret);
    return ret;
}
long OP_KeyPress(libop* _op, long vk_code)
{
    long ret;
    _op->KeyPress(vk_code, &ret);
    return ret;
}
long OP_KeyPressChar(libop* _op, const wchar_t* vk_code)
{
    long ret;
    _op->KeyPressChar(vk_code, &ret);
    return ret;
}
long OP_KeyUp(libop* _op, long vk_code)
{
    long ret;
    _op->KeyUp(vk_code, &ret);
    return ret;
}
long OP_KeyUpChar(libop* _op, const wchar_t* vk_code)
{
    long ret;
    _op->KeyUpChar(vk_code, &ret);
    return ret;
}
long OP_LeftClick(libop* _op)
{
    long ret;
    _op->LeftClick(&ret);
    return ret;
}
long OP_LeftDoubleClick(libop* _op)
{
    long ret;
    _op->LeftDoubleClick(&ret);
    return ret;
}
long OP_LeftDown(libop* _op)
{
    long ret;
    _op->LeftDown(&ret);
    return ret;
}
long OP_LeftUp(libop* _op)
{
    long ret;
    _op->LeftUp(&ret);
    return ret;
}
long OP_LoadMemPic(libop* _op, const wchar_t* file_name, void* data, long size)
{
    long ret;
    _op->LoadMemPic(file_name, data, size, &ret);
    return ret;
}
long OP_LoadPic(libop* _op, const wchar_t* file_name)
{
    long ret;
    _op->LoadPic(file_name, &ret);
    return ret;
}
int OP_MatchPicName(libop* _op, const wchar_t* pic_name, wchar_t* _pStr, int _nSize)
{
    std::wstring retstr;
    _op->MatchPicName(pic_name, retstr);
    if (_pStr == nullptr || _nSize <= (int)(retstr.length() * sizeof(wchar_t)))
        return (int)(retstr.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), retstr.c_str());
    return 0;
}
long OP_MiddleClick(libop* _op)
{
    long ret;
    _op->MiddleClick(&ret);
    return ret;
}
long OP_MiddleDown(libop* _op)
{
    long ret;
    _op->MiddleDown(&ret);
    return ret;
}
long OP_MiddleUp(libop* _op)
{
    long ret;
    _op->MiddleUp(&ret);
    return ret;
}
long OP_MoveR(libop* _op, long x, long y)
{
    long ret;
    _op->MoveR(x, y, &ret);
    return ret;
}
long OP_MoveTo(libop* _op, long x, long y)
{
    long ret;
    _op->MoveTo(x, y, &ret);
    return ret;
}
long OP_MoveToEx(libop* _op, long x, long y, long w, long h)
{
    long ret;
    _op->MoveToEx(x, y, w, h, &ret);
    return ret;
}
long OP_MoveWindow(libop* _op, long hwnd, long x, long y)
{
    long ret;
    _op->MoveWindow(hwnd, x, y, &ret);
    return ret;
}
int OP_Ocr(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* color, double sim, wchar_t* _pStr, int _nSize)
{
    std::wstring ret_str;
    _op->Ocr(x1, y1, x2, y2, color, sim, ret_str);
    if (_pStr == nullptr || _nSize <= (int)(ret_str.length() * sizeof(wchar_t)))
        return (int)(ret_str.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), ret_str.c_str());
    return 0;
}
int OP_OcrAuto(libop* _op, long x1, long y1, long x2, long y2, double sim, wchar_t* _pStr, int _nSize)
{
    std::wstring ret_str;
    _op->OcrAuto(x1, y1, x2, y2, sim, ret_str);
    if (_pStr == nullptr || _nSize <= (int)(ret_str.length() * sizeof(wchar_t)))
        return (int)(ret_str.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), ret_str.c_str());
    return 0;
}
int OP_OcrAutoFromFile(libop* _op, const wchar_t* file_name, double sim, wchar_t* _pStr, int _nSize)
{
    std::wstring retstr;
    _op->OcrAutoFromFile(file_name, sim, retstr);
    if (_pStr == nullptr || _nSize <= (int)(retstr.length() * sizeof(wchar_t)))
        return (int)(retstr.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), retstr.c_str());
    return 0;
}
int OP_OcrEx(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* color, double sim, wchar_t* _pStr, int _nSize)
{
    std::wstring ret_str;
    _op->OcrEx(x1, y1, x2, y2, color, sim, ret_str);
    if (_pStr == nullptr || _nSize <= (int)(ret_str.length() * sizeof(wchar_t)))
        return (int)(ret_str.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), ret_str.c_str());
    return 0;
}
int OP_OcrFromFile(libop* _op, const wchar_t* file_name, const wchar_t* color_format, double sim, wchar_t* _pStr, int _nSize)
{
    std::wstring retstr;
    _op->OcrFromFile(file_name, color_format, sim, retstr);
    if (_pStr == nullptr || _nSize <= (int)(retstr.length() * sizeof(wchar_t)))
        return (int)(retstr.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), retstr.c_str());
    return 0;
}
int OP_ReadData(libop* _op, long hwnd, const wchar_t* address, long size, wchar_t* _pStr, int _nSize)
{
    std::wstring retstr;
    _op->ReadData(hwnd, address, size, retstr);
    if (_pStr == nullptr || _nSize <= (int)(retstr.length() * sizeof(wchar_t)))
        return (int)(retstr.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), retstr.c_str());
    return 0;
}
long OP_RightClick(libop* _op)
{
    long ret;
    _op->RightClick(&ret);
    return ret;
}
long OP_RightDown(libop* _op)
{
    long ret;
    _op->RightDown(&ret);
    return ret;
}
long OP_RightUp(libop* _op)
{
    long ret;
    _op->RightUp(&ret);
    return ret;
}
long OP_RunApp(libop* _op, const wchar_t* cmdline, long mode)
{
    long ret;
    _op->RunApp(cmdline, mode, &ret);
    return ret;
}
long OP_ScreenToClient(libop* _op, long hwnd, long* x, long* y)
{
    long ret;
    _op->ScreenToClient(hwnd, x, y, &ret);
    return ret;
}
long OP_SendPaste(libop* _op, long hwnd)
{
    long ret;
    _op->SendPaste(hwnd, &ret);
    return ret;
}
long OP_SendString(libop* _op, long hwnd, const wchar_t* str)
{
    long ret;
    _op->SendString(hwnd, str, &ret);
    return ret;
}
long OP_SendStringIme(libop* _op, long hwnd, const wchar_t* str)
{
    long ret;
    _op->SendStringIme(hwnd, str, &ret);
    return ret;
}
long OP_SetClientSize(libop* _op, long hwnd, long width, long hight)
{
    long ret;
    _op->SetClientSize(hwnd, width, hight, &ret);
    return ret;
}
long OP_SetClipboard(libop* _op, const wchar_t* str)
{
    long ret;
    _op->SetClipboard(str, &ret);
    return ret;
}
long OP_SetDict(libop* _op, long idx, const wchar_t* file_name)
{
    long ret;
    _op->SetDict(idx, file_name, &ret);
    return ret;
}
long OP_SetDisplayInput(libop* _op, const wchar_t* mode)
{
    long ret;
    _op->SetDisplayInput(mode, &ret);
    return ret;
}
long OP_SetKeypadDelay(libop* _op, const wchar_t* type, long delay)
{
    long ret;
    _op->SetKeypadDelay(type, delay, &ret);
    return ret;
}
long OP_SetMemDict(libop* _op, long idx, const wchar_t* data, long size)
{
    long ret;
    _op->SetMemDict(idx, data, size, &ret);
    return ret;
}
long OP_SetMouseDelay(libop* _op, const wchar_t* type, long delay)
{
    long ret;
    _op->SetMouseDelay(type, delay, &ret);
    return ret;
}
long OP_SetOcrEngine(libop* _op, const wchar_t* path_of_engine, const wchar_t* dll_name, const wchar_t* argv)
{
    return _op->SetOcrEngine(path_of_engine, dll_name, argv);
}
long OP_SetPath(libop* _op, const wchar_t* path)
{
    long ret;
    _op->SetPath(path, &ret);
    return ret;
}
long OP_SetScreenDataMode(libop* _op, long mode)
{
    long ret;
    _op->SetScreenDataMode(mode, &ret);
    return ret;
}
long OP_SetShowErrorMsg(libop* _op, long show_type)
{
    long ret;
    _op->SetShowErrorMsg(show_type, &ret);
    return ret;
}
long OP_SetWindowSize(libop* _op, long hwnd, long width, long height)
{
    long ret;
    _op->SetWindowSize(hwnd, width, height, &ret);
    return ret;
}
long OP_SetWindowState(libop* _op, long hwnd, long flag)
{
    long ret;
    _op->SetWindowState(hwnd, flag, &ret);
    return ret;
}
long OP_SetWindowText(libop* _op, long hwnd, const wchar_t* title)
{
    long ret;
    _op->SetWindowText(hwnd, title, &ret);
    return ret;
}
long OP_SetWindowTransparent(libop* _op, long hwnd, long trans)
{
    long ret;
    _op->SetWindowTransparent(hwnd, trans, &ret);
    return ret;
}
long OP_Sleep(libop* _op, long millseconds)
{
    long ret;
    _op->Sleep(millseconds, &ret);
    return ret;
}
long OP_UnBindWindow(libop* _op)
{
    long ret;
    _op->UnBindWindow(&ret);
    return ret;
}
long OP_UseDict(libop* _op, long idx)
{
    long ret;
    _op->UseDict(idx, &ret);
    return ret;
}
int OP_Ver(libop* _op, wchar_t* _pStr, int _nSize)
{
    std::wstring wstr = _op->Ver();
    if (_pStr == nullptr || _nSize <= (int)(wstr.length() * sizeof(wchar_t)))
        return (int)(wstr.length() + 1) * sizeof(wchar_t);
    wcscpy_s(_pStr, _nSize / sizeof(wchar_t), wstr.c_str());
    return 0;
}
long OP_WaitKey(libop* _op, long vk_code, long time_out)
{
    long ret;
    _op->WaitKey(vk_code, time_out, &ret);
    return ret;
}
long OP_WheelDown(libop* _op)
{
    long ret;
    _op->WheelDown(&ret);
    return ret;
}
long OP_WheelUp(libop* _op)
{
    long ret;
    _op->WheelUp(&ret);
    return ret;
}
long OP_WinExec(libop* _op, const wchar_t* cmdline, long cmdshow)
{
    long ret;
    _op->WinExec(cmdline, cmdshow, &ret);
    return ret;
}
long OP_WriteData(libop* _op, long hwnd, const wchar_t* address, const wchar_t* data, long size)
{
    long ret;
    _op->WriteData(hwnd, address, data, size, &ret);
    return ret;
}