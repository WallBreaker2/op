// OpInterface.cpp: OpInterface 的实现

#include "stdafx.h"
#include "libop.h"
#include "optype.h"
#include "globalVar.h"
#include "helpfunc.h"
#include "WinApi.h"
#include "BKbase.h"
#include "ImageProc.h"
#include "Cmder.h"
#include "Injecter.h"

#include "AStar.hpp"
#include "MemoryEx.h"
#include<fstream>
// OpInterface

libop::libop() {
	_winapi = new WinApi;
	_bkproc = new bkbase;
	_image_proc = new ImageProc;

	//初始化目录
	wchar_t buff[256];
	::GetCurrentDirectoryW(256, buff);
	_curr_path = buff;
	_image_proc->_curr_path = _curr_path;
	//初始化键码表
	_vkmap[L"back"] = VK_BACK; _vkmap[L"ctrl"] = VK_CONTROL;
	_vkmap[L"alt"] = LVKF_ALT; _vkmap[L"shift"] = VK_SHIFT;
	_vkmap[L"win"] = VK_LWIN;
	_vkmap[L"space"] = L' '; _vkmap[L"tab"] = VK_TAB;
	_vkmap[L"esc"] = VK_CANCEL;
	_vkmap[L"enter"] = L'\r'; _vkmap[L"up"] = VK_UP;
	_vkmap[L"down"] = VK_DOWN; _vkmap[L"left"] = VK_LEFT;
	_vkmap[L"right"] = VK_RIGHT;
	_vkmap[L"f1"] = VK_F1; _vkmap[L"f2"] = VK_F2;
	_vkmap[L"f3"] = VK_F3; _vkmap[L"f4"] = VK_F4;
	_vkmap[L"f5"] = VK_F5; _vkmap[L"f6"] = VK_F6;
	_vkmap[L"f7"] = VK_F7; _vkmap[L"f8"] = VK_F8;
	_vkmap[L"f9"] = VK_F9; _vkmap[L"f10"] = VK_F10;
	_vkmap[L"f11"] = VK_F11; _vkmap[L"f12"] = VK_F12;
	//初始化 op 路径 & name
	static bool is_init = false;
	if (!is_init) {
		g_op_path.resize(512);
		DWORD real_size = ::GetModuleFileNameW(gInstance, g_op_path.data(), 512);
		g_op_path.resize(real_size);

		g_op_name = g_op_path.substr(g_op_path.rfind(L"\\") + 1);
		g_op_path = g_op_path.substr(0, g_op_path.rfind(L"\\"));

		is_init = true;
	}
}

libop::~libop() {
	delete _winapi;
	delete _bkproc;
	delete _image_proc;
}

long  libop::Ver(std::wstring& ret) {
	
	//Tool::setlog("address=%d,str=%s", ver, ver);
	ret = _T(OP_VERSION);
	return S_OK;
}

long  libop::SetPath(const wchar_t* path, long* ret) {
	wstring fpath = path;
	replacew(fpath, L"/", L"\\");
	if (fpath.find(L'\\') != -1 && ::PathFileExistsW(fpath.data())) {
		_curr_path = fpath;
		_image_proc->_curr_path = _curr_path;
		_bkproc->_curr_path = _curr_path;
		*ret = 1;
	}
	else {

		if (!fpath.empty() && fpath[0] != L'\\')
			fpath = _curr_path + L'\\' + fpath;
		else
			fpath = _curr_path + fpath;
		if (::PathFileExistsW(fpath.data())) {
			_curr_path = path;
			_image_proc->_curr_path = _curr_path;
			_bkproc->_curr_path = _curr_path;
			*ret = 1;
		}
		else
			*ret = 0;
	}
	
	return S_OK;
}

long  libop::GetPath(std::wstring& path) {
	path = _curr_path;
	return S_OK;
}

long  libop::GetBasePath(std::wstring& path){
	wchar_t basepath[1024];
	::GetModuleFileName(gInstance, basepath, 1024);
	path = basepath;
	int index = path.rfind(L'\\');
	path = path.substr(0, index);
	return S_OK;
}

long libop::GetID(long* ret) {
	*ret = (long)this;
	return 0;
}

long libop::GetLastError(long* ret) {
	*ret = ::GetLastError();
	return 0;
}


