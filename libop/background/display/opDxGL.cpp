
//#include "stdafx.h"

#include "opDxGL.h"
#include "./core/globalVar.h"
#include "./core/helpfunc.h"
#include "./core/opEnv.h"
#include <exception>
#include "BlackBone/Process/Process.h"
#include "BlackBone/Process/RPC/RemoteFunction.hpp"

#include "./include/Image.hpp"

#include <sstream>
opDxGL::opDxGL() :IDisplay(),m_opPath(opEnv::getBasePath())
{
}


opDxGL::~opDxGL()
{
	//do clear
	UnBindEx();
}


long opDxGL::BindEx(HWND hwnd, long render_type) {
	//setlog("BindEx");
	_hwnd = hwnd;
	long bind_ret = 0;
	if (render_type == RDT_GL_NOX) {
		bind_ret = BindNox(hwnd, render_type);
	}
	else {
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


		if (NT_SUCCESS(hr)) {
			wstring dllname = opEnv::getOpName();
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
				wstring opFile = m_opPath + L"\\" + dllname;
				if (::PathFileExistsW(opFile.data())) {
					auto iret = proc.modules().Inject(opFile);
					injected = (iret ? true : false);
				}
				else {
					setlog(L"file:<%s> not exists!", opFile.data());
				}

			}
			if (injected) {
				//setlog("before MakeRemoteFunction");
				using my_func_t = long(__stdcall*)(HWND, int);
				auto pSetXHook = blackbone::MakeRemoteFunction<my_func_t>(proc, dllname, "SetDisplayHook");
				if (pSetXHook) {
					//setlog("after MakeRemoteFunction");
					auto cret = pSetXHook(hwnd, render_type);
					//setlog("after pSetXHook");
					bind_ret = cret.result();
					//setlog("after result");
				}
				else {
					//setlog(L"remote function not found.");
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
		//setlog("after Detach");
	}

	if (bind_ret == -1) {
		setlog("UnknownError");
	}
	else if (bind_ret == -2) {
		setlog("NotSupportedError");
	}
	else if (bind_ret == -3) {
		setlog("ModuleNotFoundError");
	}
	return bind_ret;
}
//long bkdo::UnBind(HWND hwnd) {
//	_hwnd = hwnd;
//	_bind_state = 1;
//	return UnBind();
//}

long opDxGL::UnBindEx() {
	//setlog("bkdo::UnBindEx()");
	if (_render_type == RDT_GL_NOX)
		return UnBindNox();
	DWORD id;
	::GetWindowThreadProcessId(_hwnd, &id);

	//attach 进程s
	blackbone::Process proc;
	NTSTATUS hr;
	//setlog("bkdo::Attach");
	hr = proc.Attach(id);

	if (NT_SUCCESS(hr)) {
		wstring dllname =  opEnv::getOpName();
		//检查是否与插件相同的32/64位,如果不同，则使用另一种dll
		BOOL is64 = proc.modules().GetMainModule()->type == blackbone::eModType::mt_mod64;
		if (is64 != OP64) {
			dllname = is64 ? L"op_x64.dll" : L"op_x86.dll";
		}
		//setlog(L"bkdo::dllname=%s",dllname);
		using my_func_t = long(__stdcall*)(void);
		auto pUnXHook = blackbone::MakeRemoteFunction<my_func_t>(proc, dllname, "ReleaseDisplayHook");
		if (pUnXHook) {
			//setlog(L"bkdo::pUnXHook");
			pUnXHook();
			//BOOL fret = ::FreeLibrary((HMODULE)proc.modules().GetModule(dllname)->baseAddress);
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
	//setlog(L"bkdo::Detach");
	proc.Detach();
	//bind_release();
	return 1;
}

long opDxGL::BindNox(HWND hwnd, long render_type) {
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
	NTSTATUS hr = -1;


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
			wstring opFile = m_opPath + L"\\" + dllname;
			if (::PathFileExistsW(opFile.data())) {
				auto iret = proc.modules().Inject(opFile);
				injected = (iret ? true : false);
			}
			else {
				setlog(L"file:<%s> not exists!", opFile.data());
			}
		}
		if (injected) {
			using my_func_t = long(__stdcall*)(HWND, int);
			auto pSetXHook = blackbone::MakeRemoteFunction<my_func_t>(proc, dllname, "SetDisplayHook");
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

long opDxGL::UnBindNox() {

	//attach 进程
	blackbone::Process proc;
	NTSTATUS hr;

	hr = proc.Attach(L"NoxVMHandle.exe");
	wstring dllname = L"op_x64.dll";


	if (NT_SUCCESS(hr)) {

		using my_func_t = long(__stdcall*)(void);
		auto pUnXHook = blackbone::MakeRemoteFunction<my_func_t>(proc, dllname, "ReleaseDisplayHook");
		if (pUnXHook) {
			pUnXHook();

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



bool opDxGL::requestCapture(int x1, int y1, int w, int h, Image& img) {
	img.create(w, h);
	_pmutex->lock();
	uchar* const ppixels = _shmem->data<byte>() + sizeof(FrameInfo);
	FrameInfo* pInfo = (FrameInfo*)_shmem->data<byte>();
	static bool first = true;
	if (first&&(pInfo->width != _width || pInfo->height != _height)) {
		first = false;
		std::wstringstream ss(std::wstringstream::in | std::wstringstream::out);
		ss << (*pInfo);
		setlog(L"error pInfo->width != _width || pInfo->height != _height\nframe info:\n%s", ss.str().data());
	
	}


	if (GET_RENDER_TYPE(_render_type) == RENDER_TYPE::DX) {//NORMAL

		for (int i = 0; i < h; i++) {
			memcpy(img.ptr<uchar>(i), ppixels + (i + y1) * 4 * _width + x1 * 4, 4 * w);
		}
	}
	else {

		for (int i = 0; i < h; i++) {
			memcpy(img.ptr<uchar>(i), ppixels + (_height - 1 - i - y1) * _width * 4 + x1 * 4, 4 * w);
		}
	}


	_pmutex->unlock();
	return true;
}

