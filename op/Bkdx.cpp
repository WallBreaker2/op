#include "stdafx.h"
#include "Bkdx.h"
#include <d3dx9.h>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/windows_shared_memory.hpp> 
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <exception>
#include "Tool.h"

//以下函数用于 user call


Bkdx::Bkdx() :_hwnd(NULL), _pmutex(nullptr), _is_bind(0)
{
	_region = nullptr;
}


Bkdx::~Bkdx()
{
	UnBind();

}


long Bkdx::Bind(HWND hwnd) {
	if (_is_bind)// 如果已经绑定，先解绑
		UnBind();
	DWORD id;
	::GetWindowThreadProcessId(hwnd, &id);
	RECT rc;
	//获取客户区大小
	::GetClientRect(hwnd, &rc);
	_width = rc.right - rc.left;
	_height = rc.bottom - rc.top;
	//setlog(L"Bkdx::Bind,width=%d,height=%d", _width, _height);
	_hwnd = hwnd;
	//attach 进程
	auto hr = _process.Attach(id);
	long bind_ret = 0;
	if (NT_SUCCESS(hr)) {
		//获取当前模块文件名
		wchar_t buff[256];
		::GetModuleFileName(gInstance, buff, 256);
		_dllname = buff;
		_dllname = _dllname.substr(_dllname.rfind(L"\\") + 1);
		_process.Resume();
		blackbone::call_result_t<blackbone::ModuleDataPtr> reg_ret;
		//判断是否已经注入
		auto _dllptr = _process.modules().GetModule(_dllname);
		if (!_dllptr) {
			//setlog(L"inject..");
			reg_ret = _process.modules().Inject(buff);
			//setlog(L"inject finish...");
		}
		else {
			//setlog("alreadly inject.");
			reg_ret.status = 0;
		}
		//恢复进程
		_process.Resume();
		if (NT_SUCCESS(reg_ret.status)) {
			//wait some time 
			::Sleep(200);
			using my_func_t = long(__stdcall*)(HWND);

			//setlog(_dllname.c_str());
			auto SetDX9HookPtr = blackbone::MakeRemoteFunction<my_func_t>(_process, _dllname, "SetDX9Hook");
			if (SetDX9HookPtr) {
				bind_init();
				SetDX9HookPtr(hwnd);
				bind_ret = 1;
			}
			else {
				Tool::setlog(L"remote function not found.");
			}
		}
		else {
			Tool::setlog(L"Inject false.");
		}
	}
	else {
		Tool::setlog(L"attach false.");
	}
	_process.Detach();
	_hwnd = bind_ret ? hwnd : NULL;
	if (bind_ret) {
		_is_bind = 1;
		//setlog("shared_res_name=%s mutex_name=%s",_shared_res_name,_mutex_name);

	}

	return bind_ret;
}

long Bkdx::UnBind() {
	DWORD id = 0;
	if (_is_bind&&::IsWindow(_hwnd))
		::GetWindowThreadProcessId(_hwnd, &id);
	auto hr = _process.Attach(id);
	long bind_ret = 0;
	if (NT_SUCCESS(hr)) {
		//wait some time 
		::Sleep(200);
		using my_func_t = long(__stdcall*)(void);
		auto UnDX9HookPtr = blackbone::MakeRemoteFunction<my_func_t>(_process, _dllname, "UnDX9Hook");
		//auto UnDX9HookPtr = blackbone::MakeRemoteFunction<my_func_t>(_process, L"dll_test.dll", "UnDX9Hook");
		if (UnDX9HookPtr) {
			UnDX9HookPtr();
			bind_ret = 1;
		}
		else {
			Tool::setlog(L"get unhook ptr false.");
		}
#ifndef _WIN64
		_process.modules().RemoveManualModule(_dllname, blackbone::eModType::mt_mod32);
#else
		_process.modules().RemoveManualModule(_dllname, blackbone::eModType::mt_mod64);
#endif
	}
	else {
		Tool::setlog("attach false.");
	}
	_process.Detach();
	_hwnd = NULL;
	bind_release();
	return bind_ret;
}

long Bkdx::bind_init() {
	sprintf(_shared_res_name, SHARED_RES_NAME_FORMAT, _hwnd);
	sprintf(_mutex_name, MUTEX_NAME_FORMAT, _hwnd);
	try {
		boost::interprocess::shared_memory_object shm(
			boost::interprocess::create_only,
			_shared_res_name,
			boost::interprocess::read_write);
		shm.truncate(SHARED_MEMORY_SIZE);
		_region = new boost::interprocess::mapped_region(shm, boost::interprocess::read_only);
		_image_data = (byte*)_region->get_address();
		_pmutex = new boost::interprocess::named_mutex(boost::interprocess::create_only, _mutex_name);
	}
	catch (std::exception&e) {
		Tool::setlog("Bkdx::bind_init %s exception:%s", _shared_res_name, e.what());
	}

	return 0;
}

long Bkdx::bind_release() {
	try {
		using namespace boost::interprocess;
		shared_memory_object::remove(_shared_res_name);
		named_mutex::remove(_mutex_name);
		SAFE_DELETE(_region);
		SAFE_DELETE(_pmutex);

	}
	catch (std::exception&e) {
		Tool::setlog("Bkdx::bind_release std::exception:%s", e.what());
	}

	_image_data = nullptr;
	return 0;
}

long Bkdx::capture(const std::wstring& file_name) {
	//setlog(L"Bkdx::capture");
	if (!_is_bind)
		return 0;
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
	//setlog("file.write((char*)_image_data=%p", _image_data);
	try {
		/*boost::interprocess::shared_memory_object shm(
			boost::interprocess::open_only,
			_shared_res_name,
			boost::interprocess::read_only);
		boost::interprocess::mapped_region region(shm, boost::interprocess::read_only);
		_image_data = (byte*)region.get_address();
		*/
		_pmutex->lock();
		file.write((char*)_image_data, bih.biSizeImage);
		_pmutex->unlock();
	}
	catch (std::exception&e) {
		Tool::setlog("cap exception:%s", e.what());
	}

	file.close();
	return 1;
}

byte* Bkdx::get_data() {

	/*try {
		boost::interprocess::shared_memory_object shm(
			boost::interprocess::open_only,
			_shared_res_name,
			boost::interprocess::read_only);
		boost::interprocess::mapped_region region(shm, boost::interprocess::read_only);
		_image_data = (byte*)region.get_address();
	}
	catch (std::exception&e) {
		setlog(" Bkdx::FindPic exception:%s", e.what());
	}*/
	return _image_data;
}
