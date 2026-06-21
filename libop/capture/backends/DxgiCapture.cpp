// DXGIDuplicator.cpp

#include "DxgiCapture.h"
#include "../../image/Image.h"
#include "../../runtime/AutomationModes.h"
#include "../../runtime/RuntimeUtils.h"
#include <atlbase.h>
#include <cstring>
#include <string>

namespace op::capture {
namespace {

using ATL::CComPtr;

template <typename Target, typename Value> bool set_out(Target *target, Value value) {
    if (!target)
        return false;
    *target = static_cast<Target>(value);
    return true;
}

class DxgiFrameLease {
  public:
    explicit DxgiFrameLease(IDXGIOutputDuplication *duplication) : duplication_(duplication) {
    }

    ~DxgiFrameLease() {
        if (acquired_) {
            duplication_->ReleaseFrame();
        }
    }

    DxgiFrameLease(const DxgiFrameLease &) = delete;
    DxgiFrameLease &operator=(const DxgiFrameLease &) = delete;

    HRESULT acquire(DXGI_OUTDUPL_FRAME_INFO *frame_info, IDXGIResource **resource) {
        if (!duplication_ || !frame_info || !resource) {
            return E_POINTER;
        }
        HRESULT hr = duplication_->AcquireNextFrame(0, frame_info, resource);
        if (SUCCEEDED(hr)) {
            acquired_ = true;
        }
        return hr;
    }

    HRESULT release() {
        if (!acquired_) {
            return S_OK;
        }
        acquired_ = false;
        return duplication_->ReleaseFrame();
    }

