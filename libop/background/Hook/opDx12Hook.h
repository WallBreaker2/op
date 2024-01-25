
#pragma once
//ref https://github.com/eugen15/directx-present-hook.git
#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>
#include <string>
#include <string_view>

class opDx12Hook final {
public:
    static opDx12Hook* Get();

    // This method must be called only once!
    // There is no additional check inside.
    //HRESULT Hook();

    // Captures some frames.
    HRESULT CaptureFrames(HWND windowHandleToCapture,
        std::wstring_view folderToSaveFrames, int maxFrames);
    void CaptureFrame(IDXGISwapChain* swapChain);
private:
    opDx12Hook();
    ~opDx12Hook();

   


    // The command chain offset from the swap chain object pointer.
    std::uintptr_t commandQueueOffset_ = 0;

    // Hook pointers.
    std::uint64_t presentPointer_ = 0;
    std::uint64_t presentTrampoline_ = 0;

    // The resource to read frames from GPU.
    Microsoft::WRL::ComPtr<ID3D12Resource> readbackResource_;

    // Command allocator for a command list to copy the textures.
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;

    UINT readbackDataWidth_ = 0;
    UINT readbackDataHeight_ = 0;
    UINT readbackDataPitch_ = 0;

    void* readbackData_ = nullptr;

    // Capture details.
    HWND windowHandleToCapture_ = NULL;
    std::wstring folderToSaveFrames_;
    int frameIndex_ = 0;
    int maxFrames_ = 0;
};