long  libop::SetShowErrorMsg(long show_type, long* ret){
	gShowError = show_type;
	*ret = 1;
	return S_OK;
}



long  libop::Sleep(long millseconds, long* ret) {
	::Sleep(millseconds);
	*ret = 1;
	return S_OK;
}

long  libop::InjectDll(const wchar_t* process_name, const wchar_t* dll_name, long* ret) {
	//auto proc = _wsto_string(process_name);
	//auto dll = _wsto_string(dll_name);
	//Injecter::EnablePrivilege(TRUE);
	//auto h = Injecter::InjectDll(process_name, dll_name);
	*ret = 0;
	return S_OK;
}

long  libop::EnablePicCache(long enable, long* ret) {
	_image_proc->_enable_cache = enable;
	*ret = 1;
	return S_OK;
}

long  libop::CapturePre(const wchar_t* file, LONG* ret) {
	*ret = _image_proc->Capture(file);
	return S_OK;
}

long  libop::AStarFindPath(long mapWidth, long mapHeight, const wchar_t* disable_points, long beginX, long beginY, long endX, long endY, std::wstring& path) {
	AStar as;
	using Vec2i = AStar::Vec2i;
	vector<Vec2i>walls;
	vector<wstring> vstr;
	Vec2i tp;
	split(disable_points, vstr, L"|");
	for (auto&it : vstr) {
		if (swscanf(it.c_str(), L"%d,%d", &tp.x, &tp.y) != 2)
			break;
		walls.push_back(tp);
	}
	list<Vec2i> paths;
	
	as.set_map( mapWidth, mapHeight , walls);
	as.findpath(beginX,beginY , endX,endY , paths);
	wstring pathstr;
	wchar_t buf[20];
	for (auto it = paths.rbegin(); it != paths.rend(); ++it) {
		auto v = *it;
		wsprintf(buf, L"%d,%d", v.x, v.y);
		pathstr += buf;
		pathstr.push_back(L'|');
	}
	if (!pathstr.empty())
		pathstr.pop_back();
	return S_OK;
}


long  libop::EnumWindow(long parent, const wchar_t* title, const wchar_t* class_name, long filter, std::wstring& retstr)
{
	// TODO: 在此添加实现代码
	std::unique_ptr<wchar_t> retstring(new wchar_t[MAX_PATH * 200]);
	memset(retstring.get(), 0, sizeof(wchar_t)*MAX_PATH * 200);
	_winapi->EnumWindow((HWND)parent, title, class_name, filter, retstring.get());
	//*retstr=_bstr_t(retstring);
	retstr = retstring.get();
	return 0;
}

long  libop::EnumWindowByProcess(const wchar_t* process_name, const wchar_t* title, const wchar_t* class_name, long filter, std::wstring& retstring)
{
	// TODO: 在此添加实现代码
	std::unique_ptr<wchar_t> retstr(new wchar_t[MAX_PATH * 200]);
	memset(retstr.get(), 0, sizeof(wchar_t) * MAX_PATH * 200);
	_winapi->EnumWindow((HWND)0, title, class_name, filter, retstr.get(), process_name);
	//*retstring=_bstr_t(retstr);
	
	retstring = retstr.get();
	return S_OK;
}

long  libop::EnumProcess(const wchar_t* name, std::wstring& retstring)
{
	// TODO: 在此添加实现代码
	std::unique_ptr<wchar_t> retstr(new wchar_t[MAX_PATH * 200]);
	memset(retstr.get(), 0, sizeof(wchar_t) * MAX_PATH * 200);
	_winapi->EnumProcess(name, retstr.get());
	//*retstring=_bstr_t(retstr);
	retstring = retstr.get();
	return S_OK;
}

long  libop::ClientToScreen(long ClientToScreen, long* x, long* y, long* bret)
{
	// TODO: 在此添加实现代码

	*bret = _winapi->ClientToScreen(ClientToScreen, *x, *y);
	return S_OK;
}

long  libop::FindWindow(const wchar_t* class_name, const wchar_t* title, long* rethwnd)
{
	// TODO: 在此添加实现代码
	*rethwnd = _winapi->FindWindow(class_name, title);
	return S_OK;
}

long  libop::FindWindowByProcess(const wchar_t* process_name, const wchar_t* class_name, const wchar_t* title, long* rethwnd)
{
	// TODO: 在此添加实现代码
	_winapi->FindWindowByProcess(class_name, title, *rethwnd, process_name);
	return S_OK;
}

