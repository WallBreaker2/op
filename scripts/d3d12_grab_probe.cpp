// 自检工具不是产品代码：/W3 下 std::getenv 报 C4996。这个宏必须放在**所有 include 之前**，
// 否则 CRT 头里的 deprecated 声明已经展开，宏不生效（踩过一次）。
#define _CRT_SECURE_NO_WARNINGS

// dx.d3d12 抓帧探针：在**本进程**里建一个真实 D3D12 交换链，用已知颜色清屏并 Present，
// 然后直接调用 op::hook::D3D12Capture::GrabBackBuffer 取回像素，逐像素比对。
//
// 这条路径覆盖的正是原先一帧都抓不到的代码：设备/队列/分配器/命令列表/栅栏/回读缓冲
// 的建立与拷贝。它不碰注入、不碰共享内存，所以不需要目标进程，也不需要 vcpkg/OpenCV。
//
// 编译（VS2022 x64 开发者命令行）：
//   cl /std:c++17 /EHsc /MT /O2 /I"<repo>\build\_deps\vcpkg\installed\x64-windows\include" ^
//      d3d12_grab_probe.cpp d3d12_grab_probe_stubs.cpp "..\libop\hook\D3D12Capture.cpp" ^
//      /link d3d12.lib dxgi.lib d3dcompiler.lib user32.lib ole32.lib
//
// 退出码注意：调试层开着时进程退出码会被 D3D12 运行时改成 2173（0x87D），
// 所以判定看 stdout 的 PASS/FAIL 行；要规矩的 0/1 退出码可设 OP_PROBE_NO_DEBUG_LAYER=1。
// 用法见 scripts\run_d3d12_probe.ps1 与 doc\defect_fixes.md §70.7。

#include "../libop/hook/D3D12Capture.h"

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace {

int g_failures = 0;

void Check(bool ok, const char *what) {
    std::printf("  [%s] %s\n", ok ? " OK " : "FAIL", what);
    if (!ok) {
        ++g_failures;
    }
}

LRESULT CALLBACK ProbeWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    return ::DefWindowProcW(hwnd, msg, wp, lp);
}

HWND CreateProbeWindow(int width, int height) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = ProbeWndProc;
    wc.hInstance = ::GetModuleHandleW(nullptr);
    wc.lpszClassName = L"op_d3d12_grab_probe";
    ::RegisterClassExW(&wc);

    // 不上屏也能 Present：FLIP 模型不要求窗口可见。
    HWND hwnd = ::CreateWindowExW(0, wc.lpszClassName, L"op d3d12 probe", WS_POPUP | WS_CLIPCHILDREN, 0, 0, width,
                                  height, nullptr, nullptr, wc.hInstance, nullptr);
    return hwnd;
}

struct ProbeTarget {
    HWND hwnd = nullptr;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<IDXGISwapChain3> swapchain;
    ComPtr<ID3D12Fence> fence;
    HANDLE fence_event = nullptr;
    UINT64 fence_value = 0;
    int width = 0;
    int height = 0;
    bool debug_layer = false;
    ComPtr<ID3D12InfoQueue> info_queue;
};

// 把调试层攒下来的 ERROR 级消息取出来。返回错误条数；首条错误描述写进 detail。
//
// 这一层是探针"有没有牙"的关键：抓帧必须在拷贝前后各做一次状态转换，并把它还原成 PRESENT，
// 否则目标进程下一帧的 PRESENT -> RENDER_TARGET 就是非法转换。关掉调试层时运行时对这类
// 状态错配是静默容忍的（实测删掉还原栅栏探针照过），只有打开调试层才会报
// RESOURCE_BARRIER_BEFORE_AFTER_MISMATCH。
int DrainDebugErrors(ProbeTarget &t, std::string *detail) {
    if (!t.info_queue.Get()) {
        return 0;
    }
    int errors = 0;
    const UINT64 count = t.info_queue->GetNumStoredMessages();
    for (UINT64 i = 0; i < count; ++i) {
        SIZE_T length = 0;
        if (FAILED(t.info_queue->GetMessage(i, nullptr, &length)) || length == 0) {
            continue;
        }
        std::vector<char> buffer(length);
        auto *message = reinterpret_cast<D3D12_MESSAGE *>(buffer.data());
        if (FAILED(t.info_queue->GetMessage(i, message, &length))) {
            continue;
        }
        if (message->Severity != D3D12_MESSAGE_SEVERITY_ERROR &&
            message->Severity != D3D12_MESSAGE_SEVERITY_CORRUPTION) {
            continue;
        }
        ++errors;
        if (detail && detail->empty() && message->pDescription) {
            *detail = message->pDescription;
        }
    }
    t.info_queue->ClearStoredMessages();
    return errors;
}

