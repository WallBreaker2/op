#include "D3D10Capture.h"

#include "DisplayHook.h"
#include "DxCaptureCommon.h"
#include "SharedFrame.h"
#include "SharedFrameChannel.h"
#include "SharedFrameExchange.h"
#include "SharedFrameWriter.h"
#include "../ipc/ProcessMutex.h"
#include "../ipc/SharedMemory.h"
#include "../base/AutomationModes.h"
#include "../base/Utils.h"
#include <atlbase.h>
#include <cstddef>
#include <d3d10.h>

#define DEBUG_HOOK 0

namespace op::hook {

using ATL::CComPtr;

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

void dx10_capture(IDXGISwapChain *pswapchain) {
    // 只搬被绑定窗口的交换链：一个进程里可能有多条链（多窗口、编辑器多视口），
    // 别的窗口的帧会写进同一个共享段，让"像素区"和"响应区"来自不同链（§72.22）。
    DXGI_SWAP_CHAIN_DESC chain_desc = {};
    if (SUCCEEDED(pswapchain->GetDesc(&chain_desc)) && DisplayHook::render_hwnd != 0 &&
        chain_desc.OutputWindow != 0 && chain_desc.OutputWindow != DisplayHook::render_hwnd) {
        return;
    }
    HRESULT hr;
    CComPtr<ID3D10Device> pdevices;
    CComPtr<ID3D10Resource> backbuffer;
    CComPtr<ID3D10Texture2D> resolvedTexture;
    CComPtr<ID3D10Texture2D> textDst;

    hr = pswapchain->GetBuffer(0, __uuidof(ID3D10Resource), reinterpret_cast<void **>(&backbuffer.p));
    if (hr < 0) {
        setlog("pswapchain->GetBuffer error code=%d", hr);
        DisplayHook::note_capture_failure();
        return;
    }
    backbuffer->GetDevice(&pdevices);

    if (!pdevices) {
        DisplayHook::note_capture_failure();
        return;
    }

    DXGI_SWAP_CHAIN_DESC desc;
    hr = pswapchain->GetDesc(&desc);
    if (hr < 0) {
        setlog("pswapchain->GetDesc error code=%d", hr);
        DisplayHook::note_capture_failure();
        return;
    }
    D3D10_TEXTURE2D_DESC textDesc = {};
    textDesc.Format = NormalizeDxgiFormat(desc.BufferDesc.Format);
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
        DisplayHook::note_capture_failure();
        return;
    }

    ID3D10Resource *copySource = backbuffer;
    if (desc.SampleDesc.Count > 1) {
        // MSAA 后备缓冲不能直接复制到 staging，先 resolve 成单采样纹理再读回。
        D3D10_TEXTURE2D_DESC resolveDesc = textDesc;
        resolveDesc.Usage = D3D10_USAGE_DEFAULT;
        resolveDesc.CPUAccessFlags = 0;
        resolveDesc.BindFlags = 0;
        resolveDesc.MiscFlags = 0;
        hr = pdevices->CreateTexture2D(&resolveDesc, nullptr, &resolvedTexture);
        if (hr < 0) {
            setlog("pdevices->CreateTexture2D resolved error code=%d", hr);
            DisplayHook::note_capture_failure();
            return;
        }
        pdevices->ResolveSubresource(resolvedTexture, 0, backbuffer, 0, textDesc.Format);
        copySource = resolvedTexture;
    }

    // 宿主可能只要一块（§3.2）：按矩形拷到 staging 左上角，后面的 Map 只读那块。
    const FrameRequestSnapshot raw_request =
        ReadFrameRequestByName(DisplayHook::shared_res_name, textDesc.Width, textDesc.Height);
    const FrameRequestSnapshot request = NormalizeFrameRequest(raw_request, textDesc.Width, textDesc.Height);
    // 响应描述**这一帧缓冲区里的实际布局**（矩形/整窗），无条件写：请求 TTL 过期时
    // 注入侧退回整窗拷贝，响应必须跟着变成整窗，否则宿主会按旧矩形解释整窗帧（§72.19）。
    // 协商区不存在时 WriteFrameResponse 自己 no-op，旧宿主不受影响。
    if (request.valid) {
        D3D10_BOX box = {};
        box.left = static_cast<UINT>(request.x);
        box.top = static_cast<UINT>(request.y);
        box.front = 0;
        box.right = static_cast<UINT>(request.x + request.w);
        box.bottom = static_cast<UINT>(request.y + request.h);
        box.back = 1;
        pdevices->CopySubresourceRegion(textDst, 0, 0, 0, 0, copySource, 0, &box);
    } else {
        pdevices->CopyResource(textDst, copySource);
    }

