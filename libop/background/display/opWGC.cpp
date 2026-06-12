
#include "opWGC.h"
#include "./core/globalVar.h"
#include "./core/helpfunc.h"
#include "./core/win_version.h"
#include "./include/Image.hpp"
#include <dwmapi.h>
#include <string>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Gaming.Input.h>

#ifdef OP_ENABLE_WGC
opWGC::opWGC()
    : device_(nullptr), item_(nullptr), framePool_(nullptr), session_(nullptr), d3dDevice_(nullptr),
      d3dDeviceContext_(nullptr), stagingTexture_(nullptr), m_frameInfo(), captureWidth_(0), captureHeight_(0),
      hasFrame_(false), dx_(0), dy_(0) {
}

opWGC::~opWGC() {
    UnBindEx();
}

long opWGC::BindEx(HWND _hwnd, long render_type) {
    if (!Init(_hwnd)) {
        setlog("Init wgc failed");
        return 0;
    }
    return 1;
}

long opWGC::UnBindEx() {
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

    if (device_) {
        try {
            device_.Close();
        } catch (winrt::hresult_error &err) {
            setlog("IDirect3DDevice::Close (0x%08X): %s", err.code().value, winrt::to_string(err.message()).c_str());
        } catch (...) {
            setlog("IDirect3DDevice::Close (0x%08X)", winrt::to_hresult().value);
        }
    }

    if (d3dDevice_) {
        d3dDevice_->Release();
    }
    if (d3dDeviceContext_) {
        d3dDeviceContext_->Release();
    }
    if (stagingTexture_) {
        stagingTexture_->Release();
    }

    session_ = nullptr;
    framePool_ = nullptr;
    device_ = nullptr;
    item_ = nullptr;
    d3dDeviceContext_ = nullptr;
    d3dDevice_ = nullptr;
    stagingTexture_ = nullptr;
    frameArrivedToken_ = {};
    hasFrameArrivedToken_ = false;
    hasFrame_ = false;
    sharedWidth_ = 0;
    sharedHeight_ = 0;
    captureWidth_ = 0;
    captureHeight_ = 0;
    dx_ = 0;
    dy_ = 0;

    return 0;
}

bool opWGC::Init(HWND _hwnd) {
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

bool opWGC::requestCapture(int x1, int y1, int w, int h, Image &img) {
    img.create(w, h);
    if (!_shmem || !_pmutex) {
        setlog("requestCapture: shared memory not initialized");
        return false;
    }

    if (!updateLatestFrame() || !hasFrame_) {
        return false;
    }

    refreshWindowMetrics();
    D3D11_TEXTURE2D_DESC desc;
    {
        std::scoped_lock lock(frameMutex_);
        stagingTexture_->GetDesc(&desc);
    }
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = S_OK;
    {
        std::scoped_lock lock(frameMutex_);
        hr = d3dDeviceContext_->Map(stagingTexture_, 0, D3D11_MAP_READ, 0, &mappedResource);
    }
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
    if (src_x < 0 || src_y < 0 || src_x + w > static_cast<int>(desc.Width) ||
        src_y + h > static_cast<int>(desc.Height)) {
        setlog("error w and h src_x=%d,w=%d,desc.Width=%d,src_y=%d,h=%d,desc.Height=%d", src_x, w, desc.Width, src_y, h,
               desc.Height);
        return false;
    }

    for (int i = 0; i < h; i++) {
        memcpy(img.ptr<uchar>(i), pData + (src_y + i) * mappedResource.RowPitch + src_x * 4, 4 * w);
    }
    {
        std::scoped_lock lock(frameMutex_);
        d3dDeviceContext_->Unmap(stagingTexture_, 0);
    }
    return true;
}

bool opWGC::ensureStagingTexture(int width, int height) {
    std::scoped_lock lock(frameMutex_);
    if (stagingTexture_) {
        D3D11_TEXTURE2D_DESC existing = {};
        stagingTexture_->GetDesc(&existing);
        if ((int)existing.Width == width && (int)existing.Height == height) {
            return true;
        }
        stagingTexture_->Release();
        stagingTexture_ = nullptr;
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

bool opWGC::ensureSharedResources(int width, int height) {
    if (_shmem && _pmutex && sharedWidth_ == width && sharedHeight_ == height) {
        return true;
    }

    SAFE_DELETE(_shmem);
    SAFE_DELETE(_pmutex);

    int res_size = width * height * 4 + sizeof(FrameInfo);
    wsprintf(_shared_res_name, SHARED_RES_NAME_FORMAT, _hwnd);
    wsprintf(_mutex_name, MUTEX_NAME_FORMAT, _hwnd);
    try {
        _shmem = new sharedmem();
        _shmem->open_create(_shared_res_name, res_size);
        _pmutex = new promutex();
        _pmutex->open_create(_mutex_name);
        sharedWidth_ = width;
        sharedHeight_ = height;
        return true;
    } catch (std::exception &e) {
        setlog("bkdisplay::re bind share mem %s exception:%s", _shared_res_name, e.what());
    }

    SAFE_DELETE(_shmem);
    SAFE_DELETE(_pmutex);
    sharedWidth_ = 0;
    sharedHeight_ = 0;
    return false;
}

void opWGC::refreshWindowMetrics() {
    RECT window_rect = {};
    RECT client_rect = {};
    RECT visible_rect = {};
    POINT pt = {0, 0};
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
}

bool opWGC::copyFrameToStaging(const Direct3D11CaptureFrame &frame) {
    const auto frame_content_size = frame.ContentSize();
    if (frame_content_size.Width <= 0 || frame_content_size.Height <= 0) {
        return hasFrame_;
    }

    if (frame_content_size.Width != captureWidth_ || frame_content_size.Height != captureHeight_) {
        captureWidth_ = frame_content_size.Width;
        captureHeight_ = frame_content_size.Height;
        if (!ensureStagingTexture(captureWidth_, captureHeight_)) {
            setlog("copyFrameToStaging: resize staging texture failed");
            return hasFrame_;
        }
        if (!ensureSharedResources(captureWidth_, captureHeight_)) {
            setlog("copyFrameToStaging: resize shared resources failed");
            return hasFrame_;
        }
        framePool_.Recreate(device_, winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized, 2,
                            frame_content_size);
    }

    auto frame_surface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
    {
        std::scoped_lock lock(frameMutex_);
        d3dDeviceContext_->CopyResource(stagingTexture_, frame_surface.get());
    }
    hasFrame_ = true;
    return true;
}

Direct3D11CaptureFrame opWGC::tryGetLatestFrame(const Direct3D11CaptureFramePool &frame_pool) {
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

bool opWGC::updateLatestFrame() {
    Direct3D11CaptureFrame frame = tryGetLatestFrame(framePool_);
    if (frame) {
        return copyFrameToStaging(frame);
    }

    return hasFrame_;
}

void opWGC::fmtFrameInfo(void *dst, HWND hwnd, int w, int h, bool inc) {
    m_frameInfo.hwnd = (unsigned __int64)hwnd;
    m_frameInfo.frameId = inc ? m_frameInfo.frameId + 1 : m_frameInfo.frameId;
    m_frameInfo.time = static_cast<unsigned __int32>(::GetTickCount64());
    m_frameInfo.width = w;
    m_frameInfo.height = h;
    m_frameInfo.fmtChk();
    memcpy(dst, &m_frameInfo, sizeof(m_frameInfo));
}

#endif
