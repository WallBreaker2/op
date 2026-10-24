#include <cstdio>
#include "D3D9Capture.h"

#include "DisplayHook.h"
#include "DxCaptureCommon.h"
#include "SharedFrame.h"
#include "SharedFrameChannel.h"
#include "SharedFrameExchange.h"
#include "SharedFrameWriter.h"
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
    // 映射与跨进程互斥体按名字缓存复用，避免每帧重开映射导致整帧搬运反复缺页
    // （1080p 上实测 2.66ms → 1.01ms，见 SharedFrameChannel.h）。
    SharedFrameChannelLease channel(DisplayHook::shared_res_name, DisplayHook::mutex_name);
    if (channel.ok()) {
        if (!channel.mutex().lock_timeout(op::hook::kSharedFrameLockTimeoutMs)) {
            // 宿主没在超时内释放 → 跳过本帧，别让 Present 无限等。
            // 不记 note_capture_failure：慢的是宿主那边，记失败会误禁捕。
            return S_OK;
        }
        SharedMemory &mem = channel.mem();
        if (SharedFrameHasCapacity(mem, surface_Desc.Width, surface_Desc.Height)) {
            // 帧头与像素偏移统一由 SharedFrameWriter.h 算（§72.60）：以前五个后端各抄一份，
            // 已经分叉出过缺陷（OpenGL 双翻转、缺 request_in_bounds 校验）。
            const SharedFrameView sharedFrame =
                make_shared_frame_span(mem, surface_Desc.Width, surface_Desc.Height);
            // 宿主可能只要一块（§3.2）：GPU 侧仍是整窗 GetRenderTargetData（D3D9 没有
            // 带源矩形的同步回读），但 CPU 侧只搬请求的那几行。
            const FrameRequestSnapshot raw_request =
                ReadFrameRequestByName(DisplayHook::shared_res_name, surface_Desc.Width, surface_Desc.Height);
            const FrameRequestSnapshot request =
                NormalizeFrameRequest(raw_request, surface_Desc.Width, surface_Desc.Height);
            // 响应描述**这一帧缓冲区里的实际布局**：矩形就写矩形，整窗就写整窗。
            // 不能只在「读到合法请求」时才写 —— 请求 TTL 一过期就退回整窗拷贝，
            // 而响应还停在上一次的矩形上，宿主会按矩形去解释整窗帧（§72.19）。
            // 协商区不存在时 WriteFrameResponse 自己会 no-op，旧宿主不受影响。
            if (request.valid) {
                const char *source = reinterpret_cast<const char *>(lockedRect.pBits) +
                                     static_cast<size_t>(request.y) * static_cast<size_t>(lockedRect.Pitch) +
                                     static_cast<size_t>(request.x) * 4u;
                write_shared_frame_region(sharedFrame, DisplayHook::render_hwnd, surface_Desc.Width,
                                          surface_Desc.Height, source, request.h, request.w, lockedRect.Pitch,
                                          IBF_B8G8R8A8);
                WriteFrameResponse(mem, surface_Desc.Width, surface_Desc.Height, kFrameModeRegion, request.seq,
                                   request.x, request.y, request.w, request.h);
            } else {
                write_shared_frame(sharedFrame, DisplayHook::render_hwnd, surface_Desc.Width, surface_Desc.Height,
                                   reinterpret_cast<const char *>(lockedRect.pBits), surface_Desc.Height,
                                   surface_Desc.Width, lockedRect.Pitch, IBF_B8G8R8A8);
                WriteFrameResponse(mem, surface_Desc.Width, surface_Desc.Height, kFrameModeFull, 0, 0, 0,
                                   static_cast<int>(surface_Desc.Width), static_cast<int>(surface_Desc.Height));
            }
        } else {
            WriteSharedFrameHeader(mem, DisplayHook::render_hwnd, surface_Desc.Width, surface_Desc.Height);
        }
        channel.mutex().unlock();
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE dx9_hkEndScene(IDirect3DDevice9 *thiz) {
    typedef long(__stdcall * EndScene)(LPDIRECT3DDEVICE9);
    // 先登记「我在 detour 里」，并留一份原函数地址。release() 要等这个计数归零
    // 才清 old_address、释放 trampoline；否则正在跑的这一次会调到空指针或已回收的代码。
    void *const original = DisplayHook::old_address;
    DisplayHook::DetourScope detour_scope;
    if (!original) {
        return D3D_OK;
    }
    auto ret = ((EndScene)original)(thiz);
    if (DisplayHook::capture_enabled())
        dx9_capture(thiz);

    return ret;
}

} // namespace op::hook