long  libop::FindWindowByProcessId(long process_id, const wchar_t* class_name, const wchar_t* title, long* rethwnd)
{
	// TODO: 在此添加实现代码
	_winapi->FindWindowByProcess(class_name, title, *rethwnd, NULL, process_id);
	return S_OK;
}

long  libop::FindWindowEx(long parent, const wchar_t* class_name, const wchar_t* title, long* rethwnd)
{
	// TODO: 在此添加实现代码
	*rethwnd = _winapi->FindWindowEx(parent,class_name, title);
	return S_OK;
}

long  libop::GetClientRect(long hwnd, long* x1, long* y1, long* x2, long* y2, long* nret)
{
	// TODO: 在此添加实现代码
	
	*nret = _winapi->GetClientRect(hwnd, *x1, *y1, *x2, *y2);
	return S_OK;
}


long  libop::GetClientSize(long hwnd, long* width, long* height, long* nret)
{
	// TODO: 在此添加实现代码
	
	*nret = _winapi->GetClientSize(hwnd, *width, *height);
	return S_OK;
}

long  libop::GetForegroundFocus(long* rethwnd)
{
	// TODO: 在此添加实现代码
	*rethwnd = (LONG)::GetFocus();
	return S_OK;
}

long  libop::GetForegroundWindow(long* rethwnd)
{
	// TODO: 在此添加实现代码
	*rethwnd = (LONG)::GetForegroundWindow();
	return S_OK;
}

long  libop::GetMousePointWindow(long* rethwnd)
{
	// TODO: 在此添加实现代码
	//::Sleep(2000);
	_winapi->GetMousePointWindow(*rethwnd);
	return S_OK;
}

long  libop::GetPointWindow(long x, long y, long* rethwnd)
{
	// TODO: 在此添加实现代码
	_winapi->GetMousePointWindow(*rethwnd, x, y);
	return S_OK;
}

long  libop::GetProcessInfo(long pid, std::wstring& retstring)
{
	// TODO: 在此添加实现代码

	wchar_t retstr[MAX_PATH] = { 0 };
	_winapi->GetProcessInfo(pid, retstr);
	//* retstring=_bstr_t(retstr);
	
	retstring = retstr;
	return S_OK;
}

long  libop::GetSpecialWindow(long flag, long* rethwnd)
{
	// TODO: 在此添加实现代码
	*rethwnd = 0;
	if (flag == 0)
		*rethwnd = (LONG)GetDesktopWindow();
	else if (flag == 1)
	{
		*rethwnd = (LONG)::FindWindowW(L"Shell_TrayWnd", NULL);
	}

	return S_OK;
}

long  libop::GetWindow(long hwnd, long flag, long* nret)
{
	// TODO: 在此添加实现代码
	_winapi->TSGetWindow(hwnd, flag, *nret);
	return S_OK;
}

long  libop::GetWindowClass(long hwnd, std::wstring& retstring)
{
	// TODO: 在此添加实现代码
	wchar_t classname[MAX_PATH] = { 0 };
	::GetClassName((HWND)hwnd, classname, MAX_PATH);
	//* retstring=_bstr_t(classname);
	
	retstring = classname;
	return S_OK;
}

long  libop::GetWindowProcessId(long hwnd, long* nretpid)
{
	// TODO: 在此添加实现代码
	DWORD pid = 0;
	::GetWindowThreadProcessId((HWND)hwnd, &pid);
	*nretpid = pid;
	return S_OK;
}

long  libop::GetWindowProcessPath(long hwnd, std::wstring& retstring)
{
	// TODO: 在此添加实现代码
	DWORD pid = 0;
	::GetWindowThreadProcessId((HWND)hwnd, &pid);
	wchar_t process_path[MAX_PATH] = { 0 };
	_winapi->GetProcesspath(pid, process_path);
	//* retstring=_bstr_t(process_path);
	
	retstring = process_path;
	return S_OK;
}

long  libop::GetWindowRect(long hwnd, long* x1, long* y1, long* x2, long* y2, long* nret)
{
	// TODO: 在此添加实现代码

	RECT winrect;
	*nret = ::GetWindowRect((HWND)hwnd, &winrect);
	*x1 = winrect.left;
	*y1 = winrect.top;
	*x2 = winrect.right;
	*y2 = winrect.bottom;
	return S_OK;
}

