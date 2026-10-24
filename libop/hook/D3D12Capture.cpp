// Copyright 2022 Eugen Hartmann.
// Licensed under the MIT License (MIT).

#include "D3D12Capture.h"

#include "DisplayHook.h"
#include "DxCaptureCommon.h"
#include "SharedFrame.h"
#include "SharedFrameChannel.h"
#include "SharedFrameExchange.h"
#include "SharedFrameWriter.h"
#include <directx/d3dx12.h>
#include "../capture/FrameInfo.h"
#include "../ipc/ProcessMutex.h"
#include "../ipc/SharedMemory.h"
#include "../base/AutomationModes.h"
#include "../base/Utils.h"

#define DEBUG_HOOK 0

namespace op::hook {

using op::capture::FrameInfo;

D3D12Capture *D3D12Capture::Get() {
    static D3D12Capture hook;
    return &hook;
}

D3D12Capture::D3D12Capture() {
}

D3D12Capture::~D3D12Capture() {
    if (fenceEvent_) {
        ::CloseHandle(fenceEvent_);
        fenceEvent_ = nullptr;
    }
}

HRESULT D3D12Capture::CaptureFrames(HWND windowHandleToCapture, std::wstring_view folderToSaveFrames, int maxFrames) {
    if (windowHandleToCapture_ != NULL) {
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    }
    windowHandleToCapture_ = windowHandleToCapture;
    folderToSaveFrames_ = std::wstring(folderToSaveFrames);
    if (folderToSaveFrames_.size() && *folderToSaveFrames_.rbegin() != '\\' && *folderToSaveFrames_.rbegin() != '/') {
        folderToSaveFrames_ += '\\';
    }
    maxFrames_ = maxFrames;
    return S_OK;
}

bool D3D12Capture::WaitPendingCopy(DWORD wait_ms) {
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    if (!pendingCopy_ || !fence_.Get()) {
        return !pendingCopy_;
    }
    if (!fenceEvent_) {
        fenceEvent_ = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!fenceEvent_) {
            return false;
        }
    }
    if (fence_->GetCompletedValue() >= fenceValue_) {
        return true;
    }
    if (fence_->SetEventOnCompletion(fenceValue_, fenceEvent_) != S_OK) {
        return false;
    }
    return ::WaitForSingleObject(fenceEvent_, wait_ms) == WAIT_OBJECT_0;
}

bool D3D12Capture::EnsureResources(ID3D12Device *device, const D3D12_RESOURCE_DESC &back_buffer_desc,
                                   const D3D12_PLACED_SUBRESOURCE_FOOTPRINT &footprint, UINT64 size_in_bytes) {
    const UINT width = static_cast<UINT>(back_buffer_desc.Width);
    const UINT height = static_cast<UINT>(back_buffer_desc.Height);

    if (device_.Get() != device) {
        // 换了设备：之前那条队列/栅栏/回读缓冲都不再适用。
        pendingCopy_ = false;
        fenceValue_ = 0;
        commandList_.Reset();
        commandAllocator_.Reset();
        captureQueue_.Reset();
        fence_.Reset();
        readbackResource_.Reset();
        readbackDataWidth_ = readbackDataHeight_ = readbackDataPitch_ = 0;
        device_ = device;

        D3D12_COMMAND_QUEUE_DESC queue_desc = {};
        queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        if (FAILED(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&captureQueue_)))) {
            return false;
        }
        if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_)))) {
            return false;
        }
        if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr,
                                             IID_PPV_ARGS(&commandList_)))) {
            return false;
        }
        // CreateCommandList 出来是打开状态，先关掉，后面每次重新 Reset 使用。
        commandList_->Close();
        if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)))) {
            return false;
        }
    }

    if (readbackResource_.Get() && readbackDataWidth_ == width && readbackDataHeight_ == height &&
        readbackDataPitch_ == footprint.Footprint.RowPitch) {
        return true;
    }

    // 尺寸/对齐变化：重建回读缓冲。旧缓冲可能还有在途拷贝，等它跑完再释放。
    if (pendingCopy_) {
        WaitPendingCopy(1000);
        pendingCopy_ = false;
    }
    readbackResource_.Reset();
    readbackDataWidth_ = width;
    readbackDataHeight_ = height;
    readbackDataPitch_ = footprint.Footprint.RowPitch;

    auto readback_resource_desc = CD3DX12_RESOURCE_DESC::Buffer(size_in_bytes);
    auto readback_heap_desc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    const HRESULT hr = device->CreateCommittedResource(&readback_heap_desc, D3D12_HEAP_FLAG_NONE,
                                                       &readback_resource_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                       IID_PPV_ARGS(&readbackResource_));
    return SUCCEEDED(hr);
}

