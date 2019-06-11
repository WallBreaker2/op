#include "stdafx.h"

#include "bkopengl.h"
#include "Tool.h"
#include "Injecter.h"
bkopengl::bkopengl()
{
	_process_id = 0;
}


bkopengl::~bkopengl()
{
}

long bkopengl::Bind(HWND hwnd, long flag) {
	//setlog("bkopengl::Bind");
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
		//检查是否与插件相同的32/64位
		auto &mod = _process.modules().GetMainModule();
		constexpr blackbone::eModType curModType = (SYSTEM_BITS == 32 ? blackbone::eModType::mt_mod32 : blackbone::eModType::mt_mod64);
		if (mod&&mod->type == curModType) {
			//获取当前模块文件名
			wchar_t buff[256];
			::GetModuleFileName(gInstance, buff, 256);
			_dllname = buff;
			_dllname = _dllname.substr(_dllname.rfind(L"\\") + 1);

			//_process.Resume();
			bool injected = false;
			//判断是否已经注入
			auto _dllptr = _process.modules().GetModule(_dllname);
			if (_dllptr) {
				injected = true;
			}
			else {
				injected = (_process.modules().Inject(buff) ? true : false);
			}

			//恢复进程
			//_process.Resume();
			if (injected) {
				//wait some time 
				::Sleep(200);
				using my_func_t = long(__stdcall*)(HWND);
				auto HookPtr = blackbone::MakeRemoteFunction<my_func_t>(_process, _dllname, "SetOpenglHook");
				if (HookPtr) {
					bind_init();
					auto cret = HookPtr(hwnd);
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
	_process.Detach();
	setlog("bind ret:%d", bind_ret);
	_hwnd = bind_ret ? hwnd : NULL;
	if (bind_ret) {//bind ok
		_bind_state = 1;
	}
	else {//bind error
		_bind_state = 0;
		bind_release();
	}
	return bind_ret;
}

long bkopengl::UnBind() {
	auto hr = _process.Attach(_process_id);
	if (NT_SUCCESS(hr)) {
		//wait some time 
		::Sleep(200);
		using my_func_t = long(__stdcall*)(void);
		auto UnDX9HookPtr = blackbone::MakeRemoteFunction<my_func_t>(_process, _dllname, "UnOpenglHook");
		if (UnDX9HookPtr) {
			UnDX9HookPtr();
		}
		else {
			setlog(L"get unhook ptr false.");
		}
	}

	_process.Detach();
	_hwnd = NULL;
	bind_release();
	_bind_state = 0;
	return 1;
}


long bkopengl::capture(const std::wstring& file_name) {
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
	bih.biHeight = _height;//高度
	bih.biPlanes = 1;
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biSizeImage = _width * _height * 4;//图像数据大小
	bih.biWidth = _width;//宽度
	file.write((char*)&bfh, sizeof(BITMAPFILEHEADER));
	file.write((char*)&bih, sizeof(BITMAPINFOHEADER));
	//setlog("file.write((char*)_image_data=%p", _image_data);
	try {

		_pmutex->lock();
		file.write(_shmem->data<char>(), bih.biSizeImage);
		_pmutex->unlock();
	}
	catch (std::exception&e) {
		setlog("cap exception:%s", e.what());
	}

	file.close();
	return 1;
}
