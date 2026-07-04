#pragma once
#ifndef OP_CAPTURE_BACKENDS_GDI_CAPTURE_H_
#define OP_CAPTURE_BACKENDS_GDI_CAPTURE_H_
#include "../ICaptureBackend.h"
#include "../../base/Types.h"
#include <thread>
namespace op {
struct Image;
}

namespace op::capture {

class GdiCapture : public ICaptureBackend {
  public:
    GdiCapture();
    ~GdiCapture();
    // 绑定
    long BindEx(HWND _hwnd, long render_type) override;
    // 解绑
    long UnBindEx() override;

    virtual bool requestCapture(int x1, int y1, int w, int h, Image &img) override;

  private:
    // 设备句柄
    HDC _hdc = NULL;
    int _device_caps = 0;
    HDC _hmdc = NULL;
    // 位图句柄
    HBITMAP _hbmpscreen = NULL;
    HBITMAP _hbmp_old = NULL;
    // bmp 文件头
    BITMAPFILEHEADER _bfh = {0};
    BITMAPINFOHEADER _bih = {0}; // 位图信息头
    int dx_, dy_;                // 去除标题栏
    // bytearray temp_src;
    FrameInfo m_frameInfo;
    void release_device_context();
    void fmtFrameInfo(void *dst, HWND hwnd, int w, int h);
};

} // namespace op::capture

#endif // OP_CAPTURE_BACKENDS_GDI_CAPTURE_H_
