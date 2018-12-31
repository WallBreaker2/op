// OpInterface.cpp: OpInterface 的实现

#include "stdafx.h"
#include "OpInterface.h"
#include <comutil.h>
#pragma comment(lib, "comsuppw.lib")
#include "Cmder.h"
// OpInterface

HRESULT OpInterface::GetVersion(long* ret) {
	*ret = MAKE_VERSION(0, 1, 0, 1);
	return S_OK;
}

STDMETHODIMP OpInterface::EnumWindow(LONG parent, BSTR title, BSTR class_name, LONG filter, BSTR* retstr)
{
	// TODO: 在此添加实现代码
	wchar_t retstring[MAX_PATH * 200] = { 0 };
	//memset(retstring, 0, sizeof(wchar_t)*MAX_PATH * 200);
	_winapi.EnumWindow((HWND)parent, title, class_name, filter, retstring);
	//*retstr=_bstr_t(retstring);
	CComBSTR newbstr;
	newbstr.Append(retstring);
	newbstr.CopyTo(retstr);
	return S_OK;
}

STDMETHODIMP OpInterface::EnumWindowByProcess(BSTR process_name, BSTR title, BSTR class_name, LONG filter, BSTR* retstring)
{
	// TODO: 在此添加实现代码
	wchar_t retstr[MAX_PATH * 200] = { 0 };
	_winapi.EnumWindow((HWND)0, title, class_name, filter, retstr, process_name);
	//*retstring=_bstr_t(retstr);
	CComBSTR newbstr;
	newbstr.Append(retstr);
	newbstr.CopyTo(retstring);
	return S_OK;
}

STDMETHODIMP OpInterface::EnumProcess(BSTR name, BSTR* retstring)
{
	// TODO: 在此添加实现代码
	wchar_t retstr[MAX_PATH * 100] = { 0 };
	_winapi.EnumProcess(name, retstr);
	//*retstring=_bstr_t(retstr);
	CComBSTR newbstr;
	newbstr.Append(retstr);
	newbstr.CopyTo(retstring);
	return S_OK;
}

STDMETHODIMP OpInterface::ClientToScreen(LONG ClientToScreen, VARIANT* x, VARIANT* y, LONG* bret)
{
	// TODO: 在此添加实现代码
	x->vt = VT_I4;
	y->vt = VT_I4;
	*bret = _winapi.ClientToScreen(ClientToScreen, x->lVal, y->lVal);
	return S_OK;
}

STDMETHODIMP OpInterface::FindWindow(BSTR class_name, BSTR title, LONG* rethwnd)
{
	// TODO: 在此添加实现代码
	_winapi.FindWindow(class_name, title, *rethwnd);
	return S_OK;
}

STDMETHODIMP OpInterface::FindWindowByProcess(BSTR process_name, BSTR class_name, BSTR title, LONG* rethwnd)
{
	// TODO: 在此添加实现代码
	_winapi.FindWindowByProcess(class_name, title, *rethwnd, process_name);
	return S_OK;
}

STDMETHODIMP OpInterface::FindWindowByProcessId(LONG process_id, BSTR class_name, BSTR title, LONG* rethwnd)
{
	// TODO: 在此添加实现代码
	_winapi.FindWindowByProcess(class_name, title, *rethwnd, NULL, process_id);
	return S_OK;
}

STDMETHODIMP OpInterface::FindWindowEx(LONG parent, BSTR class_name, BSTR title, LONG* rethwnd)
{
	// TODO: 在此添加实现代码
	_winapi.FindWindow(class_name, title, *rethwnd, parent);
	return S_OK;
}

STDMETHODIMP OpInterface::GetClientRect(LONG hwnd, VARIANT* x1, VARIANT* y1, VARIANT* x2, VARIANT* y2, LONG* nret)
{
	// TODO: 在此添加实现代码
	x1->vt = VT_I4;
	y1->vt = VT_I4;
	x2->vt = VT_I4;
	y2->vt = VT_I4;
	*nret = _winapi.GetClientRect(hwnd, x1->lVal, y1->lVal, x2->lVal, y2->lVal);
	return S_OK;
}


