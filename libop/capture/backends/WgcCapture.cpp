
#include "WgcCapture.h"
#include "../../hook/DxCaptureCommon.h"
#include "../../image/Image.h"
#include "../../base/AutomationModes.h"
#include "../../base/Utils.h"
#include "../../base/WindowsVersion.h"
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <dwmapi.h>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Gaming.Input.h>

#ifdef OP_ENABLE_WGC
namespace op::capture {
namespace {

using op::hook::D3D11TextureMap;

template <typename Target, typename Value> void set_out(Target *target, Value value) {
    if (target)
        *target = static_cast<Target>(value);
}

struct ClientBoxCandidate {
    D3D11_BOX box{};
    int width = 0;
    int height = 0;
    int clipped_pixels = 0;
    int frame_mismatch = 0;
    bool valid = false;
};

bool wgcSessionPropertyPresent(const wchar_t *property_name) {
    try {
        return winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(
            L"Windows.Graphics.Capture.GraphicsCaptureSession", property_name);
    } catch (...) {
        return false;
    }
}

template <typename Func, typename Result> Result runInMtaApartment(Func func, Result fallback) {
    Result result = fallback;
    bool apartment_initialized = false;
    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        apartment_initialized = true;
        result = func();
    } catch (winrt::hresult_error &err) {
        setlog("WGC worker thread (0x%08X): %s", err.code().value, winrt::to_string(err.message()).c_str());
    } catch (...) {
        setlog("WGC worker thread (0x%08X)", winrt::to_hresult().value);
    }

    if (apartment_initialized) {
        try {
            winrt::uninit_apartment();
        } catch (...) {
        }
    }
    return result;
}

bool shouldUseWgcWorkerThread() {
    return !IsWindows10BuildOrGreater(kWindows11Build22000);
}

constexpr unsigned long kWin10RequestInitWaitMs = 300;
constexpr unsigned long kWin10InitialFrameWaitMs = 500;
constexpr unsigned long kWin10MetricsFrameWaitMs = 800;
constexpr unsigned long kWin10FreshFrameWaitMs = 100;
constexpr unsigned long kWin10CloseWaitMs = 1500;
constexpr unsigned long kWin10CleanupBindWaitMs = 200;
std::atomic<int> g_win10WgcCleanupInFlight{0};

bool waitForWin10WgcCleanup(unsigned long timeout_ms) {
    const unsigned long long deadline = ::GetTickCount64() + timeout_ms;
    while (g_win10WgcCleanupInFlight.load() > 0) {
        if (::GetTickCount64() >= deadline) {
            return false;
        }
        ::Sleep(10);
    }
    return true;
}

void debugWgcCloseMessage(const char *message) {
    ::OutputDebugStringA(message);
    ::OutputDebugStringA("\n");
}

void debugWgcCloseHresult(const char *operation, winrt::hresult_error &err) {
    char buffer[512] = {};
    std::snprintf(buffer, sizeof(buffer), "%s (0x%08X): %s", operation, err.code().value,
                  winrt::to_string(err.message()).c_str());
    debugWgcCloseMessage(buffer);
}

// Windows 10 this runs in the WGC worker thread, so a slow Borderless access
// request does not block the caller's BindWindow thread.
void applySessionOptions(const winrt::Windows::Graphics::Capture::GraphicsCaptureSession &session) {
    try {
        if (wgcSessionPropertyPresent(L"IsBorderRequired")) {
            winrt::Windows::Graphics::Capture::GraphicsCaptureAccess::RequestAccessAsync(
                winrt::Windows::Graphics::Capture::GraphicsCaptureAccessKind::Borderless)
                .get();
            session.IsBorderRequired(false);
        }
    } catch (winrt::hresult_error &err) {
        setlog("Request WGC borderless access failed (0x%08X): %s", err.code().value,
               winrt::to_string(err.message()).c_str());
    } catch (...) {
        setlog("Request WGC borderless access failed (0x%08X)", winrt::to_hresult().value);
    }
    try {
        if (wgcSessionPropertyPresent(L"IsCursorCaptureEnabled")) {
            session.IsCursorCaptureEnabled(false);
        }
    } catch (...) {
    }
}

} // namespace

WgcCapture::WgcCapture() = default;

WgcCapture::~WgcCapture() {
    captureStopping_.store(true);
    UnBindEx();
}

long WgcCapture::BindEx(HWND _hwnd, long render_type) {
    (void)render_type;
    if (shouldUseWgcWorkerThread()) {
        return BindExOnWindows10(_hwnd);
    }

    if (!Init(_hwnd)) {
        setlog("Init wgc failed");
        return 0;
    }
    return 1;
}

long WgcCapture::UnBindEx() {
    if (shouldUseWgcWorkerThread()) {
        return UnBindExOnWindows10();
    }
    return UnBindExInternal();
}

bool WgcCapture::DeferBindReleaseAfterUnBind() const {
    return shouldUseWgcWorkerThread() && win10WorkerRunning_.load();
}

