#pragma once

#include <Windows.h>

#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include <string_view>
#include <wrl/client.h>

namespace op::hook {

HRESULT __stdcall dx12_hkPresent(IDXGISwapChain *thiz, UINT SyncInterval, UINT Flags);

class D3D12Capture final {
  public:
    static D3D12Capture *Get();

    HRESULT CaptureFrames(HWND windowHandleToCapture, std::wstring_view folderToSaveFrames, int maxFrames);
    void CaptureFrame(IDXGISwapChain *swapChain);

  private:
    D3D12Capture();
    ~D3D12Capture();

    std::uintptr_t commandQueueOffset_ = 0;
    std::uint64_t presentPointer_ = 0;
    std::uint64_t presentTrampoline_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> readbackResource_;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;

    UINT readbackDataWidth_ = 0;
    UINT readbackDataHeight_ = 0;
    UINT readbackDataPitch_ = 0;

    void *readbackData_ = nullptr;

    HWND windowHandleToCapture_ = NULL;
    std::wstring folderToSaveFrames_;
    int frameIndex_ = 0;
    int maxFrames_ = 0;
};

} // namespace op::hook
