// DXGIDuplicator.cpp

#include "DxgiCapture.h"
#include <mutex>
#include "../../base/AutomationModes.h"
#include "../../base/Utils.h"
#include "../../hook/DxCaptureCommon.h"
#include "../../image/Image.h"
#include <algorithm>
#include <atlbase.h>
#include <cstring>
#include <string>

namespace op::capture {
namespace {

using ATL::CComPtr;
using op::hook::D3D11TextureMap;

// duplication 重建失败后的退避时间。一次重建要枚举适配器/输出、CreateD3D11Device、
// DuplicateOutput，代价远高于一帧；失败时（被其他采集器占用、安全桌面、驱动拒绝）
// 如果每帧都重试，单帧耗时会出现几十毫秒的长尾。
constexpr unsigned long long kRebuildBackoffMs = 300;

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

    HRESULT acquire(UINT timeout_ms, DXGI_OUTDUPL_FRAME_INFO *frame_info, IDXGIResource **resource) {
        if (!duplication_ || !frame_info || !resource) {
            return E_POINTER;
        }
        HRESULT hr = duplication_->AcquireNextFrame(timeout_ms, frame_info, resource);
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

// 设备寄存：D3D11CreateDevice 实测 94~141ms（占 BindWindow 的 60~80%），而
// BindingSession::UnBindWindow() 会 _capture.reset() 销毁 DxgiCapture，成员活不到下次绑定。
// 所以把设备放在进程级 holder 里，跨对象生命周期存活。
//
// 故意 leak：静态 CComPtr 在进程/模块退出时析构会碰 loader lock，交由 OS 回收。
struct SharedD3D11Device {
    ATL::CComPtr<ID3D11Device> device;
    ATL::CComPtr<ID3D11DeviceContext> context;
    LUID luid{};
    bool luid_valid{false};
    bool lost{false};
};

SharedD3D11Device &shared_device() {
    static SharedD3D11Device *holder = new SharedD3D11Device();
    return *holder;
}

std::mutex &shared_device_mutex() {
    static std::mutex *mtx = new std::mutex();
    return *mtx;
}

// 存活 capture 实例数（受 shared_device_mutex 保护）。D3D11 immediate context 不是线程安全的，
// 只有唯一实例时才允许复用寄存的设备，否则两个 capture 会并发用同一个 context。
int &live_capture_count() {
    static int *count = new int(0);
    return *count;
}

} // namespace

DxgiCapture::DxgiCapture() {
    std::lock_guard<std::mutex> lock(shared_device_mutex());
    ++live_capture_count();
}

DxgiCapture::~DxgiCapture() {
    UnBindEx();
    std::lock_guard<std::mutex> lock(shared_device_mutex());
    --live_capture_count();
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
    invalidateRegionTexture();
    lastTexture_.Release();
    // device_/deviceContext_ 不在这里释放：它们指向寄存器里的设备（见 shared_device()），
    // 对象销毁只是放掉自己的引用，设备留到下次绑定复用。真正的失效由 DEVICE_REMOVED 驱动。
    target_monitor_ = nullptr;
    output_rect_ = {};
    client_screen_origin_ = {};
    duplication_lost_ = false;
    return 0;
}

void DxgiCapture::waitForBindReady() {
    const unsigned long long deadline = ::GetTickCount64() + 1000;
    int fresh_frames = 0;
    do {
        ID3D11Texture2D *texture_raw = nullptr;
        bool fresh_frame = false;
        // 部分系统/驱动刚创建 duplication 后的第一个 fresh frame 可能还没包含目标窗口内容；
        // 这里靠帧序推进预热到下一帧，而不是用像素颜色猜测黑帧。
        if (GetDesktopFrame(&texture_raw, 50, &fresh_frame)) {
            if (texture_raw) {
                texture_raw->Release();
            }
            if (fresh_frame && ++fresh_frames >= 2) {
                return;
            }
        }

        if (duplication_lost_ && !RebuildDuplication()) {
            return;
        }
        ::Sleep(8);
    } while (::GetTickCount64() < deadline);
}

bool DxgiCapture::requestCapture(int x1, int y1, int w, int h, Image &img) {
    // 重建失败后的退避：失败态下不要在热路径上每帧完整重建设备 + duplication。
    if (duplication_lost_ && rebuild_backoff_until_ms_ != 0 &&
        ::GetTickCount64() < rebuild_backoff_until_ms_) {
        return false;
    }
    if (!refreshWindowMetrics()) {
        // refreshWindowMetrics 内部已经尝试过重建（跨显示器时），失败信息在那边记录；
        // 这里不再重复重建，避免一次调用里做两遍枚举 + CreateDevice。
        setlog("Refresh DXGI window metrics failed");
        return false;
    }

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

    if (!copyTextureToImage(texture2D, x1, y1, w, h, img)) {
        return false;
    }

    return true;
}

bool DxgiCapture::copyTextureToImage(ID3D11Texture2D *texture, int x1, int y1, int w, int h, Image &img) {
    if (!texture) {
        return false;
    }

    texture->GetDesc(&m_desc);

    // DXGI 输出纹理使用当前显示器坐标系，窗口客户区需要先转成输出内坐标。
    // 最大化窗口的客户区有时会被 DWM 报成略微越过显示器边界，例如 (-1,-1,1922,1022)。
    // DXGI 纹理只包含真实输出区域，越界部分没有可读像素；这里夹取相交区域，保持返回图尺寸不变。
    const int src_x = x1 + client_screen_origin_.x - output_rect_.left;
    const int src_y = y1 + client_screen_origin_.y - output_rect_.top;
    const int desc_width = static_cast<int>(m_desc.Width);
    const int desc_height = static_cast<int>(m_desc.Height);
    const int copy_src_x = std::clamp(src_x, 0, desc_width);
    const int copy_src_y = std::clamp(src_y, 0, desc_height);
    const int copy_src_right = std::clamp(src_x + w, 0, desc_width);
    const int copy_src_bottom = std::clamp(src_y + h, 0, desc_height);
    const int copy_w = copy_src_right - copy_src_x;
    const int copy_h = copy_src_bottom - copy_src_y;
    if (copy_w <= 0 || copy_h <= 0) {
        setlog("DXGI capture rectangle outside output src_x=%d,w=%d,desc.Width=%d,src_y=%d,h=%d,desc.Height=%d", src_x,
               w, m_desc.Width, src_y, h, m_desc.Height);
        return false;
    }
    const int dst_x = copy_src_x - src_x;
    const int dst_y = copy_src_y - src_y;

    // 只把请求的那块 GPU 矩形拷进小 staging 纹理，CPU 侧也只映射这块。
    // 老实现每帧 Map 整屏纹理（1080p 约 8MB 走 PCIe），静态画面下这笔开销完全是浪费。
    if (!ensureRegionTexture(copy_w, copy_h, m_desc.Format)) {
        setlog("Create region staging texture failed w=%d h=%d format=%d", copy_w, copy_h,
               static_cast<int>(m_desc.Format));
        return false;
    }

    const D3D11_BOX src_box = {static_cast<UINT>(copy_src_x),     static_cast<UINT>(copy_src_y),      0,
                               static_cast<UINT>(copy_src_right), static_cast<UINT>(copy_src_bottom), 1};
    // 注意：ID3D11DeviceContext::CopySubresourceRegion 返回 void，没有 HRESULT 可查。
    // 设备失效只能靠下面的 Map 失败来发现，所以 region 纹理与 device 的生命周期
    // 必须严格对齐（见 invalidateRegionTexture 的调用点）。
    deviceContext_->CopySubresourceRegion(regionTexture_, 0, 0, 0, 0, texture, 0, &src_box);

    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    D3D11TextureMap mappedTexture(deviceContext_, regionTexture_);
    HRESULT hr = mappedTexture.map(&mappedResource);
    if (FAILED(hr)) {
        setlog("Map desktop frame failed hr=0x%08X", hr);
        return false;
    }

    img.create(w, h);
    // 只有在请求区域被输出边界夹取、需要补边时才清底，正常路径整块都会被覆盖。
    if (dst_x != 0 || dst_y != 0 || copy_w != w || copy_h != h) {
        img.fill(0xff000000);
    }
    if (_pmutex && _shmem) {
        _pmutex->lock();
        fmtFrameInfo(_shmem->data<byte>(), _hwnd, w, h);
        _pmutex->unlock();
    }

    const auto *src_pixels = static_cast<const unsigned char *>(mappedResource.pData);
    unsigned char *dst_pixels = img.ptr<unsigned char>(dst_y) + static_cast<size_t>(dst_x) * 4;
    const size_t row_bytes = static_cast<size_t>(copy_w) * 4;
    if (dst_x == 0 && copy_w == img.width && mappedResource.RowPitch == row_bytes) {
        // 行距与目标一致时一次 memcpy 搞定整块。
        memcpy(dst_pixels, src_pixels, row_bytes * static_cast<size_t>(copy_h));
    } else {
        const size_t dst_pitch = static_cast<size_t>(img.width) * 4;
        for (int i = 0; i < copy_h; i++) {
            memcpy(dst_pixels + static_cast<size_t>(i) * dst_pitch,
                   src_pixels + static_cast<size_t>(i) * mappedResource.RowPitch, row_bytes);
        }
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
    // 设备会在 InitDuplication -> InitD3D11Device 里换手，区域纹理一并作废。
    invalidateRegionTexture();
    duplication_lost_ = false;
    if (InitDuplication()) {
        rebuild_backoff_until_ms_ = 0;
        return true;
    }
    duplication_lost_ = true;
    rebuild_backoff_until_ms_ = ::GetTickCount64() + kRebuildBackoffMs;
    return false;
}

bool DxgiCapture::InitD3D11Device(IDXGIAdapter *adapter, bool force_recreate) {
    if (!adapter) {
        return false;
    }

    DXGI_ADAPTER_DESC adapter_desc = {};
    const bool has_adapter_desc = SUCCEEDED(adapter->GetDesc(&adapter_desc));

    // 复用寄存的设备：适配器没变、设备没被标记失效、且当前只有本实例一个 capture。
    // 不满足任何一条就走下面的完整重建。
    {
        std::lock_guard<std::mutex> lock(shared_device_mutex());
        SharedD3D11Device &shared = shared_device();
        if (!force_recreate && !device_lost_ && !shared.lost && live_capture_count() == 1 &&
            shared.device != nullptr && shared.context != nullptr && shared.luid_valid && has_adapter_desc &&
            shared.luid.LowPart == adapter_desc.AdapterLuid.LowPart &&
            shared.luid.HighPart == adapter_desc.AdapterLuid.HighPart) {
            device_ = shared.device;
            deviceContext_ = shared.context;
            device_adapter_luid_ = shared.luid;
            device_adapter_valid_ = true;
            return true;
        }
    }

    // 设备所有权在这里换手：旧设备上的区域 staging 纹理必须一起作废。
    // 否则 ensureRegionTexture 会按 (w,h,format) 命中缓存，把旧设备纹理交给新的
    // deviceContext_ 去 CopySubresourceRegion/Map——D3D11 资源不可跨设备使用，
    // 之后每帧都会失败，而缓存不会失效，自愈路径被彻底堵死。
    invalidateRegionTexture();
    device_.Release();
    deviceContext_.Release();
    device_adapter_valid_ = false;
    device_lost_ = false;

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

    if (has_adapter_desc) {
        device_adapter_luid_ = adapter_desc.AdapterLuid;
        device_adapter_valid_ = true;
    }
    // 把新设备寄存起来，供下一次绑定复用（本对象销毁后仍然存活）。
    {
        std::lock_guard<std::mutex> lock(shared_device_mutex());
        SharedD3D11Device &shared = shared_device();
        shared.device = device_;
        shared.context = deviceContext_;
        if (has_adapter_desc) {
            shared.luid = adapter_desc.AdapterLuid;
            shared.luid_valid = true;
        }
        shared.lost = false;
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

bool DxgiCapture::GetDesktopFrame(ID3D11Texture2D **texture, DWORD timeout_ms, bool *fresh_frame) {
    if (!set_out(texture, nullptr))
        return false;
    if (fresh_frame) {
        *fresh_frame = false;
    }

    HRESULT hr = S_OK;
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    CComPtr<IDXGIResource> resource;
    CComPtr<ID3D11Texture2D> acquireFrame;
    DxgiFrameLease frameLease(duplication_);
    hr = frameLease.acquire(timeout_ms, &frameInfo, &resource);
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
            // ACCESS_LOST 只丢 duplication（设备还能用）；DEVICE_REMOVED/RESET 连设备一起废掉，
            // 必须连寄存器一起标记失效，否则下次会拿死设备去 AcquireNextFrame。
            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
                device_lost_ = true;
                std::lock_guard<std::mutex> lock(shared_device_mutex());
                shared_device().lost = true;
            }
        }
        return false;
    }

    hr = resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&acquireFrame));
    if (FAILED(hr)) {
        return false;
    }

