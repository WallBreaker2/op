#include "stdafx.h"
#include "xhook.h"

#include <d3d11.h>
#include <D3DX11.h>
#include <d3d10.h>
#include <d3dx10.h>
#include <d3d9.h>
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
#include "query_api.h"
#include <wingdi.h>
HWND xhook::render_hwnd = NULL;
int xhook::render_type = 0;
wchar_t xhook::shared_res_name[256];
wchar_t xhook::mutex_name[256];
void* xhook::old_address;

//dx9 hooked EndScene function
HRESULT __stdcall dx9_hkEndScene(IDirect3DDevice9* thiz);
//dx10
HRESULT __stdcall dx10_hkPresent(IDXGISwapChain* thiz, UINT SyncInterval, UINT Flags);
//dx11
HRESULT __stdcall dx11_hkPresent(IDXGISwapChain* thiz, UINT SyncInterval, UINT Flags);
//opengl
void __stdcall gl_hkglBegin(GLenum mode);

void __stdcall gl_hkwglSwapBuffers(HDC hdc);

int xhook::init(HWND hwnd_, int render_type_, int render_flag_) {
	xhook::render_hwnd = hwnd_;
	wsprintf(xhook::shared_res_name, SHARED_RES_NAME_FORMAT, hwnd_);
	wsprintf(xhook::mutex_name, MUTEX_NAME_FORMAT, hwnd_);
	//if(bktype_)
	if (render_type_ == RENDER_TYPE::DX) {
		if (render_flag_ == RENDER_FLAG::D3D9)
			render_type = kiero::RenderType::D3D9;
		else if (render_flag_ == RENDER_FLAG::D3D10)
			render_type = kiero::RenderType::D3D10;
		else if (render_flag_ == RENDER_FLAG::D3D11)
			render_type = kiero::RenderType::D3D11;
		else
			render_type = kiero::RenderType::D3D9;
	}
	else if (render_type_ == RENDER_TYPE::OPENGL) {
		render_type = kiero::RenderType::OpenGL;
	}
	else {
		render_type = kiero::RenderType::None;
	}
	if (kiero::init(render_type) != kiero::Status::Success)
		return 0;
	return 1;
}


static int need_screen;

int xhook::detour() {
	int idx = 0;
	void* address = nullptr;
	using kiero::RenderType;
	switch (render_type)
	{
	case RenderType::D3D9:
		idx = 42; address = dx9_hkEndScene;
		break;
	case RenderType::D3D10:
		idx = 8; address = dx10_hkPresent;
		break;
	case RenderType::D3D11:
		idx = 8; address = dx11_hkPresent;
		break;
	case RenderType::OpenGL:
		idx = 2; address = gl_hkwglSwapBuffers;
		break;
	default:
		break;
	}


	 need_screen=kiero::bind(idx, &old_address, address);
	 return need_screen;
	
}

int xhook::release() {
	need_screen = 0;
	int ret = kiero::unbind();
	//setlog(MH)
	return 1;
}


//------------------------dx9-------------------------------
//screen capture
HRESULT dx9screen_capture(LPDIRECT3DDEVICE9 pDevice) {
	//save bmp
	setlog("dx9screen_capture");
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
	if (mem.open(xhook::shared_res_name)&&mutex.open(xhook::mutex_name)) {
		mutex.lock();
		memcpy(mem.data<byte>(), (byte*)lockedRect.pBits, lockedRect.Pitch*surface_Desc.Height);
		mutex.unlock();
	}
	pTex->UnlockRect(0);
	//D3DXSaveTextureToFile(file, D3DXIFF_BMP, pTex, NULL);
	pSurface->Release();
	pTex->Release();
	pTexSurface->Release();
	//setlog(L"memcpy end.");
	return hr;
}
//dx9 hooked EndScene function
HRESULT STDMETHODCALLTYPE dx9_hkEndScene(IDirect3DDevice9* thiz)
{
	typedef long(__stdcall* EndScene)(LPDIRECT3DDEVICE9);
	auto ret = ((EndScene)xhook::old_address)(thiz);
	if (need_screen)
		dx9screen_capture(thiz);
	
	return ret;
}
//------------------------------------------------------------

