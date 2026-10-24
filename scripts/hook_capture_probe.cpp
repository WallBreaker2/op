// 后台（hook）截图探针：在**本进程**里建真实 D3D11 交换链，直接调 op::hook::dx11_capture()，
// 量每次调用花多少时间、共享内存里的像素对不对。
//
// 为什么需要它：dx.dxgi / dx.d3d11 这类"后台截图"是注入到目标进程里、在 **Present 里同步干活**的，
// 每帧 CreateTexture2D + CopyResource + Map（阻塞等 GPU）全部落在目标渲染线程上，直接影响目标帧率。
// 没有真实游戏也能量化这部分成本：本地建交换链，反复 Present + dx11_capture，统计耗时。
//
// 编译运行：scripts\run_hook_capture_probe.ps1
// 参数：
//   --w=640 --h=360        交换链尺寸
//   --iters=120            迭代次数
//   --format=bgra|rgba     后备缓冲格式（rgba 会走 CopyImageData 的通道交换路径）
//   --show                 显示窗口（默认隐藏）
//
// 退出码不可信（调试层/GPU 时序），判定看 stdout 的 PASS/FAIL 行，同 d3d12_grab_probe。

#define _CRT_SECURE_NO_WARNINGS

#include "../libop/base/AutomationModes.h"
#include "../libop/capture/FrameInfo.h"
#include "../libop/hook/D3D11Capture.h"
#include "../libop/hook/DisplayHook.h"
#include "../libop/hook/DxCaptureCommon.h"
#include "../libop/hook/SharedFrame.h"
#include "../libop/hook/SharedFrameChannel.h"
#include "../libop/hook/SharedFrameExchange.h"
#include "../libop/ipc/ProcessMutex.h"
#include "../libop/ipc/SharedMemory.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <immintrin.h>
#include <cstring>
#include <vector>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <windows.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

// D3D11Capture.cpp 里没有把这个函数导到头文件（产品里只经 Present 钩子调用）。
namespace op::hook {
void dx11_capture(IDXGISwapChain *swapchain);
}

namespace {

int g_failures = 0;

void Check(bool cond, const char *what, const char *detail = "") {
    if (cond) {
        std::printf("  [ OK ] %s\n", what);
    } else {
        std::printf("  [FAIL] %s %s\n", what, detail);
        ++g_failures;
    }
}

struct Options {
    int width = 640;
    int height = 360;
    int iters = 120;
    bool rgba = false;
    bool show = false;
    bool phase = false;
    bool copybench = false;
};

Options ParseArgs(int argc, char **argv) {
    Options opt;
    for (int i = 1; i < argc; ++i) {
        const char *a = argv[i];
        if (std::strncmp(a, "--w=", 4) == 0) {
            opt.width = std::atoi(a + 4);
        } else if (std::strncmp(a, "--h=", 4) == 0) {
            opt.height = std::atoi(a + 4);
        } else if (std::strncmp(a, "--iters=", 8) == 0) {
            opt.iters = std::atoi(a + 8);
        } else if (std::strcmp(a, "--rgba") == 0) {
            opt.rgba = true;
        } else if (std::strcmp(a, "--show") == 0) {
            opt.show = true;
        } else if (std::strcmp(a, "--phase") == 0) {
            opt.phase = true;
        } else if (std::strcmp(a, "--copybench") == 0) {
            opt.copybench = true;
        }
    }
    if (opt.width < 16)
        opt.width = 16;
    if (opt.height < 16)
        opt.height = 16;
    if (opt.iters < 1)
        opt.iters = 1;
    return opt;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_DESTROY) {
        ::PostQuitMessage(0);
    }
    return ::DefWindowProcW(hwnd, msg, wparam, lparam);
}

