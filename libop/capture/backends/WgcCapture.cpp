
#include "WgcCapture.h"
#include "../../image/Image.h"
#include "../../runtime/AutomationModes.h"
#include "../../runtime/RuntimeUtils.h"
#include "../../runtime/WindowsVersion.h"
#include <algorithm>
#include <dwmapi.h>
#include <memory>
#include <string>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Gaming.Input.h>

#ifdef OP_ENABLE_WGC
namespace op::capture {
namespace {

template <typename Target, typename Value> void set_out(Target *target, Value value) {
    if (target)
        *target = static_cast<Target>(value);
}

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
    hasFrame_ = false;
    sharedWidth_ = 0;
    sharedHeight_ = 0;
    captureWidth_ = 0;
    captureHeight_ = 0;
    dx_ = 0;
    dy_ = 0;
    frameSerial_ = 0;
    hasWindowState_ = false;
    lastWindowIconic_ = false;
    pendingMetricsChanged_ = false;
    pendingBecameIconic_ = false;
    pendingRestored_ = false;
    lastClientWidth_ = 0;
    lastClientHeight_ = 0;
    lastDx_ = 0;
    lastDy_ = 0;

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

    // 20348+ 才支持关闭捕获边框。
    if (IsWindows10BuildOrGreater(kWindowsServer2022Build)) {
        session.IsBorderRequired(false);
    }

    // 19041+ 才支持关闭光标捕获。
    if (IsWindows10BuildOrGreater(kWindows10Build2004)) {
        session.IsCursorCaptureEnabled(false);
    }

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

    if (!ensureStagingTexture(captureWidth_, captureHeight_)) {
        setlog("Failed to create staging texture");
        return false;
    }

    if (!ensureSharedResources(captureWidth_, captureHeight_)) {
        return false;
    }

    item_ = item;
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
        const unsigned int frame_count = restored ? 2 : 1;
        const unsigned long timeout_ms = restored ? 700 : 100;
        if (!waitForFramesAfter(drained_serial, frame_count, timeout_ms)) {
            if (!restored) {
                setlog("requestCapture: no fresh WGC frame after window metrics change");
            } else {
                // 从最小化恢复时 WGC 偶尔不继续吐帧，重启捕获会话能避免恢复后一直失败。
                setlog("requestCapture: no fresh WGC frame after restore, restarting capture session");
                if (!restartCaptureSession()) {
                    setlog("requestCapture: restart WGC capture session failed after restore");
                    return false;
                }
                const unsigned long long restarted_serial = currentFrameSerial();
                if (!waitForFramesAfter(restarted_serial, 1, 1000)) {
                    setlog("requestCapture: no fresh WGC frame after restore");
                    return false;
                }
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

        // WGC 这里按客户区坐标裁图，去掉标题栏和边框。
        int src_x = x1 + dx_;
        int src_y = y1 + dy_;
        bool ok = true;
        if (src_x < 0 || src_y < 0 || src_x + w > static_cast<int>(desc.Width) ||
            src_y + h > static_cast<int>(desc.Height)) {
            setlog("error w and h src_x=%d,w=%d,desc.Width=%d,src_y=%d,h=%d,desc.Height=%d", src_x, w, desc.Width,
                   src_y, h, desc.Height);
            ok = false;
        } else {
            for (int i = 0; i < h; i++) {
                memcpy(img.ptr<uchar>(i), pData + (src_y + i) * mappedResource.RowPitch + src_x * 4, 4 * w);
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
    RECT window_rect = {};
    RECT client_rect = {};
    RECT visible_rect = {};
    POINT pt = {0, 0};
    const bool was_iconic = hasWindowState_ && lastWindowIconic_;
    const bool now_iconic = ::IsIconic(_hwnd) != FALSE;
    ::GetWindowRect(_hwnd, &window_rect);
    ::GetClientRect(_hwnd, &client_rect);
    ::ClientToScreen(_hwnd, &pt);
    if (::DwmGetWindowAttribute(_hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &visible_rect, sizeof(visible_rect)) == S_OK) {
        window_rect = visible_rect;
    }

    // WGC 对外仍按客户区大小工作，上层坐标也按客户区理解。
    _width = client_rect.right - client_rect.left;
    _height = client_rect.bottom - client_rect.top;
    dx_ = pt.x - window_rect.left;
    dy_ = pt.y - window_rect.top;

    const bool changed = !hasWindowState_ || lastClientWidth_ != _width || lastClientHeight_ != _height ||
                         lastDx_ != dx_ || lastDy_ != dy_ || was_iconic != now_iconic;
    set_out(iconic_changed, hasWindowState_ && was_iconic != now_iconic);
    set_out(is_iconic, now_iconic);
    hasWindowState_ = true;
    lastWindowIconic_ = now_iconic;
    lastClientWidth_ = _width;
    lastClientHeight_ = _height;
    lastDx_ = dx_;
    lastDy_ = dy_;
    return changed;
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

    captureWidth_ = item_size.Width;
    captureHeight_ = item_size.Height;
    if (!ensureStagingTexture(captureWidth_, captureHeight_)) {
        setlog("restartCaptureSession: create staging texture failed");
        return false;
    }
    if (!ensureSharedResources(captureWidth_, captureHeight_)) {
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

        // 20348+ 才支持关闭捕获边框。
        if (IsWindows10BuildOrGreater(kWindowsServer2022Build)) {
            session_.IsBorderRequired(false);
        }

        // 19041+ 才支持关闭光标捕获。
        if (IsWindows10BuildOrGreater(kWindows10Build2004)) {
            session_.IsCursorCaptureEnabled(false);
        }

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

bool WgcCapture::copyFrameToStaging(const Direct3D11CaptureFrame &frame) {
    const auto frame_content_size = frame.ContentSize();
    if (frame_content_size.Width <= 0 || frame_content_size.Height <= 0) {
        return hasCapturedFrame();
    }

    if (frame_content_size.Width != captureWidth_ || frame_content_size.Height != captureHeight_) {
        captureWidth_ = frame_content_size.Width;
        captureHeight_ = frame_content_size.Height;
        if (!ensureStagingTexture(captureWidth_, captureHeight_)) {
            setlog("copyFrameToStaging: resize staging texture failed");
            return hasCapturedFrame();
        }
        if (!ensureSharedResources(captureWidth_, captureHeight_)) {
            setlog("copyFrameToStaging: resize shared resources failed");
            return hasCapturedFrame();
        }
        framePool_.Recreate(device_, winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized, 2,
                            frame_content_size);
    }

    auto frame_surface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
    {
        std::scoped_lock lock(frameMutex_);
        d3dDeviceContext_->CopyResource(stagingTexture_, frame_surface.get());
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

bool WgcCapture::waitForFramesAfter(unsigned long long frame_serial, unsigned int frame_count, unsigned long timeout_ms) {
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
