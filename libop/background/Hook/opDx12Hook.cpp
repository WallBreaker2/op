// Copyright 2022 Eugen Hartmann.
// Licensed under the MIT License (MIT).

#include <format>
#include <iostream>



//#include "d3d12-base-helper.h"
#include "opDx12Hook.h"

#include "d3dx12.h"
#include "DisplayHook.h"
#include "../../include/sharedmem.h"
#include "../../include/promutex.h"
#include "../../core/globalVar.h"
#include "../../core/helpfunc.h"
#include "../display/frameInfo.h"
//#include "base-window.h"
//#include "misc-helpers.h"
// The swap chain pointer will come as the first function parameter.
typedef HRESULT(WINAPI* D3D12PresentPointer)(
    IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);

opDx12Hook* opDx12Hook::Get() {
    static opDx12Hook hook;
    return &hook;
}

opDx12Hook::opDx12Hook() {
    // TODO
}

opDx12Hook::~opDx12Hook() {
    // TODO
}

//HRESULT D3D12PresentHook::Hook() {
//    HRESULT hr;
//
//    // Create a temporary window to create a sample swap chain.
//    BaseWindow temporaryWindow(L"DirectX 11 Temporary Window");
//    hr = temporaryWindow.Initialize(800, 600);
//    if (FAILED(hr)) {
//        return hr;
//    }
//
//    // A derived class to access the swap chain and command queue.
//    // You need the swap chain to find the Present method pointer.
//    // You need the command queue to find the propery offset
//    // from the beginning of the swap chain object.
//    class D3D12PresentHookHelper : public D3D12BaseHelper {
//    public:
//        inline IDXGISwapChain1* GetSwapChain() {
//            return swapChain_.Get();
//        }
//        inline ID3D12CommandQueue* GetCommandQueue() {
//            return commandQueue_.Get();
//        }
//    };
//
//    // Initialize the DirectX helper to create a swap chain for the window.
//    D3D12PresentHookHelper d3d12Helper;
//    hr = d3d12Helper.Initialize(temporaryWindow.GetHandle(), 800, 600);
//    if (FAILED(hr)) {
//        return hr;
//    }
//
//    // I could not find another way to access the command queue,
//    // so I just look for offset from the swap chain object start.
//    std::uintptr_t i = 0;
//    const char* start = static_cast<char*>(
//        static_cast<void*>(d3d12Helper.GetSwapChain()));
//    while (true) {
//        if (reinterpret_cast<std::uintptr_t>(d3d12Helper.GetCommandQueue()) ==
//            *static_cast<const std::uintptr_t*>(static_cast<const void*>(start + i))) {
//            commandQueueOffset_ = i;
//            break;
//        }
//        ++i;
//    }
//
//    // Swap chain start.
//    std::uintptr_t* swapChainPointer =
//        static_cast<std::uintptr_t*>(static_cast<void*>(
//            d3d12Helper.GetSwapChain()));
//
//    // Swap chain virtual start.
//    std::uintptr_t* virtualTablePointer =
//        reinterpret_cast<std::uintptr_t*>(swapChainPointer[0]);
//
//    // "Present" has index 8 because:
//    // - "Present" is the first original virtual method of IDXGISwapChain.
//    // - IDXGISwapChain is based on IDXGIDeviceSubObject
//    //   which has 1 original virtual method (GetDevice).
//    // - IDXGIDeviceSubObject is based on IDXGIObject
//    //   which has 4 original virtual methods (SetPrivateData, SetPrivateDataInterface, GetPrivateData, GetParent).
//    // - IDXGIObject is based on IUnknown
//    //   which has 3 original virtual methods (QueryInterface, AddRef, Release). 
//    // So 0 + 1 + 4 + 3 = 8.
//    presentPointer_ = static_cast<std::uint64_t>(virtualTablePointer[8]);
//
//    // The function which will be called instead of the original "Present".
//    std::uint64_t presentCallback =
//        reinterpret_cast<std::uint64_t>(&D3D12PresentHook::SwapChainPresentWrapper);
//
//    // Configure everything to hook.
//#if defined(_M_X64)
//    PLH::x64Detour* detour = new PLH::x64Detour(presentPointer_,
//        presentCallback, &presentTrampoline_);
//#else
//    PLH::x86Detour* detour = new PLH::x86Detour(presentPointer_,
//        presentCallback, &presentTrampoline_, dis);
//#endif
//
//    // Hook!
//    detour->hook();
//
//    return S_OK;
//}