    D3D11_TEXTURE2D_DESC desc;
    acquireFrame->GetDesc(&desc);
    if (!ensureDesktopStagingTexture(desc)) {
        return false;
    }
    deviceContext_->CopyResource(lastTexture_, acquireFrame);

    hr = frameLease.release();
    if (FAILED(hr)) {
        duplication_lost_ = hr == DXGI_ERROR_ACCESS_LOST;
        return false;
    }

    if (fresh_frame) {
        *fresh_frame = true;
    }
    return SUCCEEDED(lastTexture_.CopyTo(texture));
}

bool DxgiCapture::ensureDesktopStagingTexture(const D3D11_TEXTURE2D_DESC &source_desc) {
    // 尺寸/格式没变就复用上一帧的 staging 纹理，省掉每帧 CreateTexture2D + 释放。
    if (lastTexture_) {
        D3D11_TEXTURE2D_DESC current = {};
        lastTexture_->GetDesc(&current);
        if (current.Width == source_desc.Width && current.Height == source_desc.Height &&
            current.Format == source_desc.Format && current.SampleDesc.Count == source_desc.SampleDesc.Count) {
            return true;
        }
        lastTexture_.Release();
    }

    D3D11_TEXTURE2D_DESC desc = source_desc;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.BindFlags = 0;
    desc.MiscFlags = 0;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    const HRESULT hr = device_->CreateTexture2D(&desc, nullptr, &lastTexture_);
    if (FAILED(hr) || !lastTexture_) {
        setlog("Create desktop staging texture failed hr=0x%08X w=%u h=%u", hr, desc.Width, desc.Height);
        return false;
    }
    return true;
}

