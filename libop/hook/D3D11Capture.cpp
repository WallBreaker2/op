#include "D3D11Capture.h"

#include "DisplayHook.h"
#include "DxCaptureCommon.h"
#include "SharedFrame.h"
#include "SharedFrameChannel.h"
#include "SharedFrameExchange.h"
#include "SharedFrameWriter.h"
#include "../capture/FrameInfo.h"
#include "../ipc/ProcessMutex.h"
#include "../ipc/SharedMemory.h"
#include "../base/AutomationModes.h"
#include "../base/Utils.h"
#include <atlbase.h>
#include <cstddef>
#include <cstdio>
#include <d3d11.h>
#include <string>

#define DEBUG_HOOK 0

namespace op::hook {

using ATL::CComPtr;
using op::capture::FrameInfo;

namespace {

// ---------------------------------------------------------------------------
// 两个运行期开关（环境变量，一次性读取）
//
// OP_HOOK_PINGPONG=1       两片 staging 交替：本次 Present 只提交 CopyResource，把上一帧
//                          那片 Map 出来写共享段，Map 不再等 GPU。代价是共享段滞后一帧。
//                          默认**关** —— 实测见 doc/defect_fixes.md §72.17：vsync 场景快约
//                          28%，无 vsync 场景反而慢约 12%（CPU 整帧搬运和 GPU 拷贝抢带宽），
//                          而"滞后一帧"会改变 Capture 的语义，所以只作为可选开关。
// OP_HOOK_TIMING=1         把钩子自身耗时按窗口汇总，追加到 %TEMP%\op_hook_timing.log。
//                          用来给上面的 A/B 出真实数字，默认关。
// ---------------------------------------------------------------------------
struct HookOptions {
    bool pingpong{false};
    bool timing{false};
};

bool env_flag(const wchar_t *name) {    wchar_t buf[8] = {};
    const DWORD n = ::GetEnvironmentVariableW(name, buf, 8);
    return n > 0 && buf[0] != L'0';
}

const HookOptions &hook_options() {
    static const HookOptions options = [] {
        HookOptions o;
        o.pingpong = env_flag(L"OP_HOOK_PINGPONG");
        o.timing = env_flag(L"OP_HOOK_TIMING");
        return o;
    }();
    return options;
}

// 钩子自身耗时统计：只统计 dx11_capture 的整段（CopyResource + Map + 写共享段）。
// 只在 OP_HOOK_TIMING=1 时累加，默认路径上只有一次 bool 判断。
struct TimingStats {
    ULONGLONG samples{0};
    double total_ms{0};
    double max_ms{0};
    ULONGLONG window_samples{0};
    double window_total_ms{0};
};

TimingStats g_timing_stats;

void timing_record(double ms) {
    TimingStats &stats = g_timing_stats;
    ++stats.samples;
    stats.total_ms += ms;
    if (ms > stats.max_ms) {
        stats.max_ms = ms;
    }
    ++stats.window_samples;
    stats.window_total_ms += ms;
    if (stats.window_samples < 120) {
        return;
    }
    wchar_t temp_dir[MAX_PATH] = {};
    ::GetTempPathW(MAX_PATH, temp_dir);
    const std::wstring path = std::wstring(temp_dir) + L"op_hook_timing.log";
    FILE *file = nullptr;
    if (::_wfopen_s(&file, path.c_str(), L"a") == 0 && file) {
        fprintf(file, "mode=%s frames=%llu window_mean_us=%.1f overall_mean_us=%.1f overall_max_us=%.1f\n",
                hook_options().pingpong ? "pingpong" : "sync", (unsigned long long)stats.samples,
                stats.window_total_ms * 1000.0 / (double)stats.window_samples,
                stats.total_ms * 1000.0 / (double)stats.samples, stats.max_ms * 1000.0);
        fclose(file);
    }
    stats.window_samples = 0;
    stats.window_total_ms = 0;
}

// 后台截图的 staging 纹理按交换链缓存复用。
//
// 原来每帧都 CreateTexture2D 一张整窗 staging，再把后备缓冲 CopyResource 进去：
//   - CreateTexture2D 每帧一次，是驱动/运行时侧的分配动作，跟窗口大小成正比；
//   - MSAA 交换链还额外每帧再建一张 resolve 纹理；
//   - 这些都是纯浪费 —— 尺寸/格式没变时纹理可以一直用。
// 现在只在「窗口尺寸、格式、采样数变化」或「资源失效」时重建，换帧只做 CopyResource + Map。
//
// 缓存以交换链指针为 key：多窗口游戏每个窗口各有一条交换链，各 Present 自己的线程，
// 共用同一个槽会互相顶掉（每帧重建，比不缓存更慢）。
struct Dx11StagingCache {
    IDXGISwapChain *swapchain{nullptr}; // 只作 key，不 AddRef：Present 回调期间它必然存活
    CComPtr<ID3D11Device> device;
    CComPtr<ID3D11DeviceContext> context;
    CComPtr<ID3D11Texture2D> staging;
    // ping-pong 的第二片 staging：本次 Present 只往 write 片提交 CopyResource，
    // 同时把上一帧的 read 片 Map 出来写共享段 —— 那时 GPU 早就做完了，Map 不再等。
    CComPtr<ID3D11Texture2D> staging_alt;
    CComPtr<ID3D11Texture2D> resolve; // 仅 MSAA 交换链使用
    UINT width{0};
    UINT height{0};
    UINT sample_count{1};
    DXGI_FORMAT format{DXGI_FORMAT_UNKNOWN};
    bool pair{false};      // 是否按 ping-pong 建了两片 staging
    bool pending{false};   // 上一帧已提交 CopyResource、还没被 Map 读走
    UINT next_write{0};    // 下一帧往哪片写
    ULONGLONG last_used{0};