STDMETHODIMP OpInterface::GetClientSize(LONG hwnd, VARIANT* width, VARIANT* height, LONG* nret)
{
	// TODO: 在此添加实现代码
	width->vt = VT_I4;
	height->vt = VT_I4;
	*nret = _winapi.GetClientSize(hwnd, width->lVal, height->lVal);
	return S_OK;
}

STDMETHODIMP OpInterface::GetForegroundFocus(LONG* rethwnd)
{
	// TODO: 在此添加实现代码
	*rethwnd = (LONG)::GetFocus();
	return S_OK;
}

STDMETHODIMP OpInterface::GetForegroundWindow(LONG* rethwnd)
{
	// TODO: 在此添加实现代码
	*rethwnd = (LONG)::GetForegroundWindow();
	return S_OK;
}

STDMETHODIMP OpInterface::GetMousePointWindow(LONG* rethwnd)
{
	// TODO: 在此添加实现代码
	//::Sleep(2000);
	_winapi.GetMousePointWindow(*rethwnd);
	return S_OK;
}

STDMETHODIMP OpInterface::GetPointWindow(LONG x, LONG y, LONG* rethwnd)
{
	// TODO: 在此添加实现代码
	_winapi.GetMousePointWindow(*rethwnd, x, y);
	return S_OK;
}

STDMETHODIMP OpInterface::GetProcessInfo(LONG pid, BSTR* retstring)
{
	// TODO: 在此添加实现代码

	wchar_t retstr[MAX_PATH] = { 0 };
	_winapi.GetProcessInfo(pid, retstr);
	//* retstring=_bstr_t(retstr);
	CComBSTR newbstr;
	newbstr.Append(retstr);
	newbstr.CopyTo(retstring);
	return S_OK;
}

STDMETHODIMP OpInterface::GetSpecialWindow(LONG flag, LONG* rethwnd)
{
	// TODO: 在此添加实现代码
	*rethwnd = 0;
	if (flag == 0)
		*rethwnd = (LONG)GetDesktopWindow();
	else if (flag == 1)
	{
		*rethwnd = (LONG)::FindWindow(L"Shell_TrayWnd", NULL);
	}

	return S_OK;
}

STDMETHODIMP OpInterface::GetWindow(LONG hwnd, LONG flag, LONG* nret)
{
	// TODO: 在此添加实现代码
	_winapi.TSGetWindow(hwnd, flag, *nret);
	return S_OK;
}

STDMETHODIMP OpInterface::GetWindowClass(LONG hwnd, BSTR* retstring)
{
	// TODO: 在此添加实现代码
	wchar_t classname[MAX_PATH] = { 0 };
	::GetClassName((HWND)hwnd, classname, MAX_PATH);
	//* retstring=_bstr_t(classname);
	CComBSTR newbstr;
	newbstr.Append(classname);
	newbstr.CopyTo(retstring);
	return S_OK;
}

STDMETHODIMP OpInterface::GetWindowProcessId(LONG hwnd, LONG* nretpid)
{
	// TODO: 在此添加实现代码
	DWORD pid = 0;
	::GetWindowThreadProcessId((HWND)hwnd, &pid);
	*nretpid = pid;
	return S_OK;
}

STDMETHODIMP OpInterface::GetWindowProcessPath(LONG hwnd, BSTR* retstring)
{
	// TODO: 在此添加实现代码
	DWORD pid = 0;
	::GetWindowThreadProcessId((HWND)hwnd, &pid);
	wchar_t process_path[MAX_PATH] = { 0 };
	_winapi.GetProcesspath(pid, process_path);
	//* retstring=_bstr_t(process_path);
	CComBSTR newbstr;
	newbstr.Append(process_path);
	newbstr.CopyTo(retstring);
	return S_OK;
}

