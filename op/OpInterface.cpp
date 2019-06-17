// OpInterface.cpp: OpInterface 的实现

#include "stdafx.h"
#include "OpInterface.h"
#include "Cmder.h"
#include "Injecter.h"
#include "Tool.h"
#include "AStar.hpp"
#include <filesystem>
// OpInterface

STDMETHODIMP OpInterface::Ver(BSTR* ret) {
	
	//Tool::setlog("address=%d,str=%s", ver, ver);
	CComBSTR newstr;
	newstr.Append(OP_VERSION);
	newstr.CopyTo(ret);

	return S_OK;
}

STDMETHODIMP OpInterface::SetPath(BSTR path, LONG* ret) {
	std::filesystem::path p = path;
	auto fullpath = std::filesystem::absolute(p);
	_curr_path = fullpath.generic_wstring();
	_image_proc._curr_path = _curr_path;
	*ret = 1;
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

STDMETHODIMP OpInterface::SetShowErrorMsg(LONG show_type, LONG* ret){
	gShowError = show_type;
	*ret = 1;
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

STDMETHODIMP OpInterface::EnablePicCache(LONG enable, LONG* ret) {
	_image_proc._enable_cache = enable;
	*ret = 1;
	return S_OK;
}

STDMETHODIMP OpInterface::AStarFindPath(LONG mapWidth, LONG mapHeight, BSTR disable_points, LONG beginX, LONG beginY, LONG endX, LONG endY, BSTR* path) {
	AStar as;
	vector<Vector2i>walls;
	vector<wstring> vstr;
	Vector2i tp;
	split(disable_points, vstr, L"|");
	for (auto&it : vstr) {
		if (swscanf(it.c_str(), L"%d,%d", &tp[0], &tp[1]) != 2)
			break;
		walls.push_back(tp);
	}
	list<Vector2i> paths;
	
	as.set_map( mapWidth, mapHeight , walls);
	as.findpath(beginX,beginY , endX,endY , paths);
	wstring pathstr;
	wchar_t buf[20];
	for (auto it = paths.rbegin(); it != paths.rend(); ++it) {
		auto v = *it;
		wsprintf(buf, L"%d,%d", v[0], v[1]);
		pathstr += buf;
		pathstr.push_back(L'|');
	}
	if (!pathstr.empty())
		pathstr.pop_back();
	CComBSTR newstr;
	newstr.Append(pathstr.c_str());
	newstr.CopyTo(path);
	return S_OK;
}


STDMETHODIMP OpInterface::EnumWindow(LONG parent, BSTR title, BSTR class_name, LONG filter, BSTR* retstr)
{
	// TODO: 在此添加实现代码
	std::unique_ptr<wchar_t> retstring(new wchar_t[MAX_PATH * 200]);
	memset(retstring.get(), 0, sizeof(wchar_t)*MAX_PATH * 200);
	_winapi.EnumWindow((HWND)parent, title, class_name, filter, retstring.get());
	//*retstr=_bstr_t(retstring);
	CComBSTR newbstr;
	auto hr=newbstr.Append(retstring.get());
	hr=newbstr.CopyTo(retstr);
	return hr;
}

STDMETHODIMP OpInterface::EnumWindowByProcess(BSTR process_name, BSTR title, BSTR class_name, LONG filter, BSTR* retstring)
{
	// TODO: 在此添加实现代码
	std::unique_ptr<wchar_t> retstr(new wchar_t[MAX_PATH * 200]);
	memset(retstr.get(), 0, sizeof(wchar_t) * MAX_PATH * 200);
	_winapi.EnumWindow((HWND)0, title, class_name, filter, retstr.get(), process_name);
	//*retstring=_bstr_t(retstr);
	CComBSTR newbstr;
	newbstr.Append(retstr.get());
	newbstr.CopyTo(retstring);
	return S_OK;
}

STDMETHODIMP OpInterface::EnumProcess(BSTR name, BSTR* retstring)
{
	// TODO: 在此添加实现代码
	std::unique_ptr<wchar_t> retstr(new wchar_t[MAX_PATH * 200]);
	memset(retstr.get(), 0, sizeof(wchar_t) * MAX_PATH * 200);
	_winapi.EnumProcess(name, retstr.get());
	//*retstring=_bstr_t(retstr);
	CComBSTR newbstr;
	newbstr.Append(retstr.get());
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

STDMETHODIMP OpInterface::RunApp(BSTR cmdline, LONG mode, LONG* ret) {
	*ret = _winapi.RunApp(cmdline, mode);
	return S_OK;
}

STDMETHODIMP OpInterface::WinExec(BSTR cmdline, LONG cmdshow, LONG* ret) {
	auto str = _ws2string(cmdline);
	*ret = ::WinExec(str.c_str(), cmdshow) > 31 ? 1 : 0;
	return S_OK;
}

STDMETHODIMP OpInterface::GetCmdStr(BSTR cmd, LONG millseconds, BSTR* retstr) {
	CComBSTR bstr;
	auto strcmd = _ws2string(cmd);
	Cmder cd;
	auto str = cd.GetCmdStr(strcmd, millseconds <= 0 ? 5 : millseconds);
	auto hr = bstr.Append(str.c_str());
	hr = bstr.CopyTo(retstr);
	return hr;
}



STDMETHODIMP OpInterface::BindWindow(LONG hwnd, BSTR display, BSTR mouse, BSTR keypad, LONG mode, LONG *ret) {
	if (_bkproc.IsBind())
		_bkproc.UnBindWindow();
	*ret = _bkproc.BindWindow(hwnd, display, mouse, keypad, mode);
	if (*ret == 1) {
		_image_proc.set_offset(_bkproc._pbkdisplay->_client_x, _bkproc._pbkdisplay->_client_y);
	}
	return S_OK;
}

STDMETHODIMP OpInterface::UnBindWindow(LONG* ret) {
	*ret = _bkproc.UnBindWindow();
	return S_OK;
}

STDMETHODIMP OpInterface::GetCursorPos(VARIANT* x, VARIANT* y, LONG* ret) {
	x->vt = y->vt = VT_I4;
	*ret = _bkproc._bkmouse.GetCursorPos(x->lVal, y->lVal);
	return S_OK;
}

STDMETHODIMP OpInterface::MoveR(LONG x, LONG y, LONG* ret) {
	*ret = _bkproc._bkmouse.MoveR(x, y);
	return S_OK;
}
//把鼠标移动到目的点(x,y)
STDMETHODIMP OpInterface::MoveTo(LONG x, LONG y, LONG* ret) {
	*ret = _bkproc._bkmouse.MoveTo(x, y);
	return S_OK;
}

STDMETHODIMP OpInterface::MoveToEx(LONG x, LONG y, LONG w, LONG h, LONG* ret) {
	*ret = _bkproc._bkmouse.MoveToEx(x, y, w, h);
	return S_OK;
}

STDMETHODIMP OpInterface::LeftClick(LONG* ret) {
	*ret = _bkproc._bkmouse.LeftClick();
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
	*ret = 0;
	if (nlen > 0) {
		long vk = _vkmap.count(vk_code) ? _vkmap[vk_code] : vk_code[0];
		*ret = _bkproc._keypad.KeyDown(vk);
	}
	
	return S_OK;
}

STDMETHODIMP OpInterface::KeyUp(LONG vk_code, LONG* ret) {
	*ret = _bkproc._keypad.KeyUp(vk_code);
	return S_OK;
}

STDMETHODIMP OpInterface::KeyUpChar(BSTR vk_code, LONG* ret) {
	auto nlen = ::SysStringLen(vk_code);
	*ret = 0;
	if (nlen > 0) {
		long vk = _vkmap.count(vk_code) ? _vkmap[vk_code] : vk_code[0];
		*ret = _bkproc._keypad.KeyUp(vk);
	}
	return S_OK;
}

STDMETHODIMP OpInterface::WaitKey(LONG vk_code, LONG time_out, LONG* ret) {
	if (time_out < 0)time_out = 0;
	*ret = _bkproc._keypad.WaitKey(vk_code, time_out);
	return S_OK;
}

STDMETHODIMP OpInterface::KeyPress(LONG vk_code, LONG* ret) {
	
		*ret = _bkproc._keypad.KeyPress(vk_code);
	
	return S_OK;
}

STDMETHODIMP OpInterface::KeyPressChar(BSTR vk_code, LONG* ret) {
	auto nlen = ::SysStringLen(vk_code);
	*ret = 0;
	if (nlen > 0) {
		long vk = _vkmap.count(vk_code) ? _vkmap[vk_code] : vk_code[0];
		*ret = _bkproc._keypad.KeyPress(vk);
	}
	return S_OK;
}



//抓取指定区域(x1, y1, x2, y2)的图像, 保存为file
STDMETHODIMP OpInterface::Capture(LONG x1, LONG y1, LONG x2, LONG y2, BSTR file_name, LONG* ret) {
	
	*ret = 0;
	if (!_bkproc.IsBind()) {
		
		return S_OK;
	}
	if (_bkproc.RectConvert(x1, y1, x2, y2)) {
		_bkproc.lock_data();
		_image_proc.input_image(_bkproc.GetScreenData(), _bkproc.get_widht(), _bkproc.get_height(),
			x1, y1, x2, y2, _bkproc.get_image_type());
		_bkproc.unlock_data();
		*ret = _image_proc.Capture(file_name);
	}
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
	/*	if (*ret) {
			rx += x1; ry += y1;
			rx -= _bkproc._pbkdisplay->_client_x;
			ry -= _bkproc._pbkdisplay->_client_y;
		}*/
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
		/*if (*ret) {
			rx += x1; ry += y1;
			rx -= _bkproc._pbkdisplay->_client_x;
			ry -= _bkproc._pbkdisplay->_client_y;
		}*/
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
		*ret = _image_proc.FindPic(files, delta_color, sim, 0, rx, ry);
		/*if (*ret) {
			rx += x1; ry += y1;
			rx -= _bkproc._pbkdisplay->_client_x;
			ry -= _bkproc._pbkdisplay->_client_y;
		}*/
	}
	x->lVal = rx; y->lVal = ry;

	return S_OK;
}
//查找多个图片
STDMETHODIMP OpInterface::FindPicEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim, LONG dir, BSTR* retstr) {
	CComBSTR newstr;
	HRESULT hr;
	if (_bkproc.IsBind() && _bkproc.RectConvert(x1, y1, x2, y2)) {
		_bkproc.lock_data();
		_image_proc.input_image(_bkproc.GetScreenData(), _bkproc.get_widht(), _bkproc.get_height(),
			x1, y1, x2, y2, _bkproc.get_image_type());
		_bkproc.unlock_data();
		std::wstring str;
		_image_proc.FindPicEx(files, delta_color, sim, dir, str);
		hr=newstr.Append(str.c_str());
	}
	hr=newstr.CopyTo(retstr);
	return hr;
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

//从文件中识别图片
STDMETHODIMP OpInterface::OcrFromFile(BSTR file_name, BSTR color_format, DOUBLE sim, BSTR* retstr) {
	CComBSTR newstr;
	wstring str;
	_image_proc.OcrFromFile(file_name, color_format, sim, str);
	newstr.Append(str.data());
	newstr.CopyTo(retstr);
	return S_OK;
}
//从文件中识别图片,无需指定颜色
STDMETHODIMP OpInterface::OcrAutoFromFile(BSTR file_name, DOUBLE sim, BSTR* retstr){
	CComBSTR newstr;
	wstring str;
	_image_proc.OcrAutoFromFile(file_name, sim, str);
	newstr.Append(str.data());
	newstr.CopyTo(retstr);
	return S_OK;
}