  private:
    IDXGIOutputDuplication *duplication_;
    bool acquired_ = false;
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

} // namespace

DxgiCapture::DxgiCapture() = default;

DxgiCapture::~DxgiCapture() {
    UnBindEx();
}

long DxgiCapture::BindEx(HWND hwnd, long render_type) {
    (void)hwnd;
    (void)render_type;
    if (!refreshWindowMetrics()) {
        setlog("Refresh DXGI window metrics failed");
        return 0;
    }

    if (!InitDuplication()) {
        setlog("Init duplication failed");
        return 0;
    }
    return 1;
}

long DxgiCapture::UnBindEx() {
    duplication_.Release();
    lastTexture_.Release();
    device_.Release();
    deviceContext_.Release();
    target_monitor_ = nullptr;
    output_rect_ = {};
    client_screen_origin_ = {};
    duplication_lost_ = false;
    return 0;
}

void DxgiCapture::waitForBindReady() {
    const unsigned long long deadline = ::GetTickCount64() + 1000;
    do {
        ID3D11Texture2D *texture_raw = nullptr;
        // DXGI 首帧也是异步到达的，绑定后预热一帧，避免 normal.auto 立即截图拿到黑图。
        if (GetDesktopFrame(&texture_raw)) {
            if (texture_raw) {
                texture_raw->Release();
            }
            return;
        }

        if (duplication_lost_ && !RebuildDuplication()) {
            return;
        }
        ::Sleep(8);
    } while (::GetTickCount64() < deadline);
}

bool DxgiCapture::requestCapture(int x1, int y1, int w, int h, Image &img) {
    if (!refreshWindowMetrics()) {
        if (!duplication_lost_ || !RebuildDuplication()) {
            setlog("Refresh DXGI window metrics failed");
            return false;
        }
    }

    img.create(w, h);
    ID3D11Texture2D *texture_raw = nullptr;
    if (!GetDesktopFrame(&texture_raw)) {
        if (duplication_lost_ && RebuildDuplication() && GetDesktopFrame(&texture_raw)) {
            duplication_lost_ = false;
        } else {
            setlog("Acquire DXGI frame failed");
            return false;
        }
    }

    CComPtr<ID3D11Texture2D> texture2D;
    texture2D.Attach(texture_raw);
    if (!texture2D) {
        return false;
    }

    texture2D->GetDesc(&m_desc);

    // DXGI 输出纹理使用当前显示器坐标系，窗口客户区需要先转成输出内坐标。
    int src_x = x1 + client_screen_origin_.x - output_rect_.left;
    int src_y = y1 + client_screen_origin_.y - output_rect_.top;
    if (src_x < 0 || src_y < 0 || src_x + w > static_cast<int>(m_desc.Width) ||
        src_y + h > static_cast<int>(m_desc.Height)) {
        setlog("error w and h src_x=%d,w=%d,desc.Width=%d,src_y=%d,h=%d,desc.Height=%d", src_x, w, m_desc.Width, src_y,
               h, m_desc.Height);
        return false;
    }

    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    D3D11TextureMap mappedTexture(deviceContext_, texture2D);
    HRESULT hr = mappedTexture.map(&mappedResource);
    if (FAILED(hr)) {
        setlog("Map desktop frame failed hr=0x%08X", hr);
        return false;
    }

    uint8_t *pData = static_cast<uint8_t *>(mappedResource.pData);
    if (_pmutex && _shmem) {
        _pmutex->lock();
        fmtFrameInfo(_shmem->data<byte>(), _hwnd, w, h);
        _pmutex->unlock();
    }

    for (int i = 0; i < h; i++) {
        memcpy(img.ptr<uchar>(i), pData + (src_y + i) * mappedResource.RowPitch + src_x * 4, 4 * w);
    }
    return true;
}

void DxgiCapture::refreshMetrics() {
    refreshWindowMetrics();
}

bool DxgiCapture::refreshWindowMetrics() {
    if (!::IsWindow(_hwnd)) {
        return false;
    }

    RECT client_rect = {};
    if (!::GetClientRect(_hwnd, &client_rect)) {
        return false;
    }

    POINT client_origin = {};
    if (!::ClientToScreen(_hwnd, &client_origin)) {
        return false;
    }

    _width = client_rect.right - client_rect.left;
    _height = client_rect.bottom - client_rect.top;
    client_screen_origin_ = client_origin;

    HMONITOR current_monitor = ::MonitorFromWindow(_hwnd, MONITOR_DEFAULTTONEAREST);
    if (!current_monitor) {
        return false;
    }

    // 窗口拖到另一块显示器后，重建到对应输出，避免继续读取旧输出纹理。
    if (target_monitor_ && current_monitor != target_monitor_) {
        target_monitor_ = current_monitor;
        return RebuildDuplication();
    }

    target_monitor_ = current_monitor;
    return true;
}

bool DxgiCapture::RebuildDuplication() {
    duplication_.Release();
    lastTexture_.Release();
    duplication_lost_ = false;
    if (InitDuplication()) {
        return true;
    }
    duplication_lost_ = true;
    return false;
}

bool DxgiCapture::InitD3D11Device(IDXGIAdapter *adapter) {
    if (!adapter) {
        return false;
    }

    device_.Release();
    deviceContext_.Release();

    D3D_FEATURE_LEVEL FeatureLevels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
                                         D3D_FEATURE_LEVEL_9_1};
    UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);
    D3D_FEATURE_LEVEL FeatureLevel;
    HRESULT hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, FeatureLevels, NumFeatureLevels,
                                   D3D11_SDK_VERSION, &device_, &FeatureLevel, &deviceContext_);

    if (device_ == nullptr || deviceContext_ == nullptr) {
        setlog("Create DXGI D3D11 device failed hr=0x%08X", hr);
        return false;
    }

    return true;
}