STDMETHODIMP OpInterface::GetWindowRect(LONG hwnd, VARIANT* x1, VARIANT* y1, VARIANT* x2, VARIANT* y2, LONG* nret)
{
	// TODO: 在此添加实现代码
	x1->vt = VT_I4;
	x2->vt = VT_I4;
	y1->vt = VT_I4;
	y2->vt = VT_I4;
	RECT winrect;
	*nret = ::GetWindowRect((HWND)hwnd, &winrect);
	x1->intVal = winrect.left;
	y1->intVal = winrect.top;
	x2->intVal = winrect.right;
	y2->intVal = winrect.bottom;
	return S_OK;
}

STDMETHODIMP OpInterface::GetWindowState(LONG hwnd, LONG flag, LONG* rethwnd)
{
	// TODO: 在此添加实现代码
	*rethwnd = _winapi.GetWindowState(hwnd, flag);
	return S_OK;
}

STDMETHODIMP OpInterface::GetWindowTitle(LONG hwnd, BSTR* rettitle)
{
	// TODO: 在此添加实现代码
	wchar_t title[MAX_PATH] = { 0 };
	::GetWindowText((HWND)hwnd, title, MAX_PATH);
	//* rettitle=_bstr_t(title);
	CComBSTR newbstr;
	newbstr.Append(title);
	newbstr.CopyTo(rettitle);
	return S_OK;
}

STDMETHODIMP OpInterface::MoveWindow(LONG hwnd, LONG x, LONG y, LONG* nret)
{
	// TODO: 在此添加实现代码
	RECT winrect;
	::GetWindowRect((HWND)hwnd, &winrect);
	int width = winrect.right - winrect.left;
	int hight = winrect.bottom - winrect.top;
	*nret = ::MoveWindow((HWND)hwnd, x, y, width, hight, false);
	return S_OK;
}

STDMETHODIMP OpInterface::ScreenToClient(LONG hwnd, VARIANT* x, VARIANT* y, LONG* nret)
{
	// TODO: 在此添加实现代码
	x->vt = VT_I4;
	y->vt = VT_I4;
	POINT point;
	*nret = ::ScreenToClient((HWND)hwnd, &point);
	x->intVal = point.x;
	y->intVal = point.y;
	return S_OK;
}

STDMETHODIMP OpInterface::SendPaste(LONG hwnd, LONG* nret)
{
	// TODO: 在此添加实现代码
	*nret = _winapi.SendPaste(hwnd);
	return S_OK;
}

STDMETHODIMP OpInterface::SetClientSize(LONG hwnd, LONG width, LONG hight, LONG* nret)
{
	// TODO: 在此添加实现代码
	*nret = _winapi.SetWindowSize(hwnd, width, hight);
	return S_OK;
}

STDMETHODIMP OpInterface::SetWindowState(LONG hwnd, LONG flag, LONG* nret)
{
	// TODO: 在此添加实现代码  
	*nret = _winapi.SetWindowState(hwnd, flag);
	return S_OK;
}

STDMETHODIMP OpInterface::SetWindowSize(LONG hwnd, LONG width, LONG height, LONG* nret)
{
	// TODO: 在此添加实现代码
	*nret = _winapi.SetWindowSize(hwnd, width, height, 1);
	return S_OK;
}

STDMETHODIMP OpInterface::SetWindowText(LONG hwnd, BSTR title, LONG* nret)
{
	// TODO: 在此添加实现代码  
	//*nret=gWindowObj.TSSetWindowState(hwnd,flag);
	*nret = ::SetWindowText((HWND)hwnd, title);
	return S_OK;
}

STDMETHODIMP OpInterface::SetWindowTransparent(LONG hwnd, LONG trans, LONG* nret)
{
	// TODO: 在此添加实现代码
	*nret = _winapi.SetWindowTransparent(hwnd, trans);
	return S_OK;
}

STDMETHODIMP OpInterface::ExcuteCmd(BSTR cmd,LONG millseconds, BSTR* retstr) {
	CComBSTR bstr;
	auto strcmd = _wsto_string(cmd);
	Cmder cd;
	auto str = cd.ExcuteCmd(strcmd, millseconds <= 0 ? 5 : millseconds);
	bstr.Append(str.c_str());
	bstr.CopyTo(retstr);
	return S_OK;
}