void DxgiCapture::invalidateRegionTexture() {
    regionTexture_.Release();
    region_width_ = 0;
    region_height_ = 0;
    region_format_ = DXGI_FORMAT_UNKNOWN;
}

bool DxgiCapture::ensureRegionTexture(int width, int height, DXGI_FORMAT format) {
    if (width <= 0 || height <= 0) {
        return false;
    }
    // 容量复用，而不是精确尺寸复用：脚本常在“整窗找图”和“10x10 取色”之间切换，
    // 按精确尺寸缓存会让每个不同尺寸都 CreateTexture2D/Release 一次，复用退化成抖动。
    // 纹理比请求大不影响正确性——拷入目标固定从 (0,0) 开始，Map 回来的 RowPitch
    // 比请求行宽时会走下面的逐行分支。
    if (regionTexture_ && region_width_ >= width && region_height_ >= height && region_format_ == format) {
        return true;
    }

    invalidateRegionTexture();

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = static_cast<UINT>(width);
    desc.Height = static_cast<UINT>(height);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.BindFlags = 0;
    desc.MiscFlags = 0;

    const HRESULT hr = device_->CreateTexture2D(&desc, nullptr, &regionTexture_);
    if (FAILED(hr) || !regionTexture_) {
        setlog("Create region texture failed hr=0x%08X w=%d h=%d", hr, width, height);
        return false;
    }

    region_width_ = width;
    region_height_ = height;
    region_format_ = format;
    return true;
}

void DxgiCapture::fmtFrameInfo(void *dst, HWND hwnd, int w, int h, bool inc) {
    WriteFrameInfo(m_frameInfo, dst, hwnd, w, h, inc);
}

} // namespace op::capture
