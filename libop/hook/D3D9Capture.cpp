#include "D3D9Capture.h"

#include "DisplayHook.h"
#include "../capture/FrameInfo.h"
#include "../ipc/ProcessMutex.h"
#include "../ipc/SharedMemory.h"
#include "../base/AutomationModes.h"
#include <atlbase.h>

namespace op::hook {

using ATL::CComPtr;
using op::capture::FrameInfo;

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

HRESULT dx9_capture(LPDIRECT3DDEVICE9 pDevice) {
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

HRESULT STDMETHODCALLTYPE dx9_hkEndScene(IDirect3DDevice9 *thiz) {
    typedef long(__stdcall * EndScene)(LPDIRECT3DDEVICE9);
    auto ret = ((EndScene)DisplayHook::old_address)(thiz);
    if (DisplayHook::capture_enabled())
        dx9_capture(thiz);

    return ret;
}

} // namespace op::hook