long  libop::GetWindowState(long hwnd, long flag, long* rethwnd)
{
	// TODO: 在此添加实现代码
	*rethwnd = _winapi->GetWindowState(hwnd, flag);
	return S_OK;
}

long  libop::GetWindowTitle(long hwnd, std::wstring& rettitle)
{
	// TODO: 在此添加实现代码
	wchar_t title[MAX_PATH] = { 0 };
	::GetWindowText((HWND)hwnd, title, MAX_PATH);
	//* rettitle=_bstr_t(title);
	
	rettitle = title;
	return S_OK;
}

long  libop::MoveWindow(long hwnd, long x, long y, long* nret)
{
	// TODO: 在此添加实现代码
	RECT winrect;
	::GetWindowRect((HWND)hwnd, &winrect);
	int width = winrect.right - winrect.left;
	int hight = winrect.bottom - winrect.top;
	*nret = ::MoveWindow((HWND)hwnd, x, y, width, hight, false);
	return S_OK;
}

long  libop::ScreenToClient(long hwnd, long* x, long* y, long* nret)
{
	// TODO: 在此添加实现代码
	
	POINT point;
	*nret = ::ScreenToClient((HWND)hwnd, &point);
	*x = point.x;
	*y = point.y;
	return S_OK;
}

long  libop::SendPaste(long hwnd, long* nret)
{
	// TODO: 在此添加实现代码
	*nret = _winapi->SendPaste(hwnd);
	return S_OK;
}

long  libop::SetClientSize(long hwnd, long width, long hight, long* nret)
{
	// TODO: 在此添加实现代码
	*nret = _winapi->SetWindowSize(hwnd, width, hight);
	return S_OK;
}

long  libop::SetWindowState(long hwnd, long flag, long* nret)
{
	// TODO: 在此添加实现代码  
	*nret = _winapi->SetWindowState(hwnd, flag);
	return S_OK;
}

long  libop::SetWindowSize(long hwnd, long width, long height, long* nret)
{
	// TODO: 在此添加实现代码
	*nret = _winapi->SetWindowSize(hwnd, width, height, 1);
	return S_OK;
}

long  libop::SetWindowText(long hwnd, const wchar_t* title, long* nret)
{
	// TODO: 在此添加实现代码  
	//*nret=gWindowObj.TSSetWindowState(hwnd,flag);
	*nret = ::SetWindowText((HWND)hwnd, title);
	return S_OK;
}

long  libop::SetWindowTransparent(long hwnd, long trans, long* nret)
{
	// TODO: 在此添加实现代码
	*nret = _winapi->SetWindowTransparent(hwnd, trans);
	return S_OK;
}

long  libop::SendString(long hwnd, const wchar_t* str, long* ret) {
	*ret = _winapi->SendString((HWND)hwnd, str);
	return S_OK;
}

long  libop::SendStringIme(long hwnd, const wchar_t* str, long* ret) {
	*ret = _winapi->SendStringIme((HWND)hwnd, str);
	return S_OK;
}

long  libop::RunApp(const wchar_t* cmdline, long mode, long* ret) {
	*ret = _winapi->RunApp(cmdline, mode);
	return S_OK;
}

long  libop::WinExec(const wchar_t* cmdline, long cmdshow, long* ret) {
	auto str = _ws2string(cmdline);
	*ret = ::WinExec(str.c_str(), cmdshow) > 31 ? 1 : 0;
	return S_OK;
}

long  libop::GetCmdStr(const wchar_t* cmd, long millseconds, std::wstring& retstr) {
	auto strcmd = _ws2string(cmd);
	Cmder cd;
	auto str = cd.GetCmdStr(strcmd, millseconds <= 0 ? 5 : millseconds);
	retstr = _s2wstring(str);
	return 0;
}



long  libop::BindWindow(long hwnd, const wchar_t* display, const wchar_t* mouse, const wchar_t* keypad, long mode, long *ret) {
	if (_bkproc->IsBind())
		_bkproc->UnBindWindow();
	*ret = _bkproc->BindWindow(hwnd, display, mouse, keypad, mode);
	if (*ret == 1) {
		//_image_proc->set_offset(_bkproc->_pbkdisplay->_client_x, _bkproc->_pbkdisplay->_client_y);
	}
	return S_OK;
}

