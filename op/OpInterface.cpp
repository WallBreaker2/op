// OpInterface.cpp: OpInterface 的实现

#include "stdafx.h"
#include "OpInterface.h"

#include "Cmder.h"
#include "Injecter.h"
#include "Tool.h"
// OpInterface

HRESULT OpInterface::Ver(BSTR* ret) {
#ifndef _WIN64
	const char* ver = "0.2110.x86";
#else
	static const wchar_t* ver = L"0.2110.x64";

#endif;
	Tool::setlog("address=%d,str=%s", ver, ver);
	CComBSTR newstr;
	newstr.Append(ver);
	newstr.CopyTo(ret);

	return S_OK;
}

STDMETHODIMP OpInterface::SetPath(BSTR path, LONG* ret) {
	if (!::PathFileExists(path)) {
		wchar_t buff[256];
		::GetModuleFileName(NULL, buff, 256);
		std::wstring str = buff;
		str = str.substr(0, str.rfind(L'\\'));
		_curr_path = str + path;
	}
	else
		_curr_path = path;
	if (_curr_path.back() == L'\\')
		_curr_path.pop_back();
	*ret = ::PathFileExists(_curr_path.c_str());
	//setlog(L"%s", _curr_path.c_str());
	if (!*ret)
		_curr_path.clear();
	return S_OK;
}

STDMETHODIMP OpInterface::GetPath(BSTR* path) {
	CComBSTR bstr;
	bstr.Append(_curr_path.c_str());
	bstr.CopyTo(path);
	return S_OK;
}

STDMETHODIMP OpInterface::GetBasePath(BSTR* path){
	CComBSTR bstr;
	wchar_t basepath[256];
	::GetModuleFileName(gInstance, basepath, 256);
	bstr.Append(basepath);
	bstr.CopyTo(path);
	return S_OK;
}

STDMETHODIMP OpInterface::WinExec(BSTR cmdline,LONG cmdshow, LONG* ret){
	auto str = _wsto_string(cmdline);
	*ret = ::WinExec(str.c_str(), cmdshow) > 31 ? 1 : 0;
	return S_OK;
}

STDMETHODIMP OpInterface::Sleep(LONG millseconds, LONG* ret) {
	::Sleep(millseconds);
	*ret = 1;
	return S_OK;
}

