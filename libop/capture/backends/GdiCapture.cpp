// #include "stdafx.h"
#include "GdiCapture.h"

#include <atlimage.h>

#include <cstddef>
#include <cstring>
#include <fstream>

#include "../../base/AutomationModes.h"
#include "../../base/Utils.h"
#include "../../image/Image.h"
#include "../../window/WindowService.h"

namespace {

// 顶对齐 DIB 的行距恒等于 width*4（32bpp 已经 4 字节对齐），
// 所以整行拷贝时可以退化成一次 memcpy。
inline size_t dib_pitch(int width) {
    return static_cast<size_t>(width) * 4;
}

} // namespace

namespace op::capture {

GdiCapture::GdiCapture() {
    _render_type = 0;
    dx_ = 0;
    dy_ = 0;
}

GdiCapture::~GdiCapture() {
    // ICaptureBackend 的析构只清共享内存，DC/DIB 必须自己放，否则异常路径会漏 GDI 句柄。
    release_dib();
    release_memory_dc();
    release_device_context();
}

long GdiCapture::BindEx(HWND hwnd, long render_type) {
    if (!::IsWindow(hwnd))
        return 0;
    _hwnd = hwnd;
    _render_type = render_type;

    RECT rc = {0}, rc2 = {0};
    ::GetWindowRect(hwnd, &rc);
    ::GetClientRect(hwnd, &rc2);

    _width = rc2.right - rc2.left;
    _height = rc2.bottom - rc2.top;
    POINT pt = {0};
    ::ClientToScreen(hwnd, &pt);
    dx_ = pt.x - rc.left;
    dy_ = pt.y - rc.top;

    if (render_type == RDT_NORMAL) {
        // 桌面截图：源 DC 取自屏幕，坐标是屏幕坐标。
        _hdc = ::GetDC(NULL);
    } else {
        _hdc = ::GetDC(hwnd);
        if (render_type == RDT_GDI_DX2) {
            _device_caps = GetDeviceCaps(_hdc, BITSPIXEL);
        }
    }

    if (_hdc == NULL) {
        setlog("hdc == NULL");
        return 0;
    }

    // 创建一个与指定设备兼容的内存设备上下文环境
    _hmdc = CreateCompatibleDC(_hdc);
    if (_hmdc == NULL) {
        setlog("CreateCompatibleDC false");
        release_device_context();
        return -2;
    }

    return 1;
}

long GdiCapture::UnBindEx() {
    release_dib();
    // GetDC 得到的窗口/桌面 DC 必须用 ReleaseDC 归还。
    release_device_context();
    release_memory_dc();
    return 1;
}

void GdiCapture::refreshMetrics() {
    if (!::IsWindow(_hwnd))
        return;

    RECT rc = {0}, rc2 = {0};
    if (::GetClientRect(_hwnd, &rc2)) {
        _width = rc2.right - rc2.left;
        _height = rc2.bottom - rc2.top;
    }
    // 边框厚度一般不变，但窗口换皮肤/DPI 变化后 dx_/dy_ 会变，跟着刷新一次。
    if (::GetWindowRect(_hwnd, &rc)) {
        POINT pt = {0};
        if (::ClientToScreen(_hwnd, &pt)) {
            dx_ = pt.x - rc.left;
            dy_ = pt.y - rc.top;
        }
    }
}

void GdiCapture::release_device_context() {
    if (!_hdc)
        return;

    ::ReleaseDC(_render_type == RDT_NORMAL ? NULL : _hwnd, _hdc);
    _hdc = NULL;
}

void GdiCapture::release_memory_dc() {
    if (!_hmdc)
        return;

    ::DeleteDC(_hmdc);
    _hmdc = NULL;
}

void GdiCapture::release_dib() {
    if (_hmdc && _dib && _dib_old) {
        ::SelectObject(_hmdc, _dib_old);
    }
    _dib_old = NULL;
    if (_dib) {
        ::DeleteObject(_dib);
        _dib = NULL;
    }
    _dib_bits = nullptr;
    _dib_width = 0;
    _dib_height = 0;
}

bool GdiCapture::ensure_dib(int width, int height) {
    if (width <= 0 || height <= 0)
        return false;

    // 容量够就复用：脚本反复截小块（GetColor 的 10x10 之类）时不会反复建位图。
    if (_dib && _dib_width >= width && _dib_height >= height)
        return true;

    release_dib();

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = width;
    // 负高度 = top-down：像素行序与 Image 一致，拷进 Image 时不需要逐行倒置。
    bi.bmiHeader.biHeight = -height;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void *bits = nullptr;
    HBITMAP dib = ::CreateDIBSection(_hdc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dib || !bits) {
        if (dib)
            ::DeleteObject(dib);
        setlog("CreateDIBSection false w=%d h=%d", width, height);
        return false;
    }

    _dib = dib;
    _dib_bits = bits;
    _dib_width = width;
    _dib_height = height;
    _dib_old = static_cast<HBITMAP>(::SelectObject(_hmdc, _dib));
    if (!_dib_old || reinterpret_cast<HGDIOBJ>(_dib_old) == HGDI_ERROR) {
        setlog("SelectObject false");
        // 选不进 DC 说明位图没有被选中：直接删掉即可。
        // 不能走 release_dib()——它靠 _dib_old 判断是否需要解选，这里 _dib_old 为空，
        // 会跳过解选却去 DeleteObject 一个仍被选中的位图（删除失败并泄漏句柄）。
        _dib_old = NULL;
        ::DeleteObject(_dib);
        _dib = NULL;
        _dib_bits = nullptr;
        _dib_width = 0;
        _dib_height = 0;
        return false;
    }
    return true;
}

bool GdiCapture::render_window(HDC src_dc, int dst_width, int dst_height, const RECT *src_rect) {
    if (!ensure_dib(dst_width, dst_height))
        return false;

    if (src_rect) {
        // 源矩形越出桌面边界时 BitBlt 会裁掉越界部分。这时先把复用缓冲清黑，
        // 否则越界区域留着上一帧的像素（旧实现每次新建位图，越界部分天然是黑的）。
        const int screen_x = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
        const int screen_y = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
        const int screen_right = screen_x + ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
        const int screen_bottom = screen_y + ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
        if (src_rect->left < screen_x || src_rect->top < screen_y || src_rect->right > screen_right ||
            src_rect->bottom > screen_bottom) {
            memset(_dib_bits, 0, dib_pitch(_dib_width) * static_cast<size_t>(dst_height));
        }
        // 直接 BitBlt 进 DIB 的内存，目标数据就是最终像素，无需 GetDIBits 再转一次。
        if (!::BitBlt(_hmdc, 0, 0, dst_width, dst_height, src_dc, src_rect->left, src_rect->top, SRCCOPY)) {
            setlog("error in bitbit");
            return false;
        }
        return true;
    }

    if (_render_type != RDT_GDI) {
        ::UpdateWindow(_hwnd);
    }
    if (!::PrintWindow(_hwnd, _hmdc, 0)) {
        return false;
    }
    return true;
}

bool GdiCapture::copy_from_dib(Image &img, int dst_x, int dst_y, int src_x, int src_y, int width, int height) const {
    if (!_dib_bits || width <= 0 || height <= 0)
        return false;
    if (src_x < 0 || src_y < 0 || src_x + width > _dib_width || src_y + height > _dib_height)
        return false;

    const auto *base = static_cast<const unsigned char *>(_dib_bits);
    const size_t pitch = dib_pitch(_dib_width);
    const size_t row_bytes = static_cast<size_t>(width) * 4;
    const size_t src_offset = static_cast<size_t>(src_y) * pitch + static_cast<size_t>(src_x) * 4;
    unsigned char *dst = img.ptr<unsigned char>(dst_y) + static_cast<size_t>(dst_x) * 4;

    if (dst_x == 0 && width == img.width && pitch == row_bytes) {
        // 整块行距一致，一次大 memcpy（全屏截图走的就是这条）。
        memcpy(dst, base + src_offset, row_bytes * static_cast<size_t>(height));
        return true;
    }

    const size_t dst_pitch = dib_pitch(img.width);
    for (int i = 0; i < height; ++i) {
        memcpy(dst + static_cast<size_t>(i) * dst_pitch, base + src_offset + static_cast<size_t>(i) * pitch, row_bytes);
    }
    return true;
}

void GdiCapture::write_frame_info(int w, int h) {
    if (!_shmem)
        return;
    // 与 DXGI/WGC 对齐：帧头是跨进程可读的，写侧统一持进程互斥体，
    // 否则读者可能读到撕裂的 frameId/time/size 组合。
    if (_pmutex) {
        _pmutex->lock();
        fmtFrameInfo(_shmem->data<byte>(), _hwnd, w, h);
        _pmutex->unlock();
        return;
    }
    fmtFrameInfo(_shmem->data<byte>(), _hwnd, w, h);
}

bool GdiCapture::requestCapture(int x1, int y1, int w, int h, Image &img) {
    // step 1.判断 窗口是否存在
    if (!::IsWindow(_hwnd))
        return 0;
    if (!_hdc || !_hmdc) {
        setlog("GdiCapture::requestCapture resources not initialized");
        return false;
    }
    if (w <= 0 || h <= 0)
        return false;

    if (RDT_GDI_DX2 == _render_type) {
        ATL::CImage image;
        image.Create(w, h, _device_caps);
        ::BitBlt(image.GetDC(), 0, 0, w, h, _hdc, x1, y1, SRCCOPY);
        const bool read_ok = img.read(&image);
        image.ReleaseDC();
        if (!read_ok) {
            // 16bpp 等非常见格式会让 read() 拒绝转换；这里必须报失败，
            // 不能返回 true 让调用方去读未初始化的 Image。
            setlog("GdiCapture::requestCapture img.read failed bpp=%d w=%d h=%d", _device_caps, w, h);
            return false;
        }
        return true;
    }

    img.create(w, h);

    if (_render_type == RDT_NORMAL) {
        // normal 只截请求区域，坐标 = 区域偏移 + 窗口原点 + 客户区偏移。
        RECT rc = {0};
        if (!::GetWindowRect(_hwnd, &rc))
            return false;
        const int src_x = x1 + rc.left + dx_;
        const int src_y = y1 + rc.top + dy_;
        const RECT src_rect = {src_x, src_y, src_x + w, src_y + h};
        if (!render_window(_hdc, w, h, &src_rect))
            return false;

        // 复用 DIB 时 copy_from_dib 可能因为越界直接放弃拷贝，此时必须报失败，
        // 不能把上一帧（或另一个区域的）像素当成结果返回。
        if (!copy_from_dib(img, 0, 0, 0, 0, w, h)) {
            setlog("GdiCapture::requestCapture copy_from_dib failed w=%d h=%d dib=%dx%d", w, h, _dib_width,
                   _dib_height);
            return false;
        }
        write_frame_info(w, h);
        return true;
    }

    // PrintWindow 只能截整窗，之后再按客户区偏移取出请求区域。
    RECT rc = {0};
    if (!::GetWindowRect(_hwnd, &rc))
        return false;
    const int window_width = rc.right - rc.left;
    const int window_height = rc.bottom - rc.top;
    if (window_width <= 0 || window_height <= 0)
        return false;

    if (!render_window(_hdc, window_width, window_height, nullptr))
        return false;

    write_frame_info(w, h);

    int src_x = x1 + dx_;
    int src_y = y1 + dy_;
    int dst_x = 0;
    int dst_y = 0;
    int copy_width = w;
    int copy_height = h;
    // 客户区可能比窗口大/越界（最大化、DPI 缩放），越界部分只能留黑。
    if (src_x < 0) {
        dst_x = -src_x;
        copy_width += src_x;
        src_x = 0;
    }
    if (src_y < 0) {
        dst_y = -src_y;
        copy_height += src_y;
        src_y = 0;
    }
    if (src_x + copy_width > window_width)
        copy_width = window_width - src_x;
    if (src_y + copy_height > window_height)
        copy_height = window_height - src_y;
    if (copy_width <= 0 || copy_height <= 0) {
        setlog("GdiCapture::requestCapture client area outside window w=%d h=%d", window_width, window_height);
        return false;
    }

    if (dst_x != 0 || dst_y != 0 || copy_width != w || copy_height != h) {
        img.fill(0xff000000);
    }
    return copy_from_dib(img, dst_x, dst_y, src_x, src_y, copy_width, copy_height);
}

void GdiCapture::fmtFrameInfo(void *dst, HWND hwnd, int w, int h) {
    WriteFrameInfo(m_frameInfo, dst, hwnd, w, h, true);
}

} // namespace op::capture