long  libop::UnBindWindow(long* ret) {
	*ret = _bkproc->UnBindWindow();
	return S_OK;
}

long  libop::GetCursorPos(long* x, long* y, long* ret) {
	
	*ret = _bkproc->_bkmouse->GetCursorPos(*x, *y);
	return S_OK;
}

long  libop::MoveR(long x, long y, long* ret) {
	*ret = _bkproc->_bkmouse->MoveR(x, y);
	return S_OK;
}
//把鼠标移动到目的点(x,y)
long  libop::MoveTo(long x, long y, long* ret) {
	*ret = _bkproc->_bkmouse->MoveTo(x, y);
	return S_OK;
}

long  libop::MoveToEx(long x, long y, long w, long h, long* ret) {
	*ret = _bkproc->_bkmouse->MoveToEx(x, y, w, h);
	return S_OK;
}

long  libop::LeftClick(long* ret) {
	*ret = _bkproc->_bkmouse->LeftClick();
	return S_OK;
}

long  libop::LeftDoubleClick(long* ret) {
	*ret = _bkproc->_bkmouse->LeftDoubleClick();
	return S_OK;
}

long  libop::LeftDown(long* ret) {
	*ret = _bkproc->_bkmouse->LeftDown();
	return S_OK;
}

long  libop::LeftUp(long* ret) {
	*ret = _bkproc->_bkmouse->LeftUp();
	return S_OK;
}

long  libop::MiddleClick(long* ret) {
	*ret = _bkproc->_bkmouse->MiddleClick();
	return S_OK;
}

long  libop::MiddleDown(long* ret) {
	*ret = _bkproc->_bkmouse->MiddleDown();
	return S_OK;
}

long  libop::MiddleUp(long* ret) {
	*ret = _bkproc->_bkmouse->MiddleUp();
	return S_OK;
}

long  libop::RightClick(long* ret) {
	*ret = _bkproc->_bkmouse->RightClick();
	return S_OK;
}

long  libop::RightDown(long* ret) {
	*ret = _bkproc->_bkmouse->RightDown();
	return S_OK;
}

long  libop::RightUp(long* ret) {
	*ret = _bkproc->_bkmouse->RightUp();
	return S_OK;
}

long  libop::WheelDown(long* ret) {
	*ret = _bkproc->_bkmouse->WheelDown();
	return S_OK;
}

long  libop::WheelUp(long* ret) {
	*ret = _bkproc->_bkmouse->WheelUp();
	return S_OK;
}

long  libop::GetKeyState(long vk_code, long* ret) {
	*ret = _bkproc->_keypad->GetKeyState(vk_code);
	return S_OK;
}

long  libop::KeyDown(long vk_code, long* ret) {
	*ret = _bkproc->_keypad->KeyDown(vk_code);
	return S_OK;
}

long  libop::KeyDownChar(const wchar_t* vk_code, long* ret) {
	auto nlen = wcslen(vk_code);
	*ret = 0;
	if (nlen > 0) {
		wstring s = vk_code;
		wstring2lower(s);
		long vk = _vkmap.count(s) ? _vkmap[s] : vk_code[0];
		*ret = _bkproc->_keypad->KeyDown(vk);
	}
	
	return S_OK;
}

long  libop::KeyUp(long vk_code, long* ret) {
	*ret = _bkproc->_keypad->KeyUp(vk_code);
	return S_OK;
}

long  libop::KeyUpChar(const wchar_t* vk_code, long* ret) {
	auto nlen = wcslen(vk_code);
	*ret = 0;
	if (nlen > 0) {
		wstring s = vk_code;
		wstring2lower(s);
		long vk = _vkmap.count(s) ? _vkmap[s] : vk_code[0];
		*ret = _bkproc->_keypad->KeyUp(vk);
	}
	return S_OK;
}

long  libop::WaitKey(long vk_code, long time_out, long* ret) {
	if (time_out < 0)time_out = 0;
	*ret = _bkproc->_keypad->WaitKey(vk_code, time_out);
	return S_OK;
}

long  libop::KeyPress(long vk_code, long* ret) {
	
		*ret = _bkproc->_keypad->KeyPress(vk_code);
	
	return S_OK;
}

