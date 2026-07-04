#include "D3D10Capture.h"

#include "DisplayHook.h"
#include "DxCaptureCommon.h"
#include "../capture/FrameInfo.h"
#include "../ipc/ProcessMutex.h"
#include "../ipc/SharedMemory.h"
#include "../base/AutomationModes.h"
#include "../base/Utils.h"
#include <atlbase.h>
#include <cstddef>
#include <d3d10.h>
#include <span>

#define DEBUG_HOOK 0

namespace op::hook {

using ATL::CComPtr;
using op::capture::FrameInfo;

namespace {

std::span<std::byte> make_shared_frame_span(SharedMemory &mem, UINT width, UINT height) {
    const auto pixelBytes = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    return {mem.data<std::byte>(), sizeof(FrameInfo) + pixelBytes};
}

void write_shared_frame(std::span<std::byte> sharedFrame, HWND hwnd, UINT width, UINT height, const void *source,
                        int sourceRows, int sourceCols, int rowPitch, int format) {
    // 使用 span 明确区分帧头和像素区，避免共享内存裸指针偏移散落在捕获逻辑里。
    auto frameInfoBytes = sharedFrame.first(sizeof(FrameInfo));
    auto pixelBytes = sharedFrame.subspan(sizeof(FrameInfo));

    reinterpret_cast<FrameInfo *>(frameInfoBytes.data())->format(hwnd, width, height);
    CopyImageData(reinterpret_cast<char *>(pixelBytes.data()), static_cast<const char *>(source), sourceRows,
                  sourceCols, rowPitch, format);
}

} // namespace

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
    HRESULT hr;
    CComPtr<ID3D10Device> pdevices;
    CComPtr<ID3D10Resource> backbuffer;
    CComPtr<ID3D10Texture2D> resolvedTexture;
    CComPtr<ID3D10Texture2D> textDst;

    hr = pswapchain->GetBuffer(0, __uuidof(ID3D10Resource), reinterpret_cast<void **>(&backbuffer.p));
    if (hr < 0) {
        setlog("pswapchain->GetBuffer error code=%d", hr);
        DisplayHook::set_capture_enabled(false);
        return;
    }
    backbuffer->GetDevice(&pdevices);

    if (!pdevices) {
        DisplayHook::set_capture_enabled(false);
        return;
    }

    DXGI_SWAP_CHAIN_DESC desc;
    hr = pswapchain->GetDesc(&desc);
    if (hr < 0) {
        setlog("pswapchain->GetDesc error code=%d", hr);
        DisplayHook::set_capture_enabled(false);
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
        DisplayHook::set_capture_enabled(false);
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
            DisplayHook::set_capture_enabled(false);
            return;
        }
        pdevices->ResolveSubresource(resolvedTexture, 0, backbuffer, 0, textDesc.Format);
        copySource = resolvedTexture;
    }

    pdevices->CopyResource(textDst, copySource);

    D3D10_MAPPED_TEXTURE2D mapText = {0, 0};

    D3D10TextureMap mappedTexture(textDst);
    hr = mappedTexture.map(&mapText);
    if (hr < 0) {
        setlog("textDst->Map false,hr=%d", hr);
        DisplayHook::set_capture_enabled(false);
        return;
    }

    int fmt = GetImageBufferFormat(textDesc.Format);

    SharedMemory mem;
    ProcessMutex mutex;
    if (mem.open(DisplayHook::shared_res_name) && mutex.open(DisplayHook::mutex_name)) {
        mutex.lock();
        auto sharedFrame = make_shared_frame_span(mem, textDesc.Width, textDesc.Height);
        write_shared_frame(sharedFrame, DisplayHook::render_hwnd, textDesc.Width, textDesc.Height, mapText.pData,
                           textDesc.Height, textDesc.Width, mapText.RowPitch, fmt);
        mutex.unlock();
    } else {
#if DEBUG_HOOK
        setlog("mem.open(DisplayHook::shared_res_name) && mutex.open(DisplayHook::mutex_name)");
#endif // DEBUG_HOOK
    }
}

HRESULT STDMETHODCALLTYPE dx10_hkPresent(IDXGISwapChain *thiz, UINT SyncInterval, UINT Flags) {
    typedef long(__stdcall * Present_t)(IDXGISwapChain * pswapchain, UINT x1, UINT x2);
    // DXGI_PRESENT_TEST 只是检查 present 是否可行，不代表产生了新画面，跳过可减少无效读回。
    if (DisplayHook::capture_enabled() && !(Flags & DXGI_PRESENT_TEST))
        dx10_capture(thiz);
    return ((Present_t)DisplayHook::old_address)(thiz, SyncInterval, Flags);
}

} // namespace op::hook