HWND CreateProbeWindow(const Options &opt) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = ::GetModuleHandleW(nullptr);
    wc.lpszClassName = L"op_hook_capture_probe";
    ::RegisterClassExW(&wc);

    const DWORD style = WS_OVERLAPPEDWINDOW;
    RECT rc = {0, 0, opt.width, opt.height};
    ::AdjustWindowRect(&rc, style, FALSE);
    HWND hwnd = ::CreateWindowExW(0, wc.lpszClassName, L"hook capture probe", opt.show ? style : 0, 0, 0,
                                  rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, wc.hInstance, nullptr);
    if (hwnd && opt.show) {
        ::ShowWindow(hwnd, SW_SHOW);
    }
    return hwnd;
}

struct ProbeTarget {
    HWND hwnd = nullptr;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain> swapchain;
    ComPtr<ID3D11RenderTargetView> rtv;
    DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM;
};

bool InitTarget(const Options &opt, ProbeTarget &target) {
    target.hwnd = CreateProbeWindow(opt);
    if (!target.hwnd) {
        std::printf("  [FAIL] CreateWindowEx 失败 err=%lu\n", ::GetLastError());
        return false;
    }
    target.format = opt.rgba ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_B8G8R8A8_UNORM;

    UINT create_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1};
    D3D_FEATURE_LEVEL got_level = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = ::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_flags, levels,
                                     _countof(levels), D3D11_SDK_VERSION, &target.device, &got_level,
                                     &target.context);
    if (FAILED(hr)) {
        // 没有硬件 D3D11 时退 WARP，探针照样能跑（速度不代表真实 GPU）。
        hr = ::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, create_flags, levels, _countof(levels),
                                 D3D11_SDK_VERSION, &target.device, &got_level, &target.context);
        if (FAILED(hr)) {
            std::printf("  [FAIL] D3D11CreateDevice hr=0x%08lX\n", static_cast<unsigned long>(hr));
            return false;
        }
        std::printf("  [INFO] 使用 WARP 软件设备（本机无可用 D3D11 硬件）\n");
    }

    ComPtr<IDXGIDevice> dxgi_device;
    ComPtr<IDXGIAdapter> adapter;
    ComPtr<IDXGIFactory> factory;
    if (FAILED(target.device.As(&dxgi_device)) || FAILED(dxgi_device->GetAdapter(&adapter)) ||
        FAILED(adapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void **>(factory.GetAddressOf())))) {
        std::printf("  [FAIL] 取 IDXGIFactory 失败\n");
        return false;
    }

    DXGI_SWAP_CHAIN_DESC desc = {};
    desc.BufferCount = 1;
    desc.BufferDesc.Width = static_cast<UINT>(opt.width);
    desc.BufferDesc.Height = static_cast<UINT>(opt.height);
    desc.BufferDesc.Format = target.format;
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = target.hwnd;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    hr = factory->CreateSwapChain(target.device.Get(), &desc, &target.swapchain);
    if (FAILED(hr)) {
        std::printf("  [FAIL] CreateSwapChain hr=0x%08lX\n", static_cast<unsigned long>(hr));
        return false;
    }

    ComPtr<ID3D11Texture2D> backbuffer;
    if (FAILED(target.swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(backbuffer.GetAddressOf())))) {
        std::printf("  [FAIL] GetBuffer 失败\n");
        return false;
    }
    return SUCCEEDED(target.device->CreateRenderTargetView(backbuffer.Get(), nullptr, &target.rtv));
}

// 用已知颜色清屏并 Present，返回 0..255 的分量（RGB）。
void RenderFrame(ProbeTarget &target, int r, int g, int b) {
    const float color[4] = {r / 255.0f, g / 255.0f, b / 255.0f, 1.0f};
    target.context->ClearRenderTargetView(target.rtv.Get(), color);
    target.swapchain->Present(0, 0);
}

struct PixelStats {
    double mean_ms = 0;
    double p50_ms = 0;
    double p95_ms = 0;
    double max_ms = 0;
};

// 非临时存储（streaming）整块写：目的地 16 字节对齐后走 movntdq，收尾 sfence。
// 目标是「只写一遍、之后很久不再读」的大块内存（共享段就是这么用的），
// 绕开 cache 预取带来的 read-for-ownership，实测比 memcpy 快。
double NowMs(const LARGE_INTEGER &freq);

