// OpInterface.cpp: OpInterface 的实现

#include "libop.h"
#include "./core/optype.h"
#include "./core/globalVar.h"
#include "./core/helpfunc.h"
#include "./core/opEnv.h"
#include "./winapi/WinApi.h"
#include "./background/opBackground.h"
#include "./ImageProc/ImageProc.h"
#include "./core/Cmder.h"
#include "./winapi/Injecter.h"

#include "./algorithm/AStar.hpp"
#include "./winapi/MemoryEx.h"
#include <fstream>
#include <filesystem>
#include <regex>

#undef FindWindow
#undef FindWindowEx
#undef SetWindowText


const int small_block_size = 10;

libop::libop()
{
	_winapi = new WinApi;
	_bkproc = new opBackground;
	_image_proc = new ImageProc;

	//初始化目录
	wchar_t buff[256];
	::GetCurrentDirectoryW(256, buff);
	_curr_path = buff;
	_image_proc->_curr_path = _curr_path;
	//初始化键码表
	_vkmap[L"back"] = VK_BACK;
	_vkmap[L"ctrl"] = VK_CONTROL;
	_vkmap[L"alt"] = 18;
	_vkmap[L"shift"] = VK_SHIFT;
	_vkmap[L"win"] = VK_LWIN;
	_vkmap[L"space"] = L' ';
	_vkmap[L"tab"] = VK_TAB;
	_vkmap[L"esc"] = VK_CANCEL;
	_vkmap[L"enter"] = L'\r';
	_vkmap[L"up"] = VK_UP;
	_vkmap[L"down"] = VK_DOWN;
	_vkmap[L"left"] = VK_LEFT;
	_vkmap[L"right"] = VK_RIGHT;
	_vkmap[L"f1"] = VK_F1;
	_vkmap[L"f2"] = VK_F2;
	_vkmap[L"f3"] = VK_F3;
	_vkmap[L"f4"] = VK_F4;
	_vkmap[L"f5"] = VK_F5;
	_vkmap[L"f6"] = VK_F6;
	_vkmap[L"f7"] = VK_F7;
	_vkmap[L"f8"] = VK_F8;
	_vkmap[L"f9"] = VK_F9;
	_vkmap[L"f10"] = VK_F10;
	_vkmap[L"f11"] = VK_F11;
	_vkmap[L"f12"] = VK_F12;

	m_opPath = opEnv::getBasePath();
}

libop::~libop()
{
	delete _winapi;
	delete _bkproc;
	delete _image_proc;
}

std::wstring libop::Ver()
{

	//Tool::setlog("address=%d,str=%s", ver, ver);
	return _T(OP_VERSION);
}

void libop::SetPath(const wchar_t *path, long *ret)
{
	wstring fpath = path;
	replacew(fpath, L"/", L"\\");
	if (fpath.find(L'\\') != -1 && ::PathFileExistsW(fpath.data()))
	{
		_curr_path = fpath;
		_image_proc->_curr_path = _curr_path;
		_bkproc->_curr_path = _curr_path;
		*ret = 1;
	}
	else
	{

		if (!fpath.empty() && fpath[0] != L'\\')
			fpath = _curr_path + L'\\' + fpath;
		else
			fpath = _curr_path + fpath;
		if (::PathFileExistsW(fpath.data()))
		{
			_curr_path = path;
			_image_proc->_curr_path = _curr_path;
			_bkproc->_curr_path = _curr_path;
			*ret = 1;
		}
		else
			*ret = 0;
	}
}

void libop::GetPath(std::wstring &path)
{
	path = _curr_path;
}

void libop::GetBasePath(std::wstring &path)
{
	path = opEnv::getBasePath();
}

void libop::GetID(long *ret)
{
	*ret = (long)this;
}

void libop::GetLastError(long *ret)
{
	*ret = ::GetLastError();
}

void libop::SetShowErrorMsg(long show_type, long *ret)
{
	opEnv::m_showErrorMsg = show_type;
	*ret = 1;
}

void libop::Sleep(long millseconds, long *ret)
{
	::Sleep(millseconds);
	*ret = 1;
}

void libop::InjectDll(const wchar_t *process_name, const wchar_t *dll_name, long *ret)
{
	auto proc = _ws2string(process_name);
	auto dll = _ws2string(dll_name);
	long hwnd;
	FindWindowByProcess(process_name, L"", L"", &hwnd);
	long pid;
	GetWindowProcessId(hwnd, &pid);
	*ret = 0;
	if (Injecter::EnablePrivilege(TRUE))
	{
		long error_code = 0;
		*ret = Injecter::InjectDll(pid, dll_name, error_code);
	}
	else
	{
		setlog("EnablePrivilege false erro_code=%08X ", ::GetLastError());
	}
}