long  libop::KeyPressChar(const wchar_t* vk_code, long* ret) {
	auto nlen = wcslen(vk_code);
	*ret = 0;
	if (nlen > 0) {
		//setlog(vk_code);
		wstring s = vk_code;
		wstring2lower(s);
		long vk = _vkmap.count(s) ? _vkmap[s] : vk_code[0];
		*ret = _bkproc->_keypad->KeyPress(vk);
	}
	return S_OK;
}



//抓取指定区域(x1, y1, x2, y2)的图像, 保存为file
long  libop::Capture(long x1, long y1, long x2, long y2, const wchar_t* file_name, long* ret) {
	
	*ret = 0;
	
	if (_bkproc->check_bind()&& _bkproc->RectConvert(x1, y1, x2, y2)) {
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src)) {
			setlog("error requestCapture");
			return S_OK;
		}
		_image_proc->set_offset(x1, y1);
		
		*ret = _image_proc->Capture(file_name);
	}
	return S_OK;
}
//比较指定坐标点(x,y)的颜色
long  libop::CmpColor(long x, long y, const wchar_t* color, DOUBLE sim, long* ret) {
	//LONG rx = -1, ry = -1;
	long tx = x + 1, ty = y + 1;
	*ret = 0;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x, y, tx, ty)) {
		if(!_bkproc->requestCapture(x, y, 1, 1, _image_proc->_src)) {
			setlog("error equestCapture");
			return S_OK;
		}
		_image_proc->set_offset(x, y);
		*ret = _image_proc->CmpColor(x, y, color, sim);
	}
	

	return S_OK;
}
//查找指定区域内的颜色
long  libop::FindColor(long x1, long y1, long x2, long y2, const wchar_t* color, DOUBLE sim, long dir, long* x, long* y, long* ret) {
	
	*ret = 0;
	*x = *y = -1;
	
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2)) {
		if(!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src)) {
			setlog("error equestCapture");
			return S_OK;
		}
		_image_proc->set_offset(x1, y1);
		_image_proc->FindColor(color, sim, dir, *x, *y);
	}

	return S_OK;
}
//查找指定区域内的所有颜色
long  libop::FindColorEx(long x1, long y1, long x2, long y2, const wchar_t* color, DOUBLE sim, long dir, std::wstring& retstr) {
	wstring str;
	if (_bkproc->check_bind()&& _bkproc->RectConvert(x1, y1, x2, y2)) {
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src)) {
			setlog("error equestCapture");
			return S_OK;
		}
		_image_proc->set_offset(x1, y1);
		_image_proc->FindColoEx(color, sim, dir, str);
	}
	retstr = str;
	return S_OK;
}
//根据指定的多点查找颜色坐标
long  libop::FindMultiColor(long x1, long y1, long x2, long y2, const wchar_t* first_color, const wchar_t* offset_color, DOUBLE sim, long dir, long* x, long* y, long* ret) {
	
	*ret = 0;
	*x = *y = -1;
	
	if (_bkproc->check_bind()&& _bkproc->RectConvert(x1, y1, x2, y2)) {
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src)) {
			setlog("error equestCapture");
			return S_OK;
		}
		_image_proc->set_offset(x1, y1);
		*ret = _image_proc->FindMultiColor(first_color, offset_color, sim, dir, *x, *y);
		/*if (*ret) {
			rx += x1; ry += y1;
			rx -= _bkproc->_pbkdisplay->_client_x;
			ry -= _bkproc->_pbkdisplay->_client_y;
		}*/
	}
	
	return S_OK;
}
//根据指定的多点查找所有颜色坐标
long  libop::FindMultiColorEx(long x1, long y1, long x2, long y2, const wchar_t* first_color, const wchar_t* offset_color, DOUBLE sim, long dir, std::wstring& retstr) {
	wstring str;
	if (_bkproc->check_bind()&& _bkproc->RectConvert(x1, y1, x2, y2)) {
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src)) {
			setlog("error equestCapture");
			return S_OK;
		}
		_image_proc->set_offset(x1, y1);
		_image_proc->FindMultiColorEx(first_color, offset_color, sim, dir, str);
	}
	retstr = str;
	return S_OK;
}
//查找指定区域内的图片
long  libop::FindPic(long x1, long y1, long x2, long y2, const wchar_t* files, const wchar_t* delta_color, DOUBLE sim, long dir, long* x, long* y, long* ret) {
	
	*ret = 0;
	*x = *y = -1;
	
	if (_bkproc->check_bind()&& _bkproc->RectConvert(x1, y1, x2, y2)) {
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src)) {
			setlog("error equestCapture");
			return S_OK;
		}
		_image_proc->set_offset(x1, y1);
		*ret = _image_proc->FindPic(files, delta_color, sim, 0, *x, *y);
		/*if (*ret) {
			rx += x1; ry += y1;
			rx -= _bkproc->_pbkdisplay->_client_x;
			ry -= _bkproc->_pbkdisplay->_client_y;
		}*/
	}
	
	return S_OK;
}
//查找多个图片
long  libop::FindPicEx(long x1, long y1, long x2, long y2, const wchar_t* files, const wchar_t* delta_color, DOUBLE sim, long dir, std::wstring& retstr) {
	
	wstring str;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2)) {
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src)) {
			setlog("error equestCapture");
			return S_OK;
		}
		_image_proc->set_offset(x1, y1);
		_image_proc->FindPicEx(files, delta_color, sim, dir, str);
	}
	retstr = str;
	return 0;
}
//获取(x,y)的颜色
long  libop::GetColor(long x, long y, std::wstring& ret) {
	color_t cr;
	auto tx = x + 1, ty = y + 1;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x, y, tx, ty)) {
		if (!_bkproc->requestCapture(x, y,1, 1, _image_proc->_src)) {
			setlog("error equestCapture");
			return S_OK;
		}
		_image_proc->set_offset(x, y);
		cr = _image_proc->_src.at<color_t>(0, 0);
	}
	
	
	ret = _s2wstring(cr.tostr());

	return S_OK;
}

