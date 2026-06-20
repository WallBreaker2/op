#include "DisplayHook.h"
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3d10.h>
#include <d3d10_1.h>
// #include <D3DX11.h>

// #include <d3dx10.h>
#include "../ipc/ProcessMutex.h"
#include "../ipc/SharedMemory.h"
#include <d3d9.h>
#include <ddraw.h>
#include <exception>

#include "../../3rd_party/include/kiero.h"
#include "../capture/FrameInfo.h"
#include "../runtime/AutomationModes.h"
#include "../runtime/RuntimeUtils.h"
#include "../runtime/RuntimeEnvironment.h"
#include "../hook/ApiResolver.h"
#include "Dx12Hook.h"
#include <atlbase.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <wingdi.h>
#define DEBUG_HOOK 0

namespace op::hook {

using op::capture::FrameInfo;

HWND DisplayHook::render_hwnd = NULL;
int DisplayHook::render_type = 0;
std::wstring DisplayHook::shared_res_name;
std::wstring DisplayHook::mutex_name;
void *DisplayHook::old_address;
bool DisplayHook::is_hooked = false;
static int is_capture;

using ATL::CComPtr;

class D3D9TextureLock {
  public:
    explicit D3D9TextureLock(IDirect3DTexture9 *texture) : texture_(texture) {
    }

    ~D3D9TextureLock() {
        if (locked_) {
            texture_->UnlockRect(0);
        }
    }

    D3D9TextureLock(const D3D9TextureLock &) = delete;
    D3D9TextureLock &operator=(const D3D9TextureLock &) = delete;

    HRESULT lock(D3DLOCKED_RECT *lockedRect) {
        if (!texture_ || !lockedRect) {
            return E_POINTER;
        }
        HRESULT hr = texture_->LockRect(0, lockedRect, nullptr, D3DLOCK_READONLY);
        if (hr >= 0) {
            locked_ = true;
        }
        return hr;
    }

  private:
    IDirect3DTexture9 *texture_;
    bool locked_ = false;
};

class D3D10TextureMap {
  public:
    explicit D3D10TextureMap(ID3D10Texture2D *texture) : texture_(texture) {
    }

    ~D3D10TextureMap() {
        if (mapped_) {
            texture_->Unmap(0);
        }
    }

    D3D10TextureMap(const D3D10TextureMap &) = delete;
    D3D10TextureMap &operator=(const D3D10TextureMap &) = delete;

    HRESULT map(D3D10_MAPPED_TEXTURE2D *mapped) {
        if (!texture_ || !mapped) {
            return E_POINTER;
        }
        HRESULT hr = texture_->Map(0, D3D10_MAP_READ, 0, mapped);
        if (hr >= 0) {
            mapped_ = true;
        }
        return hr;
    }

  private:
    ID3D10Texture2D *texture_;
    bool mapped_ = false;
};

class D3D11TextureMap {
  public:
    D3D11TextureMap(ID3D11DeviceContext *context, ID3D11Resource *resource) : context_(context), resource_(resource) {
    }

    ~D3D11TextureMap() {
        if (mapped_) {
            context_->Unmap(resource_, 0);
        }
    }

    D3D11TextureMap(const D3D11TextureMap &) = delete;
    D3D11TextureMap &operator=(const D3D11TextureMap &) = delete;

    HRESULT map(D3D11_MAPPED_SUBRESOURCE *mapped) {
        if (!context_ || !resource_ || !mapped) {
            return E_POINTER;
        }
        HRESULT hr = context_->Map(resource_, 0, D3D11_MAP_READ, 0, mapped);
        if (hr >= 0) {
            mapped_ = true;
        }
        return hr;
    }