    D3D10_MAPPED_TEXTURE2D mapText = {0, 0};

    D3D10TextureMap mappedTexture(textDst);
    hr = mappedTexture.map(&mapText);
    if (hr < 0) {
        setlog("textDst->Map false,hr=%d", hr);
        DisplayHook::note_capture_failure();
        return;
    }

    int fmt = GetImageBufferFormat(textDesc.Format);

    // 映射与跨进程互斥体按名字缓存复用，避免每帧重开映射导致整帧搬运反复缺页
    // （1080p 上实测 2.66ms → 1.01ms，见 SharedFrameChannel.h）。
    SharedFrameChannelLease channel(DisplayHook::shared_res_name, DisplayHook::mutex_name);
    if (channel.ok()) {
        if (!channel.mutex().lock_timeout(op::hook::kSharedFrameLockTimeoutMs)) {
            // 宿主没在超时内释放 → 跳过本帧，别让 Present 无限等。
            // 不记 note_capture_failure：慢的是宿主那边，记失败会误禁捕。
            return;
        }
        SharedMemory &mem = channel.mem();
        if (SharedFrameHasCapacity(mem, textDesc.Width, textDesc.Height)) {
            auto sharedFrame = make_shared_frame_span(mem, textDesc.Width, textDesc.Height);
            if (request.valid) {
                // 区域已经落在 staging 左上角，源指针就是起点（不能再按 x/y 偏移）。
                write_shared_frame_region(sharedFrame, DisplayHook::render_hwnd, textDesc.Width, textDesc.Height,
                                          mapText.pData, request.h, request.w, mapText.RowPitch, fmt);
                WriteFrameResponse(mem, textDesc.Width, textDesc.Height, kFrameModeRegion, request.seq, request.x,
                                   request.y, request.w, request.h);
            } else {
                write_shared_frame(sharedFrame, DisplayHook::render_hwnd, textDesc.Width, textDesc.Height, mapText.pData,
                                   textDesc.Height, textDesc.Width, mapText.RowPitch, fmt);
                WriteFrameResponse(mem, textDesc.Width, textDesc.Height, kFrameModeFull, 0, 0, 0,
                                   static_cast<int>(textDesc.Width), static_cast<int>(textDesc.Height));
            }
        } else {
            WriteSharedFrameHeader(mem, DisplayHook::render_hwnd, textDesc.Width, textDesc.Height);
        }
        channel.mutex().unlock();
        DisplayHook::note_capture_success();
    } else {
#if DEBUG_HOOK
        setlog("mem.open(DisplayHook::shared_res_name) && mutex.open(DisplayHook::mutex_name)");
#endif // DEBUG_HOOK
    }
}

HRESULT STDMETHODCALLTYPE dx10_hkPresent(IDXGISwapChain *thiz, UINT SyncInterval, UINT Flags) {
    typedef long(__stdcall * Present_t)(IDXGISwapChain * pswapchain, UINT x1, UINT x2);
    // 先登记「我在 detour 里」，并留一份原函数地址。release() 要等这个计数归零
    // 才清 old_address、释放 trampoline；否则正在跑的这一次会调到空指针或已回收的代码。
    void *const original = DisplayHook::old_address;
    DisplayHook::DetourScope detour_scope;
    if (!original) {
        return S_OK;
    }
    // DXGI_PRESENT_TEST 只是检查 present 是否可行，不代表产生了新画面，跳过可减少无效读回。
    if (DisplayHook::capture_enabled() && !(Flags & DXGI_PRESENT_TEST))
        dx10_capture(thiz);
    return ((Present_t)original)(thiz, SyncInterval, Flags);
}

} // namespace op::hook