bool CreateTarget(ProbeTarget &t, int width, int height) {
    t.width = width;
    t.height = height;
    t.hwnd = CreateProbeWindow(width, height);
    if (!t.hwnd) {
        std::printf("CreateWindowExW failed: %lu\n", ::GetLastError());
        return false;
    }

    // 先开调试层再建设备：状态栅栏写错时只有调试层会报错，关掉时运行时静默容忍。
    // 装不了调试层（没有 Graphics Tools）时降级继续，只是探针失去这一维敏感度。
    // 调试层开着时进程退出码会被 D3D12 运行时改成 2173（0x87D），所以判定一律看 stdout 的 PASS/FAIL 行；
    // 想拿到规矩的 0/1 退出码可以设 OP_PROBE_NO_DEBUG_LAYER=1 关掉调试层重跑。
    ComPtr<ID3D12Debug> debug;
    const bool want_debug_layer = std::getenv("OP_PROBE_NO_DEBUG_LAYER") == nullptr;
    if (want_debug_layer && SUCCEEDED(::D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
        debug->EnableDebugLayer();
        std::printf("  D3D12 调试层：已启用\n");
    } else if (want_debug_layer) {
        std::printf("  D3D12 调试层：不可用（本机没装 Graphics Tools），状态栅栏错误将不可观测\n");
    } else {
        std::printf("  D3D12 调试层：按要求关闭（OP_PROBE_NO_DEBUG_LAYER）\n");
    }

    if (FAILED(::D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&t.device)))) {
        std::printf("D3D12CreateDevice failed\n");
        return false;
    }

    if (t.device.As(&t.info_queue) == S_OK) {
        t.debug_layer = true;
    }

    D3D12_COMMAND_QUEUE_DESC qdesc = {};
    qdesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    if (FAILED(t.device->CreateCommandQueue(&qdesc, IID_PPV_ARGS(&t.queue))) ||
        FAILED(t.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&t.allocator))) ||
        FAILED(t.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, t.allocator.Get(), nullptr,
                                           IID_PPV_ARGS(&t.list))) ||
        FAILED(t.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&t.fence)))) {
        std::printf("D3D12 device object creation failed\n");
        return false;
    }
    t.list->Close();
    t.fence_event = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);

    ComPtr<IDXGIFactory4> factory;
    ComPtr<IDXGIAdapter> adapter;
    if (FAILED(::CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        std::printf("CreateDXGIFactory1 failed\n");
        return false;
    }
    if (FAILED(factory->EnumAdapters(0, &adapter))) {
        std::printf("EnumAdapters failed\n");
        return false;
    }

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = static_cast<UINT>(width);
    desc.Height = static_cast<UINT>(height);
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    ComPtr<IDXGISwapChain1> sc1;
    if (FAILED(factory->CreateSwapChainForHwnd(t.queue.Get(), t.hwnd, &desc, nullptr, nullptr, &sc1))) {
        std::printf("CreateSwapChainForHwnd failed\n");
        return false;
    }
    if (FAILED(sc1.As(&t.swapchain))) {
        std::printf("IDXGISwapChain3 unavailable\n");
        return false;
    }
    return true;
}