STDMETHODIMP OpInterface::InjectDll(BSTR process_name, BSTR dll_name, LONG* ret) {
	//auto proc = _wsto_string(process_name);
	//auto dll = _wsto_string(dll_name);
	//Injecter::EnablePrivilege(TRUE);
	//auto h = Injecter::InjectDll(process_name, dll_name);
	*ret = 0;
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
	*rethwnd = _winapi.FindWindow(class_name, title);
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
	*rethwnd = _winapi.FindWindowEx(parent,class_name, title);
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

STDMETHODIMP OpInterface::SendString(LONG hwnd, BSTR str, LONG* ret) {
	*ret = _winapi.SendString((HWND)hwnd, str);
	return S_OK;
}

STDMETHODIMP OpInterface::SendStringIme(LONG hwnd, BSTR str, LONG* ret) {
	*ret = _winapi.SendStringIme((HWND)hwnd, str);
	return S_OK;
}

STDMETHODIMP OpInterface::ExcuteCmd(BSTR cmd, LONG millseconds, BSTR* retstr) {
	CComBSTR bstr;
	auto strcmd = _wsto_string(cmd);
	Cmder cd;
	auto str = cd.ExcuteCmd(strcmd, millseconds <= 0 ? 5 : millseconds);
	bstr.Append(str.c_str());
	bstr.CopyTo(retstr);
	return S_OK;
}



STDMETHODIMP OpInterface::BindWindow(LONG hwnd, BSTR display, BSTR mouse, BSTR keypad, LONG mode, LONG *ret) {
	*ret = _bkproc.BindWindow(hwnd, display, mouse, keypad, mode);
	return S_OK;
}

STDMETHODIMP OpInterface::UnBindWindow(LONG* ret) {
	*ret = _bkproc.UnBindWindow();
	return S_OK;
}


STDMETHODIMP OpInterface::GetCursorPos(VARIANT* x, VARIANT* y, LONG* ret) {
	return S_OK;
}

STDMETHODIMP OpInterface::MoveR(LONG x, LONG y, LONG* ret) {
	*ret = _bkproc._bkmouse.MoveR(x, y);
	return S_OK;
}
//把鼠标移动到目的点(x,y)
STDMETHODIMP OpInterface::MoveTo(LONG x, LONG y, LONG* ret) {
	*ret = _bkproc.MoveTo(x, y);
	return S_OK;
}

STDMETHODIMP OpInterface::MoveToEx(LONG x, LONG y, LONG w, LONG h, LONG* ret) {
	*ret = _bkproc._bkmouse.MoveToEx(x, y, w, h);
	return S_OK;
}

STDMETHODIMP OpInterface::LeftClick(LONG* ret) {
	*ret = _bkproc.LeftClick();
	return S_OK;
}

STDMETHODIMP OpInterface::LeftDoubleClick(LONG* ret) {
	*ret = _bkproc._bkmouse.LeftDoubleClick();
	return S_OK;
}

STDMETHODIMP OpInterface::LeftDown(LONG* ret) {
	*ret = _bkproc._bkmouse.LeftDown();
	return S_OK;
}

STDMETHODIMP OpInterface::LeftUp(LONG* ret) {
	*ret = _bkproc._bkmouse.LeftUp();
	return S_OK;
}

STDMETHODIMP OpInterface::MiddleClick(LONG* ret) {
	*ret = _bkproc._bkmouse.MiddleClick();
	return S_OK;
}

STDMETHODIMP OpInterface::MiddleDown(LONG* ret) {
	*ret = _bkproc._bkmouse.MiddleDown();
	return S_OK;
}

STDMETHODIMP OpInterface::MiddleUp(LONG* ret) {
	*ret = _bkproc._bkmouse.MiddleUp();
	return S_OK;
}

STDMETHODIMP OpInterface::RightClick(LONG* ret) {
	*ret = _bkproc._bkmouse.RightClick();
	return S_OK;
}

STDMETHODIMP OpInterface::RightDown(LONG* ret) {
	*ret = _bkproc._bkmouse.RightDown();
	return S_OK;
}


STDMETHODIMP OpInterface::RightUp(LONG* ret) {
	*ret = _bkproc._bkmouse.RightUp();
	return S_OK;
}

STDMETHODIMP OpInterface::WheelDown(LONG* ret) {
	*ret = _bkproc._bkmouse.WheelDown();
	return S_OK;
}

STDMETHODIMP OpInterface::WheelUp(LONG* ret) {
	*ret = _bkproc._bkmouse.WheelUp();
	return S_OK;
}

STDMETHODIMP OpInterface::GetKeyState(LONG vk_code, LONG* ret) {
	*ret = _bkproc._keypad.GetKeyState(vk_code);
	return S_OK;
}

STDMETHODIMP OpInterface::KeyDown(LONG vk_code, LONG* ret) {
	*ret = _bkproc._keypad.KeyDown(vk_code);
	return S_OK;
}

STDMETHODIMP OpInterface::KeyDownChar(BSTR vk_code, LONG* ret) {
	auto nlen = ::SysStringLen(vk_code);
	*ret = nlen > 0 ? _bkproc._keypad.KeyDown(vk_code[0]) : 0;
	return S_OK;
}

STDMETHODIMP OpInterface::KeyUp(LONG vk_code, LONG* ret) {
	*ret = _bkproc._keypad.KeyUp(vk_code);
	return S_OK;
}

STDMETHODIMP OpInterface::KeyUpChar(BSTR vk_code, LONG* ret) {
	auto nlen = ::SysStringLen(vk_code);
	*ret = nlen > 0 ? _bkproc._keypad.KeyUp(vk_code[0]) : 0;
	return S_OK;
}

STDMETHODIMP OpInterface::WaitKey(LONG vk_code, LONG time_out, LONG* ret) {
	if (time_out < 0)time_out = 0;
	*ret = _bkproc._keypad.WaitKey(vk_code, time_out);
	return S_OK;
}



//抓取指定区域(x1, y1, x2, y2)的图像, 保存为file
STDMETHODIMP OpInterface::Capture(LONG x1, LONG y1, LONG x2, LONG y2, BSTR file_name, LONG* ret) {
	*ret = _bkproc.Capture(file_name);
	return S_OK;
}
//比较指定坐标点(x,y)的颜色
STDMETHODIMP OpInterface::CmpColor(LONG x, LONG y, BSTR color, DOUBLE sim, LONG* ret) {
	//LONG rx = -1, ry = -1;
	*ret = 0;
	if (!_bkproc.IsBind())
		return S_OK;

	_bkproc.lock_data();
	_image_proc.input_image(_bkproc.GetScreenData(), _bkproc.get_widht(), _bkproc.get_height(),
		0, 0, _bkproc.get_widht(), _bkproc.get_height(), _bkproc.get_image_type());
	_bkproc.unlock_data();
	*ret = _image_proc.CmpColor(x, y, color, sim);

	return S_OK;
}
//查找指定区域内的颜色
STDMETHODIMP OpInterface::FindColor(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG dir, VARIANT* x, VARIANT* y, LONG* ret) {
	LONG rx = -1, ry = -1;
	*ret = 0;
	x->vt = y->vt = VT_I4;
	x->lVal = rx; y->lVal = ry;
	if (!_bkproc.IsBind())
		return S_OK;
	if (_bkproc.RectConvert(x1, y1, x2, y2)) {
		_bkproc.lock_data();
		_image_proc.input_image(_bkproc.GetScreenData(), _bkproc.get_widht(), _bkproc.get_height(),
			x1, y1, x2, y2, _bkproc.get_image_type());
		_bkproc.unlock_data();
		*ret = _image_proc.FindColor(color, sim, dir, rx, ry);
		if (*ret) {
			rx += x1; ry += y1;
			rx -= _bkproc._pbkdisplay->_client_x;
			ry -= _bkproc._pbkdisplay->_client_y;
		}
	}
	x->lVal = rx; y->lVal = ry;

	return S_OK;
}
//查找指定区域内的所有颜色
STDMETHODIMP OpInterface::FindColorEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG dir, BSTR* retstr) {
	CComBSTR newstr;
	if (_bkproc.IsBind()&& _bkproc.RectConvert(x1, y1, x2, y2)) {
		_bkproc.lock_data();
		_image_proc.input_image(_bkproc.GetScreenData(), _bkproc.get_widht(), _bkproc.get_height(),
			x1, y1, x2, y2, _bkproc.get_image_type());
		_bkproc.unlock_data();
		std::wstring str;
		_image_proc.FindColoEx(color, sim, dir, str);
		newstr.Append(str.c_str());
	}
	newstr.CopyTo(retstr);
	return S_OK;
}
//根据指定的多点查找颜色坐标
STDMETHODIMP OpInterface::FindMultiColor(LONG x1, LONG y1, LONG x2, LONG y2, BSTR first_color, BSTR offset_color, DOUBLE sim, LONG dir, VARIANT* x, VARIANT* y, LONG* ret) {
	LONG rx = -1, ry = -1;
	*ret = 0;
	x->vt = y->vt = VT_I4;
	x->lVal = rx; y->lVal = ry;
	if (!_bkproc.IsBind())
		return S_OK;
	if (_bkproc.RectConvert(x1, y1, x2, y2)) {
		_bkproc.lock_data();
		_image_proc.input_image(_bkproc.GetScreenData(), _bkproc.get_widht(), _bkproc.get_height(),
			x1, y1, x2, y2, _bkproc.get_image_type());
		_bkproc.unlock_data();
		*ret = _image_proc.FindMultiColor(first_color, offset_color, sim, dir, rx, ry);
		if (*ret) {
			rx += x1; ry += y1;
			rx -= _bkproc._pbkdisplay->_client_x;
			ry -= _bkproc._pbkdisplay->_client_y;
		}
	}
	x->lVal = rx; y->lVal = ry;

	return S_OK;
}
//根据指定的多点查找所有颜色坐标
STDMETHODIMP OpInterface::FindMultiColorEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR first_color, BSTR offset_color, DOUBLE sim, LONG dir, BSTR* retstr) {
	CComBSTR newstr;
	if (!_bkproc.IsBind())
		return S_OK;
	if (_bkproc.RectConvert(x1, y1, x2, y2)) {
		_bkproc.lock_data();
		_image_proc.input_image(_bkproc.GetScreenData(), _bkproc.get_widht(), _bkproc.get_height(),
			x1, y1, x2, y2, _bkproc.get_image_type());
		_bkproc.unlock_data();
		std::wstring str;
		_image_proc.FindMultiColorEx(first_color, offset_color, sim, dir, str);
		newstr.Append(str.c_str());
	}
	newstr.CopyTo(retstr);
	return S_OK;
}
//查找指定区域内的图片
STDMETHODIMP OpInterface::FindPic(LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim, LONG dir, VARIANT* x, VARIANT* y, LONG* ret) {
	long lx = -1, ly = -1;
	/*if (_background.GetDisplay() == BACKTYPE::DX)
		*ret = _background._bkdx9.FindPic(x1, y1, x2, y2, files, sim, lx, ly);
	else
		*ret = _background._bkgdi.FindPic(x1, y1, x2, y2, files, sim, lx, ly);*/
	x->vt = y->vt = VT_I4;
	x->lVal = lx; y->lVal = ly;
	return S_OK;
}
//查找多个图片
STDMETHODIMP OpInterface::FindPicEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim, LONG dir, BSTR* retstr) {
	return S_OK;
}
//获取(x,y)的颜色
STDMETHODIMP OpInterface::GetColor(LONG x, LONG y, BSTR* ret) {
	static DWORD recent_call = 0;
	DWORD t = GetTickCount();
	if (!_bkproc.IsBind())
		return S_OK;
	x += _bkproc._pbkdisplay->_client_x;
	y += _bkproc._pbkdisplay->_client_y;
	color_t cr;
	auto p = _bkproc.GetScreenData();
	cr = *(color_t*)(p + y * 4 + x);
	auto str = cr.tostr();
	CComBSTR newstr;
	newstr.Append(str.c_str());
	newstr.CopyTo(ret);
	return S_OK;
}