long WgcCapture::BindExOnWindows10(HWND hwnd) {
    if (!waitForWin10WgcCleanup(kWin10CleanupBindWaitMs)) {
        setlog("BindEx: previous WGC cleanup is still pending on Windows 10");
        return 0;
    }

    if (win10Worker_.joinable()) {
        win10Worker_.join();
    }

    win10WorkerStop_.store(false);
    win10WorkerRunning_.store(true);
    win10WorkerInitReady_.store(false);
    win10WorkerInitSucceeded_.store(false);
    win10WorkerCleaned_.store(false);
    captureStopping_.store(false);

    auto self = std::static_pointer_cast<WgcCapture>(shared_from_this());
    try {
        win10Worker_ = std::thread([self, hwnd]() { self->runWin10Worker(hwnd); });
    } catch (...) {
        win10WorkerRunning_.store(false);
        win10WorkerInitReady_.store(true);
        win10WorkerInitSucceeded_.store(false);
        return 0;
    }
    return 1;
}

long WgcCapture::UnBindExInternal() {
    captureStopping_.store(true);
    closeCaptureSession();
    revokeItemClosed();

    if (device_) {
        try {
            device_.Close();
        } catch (winrt::hresult_error &err) {
            setlog("IDirect3DDevice::Close (0x%08X): %s", err.code().value, winrt::to_string(err.message()).c_str());
        } catch (...) {
            setlog("IDirect3DDevice::Close (0x%08X)", winrt::to_hresult().value);
        }
    }

    stagingTexture_.Release();
    d3dDeviceContext_.Release();
    d3dDevice_.Release();

    session_ = nullptr;
    framePool_ = nullptr;
    device_ = nullptr;
    item_ = nullptr;
    frameArrivedToken_ = {};
    hasFrameArrivedToken_ = false;
    itemClosed_.store(false);
    hasFrame_ = false;
    sharedWidth_ = 0;
    sharedHeight_ = 0;
    captureWidth_ = 0;
    captureHeight_ = 0;
    framePoolWidth_ = 0;
    framePoolHeight_ = 0;
    frameSerial_ = 0;
    hasWindowState_ = false;
    lastWindowIconic_ = false;
    pendingMetricsChanged_ = false;
    pendingBecameIconic_ = false;
    pendingRestored_ = false;
    lastClientWidth_ = 0;
    lastClientHeight_ = 0;

    return 0;
}

long WgcCapture::UnBindExOnWindows10() {
    if (!win10WorkerRunning_.load() && win10WorkerCleaned_.load() && !win10Worker_.joinable()) {
        return 0;
    }

    captureStopping_.store(true);
    win10WorkerStop_.store(true);

    const unsigned long long deadline = ::GetTickCount64() + kWin10CloseWaitMs;
    while (win10WorkerRunning_.load() && ::GetTickCount64() < deadline) {
        ::Sleep(10);
    }

    if (win10Worker_.joinable()) {
        if (win10WorkerRunning_.load()) {
            debugWgcCloseMessage("WGC worker stop timed out on Windows 10; cleanup continues in background");
            win10Worker_.detach();
            return 0;
        } else {
            win10Worker_.join();
        }
    }

    itemClosed_.store(false);
    hasFrame_ = false;
    sharedWidth_ = 0;
    sharedHeight_ = 0;
    captureWidth_ = 0;
    captureHeight_ = 0;
    framePoolWidth_ = 0;
    framePoolHeight_ = 0;
    frameSerial_ = 0;
    hasWindowState_ = false;
    lastWindowIconic_ = false;
    pendingMetricsChanged_ = false;
    pendingBecameIconic_ = false;
    pendingRestored_ = false;
    lastClientWidth_ = 0;
    lastClientHeight_ = 0;

    return 0;
}

void WgcCapture::runWin10Worker(HWND hwnd) {
    g_win10WgcCleanupInFlight.fetch_add(1);
    (void)runInMtaApartment(
        [&]() -> long {
            const bool ok = Init(hwnd, false);
            win10WorkerInitSucceeded_.store(ok);
            win10WorkerInitReady_.store(true);
            if (ok) {
                while (!win10WorkerStop_.load() && !itemClosed_.load()) {
                    updateLatestFrame();
                    ::Sleep(8);
                }
            }
            captureStopping_.store(true);
            closeWin10WorkerObjects();
            win10WorkerCleaned_.store(true);
            return 0;
        },
        0L);
    win10WorkerRunning_.store(false);
    g_win10WgcCleanupInFlight.fetch_sub(1);
}

bool WgcCapture::waitForWin10WorkerInit(unsigned long timeout_ms) {
    const unsigned long long deadline = ::GetTickCount64() + timeout_ms;
    while (!win10WorkerInitReady_.load()) {
        if (::GetTickCount64() >= deadline) {
            return false;
        }
        ::Sleep(10);
    }
    return win10WorkerInitSucceeded_.load();
}

void WgcCapture::closeWin10WorkerObjects() {
    if (item_ && hasClosedToken_) {
        try {
            item_.Closed(closedToken_);
        } catch (...) {
        }
    }
    closedToken_ = {};
    hasClosedToken_ = false;

    if (framePool_ && hasFrameArrivedToken_) {
        try {
            framePool_.FrameArrived(frameArrivedToken_);
        } catch (...) {
        }
    }
    frameArrivedToken_ = {};
    hasFrameArrivedToken_ = false;

    if (framePool_) {
        try {
            framePool_.Close();
        } catch (winrt::hresult_error &err) {
            debugWgcCloseHresult("Direct3D11CaptureFramePool::Close", err);
        } catch (...) {
            debugWgcCloseMessage("Direct3D11CaptureFramePool::Close failed");
        }
    }

    if (session_) {
        try {
            session_.Close();
        } catch (winrt::hresult_error &err) {
            debugWgcCloseHresult("GraphicsCaptureSession::Close", err);
        } catch (...) {
            debugWgcCloseMessage("GraphicsCaptureSession::Close failed");
        }
    }

    if (device_) {
        try {
            device_.Close();
        } catch (winrt::hresult_error &err) {
            debugWgcCloseHresult("IDirect3DDevice::Close", err);
        } catch (...) {
            debugWgcCloseMessage("IDirect3DDevice::Close failed");
        }
    }

    session_ = nullptr;
    framePool_ = nullptr;
    device_ = nullptr;
    item_ = nullptr;
    stagingTexture_.Release();
    d3dDeviceContext_.Release();
    d3dDevice_.Release();
}