    bool textures_match(UINT width_, UINT height_, UINT sample_count_, DXGI_FORMAT format_, bool pair_) const {
        return staging && pair == pair_ && width == width_ && height == height_ && sample_count == sample_count_ &&
               format == format_;
    }

    ID3D11Texture2D *staging_slot(UINT index) const {
        return index == 0 ? staging.p : staging_alt.p;
    }

    void release_textures() {
        staging.Release();
        staging_alt.Release();
        resolve.Release();
        pair = false;
        pending = false;
        next_write = 0;
    }

    void reset() {
        release_textures();
        context.Release();
        device.Release();
        swapchain = nullptr;
        width = height = 0;
        sample_count = 1;
        format = DXGI_FORMAT_UNKNOWN;
        last_used = 0;
    }
};

// 槽位数 = 能同时缓存的上限。真实机器上一个进程同时 Present 的活动窗口很少超过 4 个，
// 超出的按 LRU 顶掉，行为与「不缓存」退化成一致。
constexpr size_t kMaxStagingEntries = 4;
Dx11StagingCache g_staging_cache[kMaxStagingEntries];
SRWLOCK g_staging_lock = SRWLOCK_INIT;

// 取（或建）该交换链的缓存槽。调用方必须已持有 g_staging_lock 独占。
Dx11StagingCache *acquire_entry(IDXGISwapChain *swapchain) {
    const ULONGLONG now = ::GetTickCount64();
    Dx11StagingCache *spare = nullptr;
    Dx11StagingCache *oldest = &g_staging_cache[0];
    for (auto &entry : g_staging_cache) {
        if (entry.swapchain == swapchain) {
            entry.last_used = now;
            return &entry;
        }
        if (!entry.swapchain && !spare) {
            spare = &entry;
        }
        if (entry.last_used < oldest->last_used) {
            oldest = &entry;
        }
    }
    Dx11StagingCache *entry = spare ? spare : oldest;
    entry->reset();
    entry->swapchain = swapchain;
    entry->last_used = now;
    return entry;
}

// 建立/校验该槽的设备、上下文、staging（以及 MSAA 的 resolve）。
// need_pair=true 时按 ping-pong 建两片 staging。
// 失败时按原逻辑关掉捕获，并返回 false。
bool ensure_textures(ID3D11Device *device, ID3D11DeviceContext *context, Dx11StagingCache &entry, UINT width,
                     UINT height, UINT sample_count, DXGI_FORMAT format, bool need_pair) {
    if (entry.textures_match(width, height, sample_count, format, need_pair)) {
        return true;
    }
    entry.release_textures();

    D3D11_TEXTURE2D_DESC textDesc = {};
    textDesc.Format = format;
    textDesc.Width = width;
    textDesc.Height = height;
    textDesc.MipLevels = 1;
    textDesc.ArraySize = 1;
    textDesc.SampleDesc.Count = 1;
    textDesc.Usage = D3D11_USAGE_STAGING;
    textDesc.BindFlags = 0;
    textDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    textDesc.MiscFlags = 0;
    HRESULT hr = device->CreateTexture2D(&textDesc, nullptr, &entry.staging.p);
    if (hr < 0) {
        setlog("device->CreateTexture2D,error code=%d", hr);
        return false;
    }
    if (need_pair) {
        hr = device->CreateTexture2D(&textDesc, nullptr, &entry.staging_alt.p);
        if (hr < 0) {
            setlog("device->CreateTexture2D(alt),error code=%d", hr);
            entry.release_textures();
            return false;
        }
    }
    if (sample_count > 1) {
        // MSAA 后备缓冲需要先 resolve 成单采样纹理，否则 CopyResource 到 staging 会失败。
        D3D11_TEXTURE2D_DESC resolveDesc = textDesc;
        resolveDesc.Usage = D3D11_USAGE_DEFAULT;
        resolveDesc.CPUAccessFlags = 0;
        hr = device->CreateTexture2D(&resolveDesc, nullptr, &entry.resolve.p);
        if (hr < 0) {
            setlog("device->CreateTexture2D resolved,error code=%d", hr);
            entry.release_textures();
            return false;
        }
    }
    (void)context;
    entry.width = width;
    entry.height = height;
    entry.sample_count = sample_count;
    entry.format = format;
    entry.pair = need_pair;
    entry.pending = false;
    entry.next_write = 0;
    return true;
}

// 把一张 staging 映射出来并写进共享段。返回是否真的写了帧。
// `request` 非空且不是整窗时只写请求的那块（像素区从偏移 0 开始紧凑排列）。
bool map_and_write(ID3D11DeviceContext *context, ID3D11Texture2D *staging, UINT width, UINT height,
                   DXGI_FORMAT format, const FrameRequestSnapshot *request) {
    D3D11_MAPPED_SUBRESOURCE mapSubres = {0, 0, 0};
    D3D11TextureMap mappedTexture(context, staging);
    HRESULT hr = mappedTexture.map(&mapSubres);
    if (hr < 0) {
        setlog("context->Map error code=%d", hr);
        return false;
    }
    const int fmt = GetImageBufferFormat(format);

    // 映射与跨进程互斥体按名字缓存复用（见 SharedFrameChannel.h 的说明）：
    // 每帧重开映射会让紧随其后的整帧搬运每帧重新缺页，1080p 上就是 0.63ms → 1.87ms。
    // 租约在整个写入期间持有，保证这段不会因为别的 Present 线程重开映射而把我们的视图解映射掉。
    SharedFrameChannelLease channel(DisplayHook::shared_res_name, DisplayHook::mutex_name);
    if (!channel.ok()) {
#if DEBUG_HOOK
        setlog(L"!mem.open(DisplayHook::%s)&&mutex.open(DisplayHook::%s)", DisplayHook::shared_res_name.c_str(),
               DisplayHook::mutex_name.c_str());
#endif // DEBUG_HOOK
        return false;
    }
    if (!channel.mutex().lock_timeout(op::hook::kSharedFrameLockTimeoutMs)) {
        // 宿主没在超时内释放 → 跳过本帧（返回 false 表示这次没写出帧）。
        return false;
    }
    SharedMemory &mem = channel.mem();
    if (SharedFrameHasCapacity(mem, width, height)) {
        auto sharedFrame = make_shared_frame_span(mem, width, height);
        if (request) {
            // 区域拷贝把矩形放在 staging 的**左上角**（CopySubresourceRegion 的目标偏移是 0,0），
            // 所以源指针就是 staging 起点，不能再按 (x, y) 偏移 —— 那样读到的是上一帧留在
            // 同一位置的旧像素（探针就是这么抓到第一版的）。
            const char *source = static_cast<const char *>(mapSubres.pData);
            write_shared_frame_region(sharedFrame, DisplayHook::render_hwnd, width, height, source, request->h,
                                      request->w, static_cast<int>(mapSubres.RowPitch), fmt);
            WriteFrameResponse(mem, width, height, kFrameModeRegion, request->seq, request->x, request->y,
                               request->w, request->h);
        } else {
            write_shared_frame(sharedFrame, DisplayHook::render_hwnd, width, height, mapSubres.pData, height, width,
                               mapSubres.RowPitch, fmt);
            WriteFrameResponse(mem, width, height, kFrameModeFull, 0, 0, 0, static_cast<int>(width),
                               static_cast<int>(height));
        }
    } else {
        WriteSharedFrameHeader(mem, DisplayHook::render_hwnd, width, height);
    }
    static_assert(sizeof(FrameInfo) == 28);
    channel.mutex().unlock();
    static bool first = true;
    if (first) {
        setlog("textDesc.Format= %d,fmt=%d textDesc.Height=%d\n textDesc.Width=%d\n  mapSubres.DepthPitch=%d\n "
               "mapSubres.RowPitch=%d\n",
               static_cast<int>(format), fmt, static_cast<int>(height), static_cast<int>(width),
               static_cast<int>(mapSubres.DepthPitch), static_cast<int>(mapSubres.RowPitch));
        first = false;
    }
    return true;
}

enum class CaptureResult {
    Failed,    // 这次没抓到，调用方可以重建缓存再试一次
    Submitted, // ping-pong 的第一帧：只提交了 CopyResource，还没有可读的帧
    Written,   // 已经写了共享段
    Ignored,   // 这条交换链不属于被绑定的窗口（多窗口目标），什么都不做
};

// 单次抓帧。force_recreate=true 时强制重建缓存纹理（缓存被判定失效后的重试路径）。
CaptureResult capture_once(IDXGISwapChain *swapchain, bool force_recreate) {
    const HookOptions &opt = hook_options();
    const bool pingpong = opt.pingpong;

    DXGI_SWAP_CHAIN_DESC desc;
    HRESULT hr = 0;
    hr = swapchain->GetDesc(&desc);
    if (hr < 0) {
        setlog("swapchain->GetDesc hr=%X", hr);
        return CaptureResult::Failed;
    }
    // 一个进程里可能有多条交换链（多窗口、编辑器多视口）。只有被绑定窗口的那条该被搬：
    // 否则别的窗口的帧会写进同一个共享段，而且像素区和响应区来自不同链 ——
    // 宿主看到的是 A 链写的"整窗帧"响应 + B 链只拷了左上角一块的像素区（§72.22）。
    if (DisplayHook::render_hwnd != 0 && desc.OutputWindow != 0 && desc.OutputWindow != DisplayHook::render_hwnd) {
        return CaptureResult::Ignored;
    }
    const UINT width = desc.BufferDesc.Width;
    const UINT height = desc.BufferDesc.Height;
    const UINT sample_count = desc.SampleDesc.Count;
    const DXGI_FORMAT format = NormalizeDxgiFormat(desc.BufferDesc.Format);

    // 宿主可能只想要一小块（Capture(x, y, w, h)）。请求得在 GPU 拷贝之前读到，
    // 所以走独立的只读映射（见 ReadFrameRequestByName 的说明）。
    FrameRequestSnapshot request = ReadFrameRequestByName(DisplayHook::shared_res_name, width, height);
    const bool request_is_full =
        request.valid && request.x == 0 && request.y == 0 && static_cast<UINT>(request.w) == width &&
        static_cast<UINT>(request.h) == height;
    const bool request_in_bounds = request.valid && request.x >= 0 && request.y >= 0 && request.w > 0 &&
                                   request.h > 0 && static_cast<UINT>(request.x) + static_cast<UINT>(request.w) <= width &&
                                   static_cast<UINT>(request.y) + static_cast<UINT>(request.h) <= height;
    if (request.valid && !request_in_bounds) {
        request = FrameRequestSnapshot{}; // 越界请求当没提，照旧整窗搬
    }
    // 回写响应只看"本次请求是否合法"：旧宿主的共享段没有协商区，magic 对不上就永远
    // 不会走到这里；而 sticky 标志在宿主换人之后仍然为真，会往不认识的共享段里写。
    const bool region_copy = request.valid && !request_is_full;

    // 缓存查询/建纹理在锁里完成；随后的 CopyResource + Map 是耗时段，必须在锁外，
    // 否则多窗口同时 Present 时会互相排队（等于把省下来的时间又还回去）。
    CComPtr<ID3D11Device> device;
    CComPtr<ID3D11DeviceContext> context;
    CComPtr<ID3D11Texture2D> write_staging;
    CComPtr<ID3D11Texture2D> read_staging;
    CComPtr<ID3D11Texture2D> resolve;
    UINT write_index = 0;
    bool has_pending = false;
    ::AcquireSRWLockExclusive(&g_staging_lock);
    Dx11StagingCache *entry = acquire_entry(swapchain);
    if (!entry->device || force_recreate) {
        entry->context.Release();
        entry->device.Release();
        if (force_recreate) {
            entry->release_textures();
        }
        hr = swapchain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void **>(&entry->device.p));
        if (hr < 0 || !entry->device) {
            ::ReleaseSRWLockExclusive(&g_staging_lock);
            setlog("swapchain->GetDevice hr=%X", hr);
            return CaptureResult::Failed;
        }
        entry->device->GetImmediateContext(&entry->context.p);
        if (!entry->context) {
            ::ReleaseSRWLockExclusive(&g_staging_lock);
            setlog("!context");
            return CaptureResult::Failed;
        }
    }
    const bool ready =
        ensure_textures(entry->device, entry->context, *entry, width, height, sample_count, format, pingpong);
    if (ready) {
        // AddRef 到局部，把资源带出锁：期间该槽被别的线程 LRU 顶掉也不会释放我们正在用的纹理。
        device = entry->device;
        context = entry->context;
        resolve = entry->resolve;
        if (pingpong) {
            write_index = entry->next_write;
            write_staging = entry->staging_slot(write_index);
            if (entry->pending) {
                read_staging = entry->staging_slot(1 - write_index);
                has_pending = read_staging != nullptr;
            }
        } else {
            write_staging = entry->staging;
        }
    }
    ::ReleaseSRWLockExclusive(&g_staging_lock);
    if (!ready) {
        return CaptureResult::Failed;
    }

