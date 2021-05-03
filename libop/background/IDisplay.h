#pragma once
//#include <boost/interprocess/sync/named_mutex.hpp> 
//#include <boost/interprocess/shared_memory_object.hpp>
//#include <boost/interprocess/windows_shared_memory.hpp> 
//#include <boost/interprocess/mapped_region.hpp>
//#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <exception>
#include "./include/promutex.h"
#include "./include/sharedmem.h"
#include "frameInfo.h"
struct Image;
class IDisplay
{
public:
	IDisplay();
	 ~IDisplay();
	//bind window
	long Bind(HWND hwnd, long flag);
	//unbind window
	long UnBind();
	//unbind window
	//virtual long UnBind(HWND hwnd) = 0;
	virtual bool requestCapture(int x1, int y1, int w, int h, Image& img) = 0;
	

	promutex* get_mutex() {
		return _pmutex;
	}

	long get_height() {
		return _height;
	}

	long get_width() {
		return _width;
	}
	void getFrameInfo(FrameInfo& info);
private:
	//因为各种截图方式的差异，是否成功判断较复杂，故在此实现资源的申请和释放，子类调用
	//资源申请
	long bind_init();
	//资源释放
	long bind_release();
	//virtual byte* get_data();
		//绑定状态
	long _bind_state;
protected:
	virtual long BindEx(HWND hwnd, long flag)=0;
	virtual long UnBindEx() = 0;
	//窗口句柄
	HWND _hwnd;
	//共享内存
	sharedmem* _shmem;
	//进程互斥量
	promutex* _pmutex;

	wchar_t _shared_res_name[256];

	wchar_t _mutex_name[256];

	//
	int _render_type;
	//宽度
	long _width;
	long _height;
	//客户区偏移
	int _client_x, _client_y;
	//need capture rect
	//RECT rect;
	
};