void libop::EnablePicCache(long enable, long *ret)
{
	_image_proc->_enable_cache = enable;
	*ret = 1;
}

void libop::CapturePre(const wchar_t *file, LONG *ret)
{
	*ret = _image_proc->Capture(file);
}

void libop::AStarFindPath(long mapWidth, long mapHeight, const wchar_t *disable_points, long beginX, long beginY, long endX, long endY, std::wstring &path)
{
	AStar as;
	using Vec2i = AStar::Vec2i;
	vector<Vec2i> walls;
	vector<wstring> vstr;
	Vec2i tp;
	split(disable_points, vstr, L"|");
	for (auto &it : vstr)
	{
		if (swscanf(it.c_str(), L"%d,%d", &tp.x, &tp.y) != 2)
			break;
		walls.push_back(tp);
	}
	list<Vec2i> paths;

	as.set_map(mapWidth, mapHeight, walls);
	as.findpath(beginX, beginY, endX, endY, paths);
	wstring pathstr;
	wchar_t buf[20];
	for (auto it = paths.rbegin(); it != paths.rend(); ++it)
	{
		auto v = *it;
		wsprintf(buf, L"%d,%d", v.x, v.y);
		pathstr += buf;
		pathstr.push_back(L'|');
	}
	if (!pathstr.empty())
		pathstr.pop_back();
}

void libop::FindNearestPos(const wchar_t *all_pos, long type, long x, long y, std::wstring &ret)
{
	const wchar_t *p = 0;
	wchar_t buf[256] = {0};
	wchar_t rs[256] = {0};
	double old = 1e9;
	long rx = -1, ry = -1;
	std::wstring s = std::regex_replace(all_pos, std::wregex(L","), L" ");
	p = s.data();
	while (*p)
	{
		long x2, y2;
		bool ok = false;
		if (type == 1)
		{

			if (swscanf(p, L"%d %d", &x2, &y2) == 2)
			{
				ok = true;
			}
		}
		else
		{
			if (swscanf(p, L"%s %d %d", buf, &x2, &y2) == 3)
			{
				ok = true;
			}
		}
		if (ok)
		{
			double compareDis = (x - x2) * (x - x2) + (y - y2) * (y - y2);
			if (compareDis < old)
			{
				rx = x2;
				ry = y2;
				old = compareDis;
				wcscpy(rs, buf);
			}
		}
		while (*p && *p != L'|')
			++p;
		if (*p)
			++p;
	}
	if (rs[0])
	{
		wcscpy(buf, rs);
		wsprintf(rs, L"%s,%d,%d", buf, rx, ry);
	}
	else if (type == 1 && rx != -1)
	{
		wsprintf(rs, L"%d,%d", rx, ry);
	}
	ret = rs;
}

void libop::EnumWindow(long parent, const wchar_t *title, const wchar_t *class_name, long filter, std::wstring &retstr)
{
	// TODO: 在此添加实现代码
	std::unique_ptr<wchar_t> retstring(new wchar_t[MAX_PATH * 200]);
	memset(retstring.get(), 0, sizeof(wchar_t) * MAX_PATH * 200);
	_winapi->EnumWindow((HWND)parent, title, class_name, filter, retstring.get());
	//*retstr=_bstr_t(retstring);
	retstr = retstring.get();
}

void libop::EnumWindowByProcess(const wchar_t *process_name, const wchar_t *title, const wchar_t *class_name, long filter, std::wstring &retstring)
{
	// TODO: 在此添加实现代码
	std::unique_ptr<wchar_t> retstr(new wchar_t[MAX_PATH * 200]);
	memset(retstr.get(), 0, sizeof(wchar_t) * MAX_PATH * 200);
	_winapi->EnumWindow((HWND)0, title, class_name, filter, retstr.get(), process_name);
	//*retstring=_bstr_t(retstr);

	retstring = retstr.get();
}

void libop::EnumProcess(const wchar_t *name, std::wstring &retstring)
{
	// TODO: 在此添加实现代码
	std::unique_ptr<wchar_t> retstr(new wchar_t[MAX_PATH * 200]);
	memset(retstr.get(), 0, sizeof(wchar_t) * MAX_PATH * 200);
	_winapi->EnumProcess(name, retstr.get());
	//*retstring=_bstr_t(retstr);
	retstring = retstr.get();
}

void libop::ClientToScreen(long ClientToScreen, long *x, long *y, long *bret)
{
	// TODO: 在此添加实现代码

	*bret = _winapi->ClientToScreen(ClientToScreen, *x, *y);
}

void libop::FindWindow(const wchar_t *class_name, const wchar_t *title, long *rethwnd)
{
	// TODO: 在此添加实现代码
	*rethwnd = _winapi->FindWindow(class_name, title);
}

