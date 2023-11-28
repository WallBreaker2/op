// libop的声明
/*
所有op的开放接口都从此cpp类衍生而出
*/
#pragma once

#include <string>
#include<vector>
#include "libop.h"
#include "c_libop.h"

opHwnd OP_API_CALL CreateOp() {
    return new libop();
}

void OP_API_CALL  DeleteOp(opHwnd pOp) {
    if (pOp != NULL) {
        delete (libop *) pOp;
    }
}

////////////////////////////////////
//  C  API
///////////////////////////////////


void OP_API_CALL  Ver(opHwnd pOp, wchar_t *ret) {
    std::wstring ver = ((libop *) pOp)->Ver();
    memcpy(ret, ver.c_str(), wcslen(ver.c_str()));
}

long OP_API_CALL  SetPath(opHwnd pOp, const wchar_t *path) {
    long ret;
    ((libop *) pOp)->SetPath(path, &ret);
    return ret;
}

void OP_API_CALL  GetPath(opHwnd pOp, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->GetPath(r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

void OP_API_CALL  GetBasePath(opHwnd pOp, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->GetBasePath(r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

long OP_API_CALL GetID(opHwnd pOp) {
    long ret;
    ((libop *) pOp)->GetID(&ret);
    return ret;
}

long OP_API_CALL  OPGetLastError(opHwnd pOp) {
    long ret;
    ((libop *) pOp)->GetLastError(&ret);
    return ret;
}

long OP_API_CALL SetShowErrorMsg(opHwnd pOp, long show_type) {
    long ret;
    ((libop *) pOp)->SetShowErrorMsg(show_type, &ret);
    return ret;
}

long OP_API_CALL OPSleep(opHwnd pOp, long millseconds) {
    long ret;
    ((libop *) pOp)->Sleep(millseconds, &ret);
    return ret;
}

long OP_API_CALL InjectDll(opHwnd pOp, const wchar_t *process_name, const wchar_t *dll_name) {
    long ret;
    ((libop *) pOp)->InjectDll(process_name, dll_name, &ret);
    return ret;
}

long OP_API_CALL  EnablePicCache(opHwnd pOp, long enable) {
    long ret;
    ((libop *) pOp)->EnablePicCache(enable, &ret);
    return ret;
}

long OP_API_CALL  CapturePre(opHwnd pOp, const wchar_t *file_name) {
    long ret;
    ((libop *) pOp)->CapturePre(file_name, &ret);
    return ret;
}

long OP_API_CALL SetScreenDataMode(opHwnd pOp, long mode) {
    long ret;
    ((libop *) pOp)->SetScreenDataMode(mode, &ret);
    return ret;
}

void OP_API_CALL  AStarFindPath(opHwnd pOp, long mapWidth, long mapHeight, const wchar_t *disable_points, long beginX, long beginY, long endX, long endY, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->AStarFindPath(mapWidth, mapHeight, disable_points, beginX, beginY, endX, endY, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

void OP_API_CALL  FindNearestPos(opHwnd pOp, const wchar_t *all_pos, long type, long x, long y, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->FindNearestPos(all_pos, type, x, y, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

void OP_API_CALL  EnumWindow(opHwnd pOp, long parent, const wchar_t *title, const wchar_t *class_name, long filter, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->EnumWindow(parent, title, class_name, filter, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

void OP_API_CALL  EnumWindowByProcess(opHwnd pOp, const wchar_t *process_name, const wchar_t *title, const wchar_t *class_name, long filter, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->EnumWindowByProcess(process_name, title, class_name, filter, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

void OP_API_CALL  EnumProcess(opHwnd pOp, const wchar_t *name, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->EnumProcess(name, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

long OP_API_CALL  OpClientToScreen(opHwnd pOp, long ClientToScreen, long *x, long *y) {
    long ret;
    ((libop *) pOp)->ClientToScreen(ClientToScreen, x, y, &ret);
    return ret;
}

long OP_API_CALL FindWindow(opHwnd pOp, const wchar_t *class_name, const wchar_t *title) {
    long ret;
    ((libop *) pOp)->FindWindow(class_name, title, &ret);
    return ret;
}

long OP_API_CALL FindWindowByProcess(opHwnd pOp, const wchar_t *process_name, const wchar_t *class_name, const wchar_t *title) {
    long ret;
    ((libop *) pOp)->FindWindowByProcess(process_name, class_name, title, &ret);
    return ret;
}

long OP_API_CALL FindWindowByProcessId(opHwnd pOp, long process_id, const wchar_t *class_name, const wchar_t *title) {
    long ret;
    ((libop *) pOp)->FindWindowByProcessId(process_id, class_name, title, &ret);
    return ret;
}

long OP_API_CALL FindWindowEx(opHwnd pOp, long parent, const wchar_t *class_name, const wchar_t *title) {
    long ret;
    ((libop *) pOp)->FindWindowEx(parent, class_name, title, &ret);
    return ret;
}

long OP_API_CALL OPGetClientRect(opHwnd pOp, long hwnd, long *x1, long *y1, long *x2, long *y2) {
    long ret;
    ((libop *) pOp)->GetClientRect(hwnd, x1, y1, x2, y2, &ret);
    return ret;
}

long OP_API_CALL GetClientSize(opHwnd pOp, long hwnd, long *width, long *height) {
    long ret;
    ((libop *) pOp)->GetClientSize(hwnd, width, height, &ret);
    return ret;
}

long OP_API_CALL GetForegroundFocus(opHwnd pOp) {
    long ret;
    ((libop *) pOp)->GetForegroundFocus(&ret);
    return ret;
}

long OP_API_CALL OPGetForegroundWindow(opHwnd pOp) {
    long ret;
    ((libop *) pOp)->GetForegroundWindow(&ret);
    return ret;
}

long OP_API_CALL  GetMousePointWindow(opHwnd pOp) {
    long ret;
    ((libop *) pOp)->GetMousePointWindow(&ret);
    return ret;
}

long OP_API_CALL GetPointWindow(opHwnd pOp, long x, long y) {
    long ret;
    ((libop *) pOp)->GetPointWindow(x, y, &ret);
    return ret;
}

void OP_API_CALL  GetProcessInfo(opHwnd pOp, long pid, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->GetProcessInfo(pid, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

long OP_API_CALL  GetSpecialWindow(opHwnd pOp, long flag) {
    long ret;
    ((libop *) pOp)->GetSpecialWindow(flag, &ret);
    return ret;
}

long OP_API_CALL  OPGetWindow(opHwnd pOp, long hwnd, long flag) {
    long ret;
    ((libop *) pOp)->GetWindow(hwnd, flag, &ret);
    return ret;
}

void OP_API_CALL  GetWindowClass(opHwnd pOp, long hwnd, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->GetWindowClass(hwnd, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

long OP_API_CALL GetWindowProcessId(opHwnd pOp, long hwnd) {
    long ret;
    ((libop *) pOp)->GetWindowProcessId(hwnd, &ret);
    return ret;
}

void OP_API_CALL  GetWindowProcessPath(opHwnd pOp, long hwnd, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->GetWindowProcessPath(hwnd, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

long OP_API_CALL  OPGetWindowRect(opHwnd pOp, long hwnd, long *x1, long *y1, long *x2, long *y2) {
    long ret;
    ((libop *) pOp)->GetWindowRect(hwnd, x1, y1, x2, y2, &ret);
    return ret;
}

long OP_API_CALL GetWindowState(opHwnd pOp, long hwnd, long flag) {
    long ret;
    ((libop *) pOp)->GetWindowState(hwnd, flag, &ret);
    return ret;
}

void OP_API_CALL  GetWindowTitle(opHwnd pOp, long hwnd, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->GetWindowTitle(hwnd, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

long OP_API_CALL OPMoveWindow(opHwnd pOp, long hwnd, long x, long y) {
    long ret;
    ((libop *) pOp)->MoveWindow(hwnd, x, y, &ret);
    return ret;
}

long OP_API_CALL OPScreenToClient(opHwnd pOp, long hwnd, long *x, long *y) {
    long ret;
    ((libop *) pOp)->ScreenToClient(hwnd, x, y, &ret);
    return ret;
}

long OP_API_CALL SendPaste(opHwnd pOp, long hwnd) {
    long ret;
    ((libop *) pOp)->SendPaste(hwnd, &ret);
    return ret;
}

long OP_API_CALL SetClientSize(opHwnd pOp, long hwnd, long width, long hight) {
    long ret;
    ((libop *) pOp)->SetClientSize(hwnd, width, hight, &ret);
    return ret;
}

long OP_API_CALL SetWindowState(opHwnd pOp, long hwnd, long flag) {
    long ret;
    ((libop *) pOp)->SetWindowState(hwnd, flag, &ret);
    return ret;
}

long OP_API_CALL SetWindowSize(opHwnd pOp, long hwnd, long width, long height) {
    long ret;
    ((libop *) pOp)->SetWindowSize(hwnd, width, height, &ret);
    return ret;
}

long OP_API_CALL SetWindowText(opHwnd pOp, long hwnd, const wchar_t *title) {
    long ret;
    ((libop *) pOp)->SetWindowText(hwnd, title, &ret);
    return ret;
}

long OP_API_CALL SetWindowTransparent(opHwnd pOp, long hwnd, long trans) {
    long ret;
    ((libop *) pOp)->SetWindowTransparent(hwnd, trans, &ret);
    return ret;
}

long OP_API_CALL SendString(opHwnd pOp, long hwnd, const wchar_t *str) {
    long ret;
    ((libop *) pOp)->SendString(hwnd, str, &ret);
    return ret;
}

long OP_API_CALL SendStringIme(opHwnd pOp, long hwnd, const wchar_t *str) {
    long ret;
    ((libop *) pOp)->SendStringIme(hwnd, str, &ret);
    return ret;
}

long OP_API_CALL  RunApp(opHwnd pOp, const wchar_t *cmdline, long mode) {
    long ret;
    ((libop *) pOp)->RunApp(cmdline, mode, &ret);
    return ret;
}

long OP_API_CALL OPWinExec(opHwnd pOp, const wchar_t *cmdline, long cmdshow) {
    long ret;
    ((libop *) pOp)->WinExec(cmdline, cmdshow, &ret);
    return ret;
}

void OP_API_CALL  GetCmdStr(opHwnd pOp, const wchar_t *cmd, long millseconds, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->GetCmdStr(cmd, millseconds, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

long OP_API_CALL BindWindow(opHwnd pOp, long hwnd, const wchar_t *display, const wchar_t *mouse, const wchar_t *keypad, long mode) {
    long ret;
    ((libop *) pOp)->BindWindow(hwnd, display, mouse, keypad, mode, &ret);
    return ret;
}

long OP_API_CALL  UnBindWindow(opHwnd pOp) {
    long ret;
    ((libop *) pOp)->UnBindWindow(&ret);
    return ret;
}

long OP_API_CALL OPGetCursorPos(opHwnd pOp, long *x, long *y) {
    long ret;
    ((libop *) pOp)->GetCursorPos(x, y, &ret);
    return ret;
}

long OP_API_CALL MoveR(opHwnd pOp, long x, long y) {
    long ret;
    ((libop *) pOp)->MoveR(x, y, &ret);
    return ret;
}

long OP_API_CALL  MoveTo(opHwnd pOp, long x, long y) {
    long ret;
    ((libop *) pOp)->MoveTo(x, y, &ret);
    return ret;
}

long OP_API_CALL MoveToEx(opHwnd pOp, long x, long y, long w, long h) {
    long ret;
    ((libop *) pOp)->MoveToEx(x, y, w, h, &ret);
    return ret;
}

long OP_API_CALL LeftClick(opHwnd pOp) {
    long ret;
    ((libop *) pOp)->LeftClick(&ret);
    return ret;
}

long OP_API_CALL LeftDoubleClick(opHwnd pOp) {
    long ret;
    ((libop *) pOp)->LeftDoubleClick(&ret);
    return ret;
}

long OP_API_CALL LeftDown(opHwnd pOp) {
    long ret;
    ((libop *) pOp)->LeftDown(&ret);
    return ret;
}

long OP_API_CALL  LeftUp(opHwnd pOp) {
    long ret;
    ((libop *) pOp)->LeftUp(&ret);
    return ret;
}

long OP_API_CALL MiddleClick(opHwnd pOp) {
    long ret;
    ((libop *) pOp)->MiddleClick(&ret);
    return ret;
}

long OP_API_CALL MiddleDown(opHwnd pOp) {
    long ret;
    ((libop *) pOp)->MiddleDown(&ret);
    return ret;
}

long OP_API_CALL  MiddleUp(opHwnd pOp) {
    long ret;
    ((libop *) pOp)->MiddleUp(&ret);
    return ret;
}

long OP_API_CALL RightClick(opHwnd pOp) {
    long ret;
    ((libop *) pOp)->RightClick(&ret);
    return ret;
}

long OP_API_CALL  RightDown(opHwnd pOp) {
    long ret;
    ((libop *) pOp)->RightDown(&ret);
    return ret;
}

long OP_API_CALL  RightUp(opHwnd pOp) {
    long ret;
    ((libop *) pOp)->RightUp(&ret);
    return ret;
}

long OP_API_CALL  WheelDown(opHwnd pOp) {
    long ret;
    ((libop *) pOp)->WheelDown(&ret);
    return ret;
}

long OP_API_CALL WheelUp(opHwnd pOp) {
    long ret;
    ((libop *) pOp)->WheelUp(&ret);
    return ret;
}

long OP_API_CALL  GetKeyState(opHwnd pOp, long vk_code) {
    long ret;
    ((libop *) pOp)->GetKeyState(vk_code, &ret);
    return ret;
}

long OP_API_CALL  KeyDown(opHwnd pOp, long vk_code) {
    long ret;
    ((libop *) pOp)->KeyDown(vk_code, &ret);
    return ret;
}

long OP_API_CALL  KeyDownChar(opHwnd pOp, const wchar_t *vk_code) {
    long ret;
    ((libop *) pOp)->KeyDownChar(vk_code, &ret);
    return ret;
}

long OP_API_CALL KeyUp(opHwnd pOp, long vk_code) {
    long ret;
    ((libop *) pOp)->KeyUp(vk_code, &ret);
    return ret;
}

long OP_API_CALL KeyUpChar(opHwnd pOp, const wchar_t *vk_code) {
    long ret;
    ((libop *) pOp)->KeyUpChar(vk_code, &ret);
    return ret;
}

long OP_API_CALL WaitKey(opHwnd pOp, long vk_code, long time_out) {
    long ret;
    ((libop *) pOp)->WaitKey(vk_code, time_out, &ret);
    return ret;
}

long OP_API_CALL  KeyPress(opHwnd pOp, long vk_code) {
    long ret;
    ((libop *) pOp)->KeyPress(vk_code, &ret);
    return ret;
}

long OP_API_CALL  KeyPressChar(opHwnd pOp, const wchar_t *vk_code) {
    long ret;
    ((libop *) pOp)->KeyPressChar(vk_code, &ret);
    return ret;
}

long OP_API_CALL Capture(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *file_name) {
    long ret;
    ((libop *) pOp)->Capture(x1, y1, x2, y2, file_name, &ret);
    return ret;
}

long OP_API_CALL  CmpColor(opHwnd pOp, long x, long y, const wchar_t *color, double sim) {
    long ret;
    ((libop *) pOp)->CmpColor(x, y, color, sim, &ret);
    return ret;
}

long OP_API_CALL FindColor(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *color, double sim, long dir, long *x, long *y) {
    long ret;
    ((libop *) pOp)->FindColor(x1, y1, x2, y2, color, sim, dir, x, y, &ret);
    return ret;
}

void OP_API_CALL  FindColorEx(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *color, double sim, long dir, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->FindColorEx(x1, y1, x2, y2, color, sim, dir, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

long OP_API_CALL FindMultiColor(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *first_color, const wchar_t *offset_color, double sim, long dir, long *x, long *y) {
    long ret;
    ((libop *) pOp)->FindMultiColor(x1, y1, x2, y2, first_color, offset_color, sim, dir, x, y, &ret);
    return ret;
}

void OP_API_CALL  FindMultiColorEx(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *first_color, const wchar_t *offset_color, double sim, long dir, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->FindMultiColorEx(x1, y1, x2, y2, first_color, offset_color, sim, dir, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

long OP_API_CALL FindPic(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *files, const wchar_t *delta_color, double sim, long dir, long *x, long *y) {
    long ret;
    ((libop *) pOp)->FindPic(x1, y1, x2, y2, files, delta_color, sim, dir, x, y, &ret);
    return ret;
}

void OP_API_CALL  FindPicEx(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *files, const wchar_t *delta_color, double sim, long dir, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->FindPicEx(x1, y1, x2, y2, files, delta_color, sim, dir, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

void OP_API_CALL  FindPicExS(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *files, const wchar_t *delta_color, double sim, long dir, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->FindPicExS(x1, y1, x2, y2, files, delta_color, sim, dir, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

long OP_API_CALL FindColorBlock(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *color, double sim, long count, long height, long width, long *x, long *y) {
    long ret;
    ((libop *) pOp)->FindColorBlock(x1, y1, x2, y2, color, sim, count, height, width, x, y, &ret);
    return ret;
}

void OP_API_CALL  FindColorBlockEx(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *color, double sim, long count, long height, long width, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->FindColorBlockEx(x1, y1, x2, y2, color, sim, count, height, width, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

void OP_API_CALL  GetColor(opHwnd pOp, long x, long y, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->GetColor(x, y, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

long OP_API_CALL  SetDisplayInput(opHwnd pOp, const wchar_t *mode) {
    long ret;
    ((libop *) pOp)->SetDisplayInput(mode, &ret);
    return ret;
}

long OP_API_CALL  LoadPic(opHwnd pOp, const wchar_t *file_name) {
    long ret;
    ((libop *) pOp)->LoadPic(file_name, &ret);
    return ret;
}

long OP_API_CALL  FreePic(opHwnd pOp, const wchar_t *file_name) {
    long ret;
    ((libop *) pOp)->FreePic(file_name, &ret);
    return ret;
}

long OP_API_CALL  LoadMemPic(opHwnd pOp, const wchar_t *file_name, void *data, long size) {
    long ret;
    ((libop *) pOp)->LoadMemPic(file_name, data, size, &ret);
    return ret;
}

long OP_API_CALL GetScreenData(opHwnd pOp, long x1, long y1, long x2, long y2, size_t *data) {
    long ret;
    ((libop *) pOp)->GetScreenData(x1, y1, x2, y2, data, &ret);
    return ret;
}

long OP_API_CALL GetScreenDataBmp(opHwnd pOp, long x1, long y1, long x2, long y2, size_t *data, long *size) {
    long ret;
    ((libop *) pOp)->GetScreenDataBmp(x1, y1, x2, y2, data, size, &ret);
    return ret;
}

void OP_API_CALL  GetScreenFrameInfo(opHwnd pOp, long *frame_id, long *time) {
    ((libop *) pOp)->GetScreenFrameInfo(frame_id, time);
}

void OP_API_CALL  MatchPicName(opHwnd pOp, const wchar_t *pic_name, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->MatchPicName(pic_name, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

long OP_API_CALL SetDict(opHwnd pOp, long idx, const wchar_t *file_name) {
    long ret;
    ((libop *) pOp)->SetDict(idx, file_name, &ret);
    return ret;
}

long OP_API_CALL  SetMemDict(opHwnd pOp, long idx, const wchar_t *data, long size) {
    long ret;
    ((libop *) pOp)->SetMemDict(idx, data, size, &ret);
    return ret;
}

long OP_API_CALL  UseDict(opHwnd pOp, long idx) {
    long ret;
    ((libop *) pOp)->UseDict(idx, &ret);
    return ret;
}

void OP_API_CALL  Ocr(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *color, double sim, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->Ocr(x1, y1, x2, y2, color, sim, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

void OP_API_CALL  OcrEx(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *color, double sim, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->OcrEx(x1, y1, x2, y2, color, sim, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

long OP_API_CALL  FindStr(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *strs, const wchar_t *color, double sim, long *retx, long *rety) {
    long ret;
    ((libop *) pOp)->FindStr(x1, y1, x2, y2, strs, color, sim, retx, rety, &ret);
    return ret;
}

void OP_API_CALL  FindStrEx(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *strs, const wchar_t *color, double sim, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->FindStrEx(x1, y1, x2, y2, strs, color, sim, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

void OP_API_CALL  OcrAuto(opHwnd pOp, long x1, long y1, long x2, long y2, double sim, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->OcrAuto(x1, y1, x2, y2, sim, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

void OP_API_CALL  OcrFromFile(opHwnd pOp, const wchar_t *file_name, const wchar_t *color_format, double sim, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->OcrFromFile(file_name, color_format, sim, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

void OP_API_CALL  OcrAutoFromFile(opHwnd pOp, const wchar_t *file_name, double sim, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->OcrAutoFromFile(file_name, sim, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

void OP_API_CALL  FindLine(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *color, double sim, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->FindLine(x1, y1, x2, y2, color, sim, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

long OP_API_CALL WriteData(opHwnd pOp, long hwnd, const wchar_t *address, const wchar_t *data, long size) {
    long ret;
    ((libop *) pOp)->WriteData(hwnd, address, data, size, &ret);
    return ret;
}

void OP_API_CALL  ReadData(opHwnd pOp, long hwnd, const wchar_t *address, long size, wchar_t *ret) {
    std::wstring r;
    ((libop *) pOp)->ReadData(hwnd, address, size, r);
    memcpy(ret, r.c_str(), wcslen(r.c_str()) * 2);
}