//-----------------------dx10----------------------------------
//screen capture
void dx10_capture(IDXGISwapChain* pswapchain) {
	setlog("dx10_capture");
	using Texture2D = ID3D10Texture2D * ;
	//init some fucntion
	
	static auto pD3DX10SaveTextureToMemory =(decltype(D3DX10SaveTextureToMemory)*)query_api("d3dx10_43.dll","D3DX10SaveTextureToMemory");
	if (!pD3DX10SaveTextureToMemory) {
		setlog("!pD3DX10SaveTextureToMemory");
		return;
	}

	HRESULT hr;
	ID3D10Device *pdevices = nullptr;
	ID3D10Texture2D* texture = nullptr;
	Texture2D textureDest = nullptr;
	LPD3D10BLOB pblob = nullptr;
	
	//setlog("before GetBuffer");
	hr =pswapchain->GetBuffer(0, __uuidof(ID3D10Texture2D), (void**)&texture);
	if (hr < 0) {
		//setlog("hr < 0");
		return;
	}
	texture->GetDevice(&pdevices);
	
	if (!pdevices) {
		//setlog(" pswapchain->GetDevice false");
		texture->Release();
		return;
	}
	//auto p

	D3D10_TEXTURE2D_DESC desc;
	texture->GetDesc(&desc);
	// If texture is multisampled, then we can use ResolveSubresource to copy it into a non-multisampled texture
	//Texture2D textureResolved = nullptr;
	if (desc.SampleDesc.Count > 1) {
		// texture is multi-sampled, lets resolve it down to single sample
		setlog("texture is multi-sampled");
	}
	
	D3D10_TEXTURE2D_DESC desc2;
	desc2.CPUAccessFlags = DXGI_CPU_ACCESS_NONE;
	desc2.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc2.Height = desc.Height;
	desc2.Usage = D3D10_USAGE_DEFAULT;
	desc2.Width = desc.Width;
	desc2.ArraySize = 1;
	desc2.SampleDesc = DXGI_SAMPLE_DESC{ 1,0 };
	desc2.BindFlags = 0;
	desc2.MipLevels = 1;
	desc2.MiscFlags = desc.MiscFlags;
	hr = pdevices->CreateTexture2D(&desc2, nullptr, &textureDest);
	if (hr < 0) {
		pdevices->Release();
		texture->Release();
		return;
	}
	
	D3D10_BOX box = {};
	box.top = 0; box.left = 0;
	box.bottom = desc.Height;
	box.right = desc.Width;
	box.front = 0; box.back = 1;
	pdevices->CopySubresourceRegion(textureDest, 0, 0, 0, 0, texture, 0, &box);
	
	hr = pD3DX10SaveTextureToMemory(textureDest, D3DX10_IMAGE_FILE_FORMAT::D3DX10_IFF_BMP, &pblob, 0);
	if (hr < 0) {
		pdevices->Release();
		texture->Release();
		textureDest->Release();
		setlog("hr = pD3DX10SaveTextureToMemory false,hr=%d",hr);
		return;
	}
	//0x7FFF BFFB -2147467259
	//setlog("after D3DX10SaveTextureToMemory");

	sharedmem mem;
	promutex mutex;
	if (mem.open(xhook::shared_res_name) && mutex.open(xhook::mutex_name)) {
		char* ptr = (char*)pblob->GetBufferPointer();
		size_t bits = pblob->GetBufferSize();
		constexpr int offset = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		bits -= offset;
		//setlog("num of  bits=%d,width*height*4=%d", bits, desc.Width*desc.Height * 4);
		if (bits > SHARED_MEMORY_SIZE)
			bits = SHARED_MEMORY_SIZE;
		
		mutex.lock();
		memcpy(mem.data<char>(), ptr+offset,bits);
		mutex.unlock();
	}
	else {
		setlog("mem.open(xhook::shared_res_name) && mutex.open(xhook::mutex_name)");
	}
	//release
	pdevices->Release();
	texture->Release();
	textureDest->Release();
	pblob->Release();
	//setlog("pblob->Release()");
}
//dx10 hook Present
HRESULT STDMETHODCALLTYPE dx10_hkPresent(IDXGISwapChain* thiz,UINT SyncInterval,UINT Flags){
	typedef long(__stdcall* Present_t)(IDXGISwapChain* pswapchain, UINT x1, UINT x2);
	if (need_screen)
		dx10_capture(thiz);
	return ((Present_t)xhook::old_address)(thiz, SyncInterval, Flags);
	//thiz.
}
//------------------------------------------------------------

