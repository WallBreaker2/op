#pragma once

#include <Windows.h>

#include <cstdint>
#include <directx/d3d12.h>
#include <dxgi1_6.h>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>
#include <wrl/client.h>

namespace op::hook {

HRESULT __stdcall dx12_hkPresent(IDXGISwapChain *thiz, UINT SyncInterval, UINT Flags);

class D3D12Capture final {
  public:
    static D3D12Capture *Get();

    HRESULT CaptureFrames(HWND windowHandleToCapture, std::wstring_view folderToSaveFrames, int maxFrames);
    void CaptureFrame(IDXGISwapChain *swapChain);

    // 把交换链当前后台缓冲抓成紧凑的 RGBA 像素（top-down，行间无 padding）。
    //
    // 语义沿用"本次 present 发起拷贝、下次读取"的原有设计：
    //   - 没有在途拷贝时：提交一次 GPU 拷贝，返回 false；
    //   - 在途拷贝已完成：读回像素填进 pixels，返回 true；
    //   - 在途拷贝还没跑完且 wait_ms == 0：直接返回 false（不阻塞目标进程）；
    //     wait_ms > 0 时先等拷贝完成再读。
    //
    // 这一层单独拎出来是为了能脱离共享内存做验证：CaptureFrame 只负责把它拿到的像素
    // 写进共享段，抓取本身（设备、队列、栅栏、回读缓冲）与 IPC 无关。
    // format 回填交换链后台缓冲的 DXGI 格式：ReadBack 出来的是紧凑的 width*4 行，
    // 但像素到底按 BGRA 还是 RGBA 排布由交换链决定，写共享段时必须照着它选通道序
    // （写死一种格式会让 R/B 颠倒，见 doc/defect_fixes.md 里 d3d12 通道序那条）。
    bool GrabBackBuffer(IDXGISwapChain *swapChain, std::vector<unsigned char> &pixels, UINT &width, UINT &height,
                        DXGI_FORMAT &format, DWORD wait_ms = 0);

    // 等上一次提交的拷贝完成（只等它在自己那条队列上的栅栏，不碰目标进程的队列）。
    bool WaitPendingCopy(DWORD wait_ms);

  private:
    D3D12Capture();
    ~D3D12Capture();

    // 设备/尺寸变化时重建自己的拷贝资源。返回 false 表示本帧放弃。
    bool EnsureResources(ID3D12Device *device, const D3D12_RESOURCE_DESC &back_buffer_desc,
                         const D3D12_PLACED_SUBRESOURCE_FOOTPRINT &footprint, UINT64 size_in_bytes);
    bool SubmitCopy(ID3D12Resource *back_buffer, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT &footprint);
    bool ReadBack(std::vector<unsigned char> &pixels, UINT &width, UINT &height);

    std::uint64_t presentPointer_ = 0;
    std::uint64_t presentTrampoline_ = 0;

    // 抓取状态（命令列表/栅栏/回读缓冲/pendingCopy_）是单例共享的，而一个进程里
    // 可能有多条交换链、各自在不同线程 Present（真游戏、编辑器多视口都这样）。
    // 不加锁时两条链会互相 Reset 对方的在途命令列表，栅栏再也等不到，
    // 共享段就冻在第一帧（表现为"6 次采样只有一种颜色"）。
    // 递归锁：CaptureFrame 持锁后会再调 GrabBackBuffer，同一线程不能自锁死。
    std::recursive_mutex mutex_;

    // 自己建一条 DIRECT 队列来做拷贝。
    //
    // 原实现想去交换链对象里按偏移摸出目标进程的 ID3D12CommandQueue，但那个偏移
    // 全仓没有任何赋值点（恒为 0），所以 dx.d3d12 一帧也抓不到；而把 swapchain 的
    // vtable 指针当队列用会让目标进程野跳崩溃。D3D12 允许同一设备上有多条队列，
    // 我们自己建一条，拷贝完再把后台缓冲的状态还原成 PRESENT 交还给目标进程。
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> captureQueue_;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    HANDLE fenceEvent_ = nullptr;
    UINT64 fenceValue_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> readbackResource_;
    UINT readbackDataWidth_ = 0;
    UINT readbackDataHeight_ = 0;
    UINT readbackDataPitch_ = 0;

    bool pendingCopy_ = false;

    HWND windowHandleToCapture_ = NULL;
    std::wstring folderToSaveFrames_;
    int frameIndex_ = 0;
    int maxFrames_ = 0;
};

} // namespace op::hook
