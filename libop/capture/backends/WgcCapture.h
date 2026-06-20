#pragma once
#include <atlbase.h>
#include <d3d11.h>
#include <string>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <winrt/windows.graphics.capture.h>
#include <winrt/windows.graphics.directx.direct3d11.h>
// #include <unknwn.h>
#include "../ICaptureBackend.h"
#include <dxgi1_2.h>
#include <inspectable.h>
#include <mutex>
#pragma comment(lib, "d3d11.lib")
#ifdef OP_ENABLE_WGC
// this code ref https://www.jianshu.com/p/e775b0f45376
using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;

namespace op::capture {

class WgcCapture : public ICaptureBackend {
  public:
    WgcCapture();
    ~WgcCapture();
    // 绑定
    long BindEx(HWND _hwnd, long render_type) override;
    // 解绑
    long UnBindEx() override;

    virtual bool requestCapture(int x1, int y1, int w, int h, Image &img) override;
    void refreshMetrics() override;

    bool Init(HWND _hwnd);

  private:
    ATL::CComPtr<ID3D11Device> d3dDevice_;
    ATL::CComPtr<ID3D11DeviceContext> d3dDeviceContext_;
    ATL::CComPtr<ID3D11Texture2D> stagingTexture_;
    IDirect3DDevice device_{nullptr};
    GraphicsCaptureItem item_{nullptr};
    Direct3D11CaptureFramePool framePool_{nullptr};
    GraphicsCaptureSession session_{nullptr};
    winrt::event_token frameArrivedToken_{};
    bool hasFrameArrivedToken_{false};
    FrameInfo m_frameInfo{};
    long captureWidth_{0};
    long captureHeight_{0};
    bool hasFrame_{false};
    int sharedWidth_{0};
    int sharedHeight_{0};
    int dx_{0};
    int dy_{0};
    unsigned long long frameSerial_{0};
    bool hasWindowState_{false};
    bool lastWindowIconic_{false};
    bool pendingMetricsChanged_{false};
    bool pendingBecameIconic_{false};
    bool pendingRestored_{false};
    long lastClientWidth_{0};
    long lastClientHeight_{0};
    int lastDx_{0};
    int lastDy_{0};
    std::mutex frameMutex_;

    bool ensureStagingTexture(int width, int height);
    bool ensureSharedResources(int width, int height);
    bool refreshWindowMetrics(bool *iconic_changed = nullptr, bool *is_iconic = nullptr);
    void closeCaptureSession();
    bool restartCaptureSession();
    bool copyFrameToStaging(const Direct3D11CaptureFrame &frame);
    Direct3D11CaptureFrame tryGetLatestFrame(const Direct3D11CaptureFramePool &frame_pool);
    bool updateLatestFrame();
    bool waitForFramesAfter(unsigned long long frame_serial, unsigned int frame_count, unsigned long timeout_ms);
    unsigned long long currentFrameSerial();
    bool hasCapturedFrame();
    void fmtFrameInfo(void *dst, HWND hwnd, int w, int h, bool inc = true);
};

// DEFINE_GUID(
//     IID_IGraphicsCaptureItemInterop,
//     // clang-format off
//     0x3628E81B, 0x3CAC, 0x4C60, 0xB7, 0xF4, 0x23, 0xCE, 0x0E, 0x0C, 0x33, 0x56
//     // clang-format on
//);
//
// MIDL_INTERFACE("3628E81B-3CAC-4C60-B7F4-23CE0E0C3356")
// IGraphicsCaptureItemInterop : ::IUnknown
//{
//     virtual HRESULT STDMETHODCALLTYPE CreateForWindow(
//         HWND window, REFIID riid, _COM_Outptr_ void** result) noexcept
//         = 0;
//
//     virtual HRESULT STDMETHODCALLTYPE CreateForMonitor(
//         HMONITOR monitor, REFIID riid, _COM_Outptr_ void** result) noexcept
//         = 0;
// };

template <class T> class ComPtr {

  protected:
    T *ptr;

    inline void Kill() {
        if (ptr)
            ptr->Release();
    }

    inline void Replace(T *p) {
        if (ptr != p) {
            if (p)
                p->AddRef();
            if (ptr)
                ptr->Release();
            ptr = p;
        }
    }

  public:
    inline ComPtr() : ptr(nullptr) {
    }
    inline ComPtr(T *p) : ptr(p) {
        if (ptr)
            ptr->AddRef();
    }
    inline ComPtr(const ComPtr<T> &c) : ptr(c.ptr) {
        if (ptr)
            ptr->AddRef();
    }
    inline ComPtr(ComPtr<T> &&c) noexcept : ptr(c.ptr) {
        c.ptr = nullptr;
    }
    template <class U> inline ComPtr(ComPtr<U> &&c) noexcept : ptr(c.Detach()) {
    }
    inline ~ComPtr() {
        Kill();
    }

    inline void Clear() {
        if (ptr) {
            ptr->Release();
            ptr = nullptr;
        }
    }

    inline ComPtr<T> &operator=(T *p) {
        Replace(p);
        return *this;
    }

    inline ComPtr<T> &operator=(const ComPtr<T> &c) {
        Replace(c.ptr);
        return *this;
    }

    inline ComPtr<T> &operator=(ComPtr<T> &&c) noexcept {
        if (&ptr != &c.ptr) {
            Kill();
            ptr = c.ptr;
            c.ptr = nullptr;
        }

        return *this;
    }

    template <class U> inline ComPtr<T> &operator=(ComPtr<U> &&c) noexcept {
        Kill();
        ptr = c.Detach();

        return *this;
    }

    inline T *Detach() {
        T *out = ptr;
        ptr = nullptr;
        return out;
    }

    inline void CopyTo(T **out) {
        if (out) {
            if (ptr)
                ptr->AddRef();
            *out = ptr;
        }
    }

    inline ULONG Release() {
        ULONG ref;

        if (!ptr)
            return 0;
        ref = ptr->Release();
        ptr = nullptr;
        return ref;
    }

    inline T **Assign() {
        Clear();
        return &ptr;
    }
    inline void Set(T *p) {
        Kill();
        ptr = p;
    }

    inline T *Get() const {
        return ptr;
    }

    inline T **operator&() {
        return Assign();
    }

    inline operator T *() const {
        return ptr;
    }
    inline T *operator->() const {
        return ptr;
    }

    inline bool operator==(T *p) const {
        return ptr == p;
    }
    inline bool operator!=(T *p) const {
        return ptr != p;
    }

    inline bool operator!() const {
        return !ptr;
    }
};

struct __declspec(uuid("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1")) IDirect3DDxgiInterfaceAccess : ::IUnknown {
    virtual HRESULT __stdcall GetInterface(GUID const &id, void **object) = 0;
};

template <typename T>
static winrt::com_ptr<T> GetDXGIInterfaceFromObject(winrt::Windows::Foundation::IInspectable const &object) {
    auto access = object.as<IDirect3DDxgiInterfaceAccess>();
    winrt::com_ptr<T> result;
    winrt::check_hresult(access->GetInterface(winrt::guid_of<T>(), result.put_void()));
    return result;
}

} // namespace op::capture

#endif
