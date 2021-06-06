#pragma once
#ifndef __GLOBALVAR_H_
#define __GLOBALVAR_H_
#include "optype.h"
#define SAFE_CLOSE(h)   \
	if (h)              \
		CloseHandle(h); \
	h = NULL;
template <class Type>
void SAFE_DELETE(Type *&ptr)
{
	delete ptr;
	ptr = nullptr;
}

#define SAFE_DELETE_ARRAY(ptr) \
	if (ptr)                   \
		delete[] ptr;          \
	ptr = nullptr

#define SAFE_RELEASE(obj) \
	if (obj)              \
		obj->Release();   \
	obj = nullptr

//#define _sto_wstring(s) boost::locale::conv::to_utf<wchar_t>(s, "GBK")
//#define _wsto_string(s)  boost::locale::conv::from_utf(s,"GBK")

#define DLL_API extern "C" _declspec(dllexport)
//normal windows,gdi;,dx;opengl;
enum RENDER_TYPE
{
	NORMAL = 0,
	GDI = 1,
	DX = 2,
	OPENGL = 3
};

#define MAKE_RENDER(type, flag) ((type << 16) | flag)

#define GET_RENDER_TYPE(t) (t >> 16)

#define GET_RENDER_FLAG(t) (t & 0xffff)

constexpr int RDT_NORMAL = MAKE_RENDER(NORMAL, 0);
constexpr int RDT_GDI = MAKE_RENDER(GDI, 0);
constexpr int RDT_GDI2 = MAKE_RENDER(GDI, 1);
constexpr int RDT_GDI_DX2 = MAKE_RENDER(GDI, 2);
constexpr int RDT_DX_DEFAULT = MAKE_RENDER(DX, 0);
constexpr int RDT_DX_D3D9 = MAKE_RENDER(DX, 1);
constexpr int RDT_DX_D3D10 = MAKE_RENDER(DX, 2);
constexpr int RDT_DX_D3D11 = MAKE_RENDER(DX, 3);
constexpr int RDT_GL_DEFAULT = MAKE_RENDER(OPENGL, 0);
constexpr int RDT_GL_STD = MAKE_RENDER(OPENGL, 1);
constexpr int RDT_GL_NOX = MAKE_RENDER(OPENGL, 2);
constexpr int RDT_GL_ES = MAKE_RENDER(OPENGL, 3);
constexpr int RDT_GL_FI = MAKE_RENDER(OPENGL, 4); //glFinish

enum INPUT_TYPE
{
	IN_NORMAL = 0,
	IN_WINDOWS = 1,
	IN_DX = 2
};
//define Image byte format
constexpr int IBF_R8G8B8A8 = 0;
constexpr int IBF_B8G8R8A8 = 1;
constexpr int IBF_R8G8B8 = 2;

//const size_t MAX_IMAGE_WIDTH = 1<<11;
//const size_t SHARED_MEMORY_SIZE = 1080 * 1928 * 4;

constexpr auto SHARED_RES_NAME_FORMAT = L"op_mutex_%d";
constexpr auto MUTEX_NAME_FORMAT = L"op_shared_mem_%d";

#ifndef _M_X64
#define OP64 0
#else
#define OP64 1
#endif

#define _TOSTRING(x) #x

#define MAKE_OP_VERSION(a, b, c, d) _TOSTRING(a##.##b##.##c##.##d)

#define OP_VERSION MAKE_OP_VERSION(0, 4, 0, 0)
//模块句柄
//extern HINSTANCE gInstance;
//是否显示错误信息
//extern int gShowError;
//op 路径
//extern wstring m_opPath;

//extern wstring g_op_name;

#endif
