#ifndef __IDISPLAY_H_
#define __IDISPLAY_H_
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
	
	long bind_init();
	
	long bind_release();

	long _bind_state;
protected:
	virtual long BindEx(HWND hwnd, long flag)=0;
	virtual long UnBindEx() = 0;

	HWND _hwnd;

	sharedmem* _shmem;
	
	promutex* _pmutex;

	wchar_t _shared_res_name[256];

	wchar_t _mutex_name[256];

	//
	int _render_type;

	long _width;
	long _height;

	int _client_x, _client_y;
	//need capture rect
	//RECT rect;
	
};

#endif