bool WgcCapture::Init(HWND _hwnd, bool use_frame_arrived_event) {
    auto activation_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
    auto interop_factory = activation_factory.as<IGraphicsCaptureItemInterop>();
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = {nullptr};

    try {
        const HRESULT hr = interop_factory->CreateForWindow(_hwnd, winrt::guid_of<IGraphicsCaptureItem>(),
                                                            reinterpret_cast<void **>(winrt::put_abi(item)));
        if (FAILED(hr)) {
            setlog("CreateForWindow (0x%08X)", hr);
            return false;
        }
    } catch (winrt::hresult_error &err) {
        setlog("CreateForWindow (0x%08X): %s", err.code().value, winrt::to_string(err.message()).c_str());
        return false;
    } catch (...) {
        setlog("CreateForWindow (0x%08X)", winrt::to_hresult().value);
        return false;
    }

    if (!item) {
        setlog("GraphicsCaptureItem is null");
        return false;
    }

    D3D_DRIVER_TYPE DriverTypes[] = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

    D3D_FEATURE_LEVEL FeatureLevels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
                                         D3D_FEATURE_LEVEL_9_1};
    UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);
    D3D_FEATURE_LEVEL FeatureLevel;
    HRESULT create_device_hr = S_OK;
    stagingTexture_.Release();
    d3dDeviceContext_.Release();
    d3dDevice_.Release();

    for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex) {
        create_device_hr =
            D3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, 0, FeatureLevels, NumFeatureLevels,
                              D3D11_SDK_VERSION, &d3dDevice_, &FeatureLevel, &d3dDeviceContext_);
        if (SUCCEEDED(create_device_hr)) {
            break;
        }
    }
    if (d3dDevice_ == nullptr || d3dDeviceContext_ == nullptr) {
        setlog("D3D11CreateDevice failed hr=0x%08X", create_device_hr);
        return false;
    }
    ComPtr<IDXGIDevice> dxgi_device;
    if (FAILED(d3dDevice_->QueryInterface(&dxgi_device))) {
        setlog("Failed to get DXGI device wgc");
        return false;
    }

    winrt::com_ptr<IInspectable> inspectable;
    if (FAILED(CreateDirect3D11DeviceFromDXGIDevice(dxgi_device.Get(), inspectable.put()))) {
        setlog("Failed to get WinRT device wgc");
        return false;
    }

    const winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice device =
        inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
    const winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool frame_pool =
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::CreateFreeThreaded(
            device, winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, item.Size());
    const winrt::Windows::Graphics::Capture::GraphicsCaptureSession session = frame_pool.CreateCaptureSession(item);

    applySessionOptions(session);

    const auto item_size = item.Size();
    captureWidth_ = item_size.Width;
    captureHeight_ = item_size.Height;
    framePoolWidth_ = item_size.Width;
    framePoolHeight_ = item_size.Height;
    if (captureWidth_ <= 0 || captureHeight_ <= 0) {
        setlog("Invalid WGC item size width=%d height=%d", captureWidth_, captureHeight_);
        return false;
    }

    refreshWindowMetrics();
    if (_width <= 0 || _height <= 0) {
        setlog("Invalid client size width=%ld height=%ld", _width, _height);
        return false;
    }

    if (!ensureStagingTexture(_width, _height)) {
        setlog("Failed to create staging texture");
        return false;
    }

    if (!ensureSharedResources(_width, _height)) {
        return false;
    }

    item_ = item;
    itemClosed_.store(false);
    captureStopping_.store(false);
    closedToken_ = item_.Closed([this](auto const &, auto const &) { itemClosed_.store(true); });
    hasClosedToken_ = true;
    device_ = device;
    framePool_ = frame_pool;
    session_ = session;
    if (use_frame_arrived_event) {
        frameArrivedToken_ = framePool_.FrameArrived([this](const Direct3D11CaptureFramePool &sender, auto const &) {
            if (captureStopping_.load()) {
                return;
            }
            auto frame = tryGetLatestFrame(sender);
            if (frame && !captureStopping_.load()) {
                copyFrameToStaging(frame);
            }
        });
        hasFrameArrivedToken_ = true;
    } else {
        frameArrivedToken_ = {};
        hasFrameArrivedToken_ = false;
    }

    try {
        session_.StartCapture();
    } catch (winrt::hresult_error &err) {
        setlog("StartCapture (0x%08X): %s", err.code().value, winrt::to_string(err.message()).c_str());
        return false;
    } catch (...) {
        setlog("StartCapture (0x%08X)", winrt::to_hresult().value);
        return false;
    }

    return true;
}