  private:
    ID3D11DeviceContext *context_;
    ID3D11Resource *resource_;
    bool mapped_ = false;
};

// dx9 hooked EndScene function
HRESULT __stdcall dx9_hkEndScene(IDirect3DDevice9 *thiz);
// dx10
HRESULT __stdcall dx10_hkPresent(IDXGISwapChain *thiz, UINT SyncInterval, UINT Flags);
// dx11
HRESULT __stdcall dx11_hkPresent(IDXGISwapChain *thiz, UINT SyncInterval, UINT Flags);
// dx12
HRESULT __stdcall dx12_hkPresent(IDXGISwapChain *thiz, UINT SyncInterval, UINT Flags);
// opengl_std
void __stdcall gl_hkglBegin(GLenum mode);
// opengl dn,nox
void __stdcall gl_hkwglSwapBuffers(HDC hdc);
// egl
unsigned int __stdcall gl_hkeglSwapBuffers(void *dpy, void *surface);
// glfinish
void __stdcall gl_hkglFinish(void);

int DisplayHook::setup(HWND hwnd_, int render_type_) {
    DisplayHook::render_hwnd = hwnd_;
    DisplayHook::shared_res_name = MakeOpSharedResourceName(hwnd_);
    DisplayHook::mutex_name = MakeOpMutexName(hwnd_);

    int idx = 0;
    void *address = nullptr;
    if (render_type_ == RDT_DX_DEFAULT || render_type_ == RDT_DX_D3D9) {

        render_type = kiero::RenderType::D3D9;
        idx = 42;
        address = dx9_hkEndScene;
    } else if (render_type_ == RDT_DX_D3D10) {
        render_type = kiero::RenderType::D3D10;
        idx = 8;
        address = dx10_hkPresent;
    } else if (render_type_ == RDT_DX_D3D11) {
        render_type = kiero::RenderType::D3D11;
        idx = 8;
        address = dx11_hkPresent;
    } else if (render_type_ == RDT_DX_D3D12) {
        render_type = kiero::RenderType::D3D12;
        idx = 140;
        address = dx12_hkPresent;
    } else if (render_type_ == RDT_GL_DEFAULT || render_type_ == RDT_GL_NOX) {
        render_type = kiero::RenderType::OpenGL;
        idx = 2;
        address = gl_hkwglSwapBuffers;
    } else if (render_type_ == RDT_GL_STD) {
        render_type = kiero::RenderType::OpenGL;
        idx = 0;
        address = gl_hkglBegin;
    } else if (render_type_ == RDT_GL_ES) {
        render_type = kiero::RenderType::OpenglES;
        idx = 0;
        address = gl_hkeglSwapBuffers;
    } else if (render_type_ == RDT_GL_FI) {
        render_type = kiero::RenderType::OpenGL;
        idx = 3;
        address = gl_hkglFinish;

    } else {
        render_type = kiero::RenderType::None;
    }
    kiero::Status::Enum ret = kiero::init(render_type);
    if (ret != kiero::Status::Success) {
        return ret;
    }

    is_capture = kiero::bind(idx, &old_address, address);
    return is_capture;
}

int DisplayHook::release() {
    is_capture = 0;
    kiero::unbind();
    kiero::shutdown();
    old_address = nullptr;
    render_hwnd = NULL;
    render_type = kiero::RenderType::None;
    return 1;
}

void CopyImageData(char *dst_, const char *src_, int rows_, int cols_, int rowPitch, int fmt_) {
    // assert(rowsPitch >= cols_ * 4);
    if (rowPitch == cols_ * (fmt_ == IBF_R8G8B8 ? 3 : 4)) {
        if (fmt_ == IBF_B8G8R8A8) {
            ::memcpy(dst_, src_, rows_ * cols_ * 4);
        } else if (fmt_ == IBF_R8G8B8A8) {
            // pixels count
            int n = rows_ * cols_;

            for (int i = 0; i < n; ++i) {
                dst_[0] = src_[2]; // b
                dst_[1] = src_[1]; // g
                dst_[2] = src_[0]; // r
                dst_[3] = src_[3]; // a
                dst_ += 4;
                src_ += 4;
            }
        } else {
            // pixels count
            int n = rows_ * cols_;
            for (int i = 0; i < n; ++i) {
                *dst_++ = *src_++;
                *dst_++ = *src_++;
                *dst_++ = *src_++;
                *dst_++ = (char)0xff; // dst is 4 B
            }
        }
    } else {
        const int dstPitch = cols_ * 4;
        if (fmt_ == IBF_B8G8R8A8) {
            for (int i = 0; i < rows_; ++i) {
                ::memcpy(dst_, src_, dstPitch);
                dst_ += dstPitch;
                src_ += rowPitch;
            }

        } else if (fmt_ == IBF_R8G8B8A8) {
            // pixels count

            for (int i = 0; i < rows_; ++i) {
                for (int j = 0; j < cols_; ++j) {
                    const char *p = src_ + j * 4; // offset
                    dst_[0] = p[2];               // b
                    dst_[1] = p[1];               // g
                    dst_[2] = p[0];               // r
                    dst_[3] = p[3];               // a
                    dst_ += 4;                    // notirc that dst ptr is increasing
                }
                src_ += rowPitch; // row increase
            }

        } else {
            for (int i = 0; i < rows_; ++i) {
                for (int j = 0; j < cols_; ++j) {
                    const char *p = src_ + j * 3; // offset
                    dst_[0] = p[0];               // b
                    dst_[1] = p[1];               // g
                    dst_[2] = p[2];               // r
                    dst_[3] = (char)0xff;         // a
                    dst_ += 4;                    // notice that dst ptr is increasing
                }
                src_ += rowPitch; // row increase
            }
        }
    }
}

static DXGI_FORMAT GetDxgiFormat(DXGI_FORMAT format) {
    if (format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB) {
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    }
    if (format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) {
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    return format;
}

//------------------------dx9-------------------------------
// screen capture
HRESULT dx9_capture(LPDIRECT3DDEVICE9 pDevice) {
    // save bmp
    // setlog("dx9screen_capture");
    HRESULT hr = NULL;
    CComPtr<IDirect3DSurface9> pSurface;
    hr = pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pSurface);
    if (FAILED(hr))
        return hr;

    D3DSURFACE_DESC surface_Desc;
    hr = pSurface->GetDesc(&surface_Desc);
    if (FAILED(hr))
        return hr;

    CComPtr<IDirect3DTexture9> pTex;
    CComPtr<IDirect3DSurface9> pTexSurface;
    hr = pDevice->CreateTexture(surface_Desc.Width, surface_Desc.Height, 1, 0, surface_Desc.Format,
                                D3DPOOL_SYSTEMMEM, // 必须为这个
                                &pTex, NULL);
    if (hr < 0) {
        return hr;
    }
    hr = pTex->GetSurfaceLevel(0, &pTexSurface);
    if (hr < 0)
        return hr;
    hr = pDevice->GetRenderTargetData(pSurface, pTexSurface);
    if (FAILED(hr))
        return hr;

    D3DLOCKED_RECT lockedRect = {};

    D3D9TextureLock textureLock(pTex);
    hr = textureLock.lock(&lockedRect);
    if (FAILED(hr))
        return hr;
    // 取像素
    SharedMemory mem;
    ProcessMutex mutex;
    if (mem.open(DisplayHook::shared_res_name) && mutex.open(DisplayHook::mutex_name)) {
        mutex.lock();
        uchar *pshare = mem.data<byte>();
        reinterpret_cast<FrameInfo *>(pshare)->format(DisplayHook::render_hwnd, surface_Desc.Width,
                                                      surface_Desc.Height);
        memcpy(pshare + sizeof(FrameInfo), (byte *)lockedRect.pBits, lockedRect.Pitch * surface_Desc.Height);
        mutex.unlock();
    }

    return hr;
}
// dx9 hooked EndScene function
HRESULT STDMETHODCALLTYPE dx9_hkEndScene(IDirect3DDevice9 *thiz) {
    typedef long(__stdcall * EndScene)(LPDIRECT3DDEVICE9);
    auto ret = ((EndScene)DisplayHook::old_address)(thiz);
    if (is_capture)
        dx9_capture(thiz);

    return ret;
}
//------------------------------------------------------------

//-----------------------dx10----------------------------------
// screen capture
void dx10_capture(IDXGISwapChain *pswapchain) {
    HRESULT hr;
    CComPtr<ID3D10Device> pdevices;
    CComPtr<ID3D10Resource> backbuffer;
    CComPtr<ID3D10Texture2D> textDst;
    // LPD3D10BLOB pblob = nullptr;

    // setlog("before GetBuffer");
    hr = pswapchain->GetBuffer(0, __uuidof(ID3D10Resource), reinterpret_cast<void **>(&backbuffer.p));
    if (hr < 0) {
        setlog("pswapchain->GetBuffer error code=%d", hr);
        is_capture = 0;
        return;
    }
    backbuffer->GetDevice(&pdevices);

    if (!pdevices) {
        // setlog(" pswapchain->GetDevice false");
        is_capture = 0;
        return;
    }
    // auto p

    DXGI_SWAP_CHAIN_DESC desc;
    hr = pswapchain->GetDesc(&desc);
    if (hr < 0) {
        setlog("pswapchain->GetDesc error code=%d", hr);
        is_capture = 0;
        return;
    }
    // backbuffer->GetDesc(&desc);
    //  If texture is multisampled, then we can use ResolveSubresource to copy it into a non-multisampled texture
    // Texture2D textureResolved = nullptr;

    D3D10_TEXTURE2D_DESC textDesc = {};
    textDesc.Format = GetDxgiFormat(desc.BufferDesc.Format);
    textDesc.Width = desc.BufferDesc.Width;
    textDesc.Height = desc.BufferDesc.Height;
    textDesc.MipLevels = 1;
    textDesc.ArraySize = 1;
    textDesc.SampleDesc.Count = 1;
    textDesc.Usage = D3D10_USAGE_STAGING;
    textDesc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
    hr = pdevices->CreateTexture2D(&textDesc, nullptr, &textDst);
    if (hr < 0) {
        setlog("pdevices->CreateTexture2D error code=%d", hr);
        is_capture = 0;
        return;
    }

    pdevices->CopyResource(textDst, backbuffer);

    D3D10_MAPPED_TEXTURE2D mapText = {0, 0};

    D3D10TextureMap mappedTexture(textDst);
    hr = mappedTexture.map(&mapText);

    // hr = pD3DX10SaveTextureToMemory(textureDest, D3DX10_IMAGE_FILE_FORMAT::D3DX10_IFF_BMP, &pblob, 0);
    if (hr < 0) {
        setlog("textDst->Map false,hr=%d", hr);
        is_capture = 0;
        return;
    }

    int fmt = IBF_R8G8B8A8;
    if (textDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM || textDesc.Format == DXGI_FORMAT_B8G8R8X8_UNORM ||
        textDesc.Format == DXGI_FORMAT_B8G8R8A8_TYPELESS || textDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB ||
        textDesc.Format == DXGI_FORMAT_B8G8R8X8_TYPELESS || textDesc.Format == DXGI_FORMAT_B8G8R8X8_UNORM_SRGB) {
        fmt = IBF_B8G8R8A8;
    }

    SharedMemory mem;
    ProcessMutex mutex;
    if (mem.open(DisplayHook::shared_res_name) && mutex.open(DisplayHook::mutex_name)) {

        mutex.lock();
        // memcpy(mem.data<char>(), mapText.pData, textDesc.Width*textDesc.Height * 4);
        uchar *pshare = mem.data<byte>();
        reinterpret_cast<FrameInfo *>(pshare)->format(DisplayHook::render_hwnd, textDesc.Width, textDesc.Height);
        CopyImageData((char *)pshare + sizeof(FrameInfo), (char *)mapText.pData, textDesc.Height, textDesc.Width,
                      mapText.RowPitch, fmt);
        mutex.unlock();
    } else {

#if DEBUG_HOOK
        setlog("mem.open(DisplayHook::shared_res_name) && mutex.open(DisplayHook::mutex_name)");
#endif // DEBUG_HOOK
    }
    // pblob->Release();
    // setlog("pblob->Release()");
}
// dx10 hook Present
HRESULT STDMETHODCALLTYPE dx10_hkPresent(IDXGISwapChain *thiz, UINT SyncInterval, UINT Flags) {
    typedef long(__stdcall * Present_t)(IDXGISwapChain * pswapchain, UINT x1, UINT x2);
    if (is_capture)
        dx10_capture(thiz);
    return ((Present_t)DisplayHook::old_address)(thiz, SyncInterval, Flags);
    // thiz.
}
//------------------------------------------------------------

//------------------------dx11----------------------------------
// screen capture
void dx11_capture(IDXGISwapChain *swapchain) {

    // setlog("d3d11 cap");
    HRESULT hr = 0;
    CComPtr<IDXGIResource> backbufferptr;
    CComPtr<ID3D11Resource> backbuffer;
    CComPtr<ID3D11Texture2D> textDst;
    CComPtr<ID3D11Device> device;
    CComPtr<ID3D11DeviceContext> context;

    hr = swapchain->GetBuffer(0, __uuidof(IDXGIResource), reinterpret_cast<void **>(&backbufferptr.p));
    if (hr < 0) {
        setlog("pswapchain->GetBuffer,error code=%X", hr);
        is_capture = 0;
        return;
    }
    hr = backbufferptr->QueryInterface(__uuidof(ID3D11Resource), reinterpret_cast<void **>(&backbuffer.p));
    if (hr < 0) {
        setlog("backbufferptr->QueryInterface,error code=%X", hr);
        is_capture = 0;
        return;
    }
    hr = swapchain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void **>(&device.p));
    if (hr < 0) {
        setlog("swapchain->GetDevice hr=%X", hr);
        is_capture = 0;
        return;
    }
    DXGI_SWAP_CHAIN_DESC desc;
    hr = swapchain->GetDesc(&desc);
    if (hr < 0) {
        setlog("swapchain->GetDesc hr=%X", hr);
        is_capture = 0;
        return;
    }

