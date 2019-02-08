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
			//reg_ret = _process.modules().Inject(buff);
			/*long error_code = 0;
			if (!Injecter::EnablePrivilege(true))
				Tool::setlog("Injecter::EnablePrivilege False.");
			reg_ret.status = Injecter::InjectDll(id, buff, error_code);
			Tool::setlog("inject ret=%d,error_code=%d", reg_ret.status, error_code);
			*/
			setlog(buff);
			auto& modules = _process.modules();
			reg_ret = modules.Inject(buff);
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
			auto HookPtr = blackbone::MakeRemoteFunction<my_func_t>(_process, _dllname, "SetOpenglHook");
			if (HookPtr) {
				bind_init();
				HookPtr(hwnd);
				bind_ret = 1;
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
	_process.Detach();
	_hwnd = bind_ret ? hwnd : NULL;
	if (bind_ret) {
		_bind_state = 1;
		//setlog("shared_res_name=%s mutex_name=%s",_shared_res_name,_mutex_name);

	}
	//setlog("bkopengl::Bind finish");
	return bind_ret;
}

long bkopengl::UnBind() {
	auto hr = _process.Attach(_process_id);
	long bind_ret = 0;
	if (NT_SUCCESS(hr)) {
		//wait some time 
		::Sleep(200);
		using my_func_t = long(__stdcall*)(void);
		auto UnDX9HookPtr = blackbone::MakeRemoteFunction<my_func_t>(_process, _dllname, "UnOpenglHook");
		if (UnDX9HookPtr) {
			UnDX9HookPtr();
			bind_ret = 1;
		}
		else {
			setlog(L"get unhook ptr false.");
		}
	}
	else {
		setlog("attach false.");
	}
	_process.Detach();
	_hwnd = NULL;
	bind_release();
	return bind_ret;
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
		/*boost::interprocess::shared_memory_object shm(
			boost::interprocess::open_only,
			_shared_res_name,
			boost::interprocess::read_only);
		boost::interprocess::mapped_region region(shm, boost::interprocess::read_only);
		_image_data = (byte*)region.get_address();
		*/
		_pmutex->lock();
		file.write((char*)_region->get_address(), bih.biSizeImage);
		_pmutex->unlock();
	}
	catch (std::exception&e) {
		setlog("cap exception:%s", e.what());
	}

	file.close();
	return 1;
}