void streaming_copy(unsigned char *dst, const unsigned char *src, size_t bytes) {
    size_t prefixed = 0;
    while (prefixed < bytes && (reinterpret_cast<uintptr_t>(dst + prefixed) & 15) != 0) {
        dst[prefixed] = src[prefixed];
        ++prefixed;
    }
    size_t i = prefixed;
    for (; i + 64 <= bytes; i += 64) {
        const __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i));
        const __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i + 16));
        const __m128i c = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i + 32));
        const __m128i d = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i + 48));
        _mm_stream_si128(reinterpret_cast<__m128i *>(dst + i), a);
        _mm_stream_si128(reinterpret_cast<__m128i *>(dst + i + 16), b);
        _mm_stream_si128(reinterpret_cast<__m128i *>(dst + i + 32), c);
        _mm_stream_si128(reinterpret_cast<__m128i *>(dst + i + 48), d);
    }
    for (; i + 16 <= bytes; i += 16) {
        _mm_stream_si128(reinterpret_cast<__m128i *>(dst + i),
                         _mm_loadu_si128(reinterpret_cast<const __m128i *>(src + i)));
    }
    _mm_sfence();
    for (; i < bytes; ++i) {
        dst[i] = src[i];
    }
}

// 整帧搬运方案的横向对比：同一份像素，分别测 memcpy 写共享段、streaming 写共享段、
// 从共享段读回本地堆。用来决定「写共享段」该走哪条路径。
bool RunCopyBench(ProbeTarget &target, const Options &opt, op::SharedMemory &shmem) {
    const size_t bytes = static_cast<size_t>(opt.width) * static_cast<size_t>(opt.height) * 4;
    auto *server = static_cast<unsigned char *>(std::malloc(bytes));
    if (!server) {
        std::printf("  [FAIL] 拷贝对比：内存不足\n");
        return false;
    }
    for (size_t i = 0; i < bytes; i += 4096) {
        server[i] = static_cast<unsigned char>(i);
    }
    unsigned char *shared_pixels = shmem.data<unsigned char>() + sizeof(op::capture::FrameInfo);
    LARGE_INTEGER freq = {};
    ::QueryPerformanceFrequency(&freq);

    double local_ms = 0;
    double shared_ms = 0;
    double stream_ms = 0;
    // 宿主（op_x64.dll）是在另一个进程/线程里紧跟读取的。非临时写绕过 cache，
    // 如果读端紧跟着读同一段，就得从内存重新取 —— 收益可能在读侧还回去。
    // 这两项就是「写完之后立刻读回」的耗时，用来判断这笔账是不是正收益。
    double read_after_memcpy_ms = 0;
    double read_after_stream_ms = 0;
    const int rounds = (std::max)(10, opt.iters / 2);
    for (int i = 0; i < rounds; ++i) {
        const double t0 = NowMs(freq);
        ::memcpy(shared_pixels, server, bytes); // 写共享段（常规做法）
        const double t1 = NowMs(freq);
        shared_ms += t1 - t0;

        const double t2 = NowMs(freq);
        ::memcpy(server, shared_pixels, bytes); // 紧跟常规写的读回
        const double t3 = NowMs(freq);
        read_after_memcpy_ms += t3 - t2;

        const double t4 = NowMs(freq);
        streaming_copy(shared_pixels, server, bytes); // 非临时写共享段
        const double t5 = NowMs(freq);
        stream_ms += t5 - t4;

        const double t6 = NowMs(freq);
        ::memcpy(server, shared_pixels, bytes); // 紧跟非临时写的读回
        const double t7 = NowMs(freq);
        read_after_stream_ms += t7 - t6;

        const double t8 = NowMs(freq);
        ::memcpy(server, shared_pixels, bytes); // 纯带宽参照（此时镜头内已常驻）
        const double t9 = NowMs(freq);
        local_ms += t9 - t8;
    }
    const double n = static_cast<double>(rounds);
    const double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
    const double gbps_shared = (mb / 1024.0) / (shared_ms / n / 1000.0);
    const double gbps_stream = (mb / 1024.0) / (stream_ms / n / 1000.0);
    const double gbps_read = (mb / 1024.0) / (local_ms / n / 1000.0);
    std::printf("整帧 %.2f MB 搬运对比（每次平均，%d 次）：\n", mb, rounds);
    std::printf("  memcpy→共享段      %7.3f ms  %.1f GB/s\n", shared_ms / n, gbps_shared);
    std::printf("  流式写→共享段      %7.3f ms  %.1f GB/s\n", stream_ms / n, gbps_stream);
    std::printf("  写后立即读回(memcpy) %7.3f ms\n", read_after_memcpy_ms / n);
    std::printf("  写后立即读回(流式)   %7.3f ms\n", read_after_stream_ms / n);
    std::printf("  常驻读回→本地      %7.3f ms  %.1f GB/s\n", local_ms / n, gbps_read);
    std::printf("  「写+紧跟读」合计   memcpy=%.3f ms  流式=%.3f ms\n", (shared_ms + read_after_memcpy_ms) / n,
                (stream_ms + read_after_stream_ms) / n);
    std::printf(
        "METRIC copy_shared_ms=%.4f copy_stream_ms=%.4f copy_readback_ms=%.4f read_after_memcpy_ms=%.4f "
        "read_after_stream_ms=%.4f\n",
        shared_ms / n, stream_ms / n, local_ms / n, read_after_memcpy_ms / n, read_after_stream_ms / n);
    std::free(server);
    (void)target;
    return true;
}