HRESULT opDx12Hook::CaptureFrames(HWND windowHandleToCapture,
    std::wstring_view folderToSaveFrames, int maxFrames) {
    if (windowHandleToCapture_ != NULL) {
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    }
    windowHandleToCapture_ = windowHandleToCapture;
    folderToSaveFrames_ = std::wstring(folderToSaveFrames);
    if (folderToSaveFrames_.size() &&
        *folderToSaveFrames_.rbegin() != '\\' &&
        *folderToSaveFrames_.rbegin() != '/') {
        folderToSaveFrames_ += '\\';
    }
    maxFrames_ = maxFrames;
    return S_OK;
}

void opDx12Hook::CaptureFrame(IDXGISwapChain* swapChain) {
    HRESULT hr;

    // --------------------------------------------------------------
    // This is a very simplified example. The previous frame of two
    // frame swap chain is captured instead of the current one.
    // It is enough if you just want to get some preview of the
    // window or to analyze the frames. It is not enough if you want
    // to modify the frame before it is shown.
    //---------------------------------------------------------------

    // IDXGISwapChain3 to call GetCurrentBackBufferIndex.
    Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain3;
    hr = swapChain->QueryInterface(__uuidof(IDXGISwapChain3), &swapChain3);
    if (FAILED(hr)) {
        return;
    }

    // This is not necessary because there is only one ID3D12Device per process.
    // Anyway, I do it here the same way how I did it for DirectX 11.
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    hr = swapChain->GetDevice(__uuidof(ID3D12Device), &device);
    if (FAILED(hr)) {
        return;
    }

    // Get back buffer index to capture a correct picture.
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    hr = swapChain->GetBuffer(swapChain3->GetCurrentBackBufferIndex(),
        __uuidof(ID3D12Resource), &resource);
    if (FAILED(hr)) {
        return;
    }

    // Get the texture description.
    D3D12_RESOURCE_DESC desc = resource->GetDesc();

    // Get details to copy.
    UINT64 sizeInBytes;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, nullptr, nullptr, &sizeInBytes);

    // If the resource to read data from the GPU does not exist yet,
    // then this is just the first frame.
    if (readbackResource_.Get()) {

        // Not the first frame.

        // If the pointer is nullptr, map it. With DirectX 12,
        // it is not necessary to unmap it between frames.    
        if (readbackData_ == nullptr) {
            hr = readbackResource_->Map(0, nullptr, &readbackData_);
            if (FAILED(hr)) {
                return;
            }
        }

        UINT frameRowPitch = readbackDataPitch_;
        UINT frameWidth = frameRowPitch / 4;
        UINT frameHeight = readbackDataHeight_;

        // Convert the frame to the BMP format.
        // Do not forget that this is the previous frame!
       // std::vector<std::uint8_t> bmp = MiscHelpers::ConvertRGBAToBMP(
       //     static_cast<std::uint8_t*>(readbackData_),
       //     frameWidth, frameHeight, frameRowPitch);

        // Save the BMP file.
        // On some machine you will see rendering freezes during this operation.
        // In a real application, probably, you will not need to save frames to a file
        // but just to place them to a buffer to generate a preview picture or analyze it.
        //std::wstring filename = std::format(L"{}{}.bmp", folderToSaveFrames_, frameIndex_++);
        //MiscHelpers::SaveDataToFile(filename, bmp.data(), bmp.size());
        //--
        sharedmem mem;
        promutex mutex;
        static int cnt = 10;
        int fmt = IBF_R8G8B8A8;
        if (mem.open(DisplayHook::shared_res_name) && mutex.open(DisplayHook::mutex_name)) {
            mutex.lock();
            uchar* pshare = mem.data<byte>();
            reinterpret_cast<FrameInfo*>(pshare)->format(DisplayHook::render_hwnd, frameWidth, frameHeight);
            //formatFrameInfo(pshare, DisplayHook::render_hwnd, textDesc.Width, textDesc.Height);
            //CopyImageData((char*)pshare + sizeof(FrameInfo), (char*)mapText.pData, textDesc.Height, textDesc.Width, fmt);
            static_assert(sizeof(FrameInfo) == 28);
           
            CopyImageData((char*)pshare + sizeof(FrameInfo), (char*)readbackData_, frameWidth, frameHeight, frameRowPitch, fmt);
            mutex.unlock();
        }
        else {
            //is_capture = 0;
#if DEBUG_HOOK
            setlog("!mem.open(xhook::%s)&&mutex.open(xhook::%s)", xhook::shared_res_name, xhook::mutex_name);
#endif // DEBUG_HOOK

        }
        static bool first = true;
        if (first) {
            int tf = IBF_R8G8B8A8;

            setlog("textDesc.Format= %d,fmt=%d textDesc.Height=%d\n textDesc.Width=%d\n  mapSubres.DepthPitch=%d\n mapSubres.RowPitch=%d\n",
                tf, fmt,
                frameHeight,
                frameWidth,
                0,
                frameRowPitch
            );
            first = false;
        }
        // --
        // Stop capturing if enough frames.
        if (frameIndex_ >= maxFrames_) {
            windowHandleToCapture_ = NULL;
        }

    }
    else {

        // This is the first frame.
        // A texture must be created to read data from the GPU.

        readbackDataWidth_ = footprint.Footprint.Width;
        readbackDataHeight_ = footprint.Footprint.Height;
        readbackDataPitch_ = footprint.Footprint.RowPitch;

        auto readbackResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes);
        auto readbackHeapDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);

        hr = device->CreateCommittedResource(&readbackHeapDesc, D3D12_HEAP_FLAG_NONE,
            &readbackResourceDesc, D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr, IID_PPV_ARGS(&readbackResource_));
        if (FAILED(hr)) {
            return;
        }

        hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&commandAllocator_));
        if (FAILED(hr)) {
            return;
        }
    }

    // Create a command list to copy data from the swap chain texture to the read back texture.
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> copyCommandList;
    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&copyCommandList));
    if (FAILED(hr)) {
        return;
    }

    D3D12_TEXTURE_COPY_LOCATION dst = CD3DX12_TEXTURE_COPY_LOCATION(readbackResource_.Get(), footprint);
    D3D12_TEXTURE_COPY_LOCATION src = CD3DX12_TEXTURE_COPY_LOCATION(resource.Get(), 0);
    copyCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    copyCommandList->Close();

    // Based on information I found, you can execute any number of command lists
    // in DirectX 12 before presenting. They will be executed in a correct order.
    // I am not sure this is universally correct but the code below works.

    // Just find the command queue object.
    const char* start = static_cast<char*>(static_cast<void*>(swapChain3.Get()));
    ID3D12CommandQueue* commandQueue =
        reinterpret_cast<ID3D12CommandQueue*>(
            *static_cast<const std::uintptr_t*>(
                static_cast<const void*>(start + commandQueueOffset_)));

    // ... then execute the command list to copy the swap chain texture to the read back texture.
    ID3D12CommandList* commandLists[] = { copyCommandList.Get() };
    commandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);

    // The read back texture does not contain the correct picture yet!
    // You will get it when the CaptureFrame is called next time.
    // Probably, it will be difficult to implement a universal solution
    // if you want to have the current frame ready here.
}