    CComPtr<IDXGIResource> backbufferptr;
    hr = swapchain->GetBuffer(0, __uuidof(IDXGIResource), reinterpret_cast<void **>(&backbufferptr.p));
    if (hr < 0) {
        setlog("pswapchain->GetBuffer,error code=%X", hr);
        return CaptureResult::Failed;
    }
    CComPtr<ID3D11Resource> backbuffer;
    hr = backbufferptr->QueryInterface(__uuidof(ID3D11Resource), reinterpret_cast<void **>(&backbuffer.p));
    if (hr < 0) {
        setlog("backbufferptr->QueryInterface,error code=%X", hr);
        return CaptureResult::Failed;
    }

    ID3D11Resource *copySource = backbuffer;
    if (sample_count > 1) {
        if (!resolve) {
            return CaptureResult::Failed;
        }
        context->ResolveSubresource(resolve, 0, backbuffer, 0, format);
        copySource = resolve;
    }

    // 只提交，不等待：拷贝排进 GPU 队列就返回。
    if (region_copy) {
        D3D11_BOX box = {};
        box.left = static_cast<UINT>(request.x);
        box.top = static_cast<UINT>(request.y);
        box.front = 0;
        box.right = static_cast<UINT>(request.x + request.w);
        box.bottom = static_cast<UINT>(request.y + request.h);
        box.back = 1;
        context->CopySubresourceRegion(write_staging, 0, 0, 0, 0, copySource, 0, &box);
    } else {
        context->CopyResource(write_staging, copySource);
    }