//------------------------dx11----------------------------------
//screen capture
void dx11_capture(IDXGISwapChain* pswapchain) {
	//query api
	setlog("dx11_capture");
	static auto pD3DX11SaveTextureToMemory = (decltype(D3DX11SaveTextureToMemory)*)query_api("d3dx11_43.dll", "D3DX11SaveTextureToMemory");
	if (!pD3DX11SaveTextureToMemory) {
		setlog("!pD3DX11SaveTextureToMemory");
		return;
	}
		
	using Texture2D = ID3D11Texture2D * ;
	HRESULT hr = 0;
	Texture2D texture = nullptr, textureDst = nullptr;
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;
	ID3D10Blob* pblob = nullptr;
	hr = pswapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&texture);
	if (hr < 0) {
		setlog("pswapchain->GetBuffer,error code=%d", hr);
		return;
	}

	D3D11_TEXTURE2D_DESC desc;
	texture->GetDesc(&desc);
	texture->GetDevice(&device);
	D3D11_TEXTURE2D_DESC desc2 = { 0 };
	desc2.CPUAccessFlags = 0;
	desc2.Format = desc.Format;
	desc2.Height = desc.Height;
	desc2.Usage = D3D11_USAGE_DEFAULT;
	desc2.Width = desc.Width;
	desc2.ArraySize = 1;
	desc2.SampleDesc = DXGI_SAMPLE_DESC{ 1,0 };
	desc2.BindFlags = 0;
	desc2.MipLevels = 1;
	desc2.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
	hr = device->CreateTexture2D(&desc2, nullptr, &textureDst);
	if (hr < 0) {

		texture->Release();
		device->Release();
		setlog("device->CreateTexture2D,error code=%d", hr);
		return;
	}
	//DXGI_KEY
	device->GetImmediateContext(&context);
	D3D11_BOX box = { 0 };
	box.top = box.left = 0;
	box.bottom = desc.Height;
	box.right = desc.Width;
	box.front = 0;
	box.back = 1;
	context->CopySubresourceRegion(textureDst, 0, 0, 0, 0, texture, 0, &box);
	
	hr = pD3DX11SaveTextureToMemory(context, textureDst, D3DX11_IMAGE_FILE_FORMAT::D3DX11_IFF_BMP, &pblob, 0);
	if (hr < 0) {
		setlog("D3DX11SaveTextureToMemory error code=%d", hr);
	}
	else {
		sharedmem mem;
		promutex mutex;
		if (mem.open(xhook::shared_res_name) && mutex.open(xhook::mutex_name)) {
			char* ptr = (char*)pblob->GetBufferPointer();
			size_t bits = pblob->GetBufferSize();
			constexpr int offset = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
			bits -= offset;
			//setlog("num of  bits=%d,width*height*4=%d", bits, desc.Width*desc.Height * 4);
			if (bits > SHARED_MEMORY_SIZE)
				bits = SHARED_MEMORY_SIZE;

			mutex.lock();
			memcpy(mem.data<char>(), ptr + offset, bits);
			mutex.unlock();
		}
		else {
			setlog("mem.open(xhook::shared_res_name) && mutex.open(xhook::mutex_name)");
		}
	}
	texture->Release();
	device->Release();
	textureDst->Release();
	context->Release();
	if (pblob)pblob->Release();
}
//hooked present
HRESULT __stdcall dx11_hkPresent(IDXGISwapChain* thiz, UINT SyncInterval, UINT Flags) {
	typedef long(__stdcall* Present_t)(IDXGISwapChain* pswapchain, UINT x1, UINT x2);
	if (need_screen)
		dx11_capture(thiz);
	return ((Present_t)xhook::old_address)(thiz, SyncInterval, Flags);
}
//------------------------------------------------------------

//-----------------------opengl-----------------------------
//screen capture
long opengl_screen_capture() {
	using glPixelStorei_t = decltype(glPixelStorei)*;
	using glReadBuffer_t = decltype(glReadBuffer)*;
	using glGetIntegerv_t = decltype(glGetIntegerv)*;
	using glReadPixels_t = decltype(glReadPixels)*;

	auto pglPixelStorei = (glPixelStorei_t)query_api("opengl32.dll", "glPixelStorei");
	auto pglReadBuffer = (glReadBuffer_t)query_api("opengl32.dll", "glReadBuffer");
	auto pglGetIntegerv = (glGetIntegerv_t)query_api("opengl32.dll", "glGetIntegerv");
	auto pglReadPixels = (glReadPixels_t)query_api("opengl32.dll", "glReadPixels");
	if (!pglPixelStorei || !pglReadBuffer || !pglGetIntegerv || !pglReadPixels) {
		need_screen = 0;
		setlog("error.!pglPixelStorei || !pglReadBuffer || !pglGetIntegerv || !pglReadPixels");
		return 0;
	}
	RECT rc;
	::GetClientRect(xhook::render_hwnd, &rc);
	int width = rc.right-rc.left, height = rc.bottom-rc.top;

	pglPixelStorei(GL_PACK_ALIGNMENT, 1);
	pglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	pglReadBuffer(GL_FRONT);

	sharedmem mem;
	promutex mutex;
	if (mem.open(xhook::shared_res_name)&&mutex.open(xhook::mutex_name)) {
		mutex.lock();
		pglReadPixels(0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, mem.data<byte>());
		mutex.unlock();
	}
	else {
		need_screen = 0;
		setlog("!mem.open(xhook::shared_res_name)&&mutex.open(xhook::mutex_name)");
	}
	//setlog("gl screen ok");
	return 0;
}
// hook glBegin or wglSwapLayerBuffers

void __stdcall gl_hkglBegin(GLenum mode) {
	static DWORD t = 0;
	using glBegin_t = decltype(glBegin)*;
	
	if (need_screen)
		opengl_screen_capture();
	((glBegin_t)xhook::old_address)(mode);
}

void __stdcall gl_hkwglSwapBuffers(HDC hdc) {
	using wglSwapBuffers_t = void (__stdcall*) (HDC hdc);
	if (need_screen)
		opengl_screen_capture();
	((wglSwapBuffers_t)xhook::old_address)(hdc);
}



//--------------export function--------------------------
long SetXHook(HWND hwnd_, int bktype_) {
	if (xhook::init(hwnd_, GET_RENDER_TYPE(bktype_), GET_RENDER_FLAG(bktype_)) != 1)
		return 0;
	setlog("in hook,hwnd=%d,bktype=%d", hwnd_, bktype_);
	return xhook::detour();
}

long UnXHook() {
	
	return xhook::release();
}