// 分阶段计时：探针自己复刻 dx11_capture 的每一步，看时间到底花在哪一步。
// 不改产品代码 —— 只为了知道该优化哪一段。
struct PhaseStats {
    double get_buffer = 0;
    double copy = 0;
    double map = 0;
    double open_shared = 0;
    double memcpy_shared = 0;
    double unmap = 0;
};

double NowMs(const LARGE_INTEGER &freq) {
    LARGE_INTEGER t = {};
    ::QueryPerformanceCounter(&t);
    return static_cast<double>(t.QuadPart) * 1000.0 / static_cast<double>(freq.QuadPart);
}

bool RunPhaseProfile(ProbeTarget &target, const Options &opt, op::SharedMemory &shmem, op::ProcessMutex &mutex) {
    (void)shmem;
    (void)mutex;
    D3D11_TEXTURE2D_DESC td = {};
    td.Format = target.format;
    td.Width = static_cast<UINT>(opt.width);
    td.Height = static_cast<UINT>(opt.height);
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_STAGING;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> staging;
    ComPtr<ID3D11DeviceContext> ctx;
    if (FAILED(target.device->CreateTexture2D(&td, nullptr, &staging))) {
        std::printf("  [FAIL] 分阶段计时：CreateTexture2D 失败\n");
        return false;
    }
    target.device->GetImmediateContext(&ctx);

    const int fmt = op::hook::GetImageBufferFormat(target.format);
    const std::wstring shm_name = ::MakeOpSharedResourceName(target.hwnd);
    const std::wstring mutex_name = ::MakeOpMutexName(target.hwnd);
    LARGE_INTEGER freq = {};
    ::QueryPerformanceFrequency(&freq);
    PhaseStats total;

    for (int i = 0; i < opt.iters; ++i) {
        RenderFrame(target, 255, 0, 0);
        double t0 = NowMs(freq);
        DXGI_SWAP_CHAIN_DESC desc;
        target.swapchain->GetDesc(&desc);
        ComPtr<ID3D11Resource> backbuffer;
        if (FAILED(target.swapchain->GetBuffer(0, __uuidof(ID3D11Resource),
                                               reinterpret_cast<void **>(backbuffer.GetAddressOf())))) {
            std::printf("  [FAIL] 分阶段计时：GetBuffer 失败\n");
            return false;
        }
        double t1 = NowMs(freq);
        ctx->CopyResource(staging.Get(), backbuffer.Get());
        double t2 = NowMs(freq);
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (FAILED(ctx->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) {
            std::printf("  [FAIL] 分阶段计时：Map 失败\n");
            return false;
        }
        double t3 = NowMs(freq);
        // 产品代码按名字缓存映射与互斥体（SharedFrameChannelLease）：名字不变时零系统调用，
        // 只在首次和每 64 帧的自愈重开时做一次 MapViewOfFile。
        op::hook::SharedFrameChannelLease channel(shm_name, mutex_name);
        double t4 = NowMs(freq);
        if (channel.ok() && op::hook::SharedFrameHasCapacity(channel.mem(), static_cast<uint64_t>(opt.width),
                                                             static_cast<uint64_t>(opt.height))) {
            channel.mutex().lock();
            op::hook::CopyImageData(
                reinterpret_cast<char *>(channel.mem().data<unsigned char>() + sizeof(op::capture::FrameInfo)),
                static_cast<const char *>(mapped.pData), opt.height, opt.width, static_cast<int>(mapped.RowPitch),
                fmt);
            channel.mutex().unlock();
        }
        double t5 = NowMs(freq);
        ctx->Unmap(staging.Get(), 0);
        double t6 = NowMs(freq);

        total.get_buffer += t1 - t0;
        total.copy += t2 - t1;
        total.map += t3 - t2;
        total.open_shared += t4 - t3;
        total.memcpy_shared += t5 - t4;
        total.unmap += t6 - t5;
    }

    const double n = static_cast<double>(opt.iters);
    const double sum =
        total.get_buffer + total.copy + total.map + total.open_shared + total.memcpy_shared + total.unmap;
    auto row = [&](const char *name, double value) {
        std::printf("  %-18s %7.3f ms  %5.1f%%\n", name, value / n, sum > 0 ? value * 100.0 / sum : 0.0);
    };
    std::printf("分阶段耗时（每帧平均，共 %d 帧，合计 %.3f ms/帧）：\n", opt.iters, sum / n);
    row("GetDesc+GetBuffer", total.get_buffer);
    row("CopyResource", total.copy);
    row("Map(等 GPU)", total.map);
    row("open 共享段/互斥体", total.open_shared);
    row("memcpy→共享段", total.memcpy_shared);
    row("Unmap", total.unmap);
    return true;
}

PixelStats Summarize(std::vector<double> samples) {
    PixelStats stats;
    if (samples.empty()) {
        return stats;
    }
    std::sort(samples.begin(), samples.end());
    double sum = 0;
    for (double v : samples) {
        sum += v;
    }
    stats.mean_ms = sum / static_cast<double>(samples.size());
    stats.p50_ms = samples[samples.size() / 2];
    stats.p95_ms = samples[static_cast<size_t>(static_cast<double>(samples.size() - 1) * 0.95)];
    stats.max_ms = samples.back();
    return stats;
}

} // namespace

