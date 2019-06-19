
#include "stdafx.h"
#include <BlackBone/Process/Process.h>
#include <BlackBone/Process/RPC/RemoteFunction.hpp>
#include "bkdo.h"
#include "Tool.h"
#include <exception>
//#include "3rd_party/include/BlackBone/Process/Process.h"
//#include "3rd_party/include/BlackBone/Process/RPC/RemoteFunction.hpp"

#ifdef _M_X64
#pragma comment(lib,"E:/git_pro/Blackbone/build/x64/Release/BlackBone.lib")
#else
#pragma comment(lib,"E:/git_pro/Blackbone/build/Win32/Release/BlackBone.lib")
#endif

bkdo::bkdo() 
{
	_render_type = 0;
}


bkdo::~bkdo()
{
	
}


long bkdo::Bind(HWND hwnd,long render_type) {

	_render_type = render_type;
	_hwnd = hwnd;
	bind_init();

	DWORD id;
	::GetWindowThreadProcessId(_hwnd, &id);
	RECT rc;
	//获取客户区大小
	::GetClientRect(hwnd, &rc);
	_width = rc.right - rc.left;
	_height = rc.bottom - rc.top;
	//setlog(L"Bkdx::Bind,width=%d,height=%d", _width, _height);

	//获取当前模块文件名
	wstring op_path;
	op_path.resize(512);
	DWORD real_size=::GetModuleFileNameW(gInstance, op_path.data(), 512);
	op_path.resize(real_size);
	_dllname = op_path.substr(op_path.rfind(L"\\") + 1);
	

	//attach 进程
	blackbone::Process proc;
	NTSTATUS hr;
#ifndef _M_X64
	if (_render_type==RDT_GL_NOX) {
		hr = proc.Attach(L"NoxVMHandle.exe");
		_dllname = L"op_x64.dll";
		op_path = op_path.substr(0,op_path.rfind(L'\\')+1) + _dllname;
		
	}
	else {
		hr = proc.Attach(id);
	}
#else
	hr = proc.Attach(id);
#endif
	long bind_ret = 0;
	if (NT_SUCCESS(hr)) {
		//检查是否与插件相同的32/64位
		constexpr bool op64= (SYSTEM_BITS == 64);
		BOOL is64 = proc.modules().GetMainModule()->type == blackbone::eModType::mt_mod64;
		//setlog("::IsWow64Process=%d",is64);
		if (is64== op64||_render_type==RDT_GL_NOX) {
			
			/*_process.Resume();*/
			bool injected = false;
			//判断是否已经注入
			auto _dllptr = proc.modules().GetModule(_dllname);
			auto mods = proc.modules().GetAllModules();
			if (_dllptr) {
				injected = true;
			}
			else {
				auto iret = proc.modules().Inject(op_path);
				injected = ( iret? true : false);
			}
			if (injected) {
				using my_func_t = long(__stdcall*)(HWND,int);
				auto pSetXHook = blackbone::MakeRemoteFunction<my_func_t>(proc, _dllname, "SetXHook");
				if (pSetXHook) {
					auto cret = pSetXHook(hwnd,render_type);
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
			setlog("error:mod->type != current_mod");
		}//end check
		
		
		
	}
	else {
		setlog(L"attach false.");
	}
	proc.Detach();
	if (bind_ret) {
		_bind_state = 1;

	}
	else {
		bind_release();
		_bind_state = 0;
	}

	return bind_ret;
}

long bkdo::UnBind() {
	
	if (_bind_state) {
		
		DWORD id;
		::GetWindowThreadProcessId(_hwnd, &id);

		//获取当前模块文件名
		wstring op_path;
		op_path.resize(512);
		DWORD real_size = ::GetModuleFileNameW(gInstance, op_path.data(), 512);
		op_path.resize(real_size);
		_dllname = op_path.substr(op_path.rfind(L"\\") + 1);


		//attach 进程
		blackbone::Process proc;
		NTSTATUS hr;
#ifndef _M_X64
		if (_render_type==RDT_GL_NOX) {
			hr = proc.Attach(L"NoxVMHandle.exe");
			_dllname = L"op_x64.dll";
			op_path = op_path.substr(0, op_path.rfind(L'\\') + 1) + _dllname;

		}
		else {
			hr = proc.Attach(id);
		}
#else
		hr = proc.Attach(id);
#endif

		if (NT_SUCCESS(hr)) {
			
			using my_func_t = long(__stdcall*)(void);
			auto pUnXHook = blackbone::MakeRemoteFunction<my_func_t>(proc, _dllname, "UnXHook");
			if (pUnXHook) {
				pUnXHook();
			}
			else {
				setlog(L"get unhook ptr false.");
			}
		}
		else {
			setlog("blackbone::MakeRemoteFunction false,errcode:%X,pid=%d,hwnd=%d",hr,id,_hwnd);
		}

		proc.Detach();
	}
	bind_release();
	return 1;
}


long bkdo::capture(const std::wstring& file_name) {
	//setlog(L"Bkdx::capture")
	std::fstream file;
	file.open(file_name, std::ios::out | std::ios::binary);
	if (!file.is_open())return 0;
	//_mutex.lock();
	BITMAPFILEHEADER bfh = { 0 };
	BITMAPINFOHEADER bih = { 0 };//位图信息头
	bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bfh.bfSize = bfh.bfOffBits + _width * _height * 4;
	bfh.bfType = static_cast<WORD>(0x4d42);

	bih.biBitCount = 32;//每个像素字节大小
	bih.biCompression = BI_RGB;
	bih.biHeight = -_height;//高度
	bih.biPlanes = 1;
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biSizeImage = _width * _height * 4;//图像数据大小
	bih.biWidth = _width;//宽度
	file.write((char*)&bfh, sizeof(BITMAPFILEHEADER));
	file.write((char*)&bih, sizeof(BITMAPINFOHEADER));

	_pmutex->lock();
	file.write(_shmem->data<char>(), bih.biSizeImage);
	_pmutex->unlock();
	
	file.close();
	return 1;
}

