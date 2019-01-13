#pragma once
#ifndef __DXBACKGROUND_H_
#define __DXBACKGROUND_H_
#include <BlackBone/Process/Process.h>
//#include <BlackBone/Patterns/PatternSearch.h>
#include <BlackBone/Process/RPC/RemoteFunction.hpp>
//#include <BlackBone/Syscalls/Syscall.h>
#include "Common.h"
#include "ImageLoc.h"
#include <boost/interprocess/sync/named_mutex.hpp> 

using std::wstring;
class Bkdx
{
public:
	Bkdx();
	~Bkdx();
	//1
	long Bind(HWND hwnd);
	long UnBind();

	long bind_init();
	long bind_release();
	//截图至文件
	long capture(const std::wstring& file_name);
	//图形定位
	long FindPic(long x1, long y1, long x2, long y2, const std::wstring& files, double sim, long& x, long &y);


private:
	int _is_bind;
	HWND _hwnd;
	wstring _dllname;
	long _width, _height;
	blackbone::Process _process;
	ImageLoc _imageloc;
	//user mutex
	boost::interprocess::named_mutex* _pmutex;
	char _shared_res_name[256];
	char _mutex_name[256];
	byte* _image_data;
};

#endif