bool WgcCapture::requestCapture(int x1, int y1, int w, int h, Image &img) {
    const bool win10_worker = shouldUseWgcWorkerThread();
    if (win10_worker && !waitForWin10WorkerInit(kWin10RequestInitWaitMs)) {
        setlog("requestCapture: WGC worker init is not ready");
        return false;
    }
    if (itemClosed_.load()) {
        setlog("requestCapture: capture item closed (target window gone)");
        return false;
    }
    if (isDeviceLost()) {
        if (win10_worker) {
            setlog("requestCapture: D3D device lost on Windows 10");
            return false;
        }
        setlog("requestCapture: D3D device lost, rebuilding WGC capture");
        recoverFromDeviceLoss();
        return false;
    }
    bool iconic_changed = false;
    bool is_iconic = false;
    bool metrics_changed = refreshWindowMetrics(&iconic_changed, &is_iconic);
    bool became_iconic = iconic_changed && is_iconic;
    bool restored = iconic_changed && !is_iconic;
    metrics_changed = metrics_changed || pendingMetricsChanged_;
    became_iconic = became_iconic || pendingBecameIconic_;
    restored = restored || pendingRestored_;
    if (restored) {
        became_iconic = false;
    }
    pendingMetricsChanged_ = false;
    pendingBecameIconic_ = false;
    pendingRestored_ = false;
    if (is_iconic || became_iconic) {
        setlog("requestCapture: window is minimized");
        return false;
    }

    if (x1 < 0 || y1 < 0 || x1 >= _width || y1 >= _height) {
        setlog("requestCapture: invalid client rect x=%d,y=%d,width=%ld,height=%ld", x1, y1, _width, _height);
        return false;
    }

    w = std::min<int>(w, static_cast<int>(_width) - x1);
    h = std::min<int>(h, static_cast<int>(_height) - y1);
    if (w <= 0 || h <= 0) {
        setlog("requestCapture: invalid capture size w=%d,h=%d,width=%ld,height=%ld", w, h, _width, _height);
        return false;
    }

    if (!hasCapturedFrame()) {
        const unsigned long long initial_serial = currentFrameSerial();
        waitForFramesAfter(initial_serial, 1, win10_worker ? kWin10InitialFrameWaitMs : 500, !win10_worker);
    }

    img.create(w, h);
    if (!_shmem || !_pmutex) {
        setlog("requestCapture: shared memory not initialized");
        return false;
    }

    if (metrics_changed || restored) {
        // 尺寸变化后队列里可能已有新尺寸帧。先尝试抽取；只有新帧已经匹配当前客户区
        // 尺寸才直接使用。Windows 10 最大化时可能先吐旧尺寸/过渡帧，过早放行会导致
        // 第一次截图仍是旧画面，第二次才更新。
        const long target_width = _width;
        const long target_height = _height;
        auto staging_matches_target = [&]() {
            std::scoped_lock lock(frameMutex_);
            if (!hasFrame_ || !stagingTexture_) {
                return false;
            }
            D3D11_TEXTURE2D_DESC staging_desc = {};
            stagingTexture_->GetDesc(&staging_desc);
            return static_cast<long>(staging_desc.Width) == target_width &&
                   static_cast<long>(staging_desc.Height) == target_height;
        };
        const unsigned long long before_update_serial = currentFrameSerial();
        const bool already_has_metric_frame = staging_matches_target();
        if (!win10_worker) {
            updateLatestFrame();
        }
        const bool got_metric_frame =
            staging_matches_target() && (already_has_metric_frame || currentFrameSerial() > before_update_serial);
        // 如果抽取时没拿到匹配新尺寸的帧，再短等 1 帧。
        const unsigned long timeout_ms = win10_worker ? kWin10MetricsFrameWaitMs : (restored ? 150 : 100);
        if (!got_metric_frame && !waitForFramesAfter(before_update_serial, 1, timeout_ms, !win10_worker)) {
            if (win10_worker) {
                // Windows 10 上同步 Close/Create/StartCapture 可能拖住调用方 GUI 线程。
                // resize 后 WGC 通常会自行继续吐新帧；本次不重启，避免截图 API 变成长阻塞点。
                setlog("requestCapture: no fresh WGC frame after metrics change on Windows 10");
                return false;
            }
            // 尺寸变化/最大化/恢复后 WGC 偶尔不继续吐帧，重启捕获会话能避免拿旧 staging 截新坐标。
            if (!restartCaptureSession()) {
                setlog("requestCapture: restart WGC capture session failed after metrics change");
                return false;
            }
            const unsigned long long restarted_serial = currentFrameSerial();
            if (!waitForFramesAfter(restarted_serial, 1, 1000)) {
                setlog("requestCapture: no fresh WGC frame after metrics change");
                return false;
            }
        }
        if (win10_worker && !staging_matches_target()) {
            setlog("requestCapture: WGC frame size not updated after metrics change on Windows 10");
            return false;
        }
    } else {
        const unsigned long long before_update_serial = currentFrameSerial();
        if (!win10_worker) {
            if (!updateLatestFrame()) {
                return false;
            }
        }
        if (currentFrameSerial() == before_update_serial) {
            const unsigned long fresh_timeout_ms = win10_worker ? kWin10FreshFrameWaitMs : 50;
            (void)waitForFramesAfter(before_update_serial, 1, fresh_timeout_ms, !win10_worker);
        }
    }

    if (!hasCapturedFrame()) {
        return false;
    }

    D3D11_TEXTURE2D_DESC desc = {};
    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    HRESULT hr = S_OK;
    {
        std::scoped_lock lock(frameMutex_);
        stagingTexture_->GetDesc(&desc);
        D3D11TextureMap mappedTexture(d3dDeviceContext_, stagingTexture_);
        hr = mappedTexture.map(&mappedResource);

        if (FAILED(hr)) {
            setlog("requestCapture: Map failed hr=0x%08X", hr);
            return false;
        }

        uint8_t *pData = (uint8_t *)mappedResource.pData;
        {
            std::scoped_lock shared_lock(sharedResourceMutex_);
            if (!_pmutex || !_shmem) {
                setlog("requestCapture: shared memory not initialized");
                return false;
            }
            _pmutex->lock();
            fmtFrameInfo(_shmem->data<byte>(), _hwnd, _width, _height);
            _pmutex->unlock();
        }

        // stagingTexture_ 已在收帧时裁成客户区，这里直接按上层客户区坐标取像素。
        int src_x = x1;
        int src_y = y1;
        int copy_w = w;
        int copy_h = h;
        if (src_x + copy_w > static_cast<int>(desc.Width))
            copy_w = static_cast<int>(desc.Width) - src_x;
        if (src_y + copy_h > static_cast<int>(desc.Height))
            copy_h = static_cast<int>(desc.Height) - src_y;
        bool ok = true;
        if (copy_w <= 0 || copy_h <= 0) {
            setlog("requestCapture: clamp empty src_x=%d,w=%d,desc.Width=%d,src_y=%d,h=%d,desc.Height=%d", src_x, w,
                   desc.Width, src_y, h, desc.Height);
            ok = false;
        } else {
            // 夹取后实际尺寸变小时按实际尺寸重建输出图，调用方按 _src 尺寸搜索，坐标仍以 (x1,y1) 为原点。
            if (copy_w != w || copy_h != h) {
                img.create(copy_w, copy_h);
            }
            for (int i = 0; i < copy_h; i++) {
                memcpy(img.ptr<uchar>(i), pData + (src_y + i) * mappedResource.RowPitch + src_x * 4, 4 * copy_w);
            }
        }

        return ok;
    }
}