long libop::SetDisplayInput(const wchar_t* mode, long* ret) {
	*ret=_bkproc->set_display_method(mode);
	return 0;
}

long libop::LoadPic(const wchar_t* file_name, long* ret) {
	*ret = 0;
	return 0;
}

long libop::FreePic(const wchar_t* file_name, long* ret) {
	*ret = 0;
	return 0;
}

long libop::GetScreenData(long x1, long y1, long x2, long y2, void** data, long* ret) {
	*data = nullptr;
	*ret = 0;
	auto& img = _image_proc->_src;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2)) {
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src)) {
			setlog("error equestCapture");
			return S_OK;
		}
		_image_proc->set_offset(x1, y1);
		_screenData.resize(img.size()*4);
		memcpy(_screenData.data(), img.pdata, img.size()*4);
		*data = _screenData.data(); *ret = 1;
	}
	return 0;
}

long libop::GetScreenDataBmp(long x1, long y1, long x2, long y2, void** data, long* size, long* ret) {
	*data = 0;
	*ret = 0;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2)) {
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src)) {
			setlog("error equestCapture");
			return S_OK;
		}
		_image_proc->set_offset(x1, y1);
		auto& img = _image_proc->_src;

		BITMAPFILEHEADER bfh = { 0 };//bmp file header
		BITMAPINFOHEADER bih = { 0 };//bmp info header
		const int szBfh = sizeof(BITMAPFILEHEADER);
		const int szBih = sizeof(BITMAPINFOHEADER);
		bfh.bfOffBits =  szBfh+szBih;
		bfh.bfSize = bfh.bfOffBits + img.width * img.height * 4;
		bfh.bfType = static_cast<WORD>(0x4d42);

		bih.biBitCount = 32;//每个像素字节大小
		bih.biCompression = BI_RGB;
		bih.biHeight = -img.height;//高度
		bih.biPlanes = 1;
		bih.biSize = sizeof(BITMAPINFOHEADER);
		bih.biSizeImage = img.width * 4 * img.height;//图像数据大小
		bih.biWidth = img.width;//宽度

		_screenDataBmp.resize(bfh.bfSize);
	/*	std::ofstream f;
		f.open("xx.bmp",std::ios::binary);
		if (f) {
			f.write((char*)&bfh, sizeof(bfh));
			f.write((char*)&bih, sizeof(bih));
			f.write((char*)img.pdata, img.size() * 4);
		}
		
		f.close();*/
		auto dst = _screenDataBmp.data();
		
		memcpy(dst, &bfh,sizeof(bfh));
		memcpy(dst +sizeof(bfh), &bih, sizeof(bih));
		memcpy(dst + sizeof(bfh)+sizeof(bih), img.pdata, img.size()*4);
		*data = dst;
		*size = bfh.bfSize;
		*ret = 1;
	}
	return 0;
}