// ---------------------------------------------------------------------------
// §3.2 只搬请求区域：注入侧必须按宿主发布的矩形搬，并把结果登记在响应里。
// 这一段直接调 dx11_capture，不经过宿主，所以能精确断言"搬了多少、搬的是哪块"。
// ---------------------------------------------------------------------------
void CheckRegionTransport(ProbeTarget &target, const Options &opt, op::SharedMemory &shmem,
                          op::capture::FrameInfo *info) {
    const std::uint32_t w = static_cast<std::uint32_t>(opt.width);
    const std::uint32_t h = static_cast<std::uint32_t>(opt.height);
    Check(op::hook::FrameExchangeFits(shmem, w, h), "共享段容得下请求协商区（尾部 72 字节）");

    const int rx = 64;
    const int ry = 32;
    const int rw = 160;
    const int rh = 90;

    // 1) 发布区域请求 → 这一帧只搬该矩形
    op::hook::PublishFrameRequest(shmem, w, h, rx, ry, rw, rh, 250);
    RenderFrame(target, 0, 255, 0); // 纯绿
    op::hook::dx11_capture(target.swapchain.Get());
    op::hook::dx11_capture(target.swapchain.Get()); // ping-pong 模式下读的是上一帧
    op::hook::FrameResponseSnapshot response = op::hook::ReadFrameResponse(shmem, w, h);
    char detail[192] = {};
    std::snprintf(detail, sizeof(detail), "valid=%d mode=%u rect=(%d,%d,%d,%d)", response.valid ? 1 : 0, response.mode,
                  response.x, response.y, response.w, response.h);
    Check(response.valid && response.mode == op::hook::kFrameModeRegion && response.x == rx && response.y == ry &&
              response.w == rw && response.h == rh,
          "区域请求生效：响应登记 mode=Region 且矩形一致", detail);

    // 2) 像素区里只有该矩形，行距 = rw*4
    unsigned char *region = shmem.data<unsigned char>() + sizeof(op::capture::FrameInfo);
    bool all_green = true;
    for (int y = 0; y < rh && all_green; ++y) {
        const unsigned char *row = region + static_cast<size_t>(y) * static_cast<size_t>(rw) * 4;
        for (int x = 0; x < rw; ++x) {
            const unsigned char *px = row + static_cast<size_t>(x) * 4;
            if (!(px[0] == 0 && px[1] == 255 && px[2] == 0)) {
                all_green = false;
                char bad[160] = {};
                std::snprintf(bad, sizeof(bad), "像素区(%d,%d)=(%u,%u,%u)，期望 BGR=(0,255,0)", x, y, px[0], px[1],
                              px[2]);
                Check(false, "区域帧的像素区只包含请求矩形（紧凑 rw*4 行距）", bad);
                break;
            }
        }
    }
    if (all_green) {
        Check(true, "区域帧的像素区只包含请求矩形（紧凑 rw*4 行距）");
    }

    // 3) 帧头仍记整窗尺寸（协商区偏移与旧宿主都靠它）
    Check(info->width == w && info->height == h, "区域帧的帧头仍记整窗尺寸");

    // 4) 越界请求被忽略 → 退回整窗
    op::hook::PublishFrameRequest(shmem, w, h, opt.width - 10, 0, 100, 100, 250);
    RenderFrame(target, 0, 0, 255);
    op::hook::dx11_capture(target.swapchain.Get());
    op::hook::dx11_capture(target.swapchain.Get());
    response = op::hook::ReadFrameResponse(shmem, w, h);
    Check(response.valid && response.mode == op::hook::kFrameModeFull, "越界请求被忽略：退回整窗搬运");

    // 5) 整窗请求 → mode=Full
    op::hook::PublishFrameRequest(shmem, w, h, 0, 0, opt.width, opt.height, 250);
    op::hook::dx11_capture(target.swapchain.Get());
    op::hook::dx11_capture(target.swapchain.Get());
    response = op::hook::ReadFrameResponse(shmem, w, h);
    Check(response.valid && response.mode == op::hook::kFrameModeFull, "整窗请求生效：mode=Full");
}