bool DxgiCapture::InitDuplication() {
    HRESULT hr = S_OK;
    duplication_.Release();
    lastTexture_.Release();
    duplication_lost_ = false;

    // 按窗口所在 HMONITOR 找到真正的 DXGI 输出，多显示器时不能默认取第一个输出。
    if (!target_monitor_) {
        target_monitor_ = ::MonitorFromWindow(_hwnd, MONITOR_DEFAULTTONEAREST);
    }
    if (!target_monitor_) {
        return false;
    }

    CComPtr<IDXGIFactory1> factory;
    hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void **>(&factory));
    if (FAILED(hr)) {
        setlog("CreateDXGIFactory1 failed hr=0x%08X", hr);
        return false;
    }

    for (UINT adapter_index = 0;; ++adapter_index) {
        CComPtr<IDXGIAdapter1> adapter;
        hr = factory->EnumAdapters1(adapter_index, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        if (FAILED(hr)) {
            continue;
        }

        for (UINT output_index = 0;; ++output_index) {
            CComPtr<IDXGIOutput> output;
            hr = adapter->EnumOutputs(output_index, &output);
            if (hr == DXGI_ERROR_NOT_FOUND) {
                break;
            }
            if (FAILED(hr)) {
                continue;
            }

            DXGI_OUTPUT_DESC desc = {};
            hr = output->GetDesc(&desc);
            if (FAILED(hr) || desc.Monitor != target_monitor_) {
                continue;
            }

            if (!InitD3D11Device(adapter)) {
                return false;
            }

            CComPtr<IDXGIOutput1> output1;
            hr = output->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void **>(&output1));
            if (FAILED(hr)) {
                return false;
            }

            hr = output1->DuplicateOutput(device_, &duplication_);
            if (FAILED(hr)) {
                setlog("DuplicateOutput failed hr=0x%08X", hr);
                return false;
            }

            output_rect_ = desc.DesktopCoordinates;
            return true;
        }
    }

    setlog("DXGI output not found for monitor=%p", target_monitor_);
    return false;
}

bool DxgiCapture::GetDesktopFrame(ID3D11Texture2D **texture) {
    if (!set_out(texture, nullptr))
        return false;

    HRESULT hr = S_OK;
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    CComPtr<IDXGIResource> resource;
    CComPtr<ID3D11Texture2D> acquireFrame;
    DxgiFrameLease frameLease(duplication_);
    hr = frameLease.acquire(&frameInfo, &resource);
    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            // 没有新桌面帧时复用上一帧，避免静态画面被当作捕获失败。
            if (!lastTexture_) {
                return false;
            }
            return SUCCEEDED(lastTexture_.CopyTo(texture));
        }
        if (hr == DXGI_ERROR_ACCESS_LOST || hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            duplication_lost_ = true;
        }
        return false;
    }

    hr = resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&acquireFrame));
    if (FAILED(hr)) {
        return false;
    }

    D3D11_TEXTURE2D_DESC desc;
    acquireFrame->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.BindFlags = 0;
    desc.MiscFlags = 0;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.SampleDesc.Count = 1;
    CComPtr<ID3D11Texture2D> copyTexture;
    hr = device_->CreateTexture2D(&desc, NULL, &copyTexture);
    if (FAILED(hr) || !copyTexture) {
        return false;
    }
    deviceContext_->CopyResource(copyTexture, acquireFrame);

    hr = frameLease.release();
    if (FAILED(hr)) {
        duplication_lost_ = hr == DXGI_ERROR_ACCESS_LOST;
        return false;
    }

    lastTexture_ = copyTexture;
    return SUCCEEDED(lastTexture_.CopyTo(texture));
}

void DxgiCapture::fmtFrameInfo(void *dst, HWND hwnd, int w, int h, bool inc) {
    m_frameInfo.hwnd = (unsigned __int64)hwnd;
    m_frameInfo.frameId = inc ? m_frameInfo.frameId + 1 : m_frameInfo.frameId;
    m_frameInfo.time = static_cast<unsigned __int32>(::GetTickCount64());
    m_frameInfo.width = w;
    m_frameInfo.height = h;
    m_frameInfo.fmtChk();
    memcpy(dst, &m_frameInfo, sizeof(m_frameInfo));
}

} // namespace op::capture
