#pragma once
#include "../ICaptureBackend.h"
#include <atlbase.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <string>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// this code ref https://www.jianshu.com/p/e775b0f45376

namespace op::capture {

class DxgiCapture : public ICaptureBackend {
  public:
    DxgiCapture();
    ~DxgiCapture();
    // 绑定
    long BindEx(HWND _hwnd, long render_type) override;
    // 解绑
    long UnBindEx() override;

    virtual bool requestCapture(int x1, int y1, int w, int h, Image &img) override;
    void waitForBindReady() override;
    void refreshMetrics() override;

    bool InitD3D11Device(IDXGIAdapter *adapter, bool force_recreate = false);

    bool InitDuplication();

    bool GetDesktopFrame(ID3D11Texture2D **texture, DWORD timeout_ms = 0, bool *fresh_frame = nullptr);

  private:
    ATL::CComPtr<ID3D11Device> device_;
    ATL::CComPtr<ID3D11DeviceContext> deviceContext_;
    ATL::CComPtr<IDXGIOutputDuplication> duplication_;
    // 整屏 staging 纹理：只在输出尺寸/格式变化时重建，用作“没有新帧时的上一帧”。
    ATL::CComPtr<ID3D11Texture2D> lastTexture_;
    // 请求区域的 staging 纹理：GPU 侧只拷需要的那块，CPU 只映射这块。
    ATL::CComPtr<ID3D11Texture2D> regionTexture_;
    int region_width_{0};
    int region_height_{0};
    DXGI_FORMAT region_format_{DXGI_FORMAT_UNKNOWN};
    HMONITOR target_monitor_{nullptr};
    RECT output_rect_{};
    POINT client_screen_origin_{};
    bool duplication_lost_{false};
    // 设备寄存用的标志：真实失效状态记在进程级 holder 里（见 .cpp 的 shared_device()），
    // 因为 DxgiCapture 对象会随 UnBindWindow 销毁，成员活不到下一次绑定。
    bool device_lost_{false};
    LUID device_adapter_luid_{};
    bool device_adapter_valid_{false};
    // 重建失败后的退避截止时间（GetTickCount64 口径，0 表示无退避）。
    unsigned long long rebuild_backoff_until_ms_{0};
    bool m_first{true};
    FrameInfo m_frameInfo{};
    D3D11_TEXTURE2D_DESC m_desc{};
    bool refreshWindowMetrics();
    bool RebuildDuplication();
    bool ensureDesktopStagingTexture(const D3D11_TEXTURE2D_DESC &source_desc);
    bool ensureRegionTexture(int width, int height, DXGI_FORMAT format);
    // 作废区域 staging 纹理。D3D11 资源与设备强绑定，device 一旦换手就必须调用，
    // 否则按 (w,h,format) 命中的缓存会把旧设备的纹理交给新 context 去用。
    void invalidateRegionTexture();
    bool copyTextureToImage(ID3D11Texture2D *texture, int x1, int y1, int w, int h, Image &img);
    void fmtFrameInfo(void *dst, HWND hwnd, int w, int h, bool inc = true);
};

} // namespace op::capture