void WgcCapture::refreshMetrics() {
    bool iconic_changed = false;
    bool is_iconic = false;
    if (refreshWindowMetrics(&iconic_changed, &is_iconic)) {
        pendingMetricsChanged_ = true;
    }
    if (iconic_changed) {
        // RectConvert 会先刷新尺寸，但真正等待新帧要留给 requestCapture 处理。
        if (is_iconic) {
            pendingBecameIconic_ = true;
            pendingRestored_ = false;
        } else {
            pendingBecameIconic_ = false;
            pendingRestored_ = true;
        }
    }
}

void WgcCapture::waitForBindReady() {
    if (shouldUseWgcWorkerThread()) {
        // Windows 10 WGC can stall while Create/StartCapture waits on the system broker.
        // BindWindow is commonly called on the GUI thread, so keep it non-blocking and let
        // requestCapture perform the bounded first-frame wait.
        return;
    }

    // WGC 绑定后由后台异步推送首帧；等第一帧到达再返回，
    // 避免 bind 后第一次 FindColor/FindPic/OCR 因首帧未就绪而假失败（与 HookCapture 行为一致）。
    if (!framePool_) {
        return;
    }
    waitForFramesAfter(0, 1, 1000);
}

bool WgcCapture::ensureStagingTexture(int width, int height) {
    std::scoped_lock lock(frameMutex_);
    if (stagingTexture_) {
        D3D11_TEXTURE2D_DESC existing = {};
        stagingTexture_->GetDesc(&existing);
        if ((int)existing.Width == width && (int)existing.Height == height) {
            return true;
        }
        stagingTexture_.Release();
    }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    return SUCCEEDED(d3dDevice_->CreateTexture2D(&desc, NULL, &stagingTexture_));
}

bool WgcCapture::ensureSharedResources(int width, int height) {
    std::scoped_lock shared_lock(sharedResourceMutex_);
    if (_shmem && _pmutex && sharedWidth_ == width && sharedHeight_ == height) {
        return true;
    }

    SAFE_DELETE(_shmem);
    SAFE_DELETE(_pmutex);
    sharedWidth_ = 0;
    sharedHeight_ = 0;

    const size_t res_size = static_cast<size_t>(width) * static_cast<size_t>(height) * 4 + sizeof(FrameInfo);
    _shared_res_name = MakeOpSharedResourceName(_hwnd);
    _mutex_name = MakeOpMutexName(_hwnd);
    try {
        auto shmem = std::make_unique<SharedMemory>();
        if (!shmem->open_create(_shared_res_name, res_size)) {
            setlog(L"bkdisplay::re bind share mem %s failed size=%llu", _shared_res_name.c_str(),
                   static_cast<unsigned long long>(res_size));
            return false;
        }
        auto mutex = std::make_unique<ProcessMutex>();
        if (!mutex->open_create(_mutex_name)) {
            setlog(L"bkdisplay::re bind mutex %s failed", _mutex_name.c_str());
            return false;
        }
        _shmem = shmem.release();
        _pmutex = mutex.release();
        sharedWidth_ = width;
        sharedHeight_ = height;
        return true;
    } catch (std::exception &e) {
        setlog(L"bkdisplay::re bind share mem %s exception:%s", _shared_res_name.c_str(), _s2wstring(e.what()).c_str());
    }

    SAFE_DELETE(_shmem);
    SAFE_DELETE(_pmutex);
    sharedWidth_ = 0;
    sharedHeight_ = 0;
    return false;
}