    D3D11_TEXTURE2D_DESC textDesc = {};
    textDesc.Format = GetDxgiFormat(desc.BufferDesc.Format);
    textDesc.Width = desc.BufferDesc.Width;
    textDesc.Height = desc.BufferDesc.Height;
    textDesc.MipLevels = 1;
    textDesc.ArraySize = 1;
    textDesc.SampleDesc.Count = 1;
    textDesc.Usage = D3D11_USAGE_STAGING;
    textDesc.BindFlags = 0;
    textDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    textDesc.MiscFlags = 0;
    hr = device->CreateTexture2D(&textDesc, nullptr, &textDst);
    if (hr < 0) {
        setlog("device->CreateTexture2D,error code=%d", hr);
        is_capture = 0;
        return;
    }
    // DXGI_KEY
    device->GetImmediateContext(&context);
    if (!context) {
        setlog("!context");
        is_capture = 0;
        return;
    }

    context->CopyResource(textDst, backbuffer);

    D3D11_MAPPED_SUBRESOURCE mapSubres = {0, 0, 0};

    // hr = pD3DX11SaveTextureToMemory(context, textureDst, D3DX11_IMAGE_FILE_FORMAT::D3DX11_IFF_BMP, &pblob, 0);
    D3D11TextureMap mappedTexture(context, textDst);
    hr = mappedTexture.map(&mapSubres);
    if (hr < 0) {
        setlog("context->Map error code=%d", hr);
        is_capture = 0;
        return;
    }
    // DXGI_FORMAT_R8G8B8A8_UNORM4
    int fmt = IBF_R8G8B8A8;
    if (textDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM || textDesc.Format == DXGI_FORMAT_B8G8R8X8_UNORM ||
        textDesc.Format == DXGI_FORMAT_B8G8R8A8_TYPELESS || textDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB ||
        textDesc.Format == DXGI_FORMAT_B8G8R8X8_TYPELESS || textDesc.Format == DXGI_FORMAT_B8G8R8X8_UNORM_SRGB) {
        fmt = IBF_B8G8R8A8;
    }