//设置字库文件
STDMETHODIMP OpInterface::SetDict(LONG idx, BSTR file_name, LONG* ret) {
	*ret = _image_proc.SetDict(idx, file_name);
	return S_OK;
}
//使用哪个字库文件进行识别
STDMETHODIMP OpInterface::UseDict(LONG idx, LONG* ret) {
	*ret = _image_proc.UseDict(idx);
	return S_OK;
}
//识别屏幕范围(x1,y1,x2,y2)内符合color_format的字符串,并且相似度为sim,sim取值范围(0.1-1.0),
STDMETHODIMP OpInterface::Ocr(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR* ret_str) {
	wstring str;
	if (_bkproc.IsBind() && _bkproc.RectConvert(x1, y1, x2, y2)) {
		_bkproc.lock_data();
		_image_proc.input_image(_bkproc.GetScreenData(), _bkproc.get_widht(), _bkproc.get_height(),
			x1, y1, x2, y2, _bkproc.get_image_type());
		_bkproc.unlock_data();
		_image_proc.OCR(color, sim, str);
	}

	CComBSTR newstr;
	newstr.Append(str.c_str());
	newstr.CopyTo(ret_str);
	return S_OK;
}
//回识别到的字符串，以及每个字符的坐标.
STDMETHODIMP OpInterface::OcrEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR* ret_str) {
	wstring str;
	if (_bkproc.IsBind() && _bkproc.RectConvert(x1, y1, x2, y2)) {
		_bkproc.lock_data();
		_image_proc.input_image(_bkproc.GetScreenData(), _bkproc.get_widht(), _bkproc.get_height(),
			x1, y1, x2, y2, _bkproc.get_image_type());
		_bkproc.unlock_data();
		_image_proc.OcrEx(color, sim, str);
	}

	CComBSTR newstr;
	newstr.Append(str.c_str());
	newstr.CopyTo(ret_str);
	return S_OK;
}
//在屏幕范围(x1,y1,x2,y2)内,查找string(可以是任意个字符串的组合),并返回符合color_format的坐标位置
STDMETHODIMP OpInterface::FindStr(LONG x1, LONG y1, LONG x2, LONG y2, BSTR strs, BSTR color, DOUBLE sim, VARIANT* retx, VARIANT* rety,LONG* ret) {
	wstring str;
	retx->vt = rety->vt = VT_INT;
	retx->lVal = rety->lVal = -1;
	if (_bkproc.IsBind() && _bkproc.RectConvert(x1, y1, x2, y2)) {
		_bkproc.lock_data();
		_image_proc.input_image(_bkproc.GetScreenData(), _bkproc.get_widht(), _bkproc.get_height(),
			x1, y1, x2, y2, _bkproc.get_image_type());
		_bkproc.unlock_data();
		*ret = _image_proc.FindStr(strs, color, sim, retx->lVal, rety->lVal);
	}

	return S_OK;
}
//返回符合color_format的所有坐标位置
STDMETHODIMP OpInterface::FindStrEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR strs, BSTR color, DOUBLE sim, BSTR* retstr) {
	wstring str;
	if (_bkproc.IsBind() && _bkproc.RectConvert(x1, y1, x2, y2)) {
		_bkproc.lock_data();
		_image_proc.input_image(_bkproc.GetScreenData(), _bkproc.get_widht(), _bkproc.get_height(),
			x1, y1, x2, y2, _bkproc.get_image_type());
		_bkproc.unlock_data();
		_image_proc.FindStrEx(strs, color, sim, str);
	}

	CComBSTR newstr;
	newstr.Append(str.c_str());
	newstr.CopyTo(retstr);
	return S_OK;
}

STDMETHODIMP OpInterface::OcrAuto(LONG x1, LONG y1, LONG x2, LONG y2, DOUBLE sim, BSTR* retstr) {
	wstring str;
	if (_bkproc.IsBind() && _bkproc.RectConvert(x1, y1, x2, y2)) {
		_bkproc.lock_data();
		_image_proc.input_image(_bkproc.GetScreenData(), _bkproc.get_widht(), _bkproc.get_height(),
			x1, y1, x2, y2, _bkproc.get_image_type());
		_bkproc.unlock_data();
		_image_proc.OcrAuto(sim, str);
	}

	CComBSTR newstr;
	newstr.Append(str.c_str());
	newstr.CopyTo(retstr);
	return S_OK;
}