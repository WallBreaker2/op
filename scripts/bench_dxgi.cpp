// DXGI 截图 CPU 侧开销微基准：对比“整屏读回 + 逐行拷贝”与“GPU 只拷请求矩形 + 一次 memcpy”。
//
// 管线 A（改动前，libop/capture/backends/DxgiCapture.cpp 旧实现）
//   每帧 CreateTexture2D(整屏 staging) -> CopyResource(整屏) -> Map(整屏) -> 逐行 memcpy 取区域
//   -> img.fill(0xff000000) -> 释放 staging
// 管线 B（改动后）
//   复用整屏 staging -> 复用区域 staging -> CopySubresourceRegion(只拷区域) -> Map(区域) -> 一次 memcpy
//
// 编译并运行（VS2022 环境）：
//   cl /nologo /O2 /std:c++17 /EHsc /utf-8 /Fe:bench_dxgi.exe scripts\bench_dxgi.cpp d3d11.lib dxgi.lib
//   bench_dxgi.exe [迭代次数]
//
// 没有可用硬件适配器时会退回 WARP 软件光栅化，此时绝对耗时不具参考性，
// 但两条管线的调度差异仍然成立。

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <d3d11.h>
#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

constexpr int kScreenWidth = 1920;
constexpr int kScreenHeight = 1080;
constexpr DXGI_FORMAT kFormat = DXGI_FORMAT_B8G8R8A8_UNORM;

template <typename T> class ComPtr {
  public:
    ComPtr() = default;
    ~ComPtr() {
        release();
    }
    ComPtr(const ComPtr &) = delete;
    ComPtr &operator=(const ComPtr &) = delete;

    T **put() {
        release();
        return &ptr_;
    }
    T *get() const {
        return ptr_;
    }
    T *operator->() const {
        return ptr_;
    }
    explicit operator bool() const {
        return ptr_ != nullptr;
    }
    void release() {
        if (ptr_) {
            ptr_->Release();
            ptr_ = nullptr;
        }
    }

  private:
    T *ptr_ = nullptr;
};

// 同 bench_capture：只报 avg 会被尖峰带偏，补 median / p95 / stdev。
struct Stats {
    double avg_ms = 0.0;
    double min_ms = 0.0;
    double median_ms = 0.0;
    double p95_ms = 0.0;
    double stdev_ms = 0.0;
};

template <typename Fn> Stats run_benchmark(int iterations, Fn &&step) {
    step(); // 预热，避开首次分配/首次提交的抖动
    std::vector<double> samples;
    samples.reserve(static_cast<size_t>(iterations));
    for (int i = 0; i < iterations; ++i) {
        const auto begin = std::chrono::steady_clock::now();
        step();
        const auto end = std::chrono::steady_clock::now();
        samples.push_back(std::chrono::duration<double, std::milli>(end - begin).count());
    }
    double sum = 0.0;
    for (double v : samples) {
        sum += v;
    }
    Stats stats;
    stats.avg_ms = sum / static_cast<double>(samples.size());
    stats.min_ms = *std::min_element(samples.begin(), samples.end());

    std::sort(samples.begin(), samples.end());
    const size_t n = samples.size();
    stats.median_ms = samples[n / 2];
    stats.p95_ms = samples[static_cast<size_t>(static_cast<double>(n - 1) * 0.95)];

    double sq = 0.0;
    for (double v : samples) {
        sq += (v - stats.avg_ms) * (v - stats.avg_ms);
    }
    stats.stdev_ms = std::sqrt(sq / static_cast<double>(n));
    return stats;
}

void print_stats(const char *label, const Stats &s) {
    std::printf("  %-6s : avg %.3f  median %.3f  p95 %.3f  min %.3f  stdev %.3f ms\n", label, s.avg_ms, s.median_ms,
                s.p95_ms, s.min_ms, s.stdev_ms);
}

// 打印能解释可复现性差异的环境元数据（虚拟显示适配器 / WARP 回退都会显著改变绝对耗时）。
void print_environment() {
    SYSTEM_INFO si = {};
    GetNativeSystemInfo(&si);
    std::printf("env: %s, %u logical cpus\n",
                si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "x64" : "x86", si.dwNumberOfProcessors);
    DISPLAY_DEVICEW dd = {};
    dd.cb = sizeof(dd);
    for (DWORD i = 0; EnumDisplayDevicesW(nullptr, i, &dd, 0); ++i) {
        if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) {
            std::wprintf(L"  display[%lu] %s\n", i, dd.DeviceString);
        }
        dd = {};
        dd.cb = sizeof(dd);
    }
}

bool create_staging(ID3D11Device *device, int width, int height, ComPtr<ID3D11Texture2D> &out) {
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = static_cast<UINT>(width);
    desc.Height = static_cast<UINT>(height);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = kFormat;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    const HRESULT hr = device->CreateTexture2D(&desc, nullptr, out.put());
    return SUCCEEDED(hr) && out;
}

struct Case {
    const char *name;
    int x;
    int y;
    int width;
    int height;
};

} // namespace

