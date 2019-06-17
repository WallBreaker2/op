#pragma once
#ifndef __COMMON_H_
#define __COMMON_H_
#include <string>
#include <fstream>
#include <vector>


#define SAFE_CLOSE(h)if(h) CloseHandle(h);h=NULL;

#define SAFE_DELETE(ptr) if(ptr)delete ptr;ptr=nullptr

#define SAFE_DELETE_ARRAY(ptr) if(ptr)delete[] ptr;ptr=nullptr


//#define _sto_wstring(s) boost::locale::conv::to_utf<wchar_t>(s, "GBK")
//#define _wsto_string(s)  boost::locale::conv::from_utf(s,"GBK")

using std::wstring;
using std::string;
using std::vector;

#define DLL_API extern "C" _declspec(dllexport)
//normal windows,gdi;,dx;opengl;
enum RENDER_TYPE {
	NORMAL = 0,
	GDI=1,
	DX = 2,//DX9
	OPENGL = 3
};
enum RENDER_FLAG {
	NONE = 0,
	D3D9 = 1,
	D3D10 = 2,
	D3D11 = 3,
	GL_STD = 4,
	GL_NOX = 5
};
#define MAKE_RENDER(type,flag) ((type<<16)|flag)

#define GET_RENDER_TYPE(t) (t>>16)

#define GET_RENDER_FLAG(t) (t&0xffff)

enum INPUT_TYPE {
	IN_NORMAL = 0,
	IN_WINDOWS=1
};

const size_t MAX_IMAGE_WIDTH = 1<<11;
const size_t SHARED_MEMORY_SIZE = 1080 * 1928 * 4;

#define SHARED_RES_NAME_FORMAT L"op_mutex_%d"
#define MUTEX_NAME_FORMAT L"op_shared_mem_%d"


#ifndef _M_X64
#define SYSTEM_BITS 32
#else
#define SYSTEM_BITS 64
#endif

#define _TOSTRING(x) #x

#define MAKE_OP_VERSION(a,b,c,d) _TOSTRING(a##.##b##.##c##.##d)

#define OP_VERSION MAKE_OP_VERSION(0,3,0,1)
//模块句柄
extern HINSTANCE gInstance;
//是否显示错误信息
extern int gShowError;

#endif