void libop::FindWindowByProcess(const wchar_t *process_name, const wchar_t *class_name, const wchar_t *title, long *rethwnd)
{
	// TODO: 在此添加实现代码
	_winapi->FindWindowByProcess(class_name, title, *rethwnd, process_name);
}

void libop::FindWindowByProcessId(long process_id, const wchar_t *class_name, const wchar_t *title, long *rethwnd)
{
	// TODO: 在此添加实现代码
	_winapi->FindWindowByProcess(class_name, title, *rethwnd, NULL, process_id);
}

void libop::FindWindowEx(long parent, const wchar_t *class_name, const wchar_t *title, long *rethwnd)
{
	// TODO: 在此添加实现代码
	*rethwnd = _winapi->FindWindowEx(parent, class_name, title);
}

void libop::GetClientRect(long hwnd, long *x1, long *y1, long *x2, long *y2, long *nret)
{
	// TODO: 在此添加实现代码

	*nret = _winapi->GetClientRect(hwnd, *x1, *y1, *x2, *y2);
}

void libop::GetClientSize(long hwnd, long *width, long *height, long *nret)
{
	// TODO: 在此添加实现代码

	*nret = _winapi->GetClientSize(hwnd, *width, *height);
}

void libop::GetForegroundFocus(long *rethwnd)
{
	// TODO: 在此添加实现代码
	*rethwnd = (LONG)::GetFocus();
}

void libop::GetForegroundWindow(long *rethwnd)
{
	// TODO: 在此添加实现代码
	*rethwnd = (LONG)::GetForegroundWindow();
}

void libop::GetMousePointWindow(long *rethwnd)
{
	// TODO: 在此添加实现代码
	//::Sleep(2000);
	_winapi->GetMousePointWindow(*rethwnd);
}

void libop::GetPointWindow(long x, long y, long *rethwnd)
{
	// TODO: 在此添加实现代码
	_winapi->GetMousePointWindow(*rethwnd, x, y);
}

void libop::GetProcessInfo(long pid, std::wstring &retstring)
{
	// TODO: 在此添加实现代码

	wchar_t retstr[MAX_PATH] = {0};
	_winapi->GetProcessInfo(pid, retstr);
	//* retstring=_bstr_t(retstr);

	retstring = retstr;
}

void libop::GetSpecialWindow(long flag, long *rethwnd)
{
	// TODO: 在此添加实现代码
	*rethwnd = 0;
	if (flag == 0)
		*rethwnd = (LONG)GetDesktopWindow();
	else if (flag == 1)
	{
		*rethwnd = (LONG)::FindWindowW(L"Shell_TrayWnd", NULL);
	}
}

void libop::GetWindow(long hwnd, long flag, long *nret)
{
	// TODO: 在此添加实现代码
	_winapi->GetWindow(hwnd, flag, *nret);
}

void libop::GetWindowClass(long hwnd, std::wstring &retstring)
{
	// TODO: 在此添加实现代码
	wchar_t classname[MAX_PATH] = {0};
	::GetClassName((HWND)hwnd, classname, MAX_PATH);
	//* retstring=_bstr_t(classname);

	retstring = classname;
}

void libop::GetWindowProcessId(long hwnd, long *nretpid)
{
	// TODO: 在此添加实现代码
	DWORD pid = 0;
	::GetWindowThreadProcessId((HWND)hwnd, &pid);
	*nretpid = pid;
}

void libop::GetWindowProcessPath(long hwnd, std::wstring &retstring)
{
	// TODO: 在此添加实现代码
	DWORD pid = 0;
	::GetWindowThreadProcessId((HWND)hwnd, &pid);
	wchar_t process_path[MAX_PATH] = {0};
	_winapi->GetProcesspath(pid, process_path);
	//* retstring=_bstr_t(process_path);

	retstring = process_path;
}

void libop::GetWindowRect(long hwnd, long *x1, long *y1, long *x2, long *y2, long *nret)
{
	// TODO: 在此添加实现代码

	RECT winrect;
	*nret = ::GetWindowRect((HWND)hwnd, &winrect);
	*x1 = winrect.left;
	*y1 = winrect.top;
	*x2 = winrect.right;
	*y2 = winrect.bottom;
}

void libop::GetWindowState(long hwnd, long flag, long *rethwnd)
{
	// TODO: 在此添加实现代码
	*rethwnd = _winapi->GetWindowState(hwnd, flag);
}

void libop::GetWindowTitle(long hwnd, std::wstring &rettitle)
{
	// TODO: 在此添加实现代码
	wchar_t title[MAX_PATH] = {0};
	::GetWindowTextW((HWND)hwnd, title, MAX_PATH);
	//* rettitle=_bstr_t(title);

	rettitle = title;
}