// §3.2 的收益量化：同一份代码，一次搬整窗、一次只搬 1/16 的矩形，比 mean 耗时。
// 整窗那组已经在主流程里测过（capture_mean_ms），这里只补区域那组。
void MeasureRegionCapture(ProbeTarget &target, const Options &opt, op::SharedMemory &shmem, double full_mean_ms) {
    const std::uint32_t w = static_cast<std::uint32_t>(opt.width);
    const std::uint32_t h = static_cast<std::uint32_t>(opt.height);
    const int rw = static_cast<int>(w / 4);
    const int rh = static_cast<int>(h / 4);
    if (rw <= 0 || rh <= 0) {
        return;
    }
    // ttl 给足，避免测到一半请求过期退回整窗。
    op::hook::PublishFrameRequest(shmem, w, h, 0, 0, rw, rh, 60000);

    std::vector<double> samples;
    samples.reserve(static_cast<size_t>(opt.iters));
    LARGE_INTEGER freq = {};
    ::QueryPerformanceFrequency(&freq);
    for (int i = 0; i < opt.iters; ++i) {
        RenderFrame(target, 0, 255, 0);
        LARGE_INTEGER begin = {};
        LARGE_INTEGER end = {};
        ::QueryPerformanceCounter(&begin);
        op::hook::dx11_capture(target.swapchain.Get());
        ::QueryPerformanceCounter(&end);
        samples.push_back((end.QuadPart - begin.QuadPart) * 1000.0 / static_cast<double>(freq.QuadPart));
    }
    const PixelStats region_stats = Summarize(samples);
    std::printf("  只搬 %dx%d 区域时每次 dx11_capture：mean=%.3fms p50=%.3fms p95=%.3fms\n", rw, rh,
                region_stats.mean_ms, region_stats.p50_ms, region_stats.p95_ms);
    std::printf("METRIC capture_region_mean_ms=%.4f capture_region_p95_ms=%.4f region_bytes_ratio=%.3f\n",
                region_stats.mean_ms, region_stats.p95_ms,
                static_cast<double>(rw) * rh / (static_cast<double>(w) * h));
    char detail[192] = {};
    std::snprintf(detail, sizeof(detail), "整窗 mean=%.3fms，区域 mean=%.3fms", full_mean_ms, region_stats.mean_ms);
    Check(region_stats.mean_ms <= full_mean_ms, "区域搬运不慢于整窗搬运", detail);

    // 复位成整窗请求，别把状态留给后面的基准。
    op::hook::PublishFrameRequest(shmem, w, h, 0, 0, opt.width, opt.height, 250);
}

