
#include "stdafx.h"

#include "bkdx_gl.h"
#include "globalVar.h"
#include "helpfunc.h"
#include <exception>
#include "3rd_party/include/BlackBone/Process/Process.h"
#include "3rd_party/include/BlackBone/Process/RPC/RemoteFunction.hpp"

#include "./include/Image.hpp"

#include "globalVar.h"

bkdo::bkdo():IDisplay()
{
}


bkdo::~bkdo()
{
	//do clear
	UnBindEx();
}


long bkdo::BindEx(HWND hwnd, long render_type) {
	_hwnd = hwnd;
	if (render_type == RDT_GL_NOX)
		return BindNox(hwnd, render_type);
	_render_type = render_type;
	RECT rc;
	//获取客户区大小
	::GetClientRect(hwnd, &rc);
	_width = rc.right - rc.left;
	_height = rc.bottom - rc.top;
	//bind_init();
	if (render_type == RDT_GL_NOX) {
	}
	DWORD id;
	::GetWindowThreadProcessId(_hwnd, &id);



	//attach 进程
	blackbone::Process proc;
	NTSTATUS hr;

	hr = proc.Attach(id);

	long bind_ret = 0;
	if (NT_SUCCESS(hr)) {
		wstring dllname = g_op_name;
		//检查是否与插件相同的32/64位,如果不同，则使用另一种dll
		BOOL is64 = proc.modules().GetMainModule()->type == blackbone::eModType::mt_mod64;
		if (is64 != OP64) {
			dllname = is64 ? L"op_x64.dll" : L"op_x86.dll";
		}

		bool injected = false;
		//判断是否已经注入
		auto _dllptr = proc.modules().GetModule(dllname);
		auto mods = proc.modules().GetAllModules();
		if (_dllptr) {
			injected = true;
		}
		else {
			auto iret = proc.modules().Inject(g_op_path + L"\\" + dllname);
			injected = (iret ? true : false);
		}
		if (injected) {
			using my_func_t = long(__stdcall*)(HWND, int);
			auto pSetXHook = blackbone::MakeRemoteFunction<my_func_t>(proc, dllname, "SetXHook");
			if (pSetXHook) {
				auto cret = pSetXHook(hwnd, render_type);
				bind_ret = cret.result();
			}
			else {
				setlog(L"remote function not found.");
			}
		}
		else {
			setlog(L"Inject false.");
		}




	}
	else {
		setlog(L"attach false.");
	}
	proc.Detach();

	return bind_ret;
}
//long bkdo::UnBind(HWND hwnd) {
//	_hwnd = hwnd;
//	_bind_state = 1;
//	return UnBind();
//}

long bkdo::UnBindEx() {
    //setlog("bkdo::UnBindEx()");
	if (_render_type == RDT_GL_NOX)
		return UnBindNox();
	DWORD id;
	::GetWindowThreadProcessId(_hwnd, &id);

	//attach 进程s
	blackbone::Process proc;
	NTSTATUS hr;

	hr = proc.Attach(id);

	if (NT_SUCCESS(hr)) {
		wstring dllname = g_op_name;
		//检查是否与插件相同的32/64位,如果不同，则使用另一种dll
		BOOL is64 = proc.modules().GetMainModule()->type == blackbone::eModType::mt_mod64;
		if (is64 != OP64) {
			dllname = is64 ? L"op_x64.dll" : L"op_x86.dll";
		}
		using my_func_t = long(__stdcall*)(void);
		auto pUnXHook = blackbone::MakeRemoteFunction<my_func_t>(proc, dllname, "UnXHook");
		if (pUnXHook) {
			pUnXHook();
			BOOL fret = ::FreeLibrary((HMODULE)proc.modules().GetModule(dllname)->baseAddress);
			//if (!fret)setlog("fret=%d", fret);
			/*proc.modules().RemoveManualModule(dllname,
				is64 ? blackbone::eModType::mt_mod64 : blackbone::eModType::mt_mod32);*/
		}
		else {
			setlog(L"get unhook ptr false.");
		}
	}
	else {
		setlog("blackbone::MakeRemoteFunction false,errcode:%X,pid=%d,hwnd=%d", hr, id, _hwnd);
	}

	proc.Detach();
	//bind_release();
	return 1;
}

long bkdo::BindNox(HWND hwnd, long render_type) {
	_render_type = render_type;
	_hwnd = hwnd;
	RECT rc;
	//获取客户区大小
	::GetClientRect(hwnd, &rc);
	_width = rc.right - rc.left;
	_height = rc.bottom - rc.top;
	//bind_init();



	//attach 进程
	blackbone::Process proc;
	NTSTATUS hr;


	wstring dllname = L"op_x64.dll";


	hr = proc.Attach(L"NoxVMHandle.exe");

	long bind_ret = 0;

	if (NT_SUCCESS(hr)) {
		/*_process.Resume();*/
		bool injected = false;
		//判断是否已经注入
		auto _dllptr = proc.modules().GetModule(dllname);
		auto mods = proc.modules().GetAllModules();
		if (_dllptr) {
			injected = true;
		}
		else {
			auto iret = proc.modules().Inject(g_op_path + L"\\" + dllname);
			injected = (iret ? true : false);
		}
		if (injected) {
			using my_func_t = long(__stdcall*)(HWND, int);
			auto pSetXHook = blackbone::MakeRemoteFunction<my_func_t>(proc, dllname, "SetXHook");
			if (pSetXHook) {
				auto cret = pSetXHook(hwnd, render_type);
				bind_ret = cret.result();
			}
			else {
				setlog(L"remote function not found.");
			}
		}
		else {
			setlog(L"Inject false.");
		}



	}
	else {
		setlog(L"attach false.");
	}
	proc.Detach();


	return bind_ret;
}

long bkdo::UnBindNox() {

	//attach 进程
	blackbone::Process proc;
	NTSTATUS hr;

	hr = proc.Attach(L"NoxVMHandle.exe");
	wstring dllname = L"op_x64.dll";


	if (NT_SUCCESS(hr)) {

		using my_func_t = long(__stdcall*)(void);
		auto pUnXHook = blackbone::MakeRemoteFunction<my_func_t>(proc, dllname, "UnXHook");
		if (pUnXHook) {
			//pUnXHook();

			/*BOOL fret = ::FreeLibrary((HMODULE)proc.modules().GetModule(dllname)->baseAddress);
			if (!fret)setlog("fret=%d", fret);*/
		}
		else {
			setlog(L"get unhook ptr false.");
		}
	}
	else {
		setlog("blackbone::MakeRemoteFunction false,errcode:%Xhwnd=%d", hr, _hwnd);
	}

	proc.Detach();

	return 1;
}



bool bkdo::requestCapture(int x1, int y1, int w, int h, Image& img) {
	img.create(w, h);
	_pmutex->lock();
	uchar* ppixels = _shmem->data<byte>()+sizeof(FrameInfo);
	if (GET_RENDER_TYPE(_render_type) == RENDER_TYPE::DX) {//NORMAL
		//setlog("cap1");
		for (int i = 0; i < h; i++) {
			memcpy(img.ptr<uchar>(i), ppixels + (i + y1) * 4 * _width + x1 * 4, 4 * w);
		}
	}
	else {
		//setlog("cap2");

		for (int i = 0; i < h; i++) {
			memcpy(img.ptr<uchar>(i), ppixels + (_height - 1 - i - y1) * _width * 4 + x1 * 4, 4 * w);
		}
	}


	_pmutex->unlock();
	return true;
}