    SharedMemory mem;
    ProcessMutex mutex;
    static int cnt = 10;
    if (mem.open(DisplayHook::shared_res_name) && mutex.open(DisplayHook::mutex_name)) {
        mutex.lock();
        uchar *pshare = mem.data<byte>();
        reinterpret_cast<FrameInfo *>(pshare)->format(DisplayHook::render_hwnd, textDesc.Width, textDesc.Height);
        // CopyImageData((char*)pshare + sizeof(FrameInfo), (char*)mapText.pData, textDesc.Height, textDesc.Width, fmt);
        static_assert(sizeof(FrameInfo) == 28);

        CopyImageData((char *)pshare + sizeof(FrameInfo), (char *)mapSubres.pData, textDesc.Height, textDesc.Width,
                      mapSubres.RowPitch, fmt);
        mutex.unlock();
    } else {
        is_capture = 0;
#if DEBUG_HOOK
        setlog(L"!mem.open(DisplayHook::%s)&&mutex.open(DisplayHook::%s)", DisplayHook::shared_res_name.c_str(),
               DisplayHook::mutex_name.c_str());
#endif // DEBUG_HOOK
    }
    static bool first = true;
    if (first) {
        int tf = textDesc.Format;

        setlog("textDesc.Format= %d,fmt=%d textDesc.Height=%d\n textDesc.Width=%d\n  mapSubres.DepthPitch=%d\n "
               "mapSubres.RowPitch=%d\n",
               tf, fmt, textDesc.Height, textDesc.Width, mapSubres.DepthPitch, mapSubres.RowPitch);
        first = false;
    }
    // if (pblob)pblob->Release();
}