int main(int argc, char **argv) {
    ::SetProcessDPIAware();
    const Options opt = ParseArgs(argc, argv);

    std::printf("后台（hook）截图探针  尺寸=%dx%d 格式=%s 迭代=%d\n", opt.width, opt.height,
                opt.rgba ? "R8G8B8A8" : "B8G8R8A8", opt.iters);

    ProbeTarget target;
    if (!InitTarget(opt, target)) {
        return 1;
    }
    Check(true, "D3D11 交换链创建成功");

    // 与产品一致的共享资源：名字由 MakeOpSharedResourceName/MakeOpMutexName 生成。
    const std::wstring shm_name = ::MakeOpSharedResourceName(target.hwnd);
    const std::wstring mutex_name = ::MakeOpMutexName(target.hwnd);
    // 多留协商区（§3.2）：产品侧 HookCapture::ensureSharedFrameCapacity 也是这么分配的。
    const size_t bytes = op::hook::RequiredSharedFrameBytesWithExchange(static_cast<uint64_t>(opt.width),
                                                                       static_cast<uint64_t>(opt.height));
    op::SharedMemory shmem;
    op::ProcessMutex mutex;
    if (!shmem.open_create(shm_name, bytes) || !mutex.open_create(mutex_name)) {
        std::printf("  [FAIL] 共享内存/互斥体创建失败\n");
        return 1;
    }
    Check(op::hook::SharedFrameHasCapacity(shmem, static_cast<uint64_t>(opt.width),
                                           static_cast<uint64_t>(opt.height)),
          "共享段容量足够容纳整帧");

    op::hook::DisplayHook::render_hwnd = target.hwnd;
    op::hook::DisplayHook::shared_res_name = shm_name;
    op::hook::DisplayHook::mutex_name = mutex_name;
    op::hook::DisplayHook::render_type = ::RDT_DX_D3D11;

    auto *info = shmem.data<op::capture::FrameInfo>();
    unsigned char *pixels = shmem.data<unsigned char>() + sizeof(op::capture::FrameInfo);

    // ---------------------------------------------------------------- 计时
    std::vector<double> samples;
    samples.reserve(static_cast<size_t>(opt.iters));
    LARGE_INTEGER freq = {};
    ::QueryPerformanceFrequency(&freq);

    RenderFrame(target, 0, 0, 0); // 首帧建立 staging
    op::hook::dx11_capture(target.swapchain.Get());

    for (int i = 0; i < opt.iters; ++i) {
        RenderFrame(target, 255, 0, 0);
        LARGE_INTEGER begin = {};
        LARGE_INTEGER end = {};
        ::QueryPerformanceCounter(&begin);
        op::hook::dx11_capture(target.swapchain.Get());
        ::QueryPerformanceCounter(&end);
        samples.push_back((end.QuadPart - begin.QuadPart) * 1000.0 / static_cast<double>(freq.QuadPart));
    }

    // 注意：--copybench / --phase 会往共享段的像素区写自己的测试数据，
    // 必须放在下面的像素校验之后，否则会把真实帧覆盖掉、让校验假红。

    const PixelStats stats = Summarize(samples);
    std::printf("  每次 dx11_capture 耗时：mean=%.3fms p50=%.3fms p95=%.3fms max=%.3fms\n", stats.mean_ms,
                stats.p50_ms, stats.p95_ms, stats.max_ms);
    // 让脚本能做 A/B 比较：把关键数字单独打一行。
    std::printf("METRIC capture_mean_ms=%.4f capture_p95_ms=%.4f\n", stats.mean_ms, stats.p95_ms);

    // ---------------------------------------------------------------- 像素校验
    op::capture::FrameInfo snapshot = *info;
    op::capture::FrameInfo expected = snapshot;
    const auto chk = expected.chk;
    expected.fmtChk();
    Check(chk == expected.chk && snapshot.hwnd == reinterpret_cast<unsigned __int64>(target.hwnd),
          "共享帧头校验通过（chk/hwnd 匹配）",
          "chk 不匹配说明帧头被半写或 hwnd 不一致");
    Check(snapshot.width == static_cast<unsigned __int32>(opt.width) &&
              snapshot.height == static_cast<unsigned __int32>(opt.height),
          "共享帧尺寸等于交换链尺寸");

    // 画面是纯红：内存里 BGRA 排列时首像素应为 B=0,G=0,R=255。
    const unsigned char *p0 = pixels;
    const bool red_ok = p0[0] == 0 && p0[1] == 0 && p0[2] == 255;
    char detail[128] = {};
    std::snprintf(detail, sizeof(detail), "首像素=(%u,%u,%u,%u)，期望 BGR=(0,0,255)", p0[0], p0[1], p0[2], p0[3]);
    Check(red_ok, "共享内存首像素是清屏色（BGRA 内存序）", detail);

    const size_t center_off = (static_cast<size_t>(opt.height) / 2 * static_cast<size_t>(opt.width) +
                               static_cast<size_t>(opt.width) / 2) * 4;
    const unsigned char *pc = pixels + center_off;
    Check(pc[0] == 0 && pc[1] == 0 && pc[2] == 255, "共享内存中心像素是清屏色");

    // 换色再取一次，确认不是"只写了第一帧"。
    //
    // OP_HOOK_PINGPONG=1 时走 staging ping-pong（见 D3D11Capture.cpp 顶部说明）：本次
    // Present 只把拷贝提交给 GPU，共享段里写的是**上一帧**，换色后要多过一帧才读到新颜色。
    // 默认不开，所以这里多调一次 dx11_capture 对两种模式都成立。
    RenderFrame(target, 0, 0, 255);
    op::hook::dx11_capture(target.swapchain.Get());
    op::hook::dx11_capture(target.swapchain.Get());
    const unsigned char *pb = pixels;
    char detail2[128] = {};
    std::snprintf(detail2, sizeof(detail2), "首像素=(%u,%u,%u,%u)，期望 BGR=(255,0,0)", pb[0], pb[1], pb[2], pb[3]);
    Check(pb[0] == 255 && pb[1] == 0 && pb[2] == 0, "换色后共享内存跟随更新", detail2);
    Check(info->frameId > snapshot.frameId, "frameId 递增（说明每次都写入了新帧头）");

    // §3.2 区域搬运（会改写共享段像素区，所以放在像素校验之后、附加基准之前）。
    CheckRegionTransport(target, opt, shmem, info);
    MeasureRegionCapture(target, opt, shmem, stats.mean_ms);

    // 附加基准放在像素校验之后：它们会改写共享段像素区。
    if (opt.copybench) {
        RunCopyBench(target, opt, shmem);
    }
    if (opt.phase) {
        RunPhaseProfile(target, opt, shmem, mutex);
    }

    ::DestroyWindow(target.hwnd);
    std::printf("\n%s: %d failure(s)\n", g_failures == 0 ? "PASS" : "FAIL", g_failures);
    return g_failures == 0 ? 0 : 1;
}
