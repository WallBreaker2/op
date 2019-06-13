#include "stdafx.h"
#include "bkdo.h"
#include "Tool.h"
#include <exception>
#include "3rd_party/include/BlackBone/Process/Process.h"
//#include <BlackBone/Patterns/PatternSearch.h>
#include "3rd_party/include/BlackBone/Process/RPC/RemoteFunction.hpp"
//#include <BlackBone/Syscalls/Syscall.h>


bkdo::bkdo() 
{
	
}


bkdo::~bkdo()
{
	
}


long bkdo::Bind(HWND hwnd,long render_type) {
	
	_hwnd = hwnd;
	DWORD id;
	::GetWindowThreadProcessId(_hwnd, &id);
	RECT rc;
	//获取客户区大小
	::GetClientRect(hwnd, &rc);
	_width = rc.right - rc.left;
	_height = rc.bottom - rc.top;
	//setlog(L"Bkdx::Bind,width=%d,height=%d", _width, _height);
	
	//attach 进程
	blackbone::Process proc;
	auto hr = proc.Attach(id);
	long bind_ret = 0;
	if (NT_SUCCESS(hr)) {
		//检查是否与插件相同的32/64位
		auto &mod = proc.modules().GetMainModule();
		constexpr blackbone::eModType curModType = (SYSTEM_BITS == 32 ? blackbone::eModType::mt_mod32 : blackbone::eModType::mt_mod64);
		if (mod&&mod->type == curModType) {
			//获取当前模块文件名
			wchar_t buff[256];
			::GetModuleFileName(gInstance, buff, 256);
			_dllname = buff;
			_dllname = _dllname.substr(_dllname.rfind(L"\\") + 1);

			/*_process.Resume();*/
			bool injected = false;
			//判断是否已经注入
			auto _dllptr = proc.modules().GetModule(_dllname);
			if (_dllptr) {
				injected = true;
			}
			else {
				injected = (proc.modules().Inject(buff) ? true : false);
			}
			if (injected) {
				using my_func_t = long(__stdcall*)(HWND,int);
				auto pSetXHook = blackbone::MakeRemoteFunction<my_func_t>(proc, _dllname, "SetXHook");
				if (pSetXHook) {
					bind_init();
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
		blackbone::Process proc;
		auto hr = proc.Attach(id);
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