// hooked present
HRESULT __stdcall dx11_hkPresent(IDXGISwapChain *thiz, UINT SyncInterval, UINT Flags) {
    typedef long(__stdcall * Present_t)(IDXGISwapChain * pswapchain, UINT x1, UINT x2);
    if (is_capture)
        dx11_capture(thiz);
    return ((Present_t)DisplayHook::old_address)(thiz, SyncInterval, Flags);
}
//------------------------------------------------------------

//------------------------dx12----------------------------------
// screen capture
void dx12_capture(IDXGISwapChain *swapChain) {
    Dx12Hook *pinst = Dx12Hook::Get();
    pinst->CaptureFrame(swapChain);
}

// hooked present
HRESULT __stdcall dx12_hkPresent(IDXGISwapChain *thiz, UINT SyncInterval, UINT Flags) {
    typedef long(__stdcall * Present_t)(IDXGISwapChain * pswapchain, UINT x1, UINT x2);
    if (is_capture)
        dx12_capture(thiz);
    return ((Present_t)DisplayHook::old_address)(thiz, SyncInterval, Flags);
}

//-----------------------opengl-----------------------------
// screen capture
long gl_capture() {
    using glPixelStorei_t = decltype(glPixelStorei) *;
    using glReadBuffer_t = decltype(glReadBuffer) *;
    using glGetIntegerv_t = decltype(glGetIntegerv) *;
    using glReadPixels_t = decltype(glReadPixels) *;

    auto pglPixelStorei = (glPixelStorei_t)ResolveApi("opengl32.dll", "glPixelStorei");
    auto pglReadBuffer = (glReadBuffer_t)ResolveApi("opengl32.dll", "glReadBuffer");
    auto pglGetIntegerv = (glGetIntegerv_t)ResolveApi("opengl32.dll", "glGetIntegerv");
    auto pglReadPixels = (glReadPixels_t)ResolveApi("opengl32.dll", "glReadPixels");
    if (!pglPixelStorei || !pglReadBuffer || !pglGetIntegerv || !pglReadPixels) {
        is_capture = 0;
#if DEBUG_HOOK
        setlog("error.!pglPixelStorei || !pglReadBuffer || !pglGetIntegerv || !pglReadPixels");
#endif // DEBUG_HOOK

        return 0;
    }
    RECT rc;
    ::GetClientRect(DisplayHook::render_hwnd, &rc);
    int width = rc.right - rc.left, height = rc.bottom - rc.top;

    pglPixelStorei(GL_PACK_ALIGNMENT, 1);
    pglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    pglReadBuffer(GL_FRONT);

    SharedMemory mem;
    ProcessMutex mutex;
    if (mem.open(DisplayHook::shared_res_name) && mutex.open(DisplayHook::mutex_name)) {
        mutex.lock();
        uchar *pshare = mem.data<byte>();
        reinterpret_cast<FrameInfo *>(pshare)->format(DisplayHook::render_hwnd, width, height);
        pglReadPixels(0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pshare + sizeof(FrameInfo));
        mutex.unlock();
    } else {
        is_capture = 0;
#if DEBUG_HOOK
        setlog(L"egl !mem.open(DisplayHook::%s)&&mutex.open(DisplayHook::%s)",
               DisplayHook::shared_res_name.c_str(), DisplayHook::mutex_name.c_str());
#endif // DEBUG_HOOK
    }
    // setlog("gl screen ok");
    return 0;
}
// hook glBegin or wglSwapLayerBuffers