void libop::MoveWindow(long hwnd, long x, long y, long *nret)
{
	// TODO: 在此添加实现代码
	RECT winrect;
	::GetWindowRect((HWND)hwnd, &winrect);
	int width = winrect.right - winrect.left;
	int hight = winrect.bottom - winrect.top;
	*nret = ::MoveWindow((HWND)hwnd, x, y, width, hight, false);
}

void libop::ScreenToClient(long hwnd, long *x, long *y, long *nret)
{
	// TODO: 在此添加实现代码

	POINT point;
	*nret = ::ScreenToClient((HWND)hwnd, &point);
	*x = point.x;
	*y = point.y;
}

void libop::SendPaste(long hwnd, long *nret)
{
	// TODO: 在此添加实现代码
	*nret = _winapi->SendPaste(hwnd);
}

void libop::SetClientSize(long hwnd, long width, long hight, long *nret)
{
	// TODO: 在此添加实现代码
	*nret = _winapi->SetWindowSize(hwnd, width, hight);
}

void libop::SetWindowState(long hwnd, long flag, long *nret)
{
	// TODO: 在此添加实现代码
	*nret = _winapi->SetWindowState(hwnd, flag);
}

void libop::SetWindowSize(long hwnd, long width, long height, long *nret)
{
	// TODO: 在此添加实现代码
	*nret = _winapi->SetWindowSize(hwnd, width, height, 1);
}

void libop::SetWindowText(long hwnd, const wchar_t *title, long *nret)
{
	// TODO: 在此添加实现代码
	//*nret=gWindowObj.TSSetWindowState(hwnd,flag);
	*nret = ::SetWindowTextW((HWND)hwnd, title);
}

void libop::SetWindowTransparent(long hwnd, long trans, long *nret)
{
	// TODO: 在此添加实现代码
	*nret = _winapi->SetWindowTransparent(hwnd, trans);
}

void libop::SendString(long hwnd, const wchar_t *str, long *ret)
{
	*ret = _winapi->SendString((HWND)hwnd, str);
}

void libop::SendStringIme(long hwnd, const wchar_t *str, long *ret)
{
	*ret = _winapi->SendStringIme((HWND)hwnd, str);
}

void libop::RunApp(const wchar_t *cmdline, long mode, long *ret)
{
	*ret = _winapi->RunApp(cmdline, mode);
}

void libop::WinExec(const wchar_t *cmdline, long cmdshow, long *ret)
{
	auto str = _ws2string(cmdline);
	*ret = ::WinExec(str.c_str(), cmdshow) > 31 ? 1 : 0;
}

void libop::GetCmdStr(const wchar_t *cmd, long millseconds, std::wstring &retstr)
{
	auto strcmd = _ws2string(cmd);
	Cmder cd;
	auto str = cd.GetCmdStr(strcmd, millseconds <= 0 ? 5 : millseconds);
	retstr = _s2wstring(str);
}

void libop::BindWindow(long hwnd, const wchar_t *display, const wchar_t *mouse, const wchar_t *keypad, long mode, long *ret)
{
	if (_bkproc->IsBind())
		_bkproc->UnBindWindow();
	*ret = _bkproc->BindWindow(hwnd, display, mouse, keypad, mode);
	if (*ret == 1)
	{
		//_image_proc->set_offset(_bkproc->_pbkdisplay->_client_x, _bkproc->_pbkdisplay->_client_y);
	}
}

void libop::UnBindWindow(long *ret)
{
	*ret = _bkproc->UnBindWindow();
}

void libop::GetCursorPos(long *x, long *y, long *ret)
{

	*ret = _bkproc->_bkmouse->GetCursorPos(*x, *y);
}

void libop::MoveR(long x, long y, long *ret)
{
	*ret = _bkproc->_bkmouse->MoveR(x, y);
}
//把鼠标移动到目的点(x,y)
void libop::MoveTo(long x, long y, long *ret)
{
	*ret = _bkproc->_bkmouse->MoveTo(x, y);
}

void libop::MoveToEx(long x, long y, long w, long h, long *ret)
{
	*ret = _bkproc->_bkmouse->MoveToEx(x, y, w, h);
}

void libop::LeftClick(long *ret)
{
	*ret = _bkproc->_bkmouse->LeftClick();
}

void libop::LeftDoubleClick(long *ret)
{
	*ret = _bkproc->_bkmouse->LeftDoubleClick();
}

void libop::LeftDown(long *ret)
{
	*ret = _bkproc->_bkmouse->LeftDown();
}

void libop::LeftUp(long *ret)
{
	*ret = _bkproc->_bkmouse->LeftUp();
}

