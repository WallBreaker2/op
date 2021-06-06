//#include "stdafx.h"
#include "IDisplay.h"
#include "./core/globalVar.h"
#include "./core/helpfunc.h"
IDisplay::IDisplay()
	:_hwnd(NULL), _shmem(nullptr), _pmutex(nullptr),
	_bind_state(0), _width(0), _height(0),
	_client_x(0), _client_y(0)
{

}


IDisplay::~IDisplay()
{
	bind_release();
	_bind_state = 0;
}

long IDisplay::Bind(HWND hwnd, long flag) {
	//step 1 check window exists
	if (!::IsWindow(hwnd)) {
		return 0;
	}
	_hwnd = hwnd;
	//step 2. 准备资源
	bind_init();
	//step 3. 调用特定的绑定函数

	if (BindEx(hwnd, flag) == 1) {
		_bind_state = 1;
	}
	else {
		bind_release();
		_bind_state = 0;
	}
		

	return _bind_state;

}

long IDisplay::UnBind() {
	//setlog("UnBind(");
	if (_bind_state) {
		UnBindEx();
	}
	bind_release();
	_bind_state = 0;
	return 1;
}

long IDisplay::bind_init() {
	int res_size = 0;
	RECT rc;
	assert(::IsWindow(_hwnd));
	::GetWindowRect(_hwnd, &rc);
	res_size = (rc.right - rc.left) * (rc.bottom - rc.top) * 4+sizeof(FrameInfo);
	wsprintf(_shared_res_name, SHARED_RES_NAME_FORMAT, _hwnd);
	wsprintf(_mutex_name, MUTEX_NAME_FORMAT, _hwnd);
	//setlog(L"mem=%s mutex=%s", _shared_res_name, _mutex_name);
	//bind_release();
	try {
		_shmem = new sharedmem();
		_shmem->open_create(_shared_res_name, res_size);
		_pmutex = new promutex();
		_pmutex->open_create(_mutex_name);
	}
	catch (std::exception& e) {
		setlog("bkdisplay::bind_init() %s exception:%s", _shared_res_name, e.what());
	}

	return 0;
}

long IDisplay::bind_release() {
	SAFE_DELETE(_shmem);
	SAFE_DELETE(_pmutex);

	_hwnd = NULL;
	//_image_data = nullptr;
	return 0;
}

void IDisplay::getFrameInfo(FrameInfo& info) {
	_pmutex->lock();
	memcpy(&info, _shmem->data<uchar>(), sizeof(FrameInfo));
	_pmutex->unlock();
}

//byte* bkdisplay::get_data() {
//	return _shmem->data<byte>();
//}