// 用给定颜色清屏，把命令提交并等 GPU 跑完，但**不** Present。
//
// 时序必须和产品一致：D3D12Capture 是在 IDXGISwapChain::Present 里、原函数之前抓帧的，
// 那时 IDXGISwapChain3::GetCurrentBackBufferIndex() 指的就是刚渲染好的这一帧；
// 如果放到 Present 之后再抓，拿到的是下一个待渲染缓冲（内容落后一到两帧），
// 所以这里先渲染+抓帧，最后才 Present。
bool RenderSolid(ProbeTarget &t, UINT8 r, UINT8 g, UINT8 b) {
    ComPtr<ID3D12Resource> back_buffer;
    if (FAILED(t.swapchain->GetBuffer(t.swapchain->GetCurrentBackBufferIndex(), IID_PPV_ARGS(&back_buffer)))) {
        return false;
    }

    t.allocator->Reset();
    t.list->Reset(t.allocator.Get(), nullptr);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = back_buffer.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    t.list->ResourceBarrier(1, &barrier);

    // ClearRenderTargetView 需要 RTV，这里省掉 RTV 堆，直接用一个临时 RTV 描写符。
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = {};
    ComPtr<ID3D12DescriptorHeap> rtv_heap;
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heap_desc.NumDescriptors = 1;
    if (FAILED(t.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&rtv_heap)))) {
        return false;
    }
    rtv = rtv_heap->GetCPUDescriptorHandleForHeapStart();
    t.device->CreateRenderTargetView(back_buffer.Get(), nullptr, rtv);
    const float color[4] = {r / 255.0f, g / 255.0f, b / 255.0f, 1.0f};
    t.list->ClearRenderTargetView(rtv, color, 0, nullptr);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    t.list->ResourceBarrier(1, &barrier);
    if (FAILED(t.list->Close())) {
        return false;
    }

    ID3D12CommandList *lists[] = {t.list.Get()};
    t.queue->ExecuteCommandLists(1, lists);

    ++t.fence_value;
    if (FAILED(t.queue->Signal(t.fence.Get(), t.fence_value))) {
        return false;
    }
    if (t.fence->GetCompletedValue() < t.fence_value) {
        t.fence->SetEventOnCompletion(t.fence_value, t.fence_event);
        ::WaitForSingleObject(t.fence_event, 5000);
    }
    return true;
}

// 取一帧：第一轮只提交拷贝，第二轮读回。返回是否拿到像素。
bool GrabOneFrame(op::hook::D3D12Capture *capture, ProbeTarget &t, std::vector<unsigned char> &pixels, UINT &w,
                  UINT &h) {
    // 交换链是 DXGI_FORMAT_B8G8R8A8_UNORM（见 CreateTarget）。GrabBackBuffer 会把后台缓冲的
    // 实际格式回填出来，上层据此决定写共享段时要不要换通道序 —— 写死一种格式会让红蓝颠倒
    // （d3d12 后端就出过这个错，见 doc/defect_fixes.md §72.4），这里把回填值也钉一下。
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    static bool format_checked = false;
    const bool first = capture->GrabBackBuffer(t.swapchain.Get(), pixels, w, h, format, 0);
    if (first) {
        // 理论上首次调用只提交、不返回像素；如果立刻有数据也接受，继续判内容。
        return true;
    }
    const bool second = capture->GrabBackBuffer(t.swapchain.Get(), pixels, w, h, format, 5000);
    if (second && !format_checked) {
        format_checked = true;
        Check(format == DXGI_FORMAT_B8G8R8A8_UNORM, "GrabBackBuffer 回填的格式等于交换链格式（B8G8R8A8）");
    }
    return second;
}

void CheckSolidColor(const std::vector<unsigned char> &pixels, UINT w, UINT h, UINT8 r, UINT8 g, UINT8 b,
                     const char *what) {
    bool ok = (w == 64 && h == 48) && pixels.size() == static_cast<size_t>(w) * h * 4;
    if (ok) {
        // DXGI_FORMAT_B8G8R8A8_UNORM：内存里依次是 B,G,R,A。
        for (size_t i = 0; i < pixels.size() && ok; i += 4) {
            if (pixels[i + 0] != b || pixels[i + 1] != g || pixels[i + 2] != r || pixels[i + 3] != 0xFF) {
                ok = false;
                std::printf("    first mismatch at pixel %zu: got BGR=(%u,%u,%u,%u) want (%u,%u,%u,255)\n", i / 4,
                            pixels[i + 0], pixels[i + 1], pixels[i + 2], pixels[i + 3], b, g, r);
            }
        }
    } else {
        std::printf("    size mismatch: %ux%u bytes=%zu (want 64x48 bytes=12288)\n", w, h, pixels.size());
    }
    Check(ok, what);
}