void libop::MiddleClick(long *ret)
{
	*ret = _bkproc->_bkmouse->MiddleClick();
}

void libop::MiddleDown(long *ret)
{
	*ret = _bkproc->_bkmouse->MiddleDown();
}

void libop::MiddleUp(long *ret)
{
	*ret = _bkproc->_bkmouse->MiddleUp();
}

void libop::RightClick(long *ret)
{
	*ret = _bkproc->_bkmouse->RightClick();
}

void libop::RightDown(long *ret)
{
	*ret = _bkproc->_bkmouse->RightDown();
}

void libop::RightUp(long *ret)
{
	*ret = _bkproc->_bkmouse->RightUp();
}

void libop::WheelDown(long *ret)
{
	*ret = _bkproc->_bkmouse->WheelDown();
}

void libop::WheelUp(long *ret)
{
	*ret = _bkproc->_bkmouse->WheelUp();
}

void libop::GetKeyState(long vk_code, long *ret)
{
	*ret = _bkproc->_keypad->GetKeyState(vk_code);
}

void libop::KeyDown(long vk_code, long *ret)
{
	*ret = _bkproc->_keypad->KeyDown(vk_code);
}

void libop::KeyDownChar(const wchar_t *vk_code, long *ret)
{
	auto nlen = wcslen(vk_code);
	*ret = 0;
	if (nlen > 0)
	{
		wstring s = vk_code;
		wstring2lower(s);
		long vk = _vkmap.count(s) ? _vkmap[s] : vk_code[0];
		*ret = _bkproc->_keypad->KeyDown(vk);
	}
}

void libop::KeyUp(long vk_code, long *ret)
{
	*ret = _bkproc->_keypad->KeyUp(vk_code);
}

void libop::KeyUpChar(const wchar_t *vk_code, long *ret)
{
	auto nlen = wcslen(vk_code);
	*ret = 0;
	if (nlen > 0)
	{
		wstring s = vk_code;
		wstring2lower(s);
		long vk = _vkmap.count(s) ? _vkmap[s] : vk_code[0];
		*ret = _bkproc->_keypad->KeyUp(vk);
	}
}

void libop::WaitKey(long vk_code, long time_out, long *ret)
{
	if (time_out < 0)
		time_out = 0;
	*ret = _bkproc->_keypad->WaitKey(vk_code, time_out);
}

void libop::KeyPress(long vk_code, long *ret)
{

	*ret = _bkproc->_keypad->KeyPress(vk_code);
}

void libop::KeyPressChar(const wchar_t *vk_code, long *ret)
{
	auto nlen = wcslen(vk_code);
	*ret = 0;
	if (nlen > 0)
	{
		//setlog(vk_code);
		wstring s = vk_code;
		wstring2lower(s);
		long vk = _vkmap.count(s) ? _vkmap[s] : vk_code[0];
		*ret = _bkproc->_keypad->KeyPress(vk);
	}
}