long libop::GetScreenFrameInfo(long* frame_id, long* time) {
	FrameInfo info = {};
	if (_bkproc->IsBind()) {
		_bkproc->_pbkdisplay->getFrameInfo(info);
	}
	*frame_id = info.frameId;
	*time = info.time;
	return 0;
}


//设置字库文件
long  libop::SetDict(long idx, const wchar_t* file_name, long* ret) {
	*ret = _image_proc->SetDict(idx, file_name);
	return S_OK;
}
//使用哪个字库文件进行识别
long  libop::UseDict(long idx, long* ret) {
	*ret = _image_proc->UseDict(idx);
	return S_OK;
}
//识别屏幕范围(x1,y1,x2,y2)内符合color_format的字符串,并且相似度为sim,sim取值范围(0.1-1.0),
long  libop::Ocr(long x1, long y1, long x2, long y2, const wchar_t* color, DOUBLE sim, std::wstring& retstr) {
	wstring str;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2)) {
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src)) {
			setlog("error equestCapture");
			return S_OK;
		}
		_image_proc->set_offset(x1, y1);
		_image_proc->OCR(color, sim, str);
	}
	retstr = str;
	return S_OK;
}
//回识别到的字符串，以及每个字符的坐标.
long  libop::OcrEx(long x1, long y1, long x2, long y2, const wchar_t* color, DOUBLE sim, std::wstring& retstr) {
	wstring str;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2)) {
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src)) {
			setlog("error equestCapture");
			return S_OK;
		}
		_image_proc->set_offset(x1, y1);
		_image_proc->OcrEx(color, sim, str);
	}
	retstr = str;
	return S_OK;
}
//在屏幕范围(x1,y1,x2,y2)内,查找string(可以是任意个字符串的组合),并返回符合color_format的坐标位置
long  libop::FindStr(long x1, long y1, long x2, long y2, const wchar_t* strs, const wchar_t* color, DOUBLE sim, long* retx, long* rety,long* ret) {
	wstring str;
	*retx = *rety = -1;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2)) {
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src)) {
			setlog("error equestCapture");
			return S_OK;
		}
		_image_proc->set_offset(x1, y1);
		*ret = _image_proc->FindStr(strs, color, sim, *retx, *rety);
	}

	return S_OK;
}
//返回符合color_format的所有坐标位置
long  libop::FindStrEx(long x1, long y1, long x2, long y2, const wchar_t* strs, const wchar_t* color, DOUBLE sim, std::wstring& retstr) {
	wstring str;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2)) {
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src)) {
			setlog("error equestCapture");
			return S_OK;
		}
		_image_proc->set_offset(x1, y1);
		_image_proc->FindStrEx(strs, color, sim, str);
	}
	retstr = str;
	return S_OK;
}

long  libop::OcrAuto(long x1, long y1, long x2, long y2, DOUBLE sim, std::wstring& retstr) {
	wstring str;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2)) {
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src)) {
			setlog("error equestCapture");
			return S_OK;
		}
		_image_proc->set_offset(x1, y1);
		_image_proc->OcrAuto(sim, str);
	}
	retstr = str;
	return S_OK;
}

//从文件中识别图片
long  libop::OcrFromFile(const wchar_t* file_name, const wchar_t* color_format, DOUBLE sim, std::wstring& retstr) {
	wstring str;
	_image_proc->OcrFromFile(file_name, color_format, sim, str);
	retstr = str;
	return S_OK;
}
//从文件中识别图片,无需指定颜色
long  libop::OcrAutoFromFile(const wchar_t* file_name, DOUBLE sim, std::wstring& retstr){
	wstring str;
	_image_proc->OcrAutoFromFile(file_name, sim, str);
	retstr = str;
	return S_OK;
}

long libop::WriteData(long hwnd, const wchar_t* address, const wchar_t* data, long size, long* ret) {
	*ret = 0;
	MemoryEx mem;
	*ret = mem.WriteData((HWND)hwnd, address, data, size);
	return S_OK;
}
//读取数据
long libop::ReadData(long hwnd, const wchar_t* address, long size, std::wstring& retstr) {
	MemoryEx mem;
	retstr = mem.ReadData((HWND)hwnd, address, size);
	
	return S_OK;
}
