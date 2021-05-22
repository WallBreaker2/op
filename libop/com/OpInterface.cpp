// OpInterface.cpp: OpInterface 的实现

#include "stdafx.h"
#include "OpInterface.h"

#include <filesystem>
#include <string>
// OpInterface
using std::wstring;

OpInterface::OpInterface() 
{
}

STDMETHODIMP OpInterface::Ver(BSTR *ret)
{

	//Tool::setlog("address=%d,str=%s", ver, ver);
	wstring s = obj.Ver();
	CComBSTR newstr;
	newstr.Append(s.data());
	newstr.CopyTo(ret);

	return S_OK;
}

STDMETHODIMP OpInterface::SetPath(BSTR path, LONG *ret)
{

	obj.SetPath(path, ret);
	return S_OK;
}

STDMETHODIMP OpInterface::GetPath(BSTR *path)
{
	wstring s;
	obj.GetPath(s);

	CComBSTR newstr;
	newstr.Append(s.data());
	newstr.CopyTo(path);
	return S_OK;
}

STDMETHODIMP OpInterface::GetBasePath(BSTR *path)
{

	wstring s;
	obj.GetBasePath(s);

	CComBSTR newstr;
	newstr.Append(s.data());
	newstr.CopyTo(path);
	return S_OK;
}

STDMETHODIMP OpInterface::GetID(LONG *ret)
{
	obj.GetID(ret);
	return S_OK;
}

STDMETHODIMP::OpInterface::GetLastError(LONG *ret)
{
	obj.GetLastError(ret);
	return S_OK;
}

