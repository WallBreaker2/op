
#include "WgcCapture.h"
#include "../../image/Image.h"
#include "../../runtime/AutomationModes.h"
#include "../../runtime/RuntimeUtils.h"
#include "../../runtime/WindowsVersion.h"
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <dwmapi.h>
#include <memory>
#include <string>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Gaming.Input.h>

#ifdef OP_ENABLE_WGC
namespace op::capture {
namespace {

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

class D3D11TextureMap {
  public:
    D3D11TextureMap(ID3D11DeviceContext *context, ID3D11Resource *resource) : context_(context), resource_(resource) {
    }

    ~D3D11TextureMap() {
        if (mapped_) {
            context_->Unmap(resource_, 0);
        }
    }

    D3D11TextureMap(const D3D11TextureMap &) = delete;
    D3D11TextureMap &operator=(const D3D11TextureMap &) = delete;

    HRESULT map(D3D11_MAPPED_SUBRESOURCE *mapped) {
        if (!context_ || !resource_ || !mapped) {
            return E_POINTER;
        }
        HRESULT hr = context_->Map(resource_, 0, D3D11_MAP_READ, 0, mapped);
        if (SUCCEEDED(hr)) {
            mapped_ = true;
        }
        return hr;
    }

  private:
    ID3D11DeviceContext *context_;
    ID3D11Resource *resource_;
    bool mapped_ = false;
};

bool wgcSessionPropertyPresent(const wchar_t *property_name) {
    try {
        return winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(
            L"Windows.Graphics.Capture.GraphicsCaptureSession", property_name);
    } catch (...) {
        return false;
    }
}

// 用 WinRT API 特性探测替代 Windows build 号判断，可靠地关闭黄框与光标，
// 避免它们污染找色/找图/OCR 的像素。Init 与 restartCaptureSession 共用。
void applySessionOptions(const winrt::Windows::Graphics::Capture::GraphicsCaptureSession &session) {
    try {
        if (wgcSessionPropertyPresent(L"IsBorderRequired")) {
            // 关黄框前先申请 Borderless 访问，确保部分 Windows 版本上 IsBorderRequired(false) 真正生效。
            // 这里不要同步 .get()。在部分宿主线程/窗口模型下，这个异步请求可能一直不返回，
            // 从而把 BindWindow("normal.wgc", ...) 整个卡死。这里退化成纯 best-effort：
            // 发起请求即可，真正能否隐藏黄框不影响 WGC 捕获主流程。
            try {
                const auto access_request =
                    winrt::Windows::Graphics::Capture::GraphicsCaptureAccess::RequestAccessAsync(
                        winrt::Windows::Graphics::Capture::GraphicsCaptureAccessKind::Borderless);
                (void)access_request;
            } catch (...) {
            }
            session.IsBorderRequired(false);
        }
    } catch (...) {
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
    UnBindEx();
}

long WgcCapture::BindEx(HWND _hwnd, long render_type) {
    if (!Init(_hwnd)) {
        setlog("Init wgc failed");
        return 0;
    }
    return 1;
}

long WgcCapture::UnBindEx() {
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

bool WgcCapture::Init(HWND _hwnd) {
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
    closedToken_ = item_.Closed([this](auto const &, auto const &) { itemClosed_.store(true); });
    hasClosedToken_ = true;
    device_ = device;
    framePool_ = frame_pool;
    session_ = session;
    frameArrivedToken_ = framePool_.FrameArrived([this](const Direct3D11CaptureFramePool &sender, auto const &) {
        auto frame = tryGetLatestFrame(sender);
        if (frame) {
            copyFrameToStaging(frame);
        }
    });
    hasFrameArrivedToken_ = true;

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
    if (itemClosed_.load()) {
        setlog("requestCapture: capture item closed (target window gone)");
        return false;
    }
    if (isDeviceLost()) {
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

    img.create(w, h);
    if (!_shmem || !_pmutex) {
        setlog("requestCapture: shared memory not initialized");
        return false;
    }

    if (metrics_changed || restored) {
        // 尺寸变化后队列里可能还有旧帧，先抽干一次，再按状态变化等待后续帧。
        updateLatestFrame();
        const unsigned long long drained_serial = currentFrameSerial();
        // 恢复后只等 1 帧、且只给很短的自恢复窗口(150ms)：WGC 常在恢复后停吐帧，
        // 与其干等再重启，不如尽快落到下面的重启会话路径(能自恢复的机器仍会在 150ms 内命中)。
        const unsigned int frame_count = 1;
        const unsigned long timeout_ms = restored ? 150 : 100;
        if (!waitForFramesAfter(drained_serial, frame_count, timeout_ms)) {
            // 尺寸变化/最大化/恢复后 WGC 偶尔不继续吐帧，重启捕获会话能避免拿旧 staging 截新坐标。
            setlog("requestCapture: no fresh WGC frame after window metrics change, restarting capture session");
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
    } else if (!updateLatestFrame()) {
        return false;
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
        if (_pmutex && _shmem) {
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

    const bool changed = !hasWindowState_ || lastClientWidth_ != _width || lastClientHeight_ != _height ||
                         was_iconic != now_iconic;
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

    D3D11_BOX client_box = {};
    int client_w = 0;
    int client_h = 0;
    if (!getClientBox(surface_w, surface_h, client_box, client_w, client_h)) {
        return hasCapturedFrame();
    }

    if (surface_w != captureWidth_ || surface_h != captureHeight_) {
        captureWidth_ = surface_w;
        captureHeight_ = surface_h;
        // 窗口尺寸增大时让 WGC 帧池跟随内容大小重建（帧池按 content size 约定）。
        const auto frame_content_size = frame.ContentSize();
        if (frame_content_size.Width > 0 && frame_content_size.Height > 0) {
            try {
                framePool_.Recreate(device_,
                                    winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized, 2,
                                    frame_content_size);
            } catch (...) {
                // 重建失败下一帧再试，不影响本帧拷贝。
            }
        }
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
    Direct3D11CaptureFrame frame = tryGetLatestFrame(framePool_);
    if (frame) {
        return copyFrameToStaging(frame);
    }

    return hasCapturedFrame();
}

bool WgcCapture::waitForFramesAfter(unsigned long long frame_serial, unsigned int frame_count,
                                    unsigned long timeout_ms) {
    const unsigned long long deadline = ::GetTickCount64() + timeout_ms;
    do {
        if (updateLatestFrame() && currentFrameSerial() >= frame_serial + frame_count) {
            return true;
        }
        if (currentFrameSerial() >= frame_serial + frame_count) {
            return true;
        }
        ::Sleep(8);
    } while (::GetTickCount64() < deadline);

    updateLatestFrame();
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