bool WgcCapture::refreshWindowMetrics(bool *iconic_changed, bool *is_iconic) {
    RECT client_rect = {};
    const bool was_iconic = hasWindowState_ && lastWindowIconic_;
    const bool iconic_before = ::IsIconic(_hwnd) != FALSE;
    ::GetClientRect(_hwnd, &client_rect);
    // ABA 防护：读取度量期间窗口若发生最小化，本次度量不可信，按最小化处理。
    const bool now_iconic = iconic_before || (::IsIconic(_hwnd) != FALSE);

    // WGC 对外仍按客户区大小工作，上层坐标也按客户区理解。
    _width = client_rect.right - client_rect.left;
    _height = client_rect.bottom - client_rect.top;

    const bool changed =
        !hasWindowState_ || lastClientWidth_ != _width || lastClientHeight_ != _height || was_iconic != now_iconic;
    set_out(iconic_changed, hasWindowState_ && was_iconic != now_iconic);
    set_out(is_iconic, now_iconic);
    hasWindowState_ = true;
    lastWindowIconic_ = now_iconic;
    lastClientWidth_ = _width;
    lastClientHeight_ = _height;
    return changed;
}

bool WgcCapture::getClientBox(int surface_width, int surface_height, D3D11_BOX &client_box, int &client_width,
                              int &client_height) {
    RECT window_rect = {};
    RECT client_rect = {};
    RECT visible_rect = {};
    POINT client_origin = {0, 0};
    const bool iconic_before = ::IsIconic(_hwnd) != FALSE;
    const bool ok = !iconic_before && ::GetWindowRect(_hwnd, &window_rect) && ::GetClientRect(_hwnd, &client_rect) &&
                    ::ClientToScreen(_hwnd, &client_origin) && !::IsIconic(_hwnd);
    if (!ok || client_rect.right <= 0 || client_rect.bottom <= 0) {
        return false;
    }

    if (::DwmGetWindowAttribute(_hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &visible_rect, sizeof(visible_rect)) != S_OK) {
        visible_rect = window_rect;
    }

    const int expected_w = client_rect.right - client_rect.left;
    const int expected_h = client_rect.bottom - client_rect.top;
    constexpr int kClientSurfaceTolerance = 8;
    auto make_full_surface_candidate = [&]() {
        ClientBoxCandidate candidate;
        const int extra_w = surface_width - expected_w;
        const int extra_h = surface_height - expected_h;
        if (extra_w < 0 || extra_h < 0 || extra_w > kClientSurfaceTolerance || extra_h > kClientSurfaceTolerance) {
            return candidate;
        }

        candidate.box.left = 0;
        candidate.box.top = 0;
        candidate.box.right = static_cast<UINT>(expected_w);
        candidate.box.bottom = static_cast<UINT>(expected_h);
        candidate.box.front = 0;
        candidate.box.back = 1;
        candidate.width = expected_w;
        candidate.height = expected_h;
        candidate.clipped_pixels = 0;
        candidate.frame_mismatch = extra_w + extra_h;
        candidate.valid = true;
        return candidate;
    };
    auto make_candidate = [&](const RECT &base_rect) {
        ClientBoxCandidate candidate;
        const int raw_left = client_origin.x - base_rect.left;
        const int raw_top = client_origin.y - base_rect.top;
        const int left = raw_left > 0 ? raw_left : 0;
        const int top = raw_top > 0 ? raw_top : 0;
        if (left >= surface_width || top >= surface_height) {
            return candidate;
        }

        const int remaining_w = surface_width - left;
        const int remaining_h = surface_height - top;
        candidate.width = expected_w < remaining_w ? expected_w : remaining_w;
        candidate.height = expected_h < remaining_h ? expected_h : remaining_h;
        if (candidate.width <= 0 || candidate.height <= 0) {
            return candidate;
        }

        candidate.box.left = static_cast<UINT>(left);
        candidate.box.top = static_cast<UINT>(top);
        candidate.box.right = static_cast<UINT>(left + candidate.width);
        candidate.box.bottom = static_cast<UINT>(top + candidate.height);
        candidate.box.front = 0;
        candidate.box.back = 1;
        candidate.clipped_pixels = (expected_w - candidate.width) + (expected_h - candidate.height);
        const int base_w = base_rect.right - base_rect.left;
        const int base_h = base_rect.bottom - base_rect.top;
        candidate.frame_mismatch = std::abs(base_w - surface_width) + std::abs(base_h - surface_height);
        candidate.valid = true;
        return candidate;
    };

    const ClientBoxCandidate candidates[] = {
        make_full_surface_candidate(),
        make_candidate(visible_rect),
        make_candidate(window_rect),
    };

    ClientBoxCandidate best;
    best.clipped_pixels = INT_MAX;
    for (const auto &candidate : candidates) {
        if (!candidate.valid) {
            continue;
        }
        if (!best.valid || candidate.clipped_pixels < best.clipped_pixels ||
            (candidate.clipped_pixels == best.clipped_pixels && candidate.frame_mismatch < best.frame_mismatch) ||
            (candidate.clipped_pixels == best.clipped_pixels && candidate.frame_mismatch == best.frame_mismatch &&
             candidate.box.left < best.box.left)) {
            best = candidate;
        }
    }
    if (!best.valid) {
        return false;
    }
    if (best.clipped_pixels > 0) {
        return false;
    }

    // 最大化窗口的 DWM 扩展边界可能和 WGC surface 原点差几个像素；
    // 同时尝试 DWM 外框和 Win32 窗口框，优先选裁剪少且外框尺寸更贴近 surface 的客户区 box。
    client_box = best.box;
    client_width = best.width;
    client_height = best.height;
    return true;
}