// 渲染一帧 → 抓帧 → 校验像素 → 再 Present。返回是否拿到像素。
bool FrameWithExpectedColor(op::hook::D3D12Capture *capture, ProbeTarget &t, UINT8 r, UINT8 g, UINT8 b,
                            std::vector<unsigned char> &pixels, UINT &w, UINT &h) {
    if (!RenderSolid(t, r, g, b)) {
        return false;
    }
    pixels.clear();
    w = h = 0;
    const bool got = GrabOneFrame(capture, t, pixels, w, h);
    // Present 必须在抓帧之后：抓帧只做 PRESENT -> COPY_SOURCE -> PRESENT，交还给 Present 是合法的。
    const HRESULT present = t.swapchain->Present(1, 0);
    if (FAILED(present)) {
        std::printf("    Present failed: 0x%08lX\n", static_cast<unsigned long>(present));
    }
    std::string detail;
    const int debug_errors = DrainDebugErrors(t, &detail);
    if (debug_errors > 0) {
        std::printf("    D3D12 调试层报 %d 条 ERROR：%s\n", debug_errors, detail.c_str());
    }
    return got && SUCCEEDED(present) && debug_errors == 0;
}

} // namespace

int main() {
    // 探针崩在调试层里时不能让 printf 缓冲把证据带走。
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::printf("dx.d3d12 GrabBackBuffer probe\n");

    ProbeTarget target;
    if (!CreateTarget(target, 64, 48)) {
        std::printf("FATAL: cannot create D3D12 target\n");
        return 1;
    }
    Check(target.swapchain.Get() != nullptr, "D3D12 交换链创建成功");

    auto *capture = op::hook::D3D12Capture::Get();

    std::vector<unsigned char> pixels;
    UINT w = 0, h = 0;

    // 第一帧：纯红。抓帧器此前从没抓到过任何一帧，这条用例就是它的第一帧。
    const bool got1 = FrameWithExpectedColor(capture, target, 255, 0, 0, pixels, w, h);
    Check(got1, "第一帧取回像素（提交拷贝 + 读回 + Present 成功）");
    if (got1) {
        CheckSolidColor(pixels, w, h, 255, 0, 0, "第一帧像素等于清屏色（红）");
    }

    // 第二帧：换颜色，确认不是缓存了上一帧。
    const bool got2 = FrameWithExpectedColor(capture, target, 0, 128, 255, pixels, w, h);
    Check(got2, "第二帧取回像素");
    if (got2) {
        CheckSolidColor(pixels, w, h, 0, 128, 255, "第二帧跟随清屏色变化（蓝）");
    }

    // 连续多帧：每帧都要能拿到当前帧的颜色，且目标 Present 不失败。
    int frames = 0;
    for (int i = 0; i < 8; ++i) {
        const UINT8 value = static_cast<UINT8>(16 * (i + 1));
        std::vector<unsigned char> frame_pixels;
        UINT fw = 0, fh = 0;
        if (!FrameWithExpectedColor(capture, target, value, value, value, frame_pixels, fw, fh)) {
            std::printf("    第 %d 帧取帧失败\n", i + 1);
            continue;
        }
        bool same = fw == static_cast<UINT>(target.width) && fh == static_cast<UINT>(target.height) &&
                    frame_pixels.size() == static_cast<size_t>(fw) * fh * 4;
        for (size_t p = 0; same && p < frame_pixels.size(); p += 4) {
            same = frame_pixels[p] == value && frame_pixels[p + 1] == value && frame_pixels[p + 2] == value;
        }
        if (same) {
            ++frames;
        } else {
            std::printf("    第 %d 帧像素不是期望的灰度 %u\n", i + 1, value);
        }
    }
    Check(frames == 8, "连续 8 帧都取回当前帧像素（且 Present 未失败）");

    std::printf("\n%s: %d failure(s)\n", g_failures == 0 ? "PASS" : "FAIL", g_failures);
    return g_failures == 0 ? 0 : 1;
}