//抓取指定区域(x1, y1, x2, y2)的图像, 保存为file
void libop::Capture(long x1, long y1, long x2, long y2, const wchar_t *file_name, long *ret)
{

	*ret = 0;

	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2))
	{
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src))
		{
			setlog("error requestCapture");
		}
		else
		{
			_image_proc->set_offset(x1, y1);

			*ret = _image_proc->Capture(file_name);
		}
	}
}
//比较指定坐标点(x,y)的颜色
void libop::CmpColor(long x, long y, const wchar_t *color, DOUBLE sim, long *ret)
{
	//LONG rx = -1, ry = -1;
	long tx = x + small_block_size, ty = y + small_block_size;
	*ret = 0;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x, y, tx, ty))
	{
		if (!_bkproc->requestCapture(x, y, small_block_size, small_block_size, _image_proc->_src))
		{
			setlog("error requestCapture");
		}
		else
		{
			_image_proc->set_offset(x, y);
			*ret = _image_proc->CmpColor(x, y, color, sim);
		}
	}
}
//查找指定区域内的颜色
void libop::FindColor(long x1, long y1, long x2, long y2, const wchar_t *color, DOUBLE sim, long dir, long *x, long *y, long *ret)
{

	*ret = 0;
	*x = *y = -1;

	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2))
	{
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src))
		{
			setlog("error requestCapture");
		}
		else
		{
			_image_proc->set_offset(x1, y1);
			*ret = _image_proc->FindColor(color, sim, dir, *x, *y);
		}
	}
}
//查找指定区域内的所有颜色
void libop::FindColorEx(long x1, long y1, long x2, long y2, const wchar_t *color, DOUBLE sim, long dir, std::wstring &retstr)
{
	//wstring str;
	retstr.clear();
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2))
	{
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src))
		{
			setlog("error requestCapture");
		}
		else
		{
			_image_proc->set_offset(x1, y1);
			_image_proc->FindColoEx(color, sim, dir, retstr);
		}
	}
	
}
//根据指定的多点查找颜色坐标
void libop::FindMultiColor(long x1, long y1, long x2, long y2, const wchar_t *first_color, const wchar_t *offset_color, DOUBLE sim, long dir, long *x, long *y, long *ret)
{

	*ret = 0;
	*x = *y = -1;

	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2))
	{
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src))
		{
			setlog("error requestCapture");
		}
		else
		{
			_image_proc->set_offset(x1, y1);
			*ret = _image_proc->FindMultiColor(first_color, offset_color, sim, dir, *x, *y);
		}

		/*if (*ret) {
			rx += x1; ry += y1;
			rx -= _bkproc->_pbkdisplay->_client_x;
			ry -= _bkproc->_pbkdisplay->_client_y;
		}*/
	}
}
//根据指定的多点查找所有颜色坐标
void libop::FindMultiColorEx(long x1, long y1, long x2, long y2, const wchar_t *first_color, const wchar_t *offset_color, DOUBLE sim, long dir, std::wstring &retstr)
{
	retstr.clear();
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2))
	{
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src))
		{
			setlog("error requestCapture");
		}
		else
		{
			_image_proc->set_offset(x1, y1);
			_image_proc->FindMultiColorEx(first_color, offset_color, sim, dir, retstr);
		}
	}
	//retstr = str;
}
//查找指定区域内的图片
void libop::FindPic(long x1, long y1, long x2, long y2, const wchar_t *files, const wchar_t *delta_color, DOUBLE sim, long dir, long *x, long *y, long *ret)
{

	*ret = 0;
	*x = *y = -1;

	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2))
	{
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src))
		{
			setlog("error requestCapture");
		}
		else
		{
			_image_proc->set_offset(x1, y1);
			*ret = _image_proc->FindPic(files, delta_color, sim, 0, *x, *y);
		}

		/*if (*ret) {
			rx += x1; ry += y1;
			rx -= _bkproc->_pbkdisplay->_client_x;
			ry -= _bkproc->_pbkdisplay->_client_y;
		}*/
	}
}
//查找多个图片
void libop::FindPicEx(long x1, long y1, long x2, long y2, const wchar_t *files, const wchar_t *delta_color, DOUBLE sim, long dir, std::wstring &retstr)
{

	//wstring str;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2))
	{
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src))
		{
			setlog("error requestCapture");
		}
		else
		{
			_image_proc->set_offset(x1, y1);
			_image_proc->FindPicEx(files, delta_color, sim, dir, retstr);
		}
	}
	//retstr = str;
}

void libop::FindPicExS(long x1, long y1, long x2, long y2, const wchar_t *files, const wchar_t *delta_color, double sim, long dir, std::wstring &retstr)
{
	//wstring str;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2))
	{
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src))
		{
			setlog("error requestCapture");
		}
		else
		{
			_image_proc->set_offset(x1, y1);
			_image_proc->FindPicEx(files, delta_color, sim, dir, retstr, false);
		}
	}
	//retstr = str;
}

void libop::FindColorBlock(long x1, long y1, long x2, long y2, const wchar_t *color, double sim, long count, long height, long width, long *x, long *y, long *ret)
{
	*ret = 0;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2))
	{
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src))
		{
			setlog("error requestCapture");
		}
		else
		{
			_image_proc->set_offset(x1, y1);
			*ret = _image_proc->FindColorBlock(color, sim, count, height, width, *x, *y);
		}
	}
}

void libop::FindColorBlockEx(long x1, long y1, long x2, long y2, const wchar_t *color, double sim, long count, long height, long width, std::wstring &retstr)
{

	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2))
	{
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src))
		{
			setlog("error requestCapture");
		}
		else
		{
			_image_proc->set_offset(x1, y1);
			_image_proc->FindColorBlockEx(color, sim, count, height, width, retstr);
		}
	}
}

//获取(x,y)的颜色
void libop::GetColor(long x, long y, std::wstring &ret)
{
	color_t cr;
	auto tx = x + small_block_size, ty = y + small_block_size;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x, y, tx, ty))
	{
		if (_bkproc->requestCapture(x, y, small_block_size, small_block_size, _image_proc->_src))
		{
			_image_proc->set_offset(x, y);
			cr = _image_proc->_src.at<color_t>(0, 0);
		}
		else
		{
			setlog("error requestCapture");
		}
	}
	else
	{
		//setlog("")
	}

	ret = cr.towstr();
}

