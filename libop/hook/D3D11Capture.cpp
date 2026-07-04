#include "D3D11Capture.h"

#include "DisplayHook.h"
#include "DxCaptureCommon.h"
#include "../capture/FrameInfo.h"
#include "../ipc/ProcessMutex.h"
#include "../ipc/SharedMemory.h"
#include "../base/AutomationModes.h"
#include "../base/Utils.h"
#include <atlbase.h>
#include <cstddef>
#include <d3d11.h>
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

void dx11_capture(IDXGISwapChain *swapchain) {
    HRESULT hr = 0;
    CComPtr<IDXGIResource> backbufferptr;
    CComPtr<ID3D11Resource> backbuffer;
    CComPtr<ID3D11Texture2D> resolvedTexture;
    CComPtr<ID3D11Texture2D> textDst;
    CComPtr<ID3D11Device> device;
    CComPtr<ID3D11DeviceContext> context;

    hr = swapchain->GetBuffer(0, __uuidof(IDXGIResource), reinterpret_cast<void **>(&backbufferptr.p));
    if (hr < 0) {
        setlog("pswapchain->GetBuffer,error code=%X", hr);
        DisplayHook::set_capture_enabled(false);
        return;
    }
    hr = backbufferptr->QueryInterface(__uuidof(ID3D11Resource), reinterpret_cast<void **>(&backbuffer.p));
    if (hr < 0) {
        setlog("backbufferptr->QueryInterface,error code=%X", hr);
        DisplayHook::set_capture_enabled(false);
        return;
    }
    hr = swapchain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void **>(&device.p));
    if (hr < 0) {
        setlog("swapchain->GetDevice hr=%X", hr);
        DisplayHook::set_capture_enabled(false);
        return;
    }
    DXGI_SWAP_CHAIN_DESC desc;
    hr = swapchain->GetDesc(&desc);
    if (hr < 0) {
        setlog("swapchain->GetDesc hr=%X", hr);
        DisplayHook::set_capture_enabled(false);
        return;
    }

    D3D11_TEXTURE2D_DESC textDesc = {};
    textDesc.Format = NormalizeDxgiFormat(desc.BufferDesc.Format);
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
        DisplayHook::set_capture_enabled(false);
        return;
    }
    device->GetImmediateContext(&context);
    if (!context) {
        setlog("!context");
        DisplayHook::set_capture_enabled(false);
        return;
    }

    ID3D11Resource *copySource = backbuffer;
    if (desc.SampleDesc.Count > 1) {
        // MSAA 后备缓冲需要先 resolve 成单采样纹理，否则 CopyResource 到 staging 会失败。
        D3D11_TEXTURE2D_DESC resolveDesc = textDesc;
        resolveDesc.Usage = D3D11_USAGE_DEFAULT;
        resolveDesc.BindFlags = 0;
        resolveDesc.CPUAccessFlags = 0;
        resolveDesc.MiscFlags = 0;
        hr = device->CreateTexture2D(&resolveDesc, nullptr, &resolvedTexture);
        if (hr < 0) {
            setlog("device->CreateTexture2D resolved,error code=%d", hr);
            DisplayHook::set_capture_enabled(false);
            return;
        }
        context->ResolveSubresource(resolvedTexture, 0, backbuffer, 0, textDesc.Format);
        copySource = resolvedTexture;
    }

    context->CopyResource(textDst, copySource);

    D3D11_MAPPED_SUBRESOURCE mapSubres = {0, 0, 0};

    D3D11TextureMap mappedTexture(context, textDst);
    hr = mappedTexture.map(&mapSubres);
    if (hr < 0) {
        setlog("context->Map error code=%d", hr);
        DisplayHook::set_capture_enabled(false);
        return;
    }
    int fmt = GetImageBufferFormat(textDesc.Format);

    SharedMemory mem;
    ProcessMutex mutex;
    static int cnt = 10;
    if (mem.open(DisplayHook::shared_res_name) && mutex.open(DisplayHook::mutex_name)) {
        mutex.lock();
        auto sharedFrame = make_shared_frame_span(mem, textDesc.Width, textDesc.Height);
        write_shared_frame(sharedFrame, DisplayHook::render_hwnd, textDesc.Width, textDesc.Height, mapSubres.pData,
                           textDesc.Height, textDesc.Width, mapSubres.RowPitch, fmt);
        static_assert(sizeof(FrameInfo) == 28);
        mutex.unlock();
    } else {
        DisplayHook::set_capture_enabled(false);
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
}

HRESULT __stdcall dx11_hkPresent(IDXGISwapChain *thiz, UINT SyncInterval, UINT Flags) {
    typedef long(__stdcall * Present_t)(IDXGISwapChain * pswapchain, UINT x1, UINT x2);
    // DXGI_PRESENT_TEST 不提交真实帧，避免把测试调用当成截图帧处理。
    if (DisplayHook::capture_enabled() && !(Flags & DXGI_PRESENT_TEST))
        dx11_capture(thiz);
    return ((Present_t)DisplayHook::old_address)(thiz, SyncInterval, Flags);
}

} // namespace op::hook