int main(int argc, char **argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    int iterations = 200;
    if (argc > 1) {
        iterations = (std::max)(1, std::atoi(argv[1]));
    }

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;
    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 3, D3D11_SDK_VERSION,
                                   device.put(), &level, context.put());
    bool hardware = SUCCEEDED(hr);
    if (!hardware) {
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels, 3, D3D11_SDK_VERSION, device.put(),
                               &level, context.put());
        if (FAILED(hr) || !device || !context) {
            std::printf("D3D11CreateDevice failed hr=0x%08X\n", hr);
            return 1;
        }
    }
    print_environment();
    std::printf("device: %s (feature level 0x%04X)\n", hardware ? "hardware" : "WARP", static_cast<unsigned>(level));
    std::printf("desktop texture: %dx%d\n\n", kScreenWidth, kScreenHeight);

    // 模拟 DXGI duplication 拿到的那张桌面纹理。
    std::vector<unsigned char> source_pixels(static_cast<size_t>(kScreenWidth) * kScreenHeight * 4);
    for (size_t i = 0; i < source_pixels.size(); ++i) {
        source_pixels[i] = static_cast<unsigned char>((i * 2654435761u) >> 24);
    }
    D3D11_TEXTURE2D_DESC src_desc = {};
    src_desc.Width = kScreenWidth;
    src_desc.Height = kScreenHeight;
    src_desc.MipLevels = 1;
    src_desc.ArraySize = 1;
    src_desc.Format = kFormat;
    src_desc.SampleDesc.Count = 1;
    src_desc.Usage = D3D11_USAGE_DEFAULT;
    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = source_pixels.data();
    init.SysMemPitch = kScreenWidth * 4;
    ComPtr<ID3D11Texture2D> source;
    if (FAILED(device->CreateTexture2D(&src_desc, &init, source.put())) || !source) {
        std::printf("create source texture failed\n");
        return 1;
    }

    // 每个用例都必须落在桌面纹理范围内，否则两边读越界的位置不同，比较没有意义。
    const Case cases[] = {
        {"full desktop", 0, 0, kScreenWidth, kScreenHeight},
        {"region 320x200", 100, 80, 320, 200},
        {"block 10x10", 300, 300, 10, 10},
    };

    bool all_identical = true;
    for (const auto &test : cases) {
        const int region_w = test.width;
        const int region_h = test.height;
        const size_t region_bytes = static_cast<size_t>(region_w) * region_h * 4;

        std::vector<unsigned char> legacy_out(region_bytes, 0);
        std::vector<unsigned char> fast_out(region_bytes, 0);

        ComPtr<ID3D11Texture2D> region_staging;
        if (!create_staging(device.get(), region_w, region_h, region_staging)) {
            std::printf("create region staging failed\n");
            return 1;
        }

        const Stats legacy = run_benchmark(iterations, [&]() {
            // --- 旧实现 ---
            ComPtr<ID3D11Texture2D> full_staging;
            if (!create_staging(device.get(), kScreenWidth, kScreenHeight, full_staging)) {
                return;
            }
            context->CopyResource(full_staging.get(), source.get());
            D3D11_MAPPED_SUBRESOURCE mapped = {};
            if (FAILED(context->Map(full_staging.get(), 0, D3D11_MAP_READ, 0, &mapped))) {
                return;
            }
            // img.fill(0xff000000) + 逐行拷贝
            std::fill(reinterpret_cast<unsigned int *>(legacy_out.data()),
                      reinterpret_cast<unsigned int *>(legacy_out.data()) + legacy_out.size() / 4, 0xff000000u);
            for (int i = 0; i < region_h; ++i) {
                memcpy(legacy_out.data() + static_cast<size_t>(i) * region_w * 4,
                       static_cast<const unsigned char *>(mapped.pData) +
                           static_cast<size_t>(test.y + i) * mapped.RowPitch + static_cast<size_t>(test.x) * 4,
                       static_cast<size_t>(region_w) * 4);
            }
            context->Unmap(full_staging.get(), 0);
        });

        const Stats fast = run_benchmark(iterations, [&]() {
            // --- 新实现 ---
            const D3D11_BOX box = {static_cast<UINT>(test.x),
                                   static_cast<UINT>(test.y),
                                   0,
                                   static_cast<UINT>(test.x + region_w),
                                   static_cast<UINT>(test.y + region_h),
                                   1};
            context->CopySubresourceRegion(region_staging.get(), 0, 0, 0, 0, source.get(), 0, &box);
            D3D11_MAPPED_SUBRESOURCE mapped = {};
            if (FAILED(context->Map(region_staging.get(), 0, D3D11_MAP_READ, 0, &mapped))) {
                return;
            }
            const auto *src = static_cast<const unsigned char *>(mapped.pData);
            const size_t row_bytes = static_cast<size_t>(region_w) * 4;
            if (mapped.RowPitch == row_bytes) {
                memcpy(fast_out.data(), src, row_bytes * static_cast<size_t>(region_h));
            } else {
                for (int i = 0; i < region_h; ++i) {
                    memcpy(fast_out.data() + static_cast<size_t>(i) * row_bytes,
                           src + static_cast<size_t>(i) * mapped.RowPitch, row_bytes);
                }
            }
            context->Unmap(region_staging.get(), 0);
        });

        const bool identical = memcmp(legacy_out.data(), fast_out.data(), region_bytes) == 0;
        all_identical = all_identical && identical;

        std::printf("[%s] %dx%d\n", test.name, region_w, region_h);
        print_stats("legacy", legacy);
        print_stats("fast", fast);
        std::printf("  speedup: avg %.2fx  median %.2fx  min %.2fx\n",
                    legacy.avg_ms / (fast.avg_ms > 0.0 ? fast.avg_ms : 1e-9),
                    legacy.median_ms / (fast.median_ms > 0.0 ? fast.median_ms : 1e-9),
                    legacy.min_ms / (fast.min_ms > 0.0 ? fast.min_ms : 1e-9));
        std::printf("  pixels identical: %s\n\n", identical ? "YES" : "NO");
    }

    std::printf(all_identical ? "RESULT: pixel-exact\n" : "RESULT: pixel mismatch\n");
    return all_identical ? 0 : 2;
}
