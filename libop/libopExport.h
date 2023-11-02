/**
This is an automatically generated class by OpExport. Please do not modify it.
License：https://github.com/WallBreaker2/op/blob/master/LICENSE 
**/

#pragma once
#ifndef LIBOP_EXPORT_H
#define LIBOP_EXPORT_H

#include "libop.h"

#define DLLAPT extern "C" __declspec(dllexport)

DLLAPT libop* OP_CreateOP();
DLLAPT void OP_ReleaseOP(libop* _op);
DLLAPT int OP_AStarFindPath(libop* _op, long mapWidth, long mapHeight, const wchar_t* disable_points, long beginX, long beginY, long endX, long endY, wchar_t* _pStr, int _nSize);
DLLAPT long OP_BindWindow(libop* _op, long hwnd, const wchar_t* display, const wchar_t* mouse, const wchar_t* keypad, long mode);
DLLAPT long OP_Capture(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* file_name);
DLLAPT long OP_CapturePre(libop* _op, const wchar_t* file_name);
DLLAPT long OP_ClientToScreen(libop* _op, long ClientToScreen, long* x, long* y);
DLLAPT long OP_CmpColor(libop* _op, long x, long y, const wchar_t* color, double sim);
DLLAPT long OP_Delay(libop* _op, long mis);
DLLAPT long OP_Delays(libop* _op, long mis_min, long mis_max);
DLLAPT long OP_EnablePicCache(libop* _op, long enable);
DLLAPT int OP_EnumProcess(libop* _op, const wchar_t* name, wchar_t* _pStr, int _nSize);
DLLAPT int OP_EnumWindow(libop* _op, long parent, const wchar_t* title, const wchar_t* class_name, long filter, wchar_t* _pStr, int _nSize);
DLLAPT int OP_EnumWindowByProcess(libop* _op, const wchar_t* process_name, const wchar_t* title, const wchar_t* class_name, long filter, wchar_t* _pStr, int _nSize);
DLLAPT long OP_FindColor(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* color, double sim, long dir, long* x, long* y);
DLLAPT long OP_FindColorBlock(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* color, double sim, long count, long height, long width, long* x, long* y);
DLLAPT int OP_FindColorBlockEx(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* color, double sim, long count, long height, long width, wchar_t* _pStr, int _nSize);
DLLAPT int OP_FindColorEx(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* color, double sim, long dir, wchar_t* _pStr, int _nSize);
DLLAPT int OP_FindLine(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* color, double sim, wchar_t* _pStr, int _nSize);
DLLAPT long OP_FindMultiColor(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* first_color, const wchar_t* offset_color, double sim, long dir, long* x, long* y);
DLLAPT int OP_FindMultiColorEx(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* first_color, const wchar_t* offset_color, double sim, long dir, wchar_t* _pStr, int _nSize);
DLLAPT int OP_FindNearestPos(libop* _op, const wchar_t* all_pos, long type, long x, long y, wchar_t* _pStr, int _nSize);
DLLAPT long OP_FindPic(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* files, const wchar_t* delta_color, double sim, long dir, long* x, long* y);
DLLAPT int OP_FindPicEx(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* files, const wchar_t* delta_color, double sim, long dir, wchar_t* _pStr, int _nSize);
DLLAPT int OP_FindPicExS(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* files, const wchar_t* delta_color, double sim, long dir, wchar_t* _pStr, int _nSize);
DLLAPT long OP_FindStr(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* strs, const wchar_t* color, double sim, long* retx, long* rety);
DLLAPT int OP_FindStrEx(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* strs, const wchar_t* color, double sim, wchar_t* _pStr, int _nSize);
DLLAPT long OP_FindWindow(libop* _op, const wchar_t* class_name, const wchar_t* title);
DLLAPT long OP_FindWindowByProcess(libop* _op, const wchar_t* process_name, const wchar_t* class_name, const wchar_t* title);
DLLAPT long OP_FindWindowByProcessId(libop* _op, long process_id, const wchar_t* class_name, const wchar_t* title);
DLLAPT long OP_FindWindowEx(libop* _op, long parent, const wchar_t* class_name, const wchar_t* title);
DLLAPT long OP_FreePic(libop* _op, const wchar_t* file_name);
DLLAPT int OP_GetBasePath(libop* _op, wchar_t* _pStr, int _nSize);
DLLAPT long OP_GetBindWindow(libop* _op);
DLLAPT long OP_GetClientRect(libop* _op, long hwnd, long* x1, long* y1, long* x2, long* y2);
DLLAPT long OP_GetClientSize(libop* _op, long hwnd, long* width, long* height);
DLLAPT int OP_GetClipboard(libop* _op, wchar_t* _pStr, int _nSize);
DLLAPT int OP_GetCmdStr(libop* _op, const wchar_t* cmd, long millseconds, wchar_t* _pStr, int _nSize);
DLLAPT int OP_GetColor(libop* _op, long x, long y, wchar_t* _pStr, int _nSize);
DLLAPT long OP_GetCursorPos(libop* _op, long* x, long* y);
DLLAPT long OP_GetForegroundFocus(libop* _op);
DLLAPT long OP_GetForegroundWindow(libop* _op);
DLLAPT long OP_GetID(libop* _op);
DLLAPT long OP_GetKeyState(libop* _op, long vk_code);
DLLAPT long OP_GetLastError(libop* _op);
DLLAPT long OP_GetMousePointWindow(libop* _op);
DLLAPT int OP_GetPath(libop* _op, wchar_t* _pStr, int _nSize);
DLLAPT long OP_GetPointWindow(libop* _op, long x, long y);
DLLAPT int OP_GetProcessInfo(libop* _op, long pid, wchar_t* _pStr, int _nSize);
DLLAPT long OP_GetScreenData(libop* _op, long x1, long y1, long x2, long y2, size_t* data);
DLLAPT long OP_GetScreenDataBmp(libop* _op, long x1, long y1, long x2, long y2, size_t* data, long* size);
DLLAPT long OP_GetScreenFrameInfo(libop* _op, long* frame_id);
DLLAPT long OP_GetSpecialWindow(libop* _op, long flag);
DLLAPT long OP_GetWindow(libop* _op, long hwnd, long flag);
DLLAPT int OP_GetWindowClass(libop* _op, long hwnd, wchar_t* _pStr, int _nSize);
DLLAPT long OP_GetWindowProcessId(libop* _op, long hwnd);
DLLAPT int OP_GetWindowProcessPath(libop* _op, long hwnd, wchar_t* _pStr, int _nSize);
DLLAPT long OP_GetWindowRect(libop* _op, long hwnd, long* x1, long* y1, long* x2, long* y2);
DLLAPT long OP_GetWindowState(libop* _op, long hwnd, long flag);
DLLAPT int OP_GetWindowTitle(libop* _op, long hwnd, wchar_t* _pStr, int _nSize);
DLLAPT long OP_InjectDll(libop* _op, const wchar_t* process_name, const wchar_t* dll_name);
DLLAPT long OP_IsBind(libop* _op);
DLLAPT long OP_KeyDown(libop* _op, long vk_code);
DLLAPT long OP_KeyDownChar(libop* _op, const wchar_t* vk_code);
DLLAPT long OP_KeyPress(libop* _op, long vk_code);
DLLAPT long OP_KeyPressChar(libop* _op, const wchar_t* vk_code);
DLLAPT long OP_KeyUp(libop* _op, long vk_code);
DLLAPT long OP_KeyUpChar(libop* _op, const wchar_t* vk_code);
DLLAPT long OP_LeftClick(libop* _op);
DLLAPT long OP_LeftDoubleClick(libop* _op);
DLLAPT long OP_LeftDown(libop* _op);
DLLAPT long OP_LeftUp(libop* _op);
DLLAPT long OP_LoadMemPic(libop* _op, const wchar_t* file_name, void* data, long size);
DLLAPT long OP_LoadPic(libop* _op, const wchar_t* file_name);
DLLAPT int OP_MatchPicName(libop* _op, const wchar_t* pic_name, wchar_t* _pStr, int _nSize);
DLLAPT long OP_MiddleClick(libop* _op);
DLLAPT long OP_MiddleDown(libop* _op);
DLLAPT long OP_MiddleUp(libop* _op);
DLLAPT long OP_MoveR(libop* _op, long x, long y);
DLLAPT long OP_MoveTo(libop* _op, long x, long y);
DLLAPT long OP_MoveToEx(libop* _op, long x, long y, long w, long h);
DLLAPT long OP_MoveWindow(libop* _op, long hwnd, long x, long y);
DLLAPT int OP_Ocr(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* color, double sim, wchar_t* _pStr, int _nSize);
DLLAPT int OP_OcrAuto(libop* _op, long x1, long y1, long x2, long y2, double sim, wchar_t* _pStr, int _nSize);
DLLAPT int OP_OcrAutoFromFile(libop* _op, const wchar_t* file_name, double sim, wchar_t* _pStr, int _nSize);
DLLAPT int OP_OcrEx(libop* _op, long x1, long y1, long x2, long y2, const wchar_t* color, double sim, wchar_t* _pStr, int _nSize);
DLLAPT int OP_OcrFromFile(libop* _op, const wchar_t* file_name, const wchar_t* color_format, double sim, wchar_t* _pStr, int _nSize);
DLLAPT int OP_ReadData(libop* _op, long hwnd, const wchar_t* address, long size, wchar_t* _pStr, int _nSize);
DLLAPT long OP_RightClick(libop* _op);
DLLAPT long OP_RightDown(libop* _op);
DLLAPT long OP_RightUp(libop* _op);
DLLAPT long OP_RunApp(libop* _op, const wchar_t* cmdline, long mode);
DLLAPT long OP_ScreenToClient(libop* _op, long hwnd, long* x, long* y);
DLLAPT long OP_SendPaste(libop* _op, long hwnd);
DLLAPT long OP_SendString(libop* _op, long hwnd, const wchar_t* str);
DLLAPT long OP_SendStringIme(libop* _op, long hwnd, const wchar_t* str);
DLLAPT long OP_SetClientSize(libop* _op, long hwnd, long width, long hight);
DLLAPT long OP_SetClipboard(libop* _op, const wchar_t* str);
DLLAPT long OP_SetDict(libop* _op, long idx, const wchar_t* file_name);
DLLAPT long OP_SetDisplayInput(libop* _op, const wchar_t* mode);
DLLAPT long OP_SetKeypadDelay(libop* _op, const wchar_t* type, long delay);
DLLAPT long OP_SetMemDict(libop* _op, long idx, const wchar_t* data, long size);
DLLAPT long OP_SetMouseDelay(libop* _op, const wchar_t* type, long delay);
DLLAPT long OP_SetOcrEngine(libop* _op, const wchar_t* path_of_engine, const wchar_t* dll_name, const wchar_t* argv);
DLLAPT long OP_SetPath(libop* _op, const wchar_t* path);
DLLAPT long OP_SetScreenDataMode(libop* _op, long mode);
DLLAPT long OP_SetShowErrorMsg(libop* _op, long show_type);
DLLAPT long OP_SetWindowSize(libop* _op, long hwnd, long width, long height);
DLLAPT long OP_SetWindowState(libop* _op, long hwnd, long flag);
DLLAPT long OP_SetWindowText(libop* _op, long hwnd, const wchar_t* title);
DLLAPT long OP_SetWindowTransparent(libop* _op, long hwnd, long trans);
DLLAPT long OP_Sleep(libop* _op, long millseconds);
DLLAPT long OP_UnBindWindow(libop* _op);
DLLAPT long OP_UseDict(libop* _op, long idx);
DLLAPT int OP_Ver(libop* _op, wchar_t* _pStr, int _nSize);
DLLAPT long OP_WaitKey(libop* _op, long vk_code, long time_out);
DLLAPT long OP_WheelDown(libop* _op);
DLLAPT long OP_WheelUp(libop* _op);
DLLAPT long OP_WinExec(libop* _op, const wchar_t* cmdline, long cmdshow);
DLLAPT long OP_WriteData(libop* _op, long hwnd, const wchar_t* address, const wchar_t* data, long size);
#endif // !LIBOP_EXPORT_H