void WgcCapture::closeCaptureSession() {
    if (framePool_ && hasFrameArrivedToken_) {
        try {
            framePool_.FrameArrived(frameArrivedToken_);
        } catch (winrt::hresult_error &err) {
            setlog("Direct3D11CaptureFramePool::FrameArrived revoke (0x%08X): %s", err.code().value,
                   winrt::to_string(err.message()).c_str());
        } catch (...) {
            setlog("Direct3D11CaptureFramePool::FrameArrived revoke (0x%08X)", winrt::to_hresult().value);
        }
        hasFrameArrivedToken_ = false;
    }

    if (framePool_) {
        try {
            framePool_.Close();
        } catch (winrt::hresult_error &err) {
            setlog("Direct3D11CaptureFramePool::Close (0x%08X): %s", err.code().value,
                   winrt::to_string(err.message()).c_str());
        } catch (...) {
            setlog("Direct3D11CaptureFramePool::Close (0x%08X)", winrt::to_hresult().value);
        }
    }

    if (session_) {
        try {
            session_.Close();
        } catch (winrt::hresult_error &err) {
            setlog("GraphicsCaptureSession::Close (0x%08X): %s", err.code().value,
                   winrt::to_string(err.message()).c_str());
        } catch (...) {
            setlog("GraphicsCaptureSession::Close (0x%08X)", winrt::to_hresult().value);
        }
    }

    session_ = nullptr;
    framePool_ = nullptr;
    frameArrivedToken_ = {};
    hasFrameArrivedToken_ = false;
}

bool WgcCapture::restartCaptureSession() {
    if (shouldUseWgcWorkerThread()) {
        setlog("restartCaptureSession: skipped synchronous WGC restart on Windows 10");
        return false;
    }
    if (!device_ || !item_) {
        return false;
    }

    closeCaptureSession();

    const auto item_size = item_.Size();
    if (item_size.Width <= 0 || item_size.Height <= 0) {
        setlog("restartCaptureSession: invalid WGC item size width=%d height=%d", item_size.Width, item_size.Height);
        return false;
    }

    refreshWindowMetrics();
    if (_width <= 0 || _height <= 0) {
        setlog("restartCaptureSession: invalid client size width=%ld height=%ld", _width, _height);
        return false;
    }

    captureWidth_ = item_size.Width;
    captureHeight_ = item_size.Height;
    framePoolWidth_ = item_size.Width;
    framePoolHeight_ = item_size.Height;
    if (!ensureStagingTexture(_width, _height)) {
        setlog("restartCaptureSession: create staging texture failed");
        return false;
    }
    if (!ensureSharedResources(_width, _height)) {
        setlog("restartCaptureSession: create shared resources failed");
        return false;
    }

    {
        std::scoped_lock lock(frameMutex_);
        hasFrame_ = false;
    }

    try {
        framePool_ = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::CreateFreeThreaded(
            device_, winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, item_size);
        session_ = framePool_.CreateCaptureSession(item_);

        applySessionOptions(session_);

        frameArrivedToken_ = framePool_.FrameArrived([this](const Direct3D11CaptureFramePool &sender, auto const &) {
            auto frame = tryGetLatestFrame(sender);
            if (frame) {
                copyFrameToStaging(frame);
            }
        });
        hasFrameArrivedToken_ = true;
        session_.StartCapture();
    } catch (winrt::hresult_error &err) {
        setlog("restartCaptureSession (0x%08X): %s", err.code().value, winrt::to_string(err.message()).c_str());
        closeCaptureSession();
        return false;
    } catch (...) {
        setlog("restartCaptureSession (0x%08X)", winrt::to_hresult().value);
        closeCaptureSession();
        return false;
    }

    return true;
}

bool WgcCapture::isDeviceLost() {
    if (!d3dDevice_) {
        return false;
    }
    return FAILED(d3dDevice_->GetDeviceRemovedReason());
}

void WgcCapture::revokeItemClosed() {
    if (item_ && hasClosedToken_) {
        try {
            item_.Closed(closedToken_);
        } catch (...) {
        }
    }
    closedToken_ = {};
    hasClosedToken_ = false;
}

bool WgcCapture::recoverFromDeviceLoss() {
    HWND hwnd = _hwnd;
    if (!hwnd) {
        return false;
    }

    // 释放旧的会话/帧池/设备（保留 _hwnd 与共享内存），随后用新设备重新初始化。
    closeCaptureSession();
    revokeItemClosed();
    if (device_) {
        try {
            device_.Close();
        } catch (...) {
        }
    }
    stagingTexture_.Release();
    d3dDeviceContext_.Release();
    d3dDevice_.Release();
    device_ = nullptr;
    item_ = nullptr;
    {
        std::scoped_lock lock(frameMutex_);
        hasFrame_ = false;
        frameSerial_ = 0;
    }

    if (!Init(hwnd)) {
        setlog("recoverFromDeviceLoss: re-init failed");
        return false;
    }
    // 重建后等一帧，让下一次 requestCapture 能直接拿到画面。
    waitForFramesAfter(0, 1, 500);
    return true;
}