void libop::SetDisplayInput(const wchar_t *mode, long *ret)
{
	*ret = _bkproc->set_display_method(mode);
}

void libop::LoadPic(const wchar_t *file_name, long *ret)
{
	*ret = _image_proc->LoadPic(file_name);
}

void libop::FreePic(const wchar_t *file_name, long *ret)
{
	*ret = _image_proc->FreePic(file_name);
}

void libop::LoadMemPic(const wchar_t *file_name, void *data, long size, long *ret)
{
	*ret = _image_proc->LoadMemPic(file_name, data, size);
}

void libop::GetScreenData(long x1, long y1, long x2, long y2, void **data, long *ret)
{
	*data = nullptr;
	*ret = 0;
	auto &img = _image_proc->_src;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2))
	{
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src))
		{
			setlog("error requestCapture");
		}
		else
		{
			_image_proc->set_offset(x1, y1);
			_screenData.resize(img.size() * 4);
			//memcpy(_screenData.data(), img.pdata, img.size()*4);
			for (int i = 0; i < img.height; i++)
			{
				memcpy(_screenData.data() + i * img.width * 4, img.ptr<char>(img.height - 1 - i), img.width * 4);
			}
			*data = _screenData.data();
			*ret = 1;
		}
	}
}

void libop::GetScreenDataBmp(long x1, long y1, long x2, long y2, void **data, long *size, long *ret)
{
	*data = 0;
	*ret = 0;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2))
	{
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src))
		{
			setlog("rerror requestCapture");
		}
		else
		{
			_image_proc->set_offset(x1, y1);
			auto &img = _image_proc->_src;

			BITMAPFILEHEADER bfh = {0}; //bmp file header
			BITMAPINFOHEADER bih = {0}; //bmp info header
			const int szBfh = sizeof(BITMAPFILEHEADER);
			const int szBih = sizeof(BITMAPINFOHEADER);
			bfh.bfOffBits = szBfh + szBih;
			bfh.bfSize = bfh.bfOffBits + img.width * img.height * 4;
			bfh.bfType = static_cast<WORD>(0x4d42);

			bih.biBitCount = 32; //每个像素字节大小
			bih.biCompression = BI_RGB;
			//bih.biHeight = -img.height;//高度 反
			bih.biHeight = img.height; //高度
			bih.biPlanes = 1;
			bih.biSize = sizeof(BITMAPINFOHEADER);
			bih.biSizeImage = img.width * 4 * img.height; //图像数据大小
			bih.biWidth = img.width;					  //宽度

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

			memcpy(dst, &bfh, sizeof(bfh));
			memcpy(dst + sizeof(bfh), &bih, sizeof(bih));
			dst += sizeof(bfh) + sizeof(bih);
			for (int i = 0; i < img.height; i++)
			{
				memcpy(dst + i * img.width * 4, img.ptr<char>(img.height - 1 - i), img.width * 4);
			}
			//memcpy(dst + sizeof(bfh)+sizeof(bih), img.pdata, img.size()*4);
			*data = _screenDataBmp.data();
			*size = bfh.bfSize;
			*ret = 1;
		}
	}
}

void libop::GetScreenFrameInfo(long *frame_id, long *time)
{
	FrameInfo info = {};
	if (_bkproc->IsBind())
	{
		_bkproc->_pbkdisplay->getFrameInfo(info);
	}
	*frame_id = info.frameId;
	*time = info.time;
}

void libop::MatchPicName(const wchar_t *pic_name, std::wstring &retstr)
{
	retstr.clear();
	std::wstring s(pic_name);
	if (s.find(L'/') != s.npos || s.find(L'\\') != s.npos)
	{
		setlog("invalid pic_name");
	}

	s = std::regex_replace(s, std::wregex(L"(\\.|\\(|\\)|\\[|\\]|\\{|\\})"), L"\\$1");
	/*s = std::regex_replace(s, std::wregex(L"\\("), L"\\(");
	s = std::regex_replace(s, std::wregex(L"\\)"), L"\\)");
	s = std::regex_replace(s, std::wregex(L"\\["), L"\\[");
	s = std::regex_replace(s, std::wregex(L"\\]"), L"\\]");*/
	s = std::regex_replace(s, std::wregex(L"\\*"), L".*?");
	s = std::regex_replace(s, std::wregex(L"\\?"), L".?");

	//setlog(s.data());
	namespace fs = std::filesystem;
	fs::path path(_curr_path);
	if (fs::exists(path))
	{
		fs::directory_iterator iter(path);
		std::wstring tmp;
		std::wregex e(s);
		for (auto &it : iter)
		{
			if (it.status().type() == fs::file_type::regular)
			{
				tmp = it.path().filename();
				try
				{
					if (std::regex_match(tmp, e))
					{
						retstr += tmp;
						retstr += L"|";
					}
				}
				catch (...)
				{
					setlog("exception!");
				}
			}
		}
		if (!retstr.empty() && retstr.back() == L'|')
			retstr.pop_back();
	}
}

