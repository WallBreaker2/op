// DXGIDuplicator.cpp

#include "DxgiCapture.h"
#include "../../image/Image.h"
#include "../../runtime/AutomationModes.h"
#include "../../runtime/RuntimeUtils.h"
#include <atlbase.h>
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

long DxgiCapture::BindEx(HWND _hwnd, long render_type) {
    if (!InitD3D11Device()) {
        setlog("Init d3d11 device failed");
        return 0;
    }

    if (!InitDuplication()) {
        setlog("Init duplication failed");
        return 0;
    }
    RECT rc, rc2;
    ::GetWindowRect(_hwnd, &rc);
    ::GetClientRect(_hwnd, &rc2);

    _width = rc2.right - rc2.left;
    _height = rc2.bottom - rc2.top;
    POINT pt = {0};
    ::ClientToScreen(_hwnd, &pt);
    dx_ = pt.x - rc.left;
    dy_ = pt.y - rc.top;
    return 1;
}

long DxgiCapture::UnBindEx() {
    duplication_.Release();
    lastTexture_.Release();
    device_.Release();
    deviceContext_.Release();
    return 0;
}

bool DxgiCapture::requestCapture(int x1, int y1, int w, int h, Image &img) {
    img.create(w, h);
    ID3D11Texture2D *texture_raw = nullptr;
    if (!GetDesktopFrame(&texture_raw)) {
        setlog("Acquire frame failed");
        return false;
    }

    CComPtr<ID3D11Texture2D> texture2D;
    texture2D.Attach(texture_raw);
    if (!texture2D) {
        return false;
    }

    texture2D->GetDesc(&m_desc);

    RECT rc;
    ::GetWindowRect(_hwnd, &rc);
    int src_x = x1 + rc.left + dx_;
    int src_y = y1 + rc.top + dy_;
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

bool DxgiCapture::InitD3D11Device() {
    device_.Release();
    deviceContext_.Release();

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

    for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex) {
        HRESULT hr = D3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, 0, FeatureLevels,
                                       NumFeatureLevels, D3D11_SDK_VERSION, &device_, &FeatureLevel, &deviceContext_);
        if (SUCCEEDED(hr)) {
            break;
        }
    }

    if (device_ == nullptr || deviceContext_ == nullptr) {
        return false;
    }

    return true;
}

bool DxgiCapture::InitDuplication() {
    HRESULT hr = S_OK;
    duplication_.Release();

    CComPtr<IDXGIDevice> dxgiDevice;
    hr = device_->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void **>(&dxgiDevice));
    if (FAILED(hr)) {
        return false;
    }

    CComPtr<IDXGIAdapter> dxgiAdapter;
    hr = dxgiDevice->GetAdapter(&dxgiAdapter);
    if (FAILED(hr)) {
        return false;
    }

    UINT output = 0;
    CComPtr<IDXGIOutput> dxgiOutput;
    while (true) {
        dxgiOutput.Release();
        hr = dxgiAdapter->EnumOutputs(output++, &dxgiOutput);
        if (hr == DXGI_ERROR_NOT_FOUND) {
            return false;
        }
        if (FAILED(hr)) {
            return false;
        }
        DXGI_OUTPUT_DESC desc;
        dxgiOutput->GetDesc(&desc);
        break;
    }

    CComPtr<IDXGIOutput1> dxgiOutput1;
    hr = dxgiOutput->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void **>(&dxgiOutput1));
    if (FAILED(hr)) {
        return false;
    }

    hr = dxgiOutput1->DuplicateOutput(device_, &duplication_);
    if (FAILED(hr)) {
        return false;
    }

    return true;
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
            return true;
        } else {
            return false;
        }
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
        return false;
    }

    set_out(texture, copyTexture.Detach());
    return true;
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
