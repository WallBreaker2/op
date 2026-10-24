#pragma once
#ifndef OP_CAPTURE_BACKENDS_GDI_CAPTURE_H_
#define OP_CAPTURE_BACKENDS_GDI_CAPTURE_H_
#include "../../base/Types.h"
#include "../ICaptureBackend.h"
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
    // 窗口移动/缩放后客户区尺寸和边框偏移都会变，截图前刷新一次，避免按旧尺寸裁剪。
    void refreshMetrics() override;

  private:
    // 设备句柄
    HDC _hdc = NULL;
    int _device_caps = 0;
    HDC _hmdc = NULL;
    // 复用的 32 位 top-down DIB。
    // BitBlt/PrintWindow 直接把窗口内容写进这块内存，省掉每次截图重建位图、
    // GetDIBits 的格式转换以及逐行倒置拷贝。
    HBITMAP _dib = NULL;
    HBITMAP _dib_old = NULL;
    void *_dib_bits = nullptr;
    int _dib_width = 0;
    int _dib_height = 0;
    int dx_, dy_; // 去除标题栏
    FrameInfo m_frameInfo;
    void release_device_context();
    void release_memory_dc();
    // 保证 DIB 至少能装下 width x height，容量够就原样复用。
    bool ensure_dib(int width, int height);
    void release_dib();
    // src_rect 非空时用 BitBlt 截该区域，为空时用 PrintWindow 截整窗。
    bool render_window(HDC src_dc, int dst_width, int dst_height, const RECT *src_rect);
    // 从 DIB 取块拷进 Image，行距不一致时逐行拷。
    // 返回 false 表示请求块越出复用 DIB（调用方不能把它当成“已经填好像素”）。
    bool copy_from_dib(Image &img, int dst_x, int dst_y, int src_x, int src_y, int width, int height) const;
    void write_frame_info(int w, int h);
    void fmtFrameInfo(void *dst, HWND hwnd, int w, int h);
};

} // namespace op::capture

#endif // OP_CAPTURE_BACKENDS_GDI_CAPTURE_H_