//设置字库文件
void libop::SetDict(long idx, const wchar_t *file_name, long *ret)
{
	*ret = _image_proc->SetDict(idx, file_name);
}

//设置内存字库文件
void libop::SetMemDict(long idx, const wchar_t *data, long size, long *ret)
{
	*ret = _image_proc->SetMemDict(idx, (void *)data, size);
}

//使用哪个字库文件进行识别
void libop::UseDict(long idx, long *ret)
{
	*ret = _image_proc->UseDict(idx);
}
//识别屏幕范围(x1,y1,x2,y2)内符合color_format的字符串,并且相似度为sim,sim取值范围(0.1-1.0),
void libop::Ocr(long x1, long y1, long x2, long y2, const wchar_t *color, DOUBLE sim, std::wstring &retstr)
{
	wstring str;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2))
	{
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src))
		{
			setlog("error requestCapture");
		}
		else
		{
			_image_proc->set_offset(x1, y1);
			_image_proc->OCR(color, sim, str);
		}
	}
	retstr = str;
}
//回识别到的字符串，以及每个字符的坐标.
void libop::OcrEx(long x1, long y1, long x2, long y2, const wchar_t *color, DOUBLE sim, std::wstring &retstr)
{
	wstring str;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2))
	{
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src))
		{
			setlog("error requestCapture");
		}
		else
		{
			_image_proc->set_offset(x1, y1);
			_image_proc->OcrEx(color, sim, str);
		}
	}
	retstr = str;
}
//在屏幕范围(x1,y1,x2,y2)内,查找string(可以是任意个字符串的组合),并返回符合color_format的坐标位置
void libop::FindStr(long x1, long y1, long x2, long y2, const wchar_t *strs, const wchar_t *color, DOUBLE sim, long *retx, long *rety, long *ret)
{
	wstring str;
	*retx = *rety = -1;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2))
	{
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src))
		{
			setlog("error requestCapture");
		}
		else
		{
			_image_proc->set_offset(x1, y1);
			*ret = _image_proc->FindStr(strs, color, sim, *retx, *rety);
		}
	}
}
//返回符合color_format的所有坐标位置
void libop::FindStrEx(long x1, long y1, long x2, long y2, const wchar_t *strs, const wchar_t *color, DOUBLE sim, std::wstring &retstr)
{
	wstring str;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2))
	{
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src))
		{
			setlog("error requestCapture");
		}
		else
		{
			_image_proc->set_offset(x1, y1);
			_image_proc->FindStrEx(strs, color, sim, str);
		}
	}
	retstr = str;
}

void libop::OcrAuto(long x1, long y1, long x2, long y2, DOUBLE sim, std::wstring &retstr)
{
	wstring str;
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2))
	{
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src))
		{
			setlog("error requestCapture");
		}
		else
		{
			_image_proc->set_offset(x1, y1);
			_image_proc->OcrAuto(sim, str);
		}
	}
	retstr = str;
}

//从文件中识别图片
void libop::OcrFromFile(const wchar_t *file_name, const wchar_t *color_format, DOUBLE sim, std::wstring &retstr)
{
	wstring str;
	_image_proc->OcrFromFile(file_name, color_format, sim, str);
	retstr = str;
}
//从文件中识别图片,无需指定颜色
void libop::OcrAutoFromFile(const wchar_t *file_name, DOUBLE sim, std::wstring &retstr)
{
	wstring str;
	_image_proc->OcrAutoFromFile(file_name, sim, str);
	retstr = str;
}

void libop::FindLine(long x1, long y1, long x2, long y2, const wchar_t *color, double sim, wstring &retstr)
{
	if (_bkproc->check_bind() && _bkproc->RectConvert(x1, y1, x2, y2))
	{
		if (!_bkproc->requestCapture(x1, y1, x2 - x1, y2 - y1, _image_proc->_src))
		{
			setlog("error requestCapture");
		}
		else
		{
			_image_proc->set_offset(x1, y1);
			_image_proc->FindLine(color, sim, retstr);
		}
	}
}

void libop::WriteData(long hwnd, const wchar_t *address, const wchar_t *data, long size, long *ret)
{
	*ret = 0;
	MemoryEx mem;
	*ret = mem.WriteData((HWND)hwnd, address, data, size);
}
//读取数据
void libop::ReadData(long hwnd, const wchar_t *address, long size, std::wstring &retstr)
{
	MemoryEx mem;
	retstr = mem.ReadData((HWND)hwnd, address, size);
}
