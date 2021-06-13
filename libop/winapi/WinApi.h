#pragma once
#ifndef __WINAPI_H_
#define __WINAPI_JH_
#include "../core/optype.h"
#undef FindWindow
#undef FindWindowEx
class WinApi
{
public:
	WinApi(void);
	~WinApi(void);


public:
	int retstringlen;
	DWORD WindowVerion;
	DWORD IsEuemprosuccess;
	DWORD npid[MAX_PATH];
	bool EnumWindow(HWND parent, const wchar_t *title, const wchar_t *class_name, LONG filter, wchar_t *retstring, const wchar_t  *process_name = NULL);
	bool EnumWindowSuper(wchar_t *spec1, LONG flag1, LONG type1, wchar_t *spec2, LONG flag2, LONG type2, LONG sort, wchar_t *retstring = NULL);
	bool EnumProcess(const wchar_t *name, wchar_t *retstring);
	bool ClientToScreen(LONG hwnd, LONG &x, LONG &y);
	long FindWindow(const wchar_t *class_name, const wchar_t*title);
	long FindWindowEx(long parent, const wchar_t *class_name, const wchar_t*title);
	bool FindWindowByProcess(const wchar_t *class_name, const wchar_t *titl, LONG &rethwnd, const wchar_t *process_name = NULL, DWORD Pid = 0);
	bool GetClientRect(LONG hwnd, LONG &x, LONG &y, LONG &x1, LONG &y1);
	bool GetClientSize(LONG hwnd, LONG &width, LONG &height);
	bool GetMousePointWindow(LONG &rethwnd, LONG x = -1, LONG y = -1);
	bool GetProcessInfo(LONG pid, wchar_t *retstring);
	bool GetWindow(LONG hwnd, LONG flag, LONG &rethwnd);
	bool GetProcesspath(DWORD ProcessID, wchar_t* process_path);
	bool GetWindowState(LONG hwnd, LONG flag);
	bool SendPaste(LONG hwnd);
	bool SetWindowSize(LONG hwnd, LONG width, LONG hight, int type = 0);
	bool SetWindowState(LONG hwnd, LONG flag, LONG rethwnd = 0);
	bool SetWindowTransparent(LONG hwnd, LONG trans);
	bool SetClipboard(wchar_t *values);
	bool GetClipboard(wchar_t *retstr);
	//2019.1
	long SendString(HWND hwnd, const wstring& str);
	long SendStringIme(HWND hwnd, const wstring& str);
	//2019.3
	long RunApp(const wstring& cmd, long mode);
	static HWND GetTopWindowSp(HWND hwnd);
private:
	DWORD  FindChildWnd(HWND hchile, const wchar_t *title, const wchar_t *classname, wchar_t *retstring, bool isGW_OWNER = false, bool isVisible = false, const wchar_t  *process_name = NULL);
	BOOL   EnumProcessbyName(DWORD   dwPID, LPCWSTR   ExeName, LONG type = 0);
	int GetProcessNumber();//获取CPU个数
	// 时间格式转换
	__int64 FileTimeToInt64(const FILETIME& time);
	double get_cpu_usage(DWORD ProcessID);	 //获取指定进程CPU使用率
	DWORD GetMemoryInfo(DWORD ProcessID);  //或者指定进程内存使用率


};


#endif