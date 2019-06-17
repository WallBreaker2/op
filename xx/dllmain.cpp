// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include<gl/GL.h>
#include<gl/GLU.h>
#include "../op/3rd_party/include/MinHook.h"
#ifdef _M_X64
#pragma comment(lib,"../op/3rd_party/lib/x64/libMinHook.x64.lib")
#else
#pragma comment(lib,"../op/3rd_party/lib/x86/libMinHook.x86.lib")
#endif // _M_X64

#include <thread>
#include<fstream>
#include <vector>

template<typename func_t>
func_t query_api(const char* mod_name, const char* func_name) {
	auto hdll = ::GetModuleHandleA(mod_name);
	if (!hdll) {
		//_error_code = -1;
		return NULL;
	}
	void* paddress = (void*)::GetProcAddress(hdll, func_name);
	if (!paddress) {
		//_error_code = -2;
		return NULL;
	}
	//_error_code = 0;
	return (func_t)paddress;

}

int capture(const std::wstring& file_name,std::vector<char>& mem,int w_,int h_) {
	//setlog(L"Bkdx::capture")
	std::fstream file;
	file.open(file_name, std::ios::out | std::ios::binary);
	if (!file.is_open())return 0;
	//_mutex.lock();
	BITMAPFILEHEADER bfh = { 0 };
	BITMAPINFOHEADER bih = { 0 };//位图信息头
	bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bfh.bfSize = bfh.bfOffBits + w_ * h_ * 4;
	bfh.bfType = static_cast<WORD>(0x4d42);

	bih.biBitCount = 32;//每个像素字节大小
	bih.biCompression = BI_RGB;
	bih.biHeight = -h_;//高度
	bih.biPlanes = 1;
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biSizeImage = w_ * h_ * 4;//图像数据大小
	bih.biWidth = w_;//宽度
	file.write((char*)&bfh, sizeof(BITMAPFILEHEADER));
	file.write((char*)&bih, sizeof(BITMAPINFOHEADER));

	
	file.write(mem.data(), bih.biSizeImage);
	

	file.close();
	

	return 1;
}
//-----------------------opengl-----------------------------
//screen capture
long opengl_screen_capture() {

	using glPixelStorei_t = decltype(glPixelStorei)*;
	using glReadBuffer_t = decltype(glReadBuffer)*;
	using glGetIntegerv_t = decltype(glGetIntegerv)*;
	using glReadPixels_t = decltype(glReadPixels)*;
	static int inhook = 0;
	auto pglPixelStorei = query_api<glPixelStorei_t>("opengl32.dll", "glPixelStorei");
	//auto pglPixelStorei = (glPixelStorei_t)query_api("opengl32.dll", "glPixelStorei");

	auto pglReadBuffer = query_api<glReadBuffer_t>("opengl32.dll", "glReadBuffer");
	//auto pglReadBuffer = (glReadBuffer_t)query_api("opengl32.dll", "glReadBuffer");

	auto pglGetIntegerv = query_api<glGetIntegerv_t>("opengl32.dll", "glGetIntegerv");
	//auto pglGetIntegerv = (glGetIntegerv_t)query_api("opengl32.dll", "glGetIntegerv");

	auto pglReadPixels = query_api<glReadPixels_t>("opengl32.dll", "glReadPixels");
	//auto pglReadPixels = (glReadPixels_t)query_api("opengl32.dll", "glReadPixels");
	if (!pglPixelStorei || !pglReadBuffer || !pglGetIntegerv || !pglReadPixels) {
		if (!inhook) {
			if (pglPixelStorei == nullptr)
				::MessageBoxA(NULL, "!pglPixelStorei", "", 0);
			else if (glReadBuffer == nullptr) {
				::MessageBoxA(NULL, "!pgl2", "", 0);
			}
			else if (!pglGetIntegerv) {
				::MessageBoxA(NULL, "!pgl3", "", 0);
			}
			else {
				::MessageBoxA(NULL, "!pgl4", "", 0);
			}
		}
		inhook = 1;
		return 0;
	}
	GLint viewport[4] = { 0 };
	pglGetIntegerv(GL_VIEWPORT, viewport);
	int width = viewport[2], height = viewport[3];

	pglPixelStorei(GL_PACK_ALIGNMENT, 1);
	pglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	pglReadBuffer(GL_FRONT);
	std::vector<char> mem;
	mem.resize(100 * 50 * 4);
	pglReadPixels(0, 0, 100, 50, GL_BGRA_EXT, GL_UNSIGNED_BYTE, mem.data());
	//
	if(!capture(L"E:\\project\\op\\bin\\x86\\sc.bmp", mem, 100, 50))
		::MessageBoxA(NULL, "not ok", "", 0);
	
	if (!inhook) {
		inhook = 1;
		//::MessageBoxA(NULL, "inhook", "", 0);
	}
	return 0;
}
// hook glBegin or wglSwapLayerBuffers
using wglSwapBuffers_t = void(__stdcall*) (HDC hdc);

using eglSwapBuffers_t = BOOL(__stdcall*) (HDC hdc,int surface);

//wglSwapBuffers_t old_address;

eglSwapBuffers_t old_address;

void __stdcall gl_hkwglSwapBuffers(HDC hdc) {
	
	opengl_screen_capture();
	((wglSwapBuffers_t)old_address)(hdc);
}

extern "C" __declspec(dllexport) long SetXHook() {
	::MessageBoxA(NULL, "start_hook ok", "", 0);
	//auto target = query_api<wglSwapBuffers_t>("opengl32.dll", "wglSwapBuffers");
	auto target = query_api<wglSwapBuffers_t>("opengl32.dll", "wglSwapBuffers");
	if (!target) { 
		::MessageBoxA(NULL, "!target", "", 0);
		return 0;
	}
	MH_Initialize();
	//MH_CreateHook((void*)target, gl_hkwglSwapBuffers, (void**)&old_address);
	MH_CreateHook((void*)target, gl_hkwglSwapBuffers, (void**)&old_address);
	MH_EnableHook((void*)target);
	return 1;
}



BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		
		break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