bool WgcCapture::copyFrameToStaging(const Direct3D11CaptureFrame &frame) {
    auto frame_surface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
    if (!frame_surface) {
        return hasCapturedFrame();
    }

    // ContentSize 不可靠，以 surface 的真实尺寸为准；客户区大小再由 getClientBox 夹到有效范围。
    D3D11_TEXTURE2D_DESC surface_desc = {};
    frame_surface->GetDesc(&surface_desc);
    const int surface_w = static_cast<int>(surface_desc.Width);
    const int surface_h = static_cast<int>(surface_desc.Height);
    if (surface_w <= 0 || surface_h <= 0) {
        return hasCapturedFrame();
    }

    const auto frame_content_size = frame.ContentSize();
    const bool content_size_changed =
        frame_content_size.Width > 0 && frame_content_size.Height > 0 &&
        (frame_content_size.Width != framePoolWidth_ || frame_content_size.Height != framePoolHeight_);
    if (surface_w != captureWidth_ || surface_h != captureHeight_) {
        captureWidth_ = surface_w;
        captureHeight_ = surface_h;
    }
    if (content_size_changed && framePool_) {
        try {
            framePool_.Recreate(device_,
                                static_cast<winrt::Windows::Graphics::DirectX::DirectXPixelFormat>(surface_desc.Format),
                                2, frame_content_size);
            framePoolWidth_ = frame_content_size.Width;
            framePoolHeight_ = frame_content_size.Height;
        } catch (...) {
            // 帧池重建失败不丢弃当前帧；下一帧继续尝试。
        }
    }

    D3D11_BOX client_box = {};
    int client_w = 0;
    int client_h = 0;
    if (!getClientBox(surface_w, surface_h, client_box, client_w, client_h)) {
        return hasCapturedFrame();
    }
    if (!ensureStagingTexture(client_w, client_h)) {
        setlog("copyFrameToStaging: resize client staging texture failed");
        return hasCapturedFrame();
    }
    if (!ensureSharedResources(client_w, client_h)) {
        setlog("copyFrameToStaging: resize client shared resources failed");
        return hasCapturedFrame();
    }

    {
        std::scoped_lock lock(frameMutex_);
        if (!stagingTexture_) {
            return hasCapturedFrame();
        }
        // 锁内再次确认 staging 与客户区尺寸一致，避免 resize 竞态后拷到错误大小的纹理。
        D3D11_TEXTURE2D_DESC staging_desc = {};
        stagingTexture_->GetDesc(&staging_desc);
        if (static_cast<int>(staging_desc.Width) != client_w || static_cast<int>(staging_desc.Height) != client_h) {
            return hasCapturedFrame();
        }
        // 直接在 GPU 侧只拷客户区，后续 CPU map 时不再需要标题栏/边框偏移。
        d3dDeviceContext_->CopySubresourceRegion(stagingTexture_, 0, 0, 0, 0, frame_surface.get(), 0, &client_box);
        _width = client_w;
        _height = client_h;
        hasFrame_ = true;
        ++frameSerial_;
    }
    return true;
}

Direct3D11CaptureFrame WgcCapture::tryGetLatestFrame(const Direct3D11CaptureFramePool &frame_pool) {
    Direct3D11CaptureFrame frame = frame_pool.TryGetNextFrame();
    if (!frame) {
        return frame;
    }

    for (;;) {
        auto newer = frame_pool.TryGetNextFrame();
        if (!newer) {
            break;
        }
        frame = newer;
    }
    return frame;
}

bool WgcCapture::updateLatestFrame() {
    if (!framePool_) {
        return hasCapturedFrame();
    }
    Direct3D11CaptureFrame frame = tryGetLatestFrame(framePool_);
    if (frame) {
        return copyFrameToStaging(frame);
    }

    return hasCapturedFrame();
}

bool WgcCapture::waitForFramesAfter(unsigned long long frame_serial, unsigned int frame_count, unsigned long timeout_ms,
                                    bool poll_latest) {
    const unsigned long long deadline = ::GetTickCount64() + timeout_ms;
    do {
        if (poll_latest && updateLatestFrame() && currentFrameSerial() >= frame_serial + frame_count) {
            return true;
        }
        if (currentFrameSerial() >= frame_serial + frame_count) {
            return true;
        }
        ::Sleep(8);
    } while (::GetTickCount64() < deadline);

    if (poll_latest) {
        updateLatestFrame();
    }
    return currentFrameSerial() >= frame_serial + frame_count;
}

unsigned long long WgcCapture::currentFrameSerial() {
    std::scoped_lock lock(frameMutex_);
    return frameSerial_;
}

bool WgcCapture::hasCapturedFrame() {
    std::scoped_lock lock(frameMutex_);
    return hasFrame_;
}

void WgcCapture::fmtFrameInfo(void *dst, HWND hwnd, int w, int h, bool inc) {
    m_frameInfo.hwnd = (unsigned __int64)hwnd;
    m_frameInfo.frameId = inc ? m_frameInfo.frameId + 1 : m_frameInfo.frameId;
    m_frameInfo.time = static_cast<unsigned __int32>(::GetTickCount64());
    m_frameInfo.width = w;
    m_frameInfo.height = h;
    m_frameInfo.fmtChk();
    memcpy(dst, &m_frameInfo, sizeof(m_frameInfo));
}

} // namespace op::capture

#endif