STDMETHODIMP OpInterface::SetShowErrorMsg(LONG show_type, LONG *ret)
{
	obj.SetShowErrorMsg(show_type, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::Sleep(LONG millseconds, LONG *ret)
{

	obj.Sleep(millseconds, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::InjectDll(BSTR process_name, BSTR dll_name, LONG *ret)
{
	//auto proc = _wsto_string(process_name);
	//auto dll = _wsto_string(dll_name);
	//Injecter::EnablePrivilege(TRUE);
	//auto h = Injecter::InjectDll(process_name, dll_name);
	obj.InjectDll(process_name, dll_name, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::EnablePicCache(LONG enable, LONG *ret)
{

	obj.EnablePicCache(enable, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::CapturePre(BSTR file, LONG *ret)
{

	obj.CapturePre(file, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::AStarFindPath(LONG mapWidth, LONG mapHeight, BSTR disable_points, LONG beginX, LONG beginY, LONG endX, LONG endY, BSTR *path)
{
	wstring s;
	obj.AStarFindPath(mapWidth,
					  mapHeight,
					  disable_points,
					  beginX,
					  beginY,
					  endX,
					  endY,
					  s);

	CComBSTR newstr;
	newstr.Append(s.data());
	newstr.CopyTo(path);
	return S_OK;
}

//根据部分Ex接口的返回值，然后在所有坐标里找出距离指定坐标最近的那个坐标.
STDMETHODIMP OpInterface::FindNearestPos(BSTR all_pos, LONG type, LONG x, LONG y, BSTR *retstr)
{
	std::wstring s;
	obj.FindNearestPos(all_pos, type, x, y, s);
	CComBSTR newbstr;
	auto hr = newbstr.Append(s.data());
	hr = newbstr.CopyTo(retstr);
	return S_OK;
}

STDMETHODIMP OpInterface::EnumWindow(LONG parent, BSTR title, BSTR class_name, LONG filter, BSTR *retstr)
{
	wstring s;
	obj.EnumWindow(parent, title, class_name, filter, s);

	CComBSTR newbstr;
	auto hr = newbstr.Append(s.data());
	hr = newbstr.CopyTo(retstr);
	return hr;
}

STDMETHODIMP OpInterface::EnumWindowByProcess(BSTR process_name, BSTR title, BSTR class_name, LONG filter, BSTR *retstring)
{
	wstring s;
	obj.EnumWindowByProcess(process_name, title, class_name, filter, s);

	CComBSTR newbstr;
	newbstr.Append(s.data());
	newbstr.CopyTo(retstring);
	return S_OK;
}

STDMETHODIMP OpInterface::EnumProcess(BSTR name, BSTR *retstring)
{
	wstring s;
	obj.EnumProcess(name, s);

	CComBSTR newbstr;
	newbstr.Append(s.data());
	newbstr.CopyTo(retstring);
	return S_OK;
}

STDMETHODIMP OpInterface::ClientToScreen(LONG ClientToScreen, VARIANT *x, VARIANT *y, LONG *bret)
{
	// TODO: 在此添加实现代码
	x->vt = VT_I4;
	y->vt = VT_I4;
	long lx, ly;
	obj.ClientToScreen(ClientToScreen, &lx, &ly, bret);
	x->lVal = lx;
	y->lVal = ly;
	return S_OK;
}

STDMETHODIMP OpInterface::FindWindow(BSTR class_name, BSTR title, LONG *rethwnd)
{
	// TODO: 在此添加实现代码
	obj.FindWindow(class_name, title, rethwnd);

	return S_OK;
}

STDMETHODIMP OpInterface::FindWindowByProcess(BSTR process_name, BSTR class_name, BSTR title, LONG *rethwnd)
{
	// TODO: 在此添加实现代码
	obj.FindWindowByProcess(process_name, class_name, title, rethwnd);

	return S_OK;
}

STDMETHODIMP OpInterface::FindWindowByProcessId(LONG process_id, BSTR class_name, BSTR title, LONG *rethwnd)
{
	// TODO: 在此添加实现代码
	obj.FindWindowByProcessId(process_id, class_name, title, rethwnd);

	return S_OK;
}

STDMETHODIMP OpInterface::FindWindowEx(LONG parent, BSTR class_name, BSTR title, LONG *rethwnd)
{
	// TODO: 在此添加实现代码
	obj.FindWindowEx(parent, class_name, title, rethwnd);

	return S_OK;
}

STDMETHODIMP OpInterface::GetClientRect(LONG hwnd, VARIANT *x1, VARIANT *y1, VARIANT *x2, VARIANT *y2, LONG *nret)
{
	// TODO: 在此添加实现代码
	x1->vt = VT_I4;
	y1->vt = VT_I4;
	x2->vt = VT_I4;
	y2->vt = VT_I4;
	obj.GetClientRect(hwnd, &x1->lVal, &y1->lVal, &x2->lVal, &y2->lVal, nret);

	return S_OK;
}

STDMETHODIMP OpInterface::GetClientSize(LONG hwnd, VARIANT *width, VARIANT *height, LONG *nret)
{
	// TODO: 在此添加实现代码
	width->vt = VT_I4;
	height->vt = VT_I4;
	obj.GetClientSize(hwnd, &width->lVal, &height->lVal, nret);

	return S_OK;
}

STDMETHODIMP OpInterface::GetForegroundFocus(LONG *rethwnd)
{
	// TODO: 在此添加实现代码
	obj.GetForegroundFocus(rethwnd);

	return S_OK;
}

STDMETHODIMP OpInterface::GetForegroundWindow(LONG *rethwnd)
{
	// TODO: 在此添加实现代码
	obj.GetForegroundWindow(rethwnd);

	return S_OK;
}

STDMETHODIMP OpInterface::GetMousePointWindow(LONG *rethwnd)
{
	// TODO: 在此添加实现代码
	//::Sleep(2000);
	obj.GetMousePointWindow(rethwnd);

	return S_OK;
}

STDMETHODIMP OpInterface::GetPointWindow(LONG x, LONG y, LONG *rethwnd)
{
	// TODO: 在此添加实现代码
	obj.GetPointWindow(x, y, rethwnd);

	return S_OK;
}

STDMETHODIMP OpInterface::GetProcessInfo(LONG pid, BSTR *retstring)
{
	// TODO: 在此添加实现代码
	wstring s;
	obj.GetProcessInfo(pid, s);

	CComBSTR newbstr;
	newbstr.Append(s.data());
	newbstr.CopyTo(retstring);
	return S_OK;
}

STDMETHODIMP OpInterface::GetSpecialWindow(LONG flag, LONG *rethwnd)
{
	// TODO: 在此添加实现代码
	obj.GetSpecialWindow(flag, rethwnd);

	return S_OK;
}

STDMETHODIMP OpInterface::GetWindow(LONG hwnd, LONG flag, LONG *nret)
{
	// TODO: 在此添加实现代码
	obj.GetWindow(hwnd, flag, nret);

	return S_OK;
}

STDMETHODIMP OpInterface::GetWindowClass(LONG hwnd, BSTR *retstring)
{
	// TODO: 在此添加实现代码
	wstring s;

	CComBSTR newbstr;
	newbstr.Append(s.data());

	newbstr.CopyTo(retstring);
	return S_OK;
}

STDMETHODIMP OpInterface::GetWindowProcessId(LONG hwnd, LONG *nretpid)
{
	// TODO: 在此添加实现代码
	obj.GetWindowProcessId(hwnd, nretpid);

	return S_OK;
}

STDMETHODIMP OpInterface::GetWindowProcessPath(LONG hwnd, BSTR *retstring)
{
	// TODO: 在此添加实现代码
	wstring s;
	obj.GetWindowProcessPath(hwnd, s);

	CComBSTR newbstr;
	newbstr.Append(s.data());
	newbstr.CopyTo(retstring);
	return S_OK;
}

STDMETHODIMP OpInterface::GetWindowRect(LONG hwnd, VARIANT *x1, VARIANT *y1, VARIANT *x2, VARIANT *y2, LONG *nret)
{
	// TODO: 在此添加实现代码
	x1->vt = VT_I4;
	x2->vt = VT_I4;
	y1->vt = VT_I4;
	y2->vt = VT_I4;

	obj.GetWindowRect(hwnd, &x1->lVal, &y1->lVal, &x2->lVal, &y2->lVal, nret);

	return S_OK;
}

STDMETHODIMP OpInterface::GetWindowState(LONG hwnd, LONG flag, LONG *rethwnd)
{
	// TODO: 在此添加实现代码
	obj.GetWindowState(hwnd, flag, rethwnd);

	return S_OK;
}

STDMETHODIMP OpInterface::GetWindowTitle(LONG hwnd, BSTR *rettitle)
{
	wstring s;
	obj.GetWindowTitle(hwnd, s);

	CComBSTR newbstr;
	newbstr.Append(s.data());
	newbstr.CopyTo(rettitle);
	return S_OK;
}

STDMETHODIMP OpInterface::MoveWindow(LONG hwnd, LONG x, LONG y, LONG *nret)
{
	// TODO: 在此添加实现代码
	obj.MoveWindow(hwnd, x, y, nret);

	return S_OK;
}

STDMETHODIMP OpInterface::ScreenToClient(LONG hwnd, VARIANT *x, VARIANT *y, LONG *nret)
{
	// TODO: 在此添加实现代码
	x->vt = VT_I4;
	y->vt = VT_I4;
	obj.ScreenToClient(hwnd, &x->lVal, &y->lVal, nret);

	return S_OK;
}

STDMETHODIMP OpInterface::SendPaste(LONG hwnd, LONG *nret)
{
	// TODO: 在此添加实现代码
	obj.SendPaste(hwnd, nret);

	return S_OK;
}

STDMETHODIMP OpInterface::SetClientSize(LONG hwnd, LONG width, LONG hight, LONG *nret)
{
	// TODO: 在此添加实现代码
	obj.SetClientSize(hwnd, width, hight, nret);

	return S_OK;
}

STDMETHODIMP OpInterface::SetWindowState(LONG hwnd, LONG flag, LONG *nret)
{
	// TODO: 在此添加实现代码
	obj.SetWindowState(hwnd, flag, nret);

	return S_OK;
}

STDMETHODIMP OpInterface::SetWindowSize(LONG hwnd, LONG width, LONG height, LONG *nret)
{
	// TODO: 在此添加实现代码
	obj.SetWindowSize(hwnd, width, height, nret);

	return S_OK;
}

STDMETHODIMP OpInterface::SetWindowText(LONG hwnd, BSTR title, LONG *nret)
{
	// TODO: 在此添加实现代码
	//*nret=gWindowObj.TSSetWindowState(hwnd,flag);
	obj.SetWindowText(hwnd, title, nret);

	return S_OK;
}

STDMETHODIMP OpInterface::SetWindowTransparent(LONG hwnd, LONG trans, LONG *nret)
{
	// TODO: 在此添加实现代码
	obj.SetWindowTransparent(hwnd, trans, nret);

	return S_OK;
}

STDMETHODIMP OpInterface::SendString(LONG hwnd, BSTR str, LONG *ret)
{
	obj.SendString(hwnd, str, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::SendStringIme(LONG hwnd, BSTR str, LONG *ret)
{
	obj.SendStringIme(hwnd, str, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::RunApp(BSTR cmdline, LONG mode, LONG *ret)
{
	obj.RunApp(cmdline, mode, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::WinExec(BSTR cmdline, LONG cmdshow, LONG *ret)
{
	obj.WinExec(cmdline, cmdshow, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::GetCmdStr(BSTR cmd, LONG millseconds, BSTR *retstr)
{
	wstring s;
	obj.GetCmdStr(cmd, millseconds, s);

	CComBSTR newstr;

	auto hr = newstr.Append(s.data());
	hr = newstr.CopyTo(retstr);
	return hr;
}

STDMETHODIMP OpInterface::BindWindow(LONG hwnd, BSTR display, BSTR mouse, BSTR keypad, LONG mode, LONG *ret)
{
	obj.BindWindow(hwnd,
				   display,
				   mouse,
				   keypad,
				   mode,
				   ret);
	return S_OK;
}

STDMETHODIMP OpInterface::UnBindWindow(LONG *ret)
{
	obj.UnBindWindow(ret);

	return S_OK;
}

STDMETHODIMP OpInterface::GetCursorPos(VARIANT *x, VARIANT *y, LONG *ret)
{
	x->vt = y->vt = VT_I4;
	obj.GetCursorPos(&x->lVal, &y->lVal, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::MoveR(LONG x, LONG y, LONG *ret)
{
	obj.MoveR(x, y, ret);

	return S_OK;
}
//把鼠标移动到目的点(x,y)
STDMETHODIMP OpInterface::MoveTo(LONG x, LONG y, LONG *ret)
{
	obj.MoveTo(x, y, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::MoveToEx(LONG x, LONG y, LONG w, LONG h, LONG *ret)
{
	obj.MoveToEx(x, y, w, h, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::LeftClick(LONG *ret)
{
	obj.LeftClick(ret);

	return S_OK;
}

STDMETHODIMP OpInterface::LeftDoubleClick(LONG *ret)
{
	obj.LeftDoubleClick(ret);

	return S_OK;
}

STDMETHODIMP OpInterface::LeftDown(LONG *ret)
{
	obj.LeftDown(ret);

	return S_OK;
}

STDMETHODIMP OpInterface::LeftUp(LONG *ret)
{
	obj.LeftUp(ret);

	return S_OK;
}

STDMETHODIMP OpInterface::MiddleClick(LONG *ret)
{
	obj.MiddleClick(ret);

	return S_OK;
}

STDMETHODIMP OpInterface::MiddleDown(LONG *ret)
{
	obj.MiddleDown(ret);

	return S_OK;
}

STDMETHODIMP OpInterface::MiddleUp(LONG *ret)
{
	obj.MiddleUp(ret);

	return S_OK;
}

STDMETHODIMP OpInterface::RightClick(LONG *ret)
{
	obj.RightClick(ret);

	return S_OK;
}

STDMETHODIMP OpInterface::RightDown(LONG *ret)
{
	obj.RightDown(ret);

	return S_OK;
}

STDMETHODIMP OpInterface::RightUp(LONG *ret)
{
	obj.RightUp(ret);

	return S_OK;
}

STDMETHODIMP OpInterface::WheelDown(LONG *ret)
{
	obj.WheelDown(ret);

	return S_OK;
}

STDMETHODIMP OpInterface::WheelUp(LONG *ret)
{
	obj.WheelUp(ret);

	return S_OK;
}

STDMETHODIMP OpInterface::GetKeyState(LONG vk_code, LONG *ret)
{
	obj.GetKeyState(vk_code, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::KeyDown(LONG vk_code, LONG *ret)
{
	obj.KeyDown(vk_code, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::KeyDownChar(BSTR vk_code, LONG *ret)
{
	obj.KeyDownChar(vk_code, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::KeyUp(LONG vk_code, LONG *ret)
{
	obj.KeyUp(vk_code, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::KeyUpChar(BSTR vk_code, LONG *ret)
{
	obj.KeyUpChar(vk_code, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::WaitKey(LONG vk_code, LONG time_out, LONG *ret)
{
	obj.WaitKey(vk_code, time_out, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::KeyPress(LONG vk_code, LONG *ret)
{

	obj.KeyPress(vk_code, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::KeyPressChar(BSTR vk_code, LONG *ret)
{
	obj.KeyPressChar(vk_code, ret);

	return S_OK;
}

//抓取指定区域(x1, y1, x2, y2)的图像, 保存为file
STDMETHODIMP OpInterface::Capture(LONG x1, LONG y1, LONG x2, LONG y2, BSTR file_name, LONG *ret)
{

	obj.Capture(x1, y1, x2, y2, file_name, ret);

	return S_OK;
}
//比较指定坐标点(x,y)的颜色
STDMETHODIMP OpInterface::CmpColor(LONG x, LONG y, BSTR color, DOUBLE sim, LONG *ret)
{
	obj.CmpColor(x, y, color, sim, ret);

	return S_OK;
}
//查找指定区域内的颜色
STDMETHODIMP OpInterface::FindColor(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG dir, VARIANT *x, VARIANT *y, LONG *ret)
{

	x->vt = y->vt = VT_I4;

	obj.FindColor(x1, y1, x2, y2, color, sim, dir, &x->lVal, &y->lVal, ret);

	return S_OK;
}
//查找指定区域内的所有颜色
STDMETHODIMP OpInterface::FindColorEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG dir, BSTR *retstr)
{
	wstring s;
	obj.FindColorEx(x1, y1, x2, y2, color, sim, dir, s);

	CComBSTR newstr;
	newstr.Append(s.data());
	newstr.CopyTo(retstr);
	return S_OK;
}
//根据指定的多点查找颜色坐标
STDMETHODIMP OpInterface::FindMultiColor(LONG x1, LONG y1, LONG x2, LONG y2, BSTR first_color, BSTR offset_color, DOUBLE sim, LONG dir, VARIANT *x, VARIANT *y, LONG *ret)
{
	LONG rx = -1, ry = -1;
	*ret = 0;
	x->vt = y->vt = VT_I4;
	obj.FindMultiColor(x1, y1, x2, y2, first_color, offset_color, sim, dir, &x->lVal, &y->lVal, ret);

	return S_OK;
}
//根据指定的多点查找所有颜色坐标
STDMETHODIMP OpInterface::FindMultiColorEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR first_color, BSTR offset_color, DOUBLE sim, LONG dir, BSTR *retstr)
{
	wstring s;
	obj.FindMultiColorEx(x1, y1, x2, y2, first_color, offset_color, sim, dir, s);

	CComBSTR newstr;
	newstr.Append(s.data());
	newstr.CopyTo(retstr);
	return S_OK;
}
//查找指定区域内的图片
STDMETHODIMP OpInterface::FindPic(LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim, LONG dir, VARIANT *x, VARIANT *y, LONG *ret)
{

	x->vt = y->vt = VT_I4;
	obj.FindPic(x1, y1, x2, y2, files, delta_color, sim, dir, &x->lVal, &y->lVal, ret);

	return S_OK;
}
//查找多个图片
STDMETHODIMP OpInterface::FindPicEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim, LONG dir, BSTR *retstr)
{
	wstring s;
	obj.FindPicEx(x1, y1, x2, y2, files, delta_color, sim, dir, s);

	CComBSTR newstr;
	HRESULT hr;
	newstr.Append(s.data());
	hr = newstr.CopyTo(retstr);
	return hr;
}
// 这个函数可以查找多个图片, 并且返回所有找到的图像的坐标.此函数同FindPicEx.只是返回值不同.(file1, x, y | file2, x, y | ...)
STDMETHODIMP OpInterface::FindPicExS(LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim, LONG dir, BSTR *retstr)
{

	wstring s;
	obj.FindPicExS(x1, y1, x2, y2, files, delta_color, sim, dir, s);
	CComBSTR newstr;
	HRESULT hr;
	newstr.Append(s.data());
	hr = newstr.CopyTo(retstr);

	return S_OK;
}
//查找指定区域内的颜色块,颜色格式"RRGGBB-DRDGDB",注意,和按键的颜色格式相反
STDMETHODIMP OpInterface::FindColorBlock(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG count, LONG height, LONG width, VARIANT *x, VARIANT *y, LONG *ret)
{
	x->vt = y->vt = VT_I4;
	obj.FindColorBlock(x1, y1, x2, y2, color, sim, count, height, width, &x->lVal, &y->lVal, ret);
	return S_OK;
}
//查找指定区域内的所有颜色块, 颜色格式"RRGGBB-DRDGDB", 注意, 和按键的颜色格式相反
STDMETHODIMP OpInterface::FindColorBlockEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG count, LONG height, LONG width, BSTR *retstr)
{
	std::wstring s;
	obj.FindColorBlockEx(x1, y1, x2, y2, color, sim, count, height, width, s);
	CComBSTR newstr;
	HRESULT hr;
	newstr.Append(s.data());
	hr = newstr.CopyTo(retstr);

	return S_OK;
}
//对插件部分接口的返回值进行解析,并返回ret中的坐标个数
STDMETHODIMP OpInterface::GetResultCount(BSTR str, LONG *ret)
{
	const wchar_t* p = str;
	int cnt = 0;
	while(p){
		if(*p==L'|') ++cnt;
		++p;
	}
	*ret = cnt;
	return S_OK;
}
//对插件部分接口的返回值进行解析,并根据指定的第index个坐标,返回具体的值
STDMETHODIMP OpInterface::GetResultPos(BSTR str, LONG index, VARIANT *x, VARIANT *y, LONG *ret)
{
	x->vt = y->vt = VT_I4;
	long cnt = 0;
	const wchar_t* p = str;
	*ret = 0;
	while(p&&index<cnt){
		if(index==cnt){
			if(swscanf(p,L"%d,%d",&x->lVal,&y->lVal)==2){
				*ret = 1;
			}else{
				*ret= 0;
			}
			break;
		}
		if(*p==L'|') ++cnt;
		++p;
	}
	return S_OK;
}
//获取(x,y)的颜色
STDMETHODIMP OpInterface::GetColor(LONG x, LONG y, BSTR *ret)
{
	wstring s;
	obj.GetColor(x, y, s);

	CComBSTR newstr;
	newstr.Append(s.data());
	newstr.CopyTo(ret);
	return S_OK;
}

STDMETHODIMP OpInterface::SetDisplayInput(BSTR mode, LONG *ret)
{
	obj.SetDisplayInput(mode, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::LoadPic(BSTR pic_name, LONG *ret)
{
	//to do;
	obj.LoadPic(pic_name, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::FreePic(BSTR pic_name, LONG *ret)
{
	obj.FreePic(pic_name, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::LoadMemPic(BSTR pic_name, long long data, LONG size, LONG *ret)
{
	obj.LoadMemPic(pic_name, (void *)data, size, ret);
	return S_OK;
}

//获取指定区域的图像,用二进制数据的方式返回
STDMETHODIMP OpInterface::GetScreenData(LONG x1, LONG y1, LONG x2, LONG y2, LONG *ret)
{
	//#if OP64
	//	data->vt = VT_I8;
	//	data->llVal = 0;
	//#else
	//	data->vt = VT_I4;
	//	data->lVal = 0;
	//#endif
	//	* ret = 0;
	//	void* data_ = nullptr;
	//	obj.GetScreenData(x1, y1, x2, y2, &data_, ret);
	//
	//#if OP64
	//	data->llVal = (long long)data_;
	//#else
	//	data->lVal = (long)data_;
	//#endif
	//	* ret = 1;
	*ret = 0;
	void *data_ = nullptr;
	obj.GetScreenData(x1, y1, x2, y2, &data_, ret);
	*ret = (long)data_;
	return S_OK;
}

STDMETHODIMP OpInterface::GetScreenDataBmp(LONG x1, LONG y1, LONG x2, LONG y2, VARIANT *data, VARIANT *size, LONG *ret)
{
#if OP64
	data->vt = VT_I8;
	size->vt = VT_I8;
	data->lVal = 0;
	size->llVal = 0;
#else
	data->vt = VT_I4;
	size->vt = VT_I4;
	data->lVal = 0;
	size->lVal = 0;
#endif
	void *data_ = nullptr;

	obj.GetScreenDataBmp(x1, y1, x2, y2, &data_, &size->lVal, ret);
#if OP64
	data->llVal = (long long)data_;
#else
	data->lVal = (long)data_;
#endif
	//size->lVal = bfh.bfSize;

	return S_OK;
}

//根据通配符获取文件集合. 方便用于FindPic和FindPicEx
STDMETHODIMP OpInterface::MatchPicName(BSTR pic_name, BSTR *ret)
{
	wstring s;
	obj.MatchPicName(pic_name, s);
	CComBSTR newstr;
	newstr.Append(s.data());
	newstr.CopyTo(ret);
	return S_OK;
}

//设置字库文件
STDMETHODIMP OpInterface::SetDict(LONG idx, BSTR file_name, LONG *ret)
{
	obj.SetDict(idx, file_name, ret);

	return S_OK;
}

//设置字库文件
STDMETHODIMP OpInterface::SetMemDict(LONG idx, BSTR data, LONG size, LONG *ret)
{
	obj.SetMemDict(idx, data, size, ret);

	return S_OK;
}

//使用哪个字库文件进行识别
STDMETHODIMP OpInterface::UseDict(LONG idx, LONG *ret)
{
	obj.UseDict(idx, ret);

	return S_OK;
}
//识别屏幕范围(x1,y1,x2,y2)内符合color_format的字符串,并且相似度为sim,sim取值范围(0.1-1.0),
STDMETHODIMP OpInterface::Ocr(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR *ret_str)
{
	wstring s;
	obj.Ocr(x1, y1, x2, y2, color, sim, s);

	CComBSTR newstr;
	newstr.Append(s.data());
	newstr.CopyTo(ret_str);
	return S_OK;
}
//回识别到的字符串，以及每个字符的坐标.
STDMETHODIMP OpInterface::OcrEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR *ret_str)
{
	wstring s;
	obj.OcrEx(x1, y1, x2, y2, color, sim, s);

	CComBSTR newstr;
	newstr.Append(s.data());
	newstr.CopyTo(ret_str);
	return S_OK;
}
//在屏幕范围(x1,y1,x2,y2)内,查找string(可以是任意个字符串的组合),并返回符合color_format的坐标位置
STDMETHODIMP OpInterface::FindStr(LONG x1, LONG y1, LONG x2, LONG y2, BSTR strs, BSTR color, DOUBLE sim, VARIANT *retx, VARIANT *rety, LONG *ret)
{

	retx->vt = rety->vt = VT_INT;
	obj.FindStr(x1, y1, x2, y2, strs, color, sim, &retx->lVal, &rety->lVal, ret);

	return S_OK;
}
//返回符合color_format的所有坐标位置
STDMETHODIMP OpInterface::FindStrEx(LONG x1, LONG y1, LONG x2, LONG y2, BSTR strs, BSTR color, DOUBLE sim, BSTR *retstr)
{
	wstring s;
	obj.FindStrEx(x1, y1, x2, y2, strs, color, sim, s);

	CComBSTR newstr;
	newstr.Append(s.data());
	newstr.CopyTo(retstr);
	return S_OK;
}

STDMETHODIMP OpInterface::OcrAuto(LONG x1, LONG y1, LONG x2, LONG y2, DOUBLE sim, BSTR *retstr)
{
	wstring s;
	obj.OcrAuto(x1, y1, x2, y2, sim, s);

	CComBSTR newstr;
	newstr.Append(s.data());
	newstr.CopyTo(retstr);
	return S_OK;
}

//从文件中识别图片
STDMETHODIMP OpInterface::OcrFromFile(BSTR file_name, BSTR color_format, DOUBLE sim, BSTR *retstr)
{
	wstring s;
	obj.OcrFromFile(file_name, color_format, sim, s);

	CComBSTR newstr;
	newstr.Append(s.data());
	newstr.CopyTo(retstr);
	return S_OK;
}
//从文件中识别图片,无需指定颜色
STDMETHODIMP OpInterface::OcrAutoFromFile(BSTR file_name, DOUBLE sim, BSTR *retstr)
{
	wstring s;
	obj.OcrAutoFromFile(file_name, sim, s);

	CComBSTR newstr;
	newstr.Append(s.data());
	newstr.CopyTo(retstr);
	return S_OK;
}

STDMETHODIMP OpInterface::FindLine(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR *retstr)
{
	wstring s;
	obj.FindLine(x1, y1, x2, y2, color, sim, s);

	CComBSTR newstr;
	newstr.Append(s.data());
	newstr.CopyTo(retstr);
	return S_OK;
}

STDMETHODIMP OpInterface::WriteData(LONG hwnd, BSTR address, BSTR data, LONG size, LONG *ret)
{
	obj.WriteData(hwnd, address, data, size, ret);

	return S_OK;
}

STDMETHODIMP OpInterface::ReadData(LONG hwnd, BSTR address, LONG size, BSTR *retstr)
{
	wstring s;
	obj.ReadData(hwnd, address, size, s);

	CComBSTR newstr;
	newstr.Append(s.data());
	newstr.CopyTo(retstr);
	return S_OK;
}