void __stdcall gl_hkglBegin(GLenum mode) {
    static DWORD t = 0;
    using glBegin_t = decltype(glBegin) *;

    if (is_capture)
        gl_capture();
    ((glBegin_t)DisplayHook::old_address)(mode);
}

void __stdcall gl_hkwglSwapBuffers(HDC hdc) {
    using wglSwapBuffers_t = void(__stdcall *)(HDC hdc);
    if (is_capture)
        gl_capture();
    ((wglSwapBuffers_t)DisplayHook::old_address)(hdc);
}

//---------------------OPENGL ES------------------------------
// es 类似 opengl 截图，只是模块不同
long egl_capture() {
    using glPixelStorei_t = decltype(glPixelStorei) *;
    using glReadBuffer_t = decltype(glReadBuffer) *;
    using glGetIntegerv_t = decltype(glGetIntegerv) *;
    using glReadPixels_t = decltype(glReadPixels) *;

    auto pglPixelStorei = (glPixelStorei_t)ResolveApi("libglesv2.dll", "glPixelStorei");
    auto pglReadBuffer = (glReadBuffer_t)ResolveApi("libglesv2.dll", "glReadBuffer");
    auto pglGetIntegerv = (glGetIntegerv_t)ResolveApi("libglesv2.dll", "glGetIntegerv");
    auto pglReadPixels = (glReadPixels_t)ResolveApi("libglesv2.dll", "glReadPixels");
    if (!pglPixelStorei || !pglReadBuffer || !pglGetIntegerv || !pglReadPixels) {
        // is_capture = 0;
#if DEBUG_HOOK
        setlog(L"egl !mem.open(DisplayHook::%s)&&mutex.open(DisplayHook::%s)",
               DisplayHook::shared_res_name.c_str(), DisplayHook::mutex_name.c_str());
#endif // DEBUG_HOOK

        return 0;
    }
    RECT rc;
    ::GetClientRect(DisplayHook::render_hwnd, &rc);
    int width = rc.right - rc.left, height = rc.bottom - rc.top;

    pglPixelStorei(GL_PACK_ALIGNMENT, 1);
    pglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    pglReadBuffer(GL_FRONT);

    SharedMemory mem;
    ProcessMutex mutex;
    if (mem.open(DisplayHook::shared_res_name) && mutex.open(DisplayHook::mutex_name)) {
        mutex.lock();
        uchar *pshare = mem.data<byte>();
        reinterpret_cast<FrameInfo *>(pshare)->format(DisplayHook::render_hwnd, width, height);
        pglReadPixels(0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pshare + sizeof(FrameInfo));
        // pglReadPixels(0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, mem.data<byte>());
        mutex.unlock();
    } else {
        is_capture = 0;
#if DEBUG_HOOK
        setlog(L"egl !mem.open(DisplayHook::%s)&&mutex.open(DisplayHook::%s)",
               DisplayHook::shared_res_name.c_str(), DisplayHook::mutex_name.c_str());
#endif // DEBUG_HOOK
    }
    // setlog("gl screen ok");
    return 0;
}

unsigned int __stdcall gl_hkeglSwapBuffers(void *dpy, void *surface) {
    using eglSwapBuffers_t = decltype(gl_hkeglSwapBuffers) *;
    if (is_capture)
        egl_capture();
    return ((eglSwapBuffers_t)DisplayHook::old_address)(dpy, surface);
}

void __stdcall gl_hkglFinish(void) {
    using glFinish_t = decltype(glFinish) *;
    if (is_capture)
        gl_capture();
    ((glFinish_t)DisplayHook::old_address)();
}

} // namespace op::hook