    CaptureResult result = CaptureResult::Submitted;
    if (pingpong) {
        if (has_pending) {
            // 上一帧的 staging：GPU 早把那次 CopyResource 做完了，Map 不会等。
            result = map_and_write(context, read_staging, width, height, format,
                                   region_copy ? &request : nullptr)
                         ? CaptureResult::Written
                         : CaptureResult::Failed;
        }
    } else {
        result = map_and_write(context, write_staging, width, height, format, region_copy ? &request : nullptr)
                     ? CaptureResult::Written
                     : CaptureResult::Failed;
    }

    // 记账：本帧已提交，下一帧改往另一片写。（重新按 key 取槽，避免 entry 已被 LRU 顶掉）
    if (result != CaptureResult::Failed) {
        ::AcquireSRWLockExclusive(&g_staging_lock);
        Dx11StagingCache *current = acquire_entry(swapchain);
        if (current->pair && current->width == width && current->height == height) {
            current->pending = true;
            current->next_write = 1 - write_index;
        }
        ::ReleaseSRWLockExclusive(&g_staging_lock);
    }
    return result;
}

} // namespace

void dx11_capture(IDXGISwapChain *swapchain) {
    const HookOptions &opt = hook_options();
    LARGE_INTEGER begin = {};
    if (opt.timing) {
        ::QueryPerformanceCounter(&begin);
    }

    CaptureResult result = capture_once(swapchain, false);
    if (result == CaptureResult::Ignored) {
        return; // 不是被绑定窗口的交换链，不记账、不重试
    }
    if (result == CaptureResult::Failed) {
        // 缓存资源可能因设备重置/驱动升级失效：重建一次再试。
        result = capture_once(swapchain, true);
        if (result == CaptureResult::Ignored) {
            return;
        }
    }

    if (opt.timing) {
        static LARGE_INTEGER frequency = [] {
            LARGE_INTEGER f = {};
            ::QueryPerformanceFrequency(&f);
            return f;
        }();
        LARGE_INTEGER end = {};
        ::QueryPerformanceCounter(&end);
        if (frequency.QuadPart > 0) {
            timing_record((double)(end.QuadPart - begin.QuadPart) * 1000.0 / (double)frequency.QuadPart);
        }
    }

    if (result == CaptureResult::Failed) {
        // 连续失败到阈值才真正停抓（一次瞬时失败不该让后台截图停摆到重新绑定为止）。
        DisplayHook::note_capture_failure();
        return;
    }
    DisplayHook::note_capture_success();
}

HRESULT __stdcall dx11_hkPresent(IDXGISwapChain *thiz, UINT SyncInterval, UINT Flags) {
    typedef long(__stdcall * Present_t)(IDXGISwapChain * pswapchain, UINT x1, UINT x2);
    // 先登记「我在 detour 里」，并留一份原函数地址。release() 要等这个计数归零
    // 才清 old_address、释放 trampoline；否则正在跑的这一次会调到空指针或已回收的代码。
    void *const original = DisplayHook::old_address;
    DisplayHook::DetourScope detour_scope;
    if (!original) {
        return S_OK;
    }
    // DXGI_PRESENT_TEST 不提交真实帧，避免把测试调用当成截图帧处理。
    if (DisplayHook::capture_enabled() && !(Flags & DXGI_PRESENT_TEST))
        dx11_capture(thiz);
    return ((Present_t)original)(thiz, SyncInterval, Flags);
}

} // namespace op::hook