bool D3D12Capture::SubmitCopy(ID3D12Resource *back_buffer, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT &footprint) {
    if (!commandList_.Get() || !commandAllocator_.Get() || !captureQueue_.Get() || !readbackResource_.Get()) {
        return false;
    }
    // 只在没有在途拷贝时走到这里，说明上一次的列表早已执行完，可以安全 Reset 分配器。
    if (FAILED(commandAllocator_->Reset())) {
        return false;
    }
    if (FAILED(commandList_->Reset(commandAllocator_.Get(), nullptr))) {
        return false;
    }

    // 后台缓冲此刻处在 PRESENT 状态（我们在 Present 里、原函数之前拿到它），
    // 拷贝前后各加一次栅栏，拷完必须还原成 PRESENT，否则目标进程下一帧的
    // PRESENT -> RENDER_TARGET 转换会踩到未定义状态。
    D3D12_RESOURCE_BARRIER to_copy_source = CD3DX12_RESOURCE_BARRIER::Transition(
        back_buffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList_->ResourceBarrier(1, &to_copy_source);

    D3D12_TEXTURE_COPY_LOCATION dst = CD3DX12_TEXTURE_COPY_LOCATION(readbackResource_.Get(), footprint);
    D3D12_TEXTURE_COPY_LOCATION src = CD3DX12_TEXTURE_COPY_LOCATION(back_buffer, 0);
    commandList_->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    D3D12_RESOURCE_BARRIER to_present = CD3DX12_RESOURCE_BARRIER::Transition(
        back_buffer, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PRESENT);
    commandList_->ResourceBarrier(1, &to_present);

    if (FAILED(commandList_->Close())) {
        return false;
    }

    ID3D12CommandList *command_lists[] = {commandList_.Get()};
    captureQueue_->ExecuteCommandLists(ARRAYSIZE(command_lists), command_lists);

    ++fenceValue_;
    if (FAILED(captureQueue_->Signal(fence_.Get(), fenceValue_))) {
        return false;
    }
    pendingCopy_ = true;
    return true;
}

bool D3D12Capture::ReadBack(std::vector<unsigned char> &pixels, UINT &width, UINT &height) {
    if (!readbackResource_.Get()) {
        return false;
    }
    void *mapped = nullptr;
    const HRESULT hr = readbackResource_->Map(0, nullptr, &mapped);
    if (FAILED(hr) || !mapped) {
        return false;
    }

    width = readbackDataWidth_;
    height = readbackDataHeight_;
    const size_t row_bytes = static_cast<size_t>(width) * 4;
    pixels.resize(row_bytes * static_cast<size_t>(height));
    const unsigned char *src = static_cast<const unsigned char *>(mapped);
    for (UINT y = 0; y < height; ++y) {
        std::memcpy(pixels.data() + static_cast<size_t>(y) * row_bytes, src + static_cast<size_t>(y) * readbackDataPitch_,
                    row_bytes);
    }

    readbackResource_->Unmap(0, nullptr);
    return true;
}

bool D3D12Capture::GrabBackBuffer(IDXGISwapChain *swapChain, std::vector<unsigned char> &pixels, UINT &width,
                                  UINT &height, DXGI_FORMAT &format, DWORD wait_ms) {
    // 多条交换链可能在不同线程同时进 Present，抓取状态是单例共享的（见头文件注释）。
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    pixels.clear();
    width = 0;
    height = 0;
    format = DXGI_FORMAT_UNKNOWN;
    if (!swapChain) {
        return false;
    }

    Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain3;
    if (FAILED(swapChain->QueryInterface(__uuidof(IDXGISwapChain3), &swapChain3))) {
        return false;
    }

    Microsoft::WRL::ComPtr<ID3D12Device> device;
    if (FAILED(swapChain->GetDevice(__uuidof(ID3D12Device), &device))) {
        return false;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> back_buffer;
    if (FAILED(swapChain->GetBuffer(swapChain3->GetCurrentBackBufferIndex(), __uuidof(ID3D12Resource), &back_buffer))) {
        return false;
    }

    const D3D12_RESOURCE_DESC desc = back_buffer->GetDesc();
    UINT64 size_in_bytes = 0;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, nullptr, nullptr, &size_in_bytes);
    if (footprint.Footprint.RowPitch == 0 || size_in_bytes == 0) {
        return false;
    }
    format = desc.Format;

    if (!EnsureResources(device.Get(), desc, footprint, size_in_bytes)) {
        return false;
    }

    if (pendingCopy_) {
        if (wait_ms > 0) {
            WaitPendingCopy(wait_ms);
        }
        if (!fence_.Get() || fence_->GetCompletedValue() < fenceValue_) {
            return false;
        }
        pendingCopy_ = false;
        return ReadBack(pixels, width, height);
    }

    SubmitCopy(back_buffer.Get(), footprint);
    if (wait_ms > 0 && pendingCopy_ && WaitPendingCopy(wait_ms)) {
        pendingCopy_ = false;
        return ReadBack(pixels, width, height);
    }
    return false;
}

void D3D12Capture::CaptureFrame(IDXGISwapChain *swapChain) {
    // 同 d3d10/d3d11：只搬被绑定窗口的交换链（§72.22）。
    DXGI_SWAP_CHAIN_DESC chain_desc = {};
    if (SUCCEEDED(swapChain->GetDesc(&chain_desc)) && DisplayHook::render_hwnd != 0 &&
        chain_desc.OutputWindow != 0 && chain_desc.OutputWindow != DisplayHook::render_hwnd) {
        return;
    }
    // 整段持锁：从"提交/读回"到"写共享段"之间不能被另一条交换链的 Present 插进来，
    // 否则写进共享段的像素可能来自另一条链、尺寸也对不上。
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    std::vector<unsigned char> pixels;
    UINT width = 0;
    UINT height = 0;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    // 不阻塞目标进程：这一帧只是把拷贝提交上去，像素下次 present 再取。
    if (!GrabBackBuffer(swapChain, pixels, width, height, format, 0)) {
        return;
    }

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
        if (SharedFrameHasCapacity(mem, width, height)) {
            static_assert(sizeof(FrameInfo) == 28);
            // 帧头与像素偏移统一由 SharedFrameWriter.h 算（§72.60）。
            const SharedFrameView sharedFrame = make_shared_frame_span(mem, width, height);

            // 通道序必须跟着交换链的实际格式走：B8G8R8A8 的帧原样搬，R8G8B8A8 的要交换
            // R/B。原来这里写死 IBF_R8G8B8A8，碰到最常见的 B8G8R8A8 交换链就会把红蓝
            // 对调（表现为纯红读成 0000FF），和 d3d10/d3d11 用 GetImageBufferFormat 的
            // 做法对齐。
            // 宿主可能只要一块（§3.2）：GPU 侧仍是整帧拷贝（D3D12 的拷贝是异步提交、
            // 下次 present 才读回，省 GPU 没意义），CPU 侧只搬请求的那几行。
            const FrameRequestSnapshot raw_request =
                ReadFrameRequestByName(DisplayHook::shared_res_name, width, height);
            const FrameRequestSnapshot request = NormalizeFrameRequest(raw_request, width, height);
            // 响应描述**这一帧缓冲区里的实际布局**（矩形/整窗），无条件写：请求 TTL 过期时
            // 注入侧退回整窗拷贝，响应必须跟着变成整窗，否则宿主会按旧矩形解释整窗帧（§72.19）。
            // 协商区不存在时 WriteFrameResponse 自己 no-op，旧宿主不受影响。
            if (request.valid) {
                const char *source = reinterpret_cast<const char *>(pixels.data()) +
                                     static_cast<size_t>(request.y) * static_cast<size_t>(width) * 4u +
                                     static_cast<size_t>(request.x) * 4u;
                write_shared_frame_region(sharedFrame, DisplayHook::render_hwnd, width, height, source, request.h,
                                          request.w, static_cast<int>(width) * 4, GetImageBufferFormat(format));
                WriteFrameResponse(mem, width, height, kFrameModeRegion, request.seq, request.x, request.y,
                                   request.w, request.h);
            } else {
                write_shared_frame(sharedFrame, DisplayHook::render_hwnd, width, height,
                                   (char *)pixels.data(), static_cast<int>(height), static_cast<int>(width),
                                   static_cast<int>(width) * 4, GetImageBufferFormat(format));
                WriteFrameResponse(mem, width, height, kFrameModeFull, 0, 0, 0, static_cast<int>(width),
                                   static_cast<int>(height));
            }
        } else {
            WriteSharedFrameHeader(mem, DisplayHook::render_hwnd, width, height);
        }
        channel.mutex().unlock();
    } else {
#if DEBUG_HOOK
        setlog(L"!mem.open(DisplayHook::%s)&&mutex.open(DisplayHook::%s)", DisplayHook::shared_res_name.c_str(),
               DisplayHook::mutex_name.c_str());
#endif // DEBUG_HOOK
    }

#if DEBUG_HOOK
    static bool first = true;
    if (first) {
        setlog("d3d12 captured %ux%u\n", width, height);
        first = false;
    }
#endif // DEBUG_HOOK

    // 只有显式要求了帧数上限才停；maxFrames_ == 0（默认、未调用 CaptureFrames）时不能把自己关掉。
    if (maxFrames_ > 0 && frameIndex_ >= maxFrames_) {
        windowHandleToCapture_ = NULL;
    }
}

void dx12_capture(IDXGISwapChain *swapChain) {
    D3D12Capture *pinst = D3D12Capture::Get();
    pinst->CaptureFrame(swapChain);
}

HRESULT __stdcall dx12_hkPresent(IDXGISwapChain *thiz, UINT SyncInterval, UINT Flags) {
    typedef long(__stdcall * Present_t)(IDXGISwapChain * pswapchain, UINT x1, UINT x2);
    // 先登记「我在 detour 里」，并留一份原函数地址。release() 要等这个计数归零
    // 才清 old_address、释放 trampoline；否则正在跑的这一次会调到空指针或已回收的代码。
    void *const original = DisplayHook::old_address;
    DisplayHook::DetourScope detour_scope;
    if (!original) {
        return S_OK;
    }
    // DXGI_PRESENT_TEST 不提交真实帧，D3D12 路径也保持和 D3D10/11 一致。
    if (DisplayHook::capture_enabled() && !(Flags & DXGI_PRESENT_TEST))
        dx12_capture(thiz);
    return ((Present_t)original)(thiz, SyncInterval, Flags);
}

} // namespace op::hook
