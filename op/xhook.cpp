#include "stdafx.h"
#include "xhook.h"
#include <d3d9.h>
#include <d3d10.h>

#include "./include/sharedmem.h"
#include "./include/promutex.h"
#include <exception>
#ifdef _M_X64
#pragma comment(lib,"./3rd_party/lib/x64/libMinHook.x64.lib")
#else
#pragma comment(lib,"./3rd_party/lib/x86/libMinHook.x86.lib")
#endif // _M_X64


#include "3rd_party/kiero/kiero.h"
#include <gl\gl.h>
#include <gl\glu.h>
#include "Tool.h"

/*name of ...*/
wchar_t g_shared_res_name[256];
wchar_t g_mutex_name[256];



/*target window hwnd*/
HWND g_hwnd = NULL;

typedef long(__stdcall* EndScene)(LPDIRECT3DDEVICE9);

//
EndScene g_oEndScene = nullptr;
//dx9 hooked screen capture
HRESULT dx9screen_capture(LPDIRECT3DDEVICE9 pDevice) {
	//save bmp
	HRESULT hr = NULL;
	IDirect3DSurface9 *pSurface;
	hr = pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pSurface);
	if (FAILED(hr)) return hr;

	D3DSURFACE_DESC surface_Desc;
	hr = pSurface->GetDesc(&surface_Desc);
	if (FAILED(hr)) return 0;

	IDirect3DTexture9 *pTex = NULL;
	IDirect3DSurface9 *pTexSurface = NULL;
	pDevice->CreateTexture(surface_Desc.Width,
		surface_Desc.Height,
		1,
		0,
		surface_Desc.Format,
		D3DPOOL_SYSTEMMEM, //必须为这个
		&pTex, NULL);
	if (pTex)
		hr = pTex->GetSurfaceLevel(0, &pTexSurface);
	if (pTexSurface)
		hr = pDevice->GetRenderTargetData(pSurface, pTexSurface);

	D3DLOCKED_RECT lockedRect;

	pTex->LockRect(0, &lockedRect, NULL, D3DLOCK_READONLY);
	/*取像素*/
	sharedmem mem;
	promutex mutex;
	if (mem.open(g_shared_res_name)&&mutex.open(g_mutex_name)) {
		mutex.lock();
		memcpy(mem.data<byte>(), (byte*)lockedRect.pBits, lockedRect.Pitch*surface_Desc.Height);
		mutex.unlock();
	}
	pTex->UnlockRect(0);
	//D3DXSaveTextureToFile(file, D3DXIFF_BMP, pTex, NULL);
	pSurface->Release();
	//setlog(L"memcpy end.");
	return hr;
}

//dx9 hooked EndScene function
HRESULT STDMETHODCALLTYPE hkEndScene(IDirect3DDevice9* thiz)
{
	auto ret = g_oEndScene(thiz);
	//每隔段时间调用截图
	static DWORD t = 0;
	if (::GetTickCount() - t > 500) {
		t = ::GetTickCount();
		dx9screen_capture(thiz);
	}
	return ret;
}
//dx10
void  STDMETHODCALLTYPE hkDraw(ID3D10Device* thiz, unsigned int VertexCount, unsigned int StartVertexLocation) {
	thiz->Draw(VertexCount, StartVertexLocation);
	//thiz.
}
//dx11

//opengl
using glEnd_t = decltype(glEnd)*;
using glReadPixels_t = decltype(glReadPixels)*;
//using glReadPixels_t = decltype(glReadPixels)*;
glEnd_t g_oglEnd = nullptr;
long opengl_screen_capture() {
	RECT rc;
	::GetClientRect(g_hwnd, &rc);

	auto pglReadPixels = (glReadPixels_t)kiero::getMethodsTable()[237];
	sharedmem mem;
	promutex mutex;
	if (mem.open(g_shared_res_name)&&mutex.open(g_mutex_name)) {
		mutex.lock();
		pglReadPixels(0, 0, rc.right - rc.left, rc.bottom - rc.top, GL_BGRA_EXT, GL_UNSIGNED_BYTE, mem.data<byte>());
		mutex.unlock();
	}

	return 0;
}
void __stdcall hkglEnd(void) {
	static DWORD t = 0;
	g_oglEnd();
	//capture
	if (GetTickCount() > t + 500)
		opengl_screen_capture();
}
void hook_init(HWND hwnd) {
	g_hwnd = hwnd;
	wsprintf(g_shared_res_name, SHARED_RES_NAME_FORMAT, hwnd);
	wsprintf(g_mutex_name, MUTEX_NAME_FORMAT, hwnd);
	
}

void hook_release() {
	//
}

//export function
long SetDX9Hook(HWND hwnd) {

	//setlog(L"step 1. init hook.");
	//initHook(hwnd);
	hook_init(hwnd);
	//step 2. init hook
	auto ret = kiero::init(kiero::RenderType::D3D9);
	//step 3. bind function
	if (ret == kiero::Status::Success) {
		kiero::bind(42, (void**)&g_oEndScene, hkEndScene);
		//setlog("bind ok.");
	}
	else {
		setlog("kiero::init false");
	}


	return 1;
}

long UnDX9Hook() {
	if (g_oEndScene) {
		kiero::unbind();
	}
	hook_release();
	return 0;
}

long SetDX10Hook(HWND hwnd) {
	hook_init(hwnd);
	//step 2. init hook
	auto ret = kiero::init(kiero::RenderType::D3D10);
	//step 3. bind function
	if (ret == kiero::Status::Success) {
		//kiero::bind(42, (void**)&g_oEndScene, hkEndScene);
		return 1;
	}
	else {
		setlog("kiero::init false");
		return 0;
	}

}

long UnDX10Hook() {
	kiero::unbind();
	hook_release();
	return 0;
}

long SetDX11Hook(HWND hwnd) {
	hook_init(hwnd);
	//step 2. init hook
	auto ret = kiero::init(kiero::RenderType::D3D11);
	//step 3. bind function
	if (ret == kiero::Status::Success) {
		//kiero::bind(42, (void**)&g_oEndScene, hkEndScene);
		return 1;
	}
	else {
		setlog("kiero::init false");
		return 0;
	}

}

long UnDX11Hook() {
	kiero::unbind();
	hook_release();
	return 0;
}

long SetOpenglHook(HWND hwnd) {
	hook_init(hwnd);
	//step 2. init hook
	auto ret = kiero::init(kiero::RenderType::OpenGL);
	//step 3. bind function
	if (ret == kiero::Status::Success) {
		kiero::bind(74, (void**)&g_oglEnd, hkglEnd);
		return 1;
	}
	else {
		setlog("kiero::init false");
		return 0;
	}

}

long UnOpenglHook() {
	kiero::unbind();
	hook_release();
	return 0;
}