//#include "stdafx.h"
#include "opGDI.h"

#include <atlimage.h>

#include <fstream>

#include "../../winapi/WinApi.h"
#include "./include/Image.hpp"
#include "globalVar.h"
#include "helpfunc.h"
opGDI::opGDI() {
  _render_type = 0;
  dx_ = 0;
  dy_ = 0;
  // 4*2^22=16*2^20=16MB
  //_image_data = new byte[MAX_IMAGE_WIDTH*MAX_IMAGE_WIDTH * 4];
}

opGDI::~opGDI() {
  // SAFE_DELETE_ARRAY(_image_data);
}

long opGDI::BindEx(HWND hwnd, long render_type) {
  if (!::IsWindow(hwnd)) return 0;
  _hwnd = hwnd;
  _render_type = render_type;

  if (render_type == RDT_NORMAL) {
    RECT rc, rc2;
    ::GetWindowRect(_hwnd, &rc);
    ::GetClientRect(hwnd, &rc2);

    _width = rc2.right - rc2.left;
    _height = rc2.bottom - rc2.top;
    POINT pt = {0};
    ::ClientToScreen(hwnd, &pt);
    dx_ = pt.x - rc.left;
    dy_ = pt.y - rc.top;
    _hdc = ::GetDC(NULL);
  } else {  // client size
    RECT rc, rc2;
    ::GetWindowRect(_hwnd, &rc);
    ::GetClientRect(hwnd, &rc2);
    _width = rc2.right - rc2.left;
    _height = rc2.bottom - rc2.top;
    POINT pt = {0};
    ::ClientToScreen(hwnd, &pt);
    dx_ = pt.x - rc.left;
    dy_ = pt.y - rc.top;
    /*setlog("dx=%d dy=%d", dx_, dy_);*/
    if (_render_type == RDT_GDI) {
      _hdc = ::GetDC(_hwnd);
      HWND topHwnd = WinApi::GetTopWindowSp(_hwnd);
      long dwExStyle = 0;
      if (GetPropA(topHwnd, "opstyle_flag")) {
        dwExStyle = GetWindowLongA(topHwnd, GWL_EXSTYLE);
      } else {
        dwExStyle = GetWindowLongA(topHwnd, GWL_EXSTYLE);
        SetPropA(topHwnd, "opstyle", (HANDLE)dwExStyle);
        SetPropA(topHwnd, "opstyle_flag", (HANDLE)HANDLE_FLAG_INHERIT);
      }
      // dmIsWindowTransParent((int)InfoStruct, (int)topHwnd, (int)&v44,
      // (int)&v42, (int)&v45);
      SetWindowLongA(topHwnd, GWL_EXSTYLE, dwExStyle | WS_EX_LAYERED);
      // SetWindowTransparent((int)InfoStruct, (int)topHwnd, 16711935, 0, 1);
      UpdateWindow(topHwnd);

    } else if (RDT_GDI_DX2 == render_type) {
      _hdc = ::GetDC(_hwnd);
      _device_caps = GetDeviceCaps(_hdc, BITSPIXEL);
    } else {
      HWND dx2TopHwnd = WinApi::GetTopWindowSp(_hwnd);
      GetPropA(dx2TopHwnd, "opstyle");
      long dx2ExStyle = 0;
      if (GetPropA(dx2TopHwnd, "opstyle_flag")) {
        dx2ExStyle = GetWindowLongA(dx2TopHwnd, GWL_EXSTYLE);
      } else {
        dx2ExStyle = GetWindowLongA(dx2TopHwnd, GWL_EXSTYLE);
        SetPropA(dx2TopHwnd, "opstyle", (HANDLE)dx2ExStyle);
        SetPropA(dx2TopHwnd, "opstyle_flag", (HANDLE)HANDLE_FLAG_INHERIT);
      }
      // dmIsWindowTransParent((int)InfoStruct, (int)dx2TopHwnd, (int)&v45,
      // (int)&v42, (int)&v44);
      SetWindowLongA(dx2TopHwnd, GWL_EXSTYLE, dx2ExStyle | WS_EX_LAYERED);
      // SetWindowTransparent((int)InfoStruct, (int)dx2TopHwnd, 0, 255, 2);
      UpdateWindow(dx2TopHwnd);
      // InfoStruct[11] = 3;

      _hdc = ::GetDC(_hwnd);
    }
  }

  if (_hdc == NULL) {
    setlog("hdc == NULL", _hdc);
    return 0;
  }

  //创建一个与指定设备兼容的内存设备上下文环境
  _hmdc = CreateCompatibleDC(_hdc);
  if (_hmdc == NULL) {
    setlog("CreateCompatibleDC false");
    return -2;
  }

  // updata_screen();
  return 1;
}

long opGDI::UnBindEx() {
  // setlog("bkgdi::UnBindEx()");
  _hbmpscreen = (HBITMAP)SelectObject(_hmdc, _hbmp_old);
  // delete[dwLen_2]hDib;
  if (_hdc) DeleteDC(_hdc);
  _hdc = NULL;
  if (_hmdc) DeleteDC(_hmdc);
  _hmdc = NULL;

  if (_hbmpscreen) DeleteObject(_hbmpscreen);
  _hbmpscreen = NULL;
  // if (_hbmp_old)DeleteObject(_hbmp_old); _hbmp_old = NULL;
  return 1;
}

bool opGDI::requestCapture(int x1, int y1, int w, int h, Image& img) {
  // step 1.判断 窗口是否存在
  if (!::IsWindow(_hwnd)) return 0;
  img.create(w, h);
  if (_render_type == RDT_NORMAL) {  // normal 拷贝的大小为实际需要的大小
    //
    /*	int w = rect.right - rect.left;
            int h = rect.bottom - rect.top;*/
    _hbmpscreen = CreateCompatibleBitmap(
        _hdc, w, h);  //创建与指定的设备环境相关的设备兼容的位图
    _hbmp_old = (HBITMAP)SelectObject(
        _hmdc, _hbmpscreen);  //选择一对象到指定的设备上下文环境中

    _bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    _bfh.bfSize = _bfh.bfOffBits + w * h * 4;
    _bfh.bfType = static_cast<WORD>(0x4d42);

    _bih.biBitCount = 32;  //每个像素字节大小
    _bih.biCompression = BI_RGB;
    _bih.biHeight = h;  //高度
    _bih.biPlanes = 1;
    _bih.biSize = sizeof(BITMAPINFOHEADER);
    _bih.biSizeImage = w * h * 4;  //图像数据大小
    _bih.biWidth = w;              //宽度

    //对指定的源设备环境区域中的像素进行位块（bit_block）转换

    RECT rc;
    ::GetWindowRect(_hwnd, &rc);
    // setlog("rect left =%d top =%d, dx =%d, dy = %d", rc.left, rc.top, dx_,
    // dy_);
    int src_x = x1 + rc.left + dx_;
    int src_y = y1 + rc.top + dy_;
    if (BitBlt(_hmdc, 0, 0, w, h, _hdc, src_x, src_y, SRCCOPY)) {
      // ok
    } else {
      setlog("error in bitbit");
    }

    //函数获取指定兼容位图的位，然后将其作一个DIB—设备无关位图（Device-Independent
    // Bitmap）使用的指定格式复制到一个缓冲区中 _pmutex->lock();
    uchar* pshare = _shmem->data<byte>();
    fmtFrameInfo(pshare, _hwnd, w, h);
    GetDIBits(_hmdc, _hbmpscreen, 0L, (DWORD)h, pshare + sizeof(FrameInfo),
              (LPBITMAPINFO)&_bih, (DWORD)DIB_RGB_COLORS);

    //_pmutex->unlock();
    if (_hbmpscreen) DeleteObject(_hbmpscreen);
    _hbmpscreen = NULL;

    //将数据拷贝到目标注意实际数据是反的

    for (int i = 0; i < h; i++) {
      memcpy(img.ptr<uchar>(i), _shmem->data<byte>() + (h - 1 - i) * 4 * w,
             4 * w);
    }
  } else if (RDT_GDI_DX2 == _render_type) {
    ATL::CImage image;
    image.Create(w, h, _device_caps);
    BitBlt(image.GetDC(), x1, y1, w, h, _hdc, 0, 0, SRCCOPY);
    img.read(&image);
    image.ReleaseDC();
  } else {  // gdi ... 由于printwindow 函数的原因
            // 截取大小为实际的窗口大小，在后续的处理中，需要转化成客户区大小
    //
    RECT rc;
    ::GetWindowRect(_hwnd, &rc);
    int ww = rc.right - rc.left;
    int wh = rc.bottom - rc.top;
    // setlog("_w w=%d %d _h h=%d %d,dx=%d dy=%d", _width, w, _height, h, dx_,
    // dy_);
    _hbmpscreen = CreateCompatibleBitmap(
        _hdc, ww, wh);  //创建与指定的设备环境相关的设备兼容的位图
    _hbmp_old = (HBITMAP)SelectObject(
        _hmdc, _hbmpscreen);  //选择一对象到指定的设备上下文环境中

    _bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    _bfh.bfSize = _bfh.bfOffBits + ww * wh * 4;
    _bfh.bfType = static_cast<WORD>(0x4d42);

    _bih.biBitCount = 32;  //每个像素字节大小
    _bih.biCompression = BI_RGB;
    _bih.biHeight = wh;  //高度
    _bih.biPlanes = 1;
    _bih.biSize = sizeof(BITMAPINFOHEADER);
    _bih.biSizeImage = ww * wh * 4;  //图像数据大小
    _bih.biWidth = ww;               //宽度

    //对指定的源设备环境区域中的像素进行位块（bit_block）转换

    if (_render_type == RDT_GDI) {
      ::PrintWindow(_hwnd, _hmdc, 0);

    } else {
      ::UpdateWindow(_hwnd);
      //::RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE |
      //:RDW_ALLCHILDREN | RDW_FRAME);
      ::PrintWindow(_hwnd, _hmdc, 0);
    }
    //函数获取指定兼容位图的位，然后将其作一个DIB—设备无关位图（Device-Independent
    // Bitmap）使用的指定格式复制到一个缓冲区中 _pmutex->lock();
    uchar* pshare = _shmem->data<byte>();
    fmtFrameInfo(pshare, _hwnd, w, h);
    GetDIBits(_hmdc, _hbmpscreen, 0L, (DWORD)wh, pshare + sizeof(FrameInfo),
              (LPBITMAPINFO)&_bih, (DWORD)DIB_RGB_COLORS);

    if (_hbmpscreen) DeleteObject(_hbmpscreen);
    _hbmpscreen = NULL;

    //将数据拷贝到目标注意实际数据是反的(注意偏移)
    auto ppixels = _shmem->data<byte>() + sizeof(FrameInfo);
    for (int i = 0; i < h; i++) {
      memcpy(img.ptr<uchar>(i),
             ppixels + (wh - 1 - i - y1 - dy_) * 4 * ww + (x1 + dx_) * 4,
             4 * w);
    }
  }
  return 1;
}
void opGDI::fmtFrameInfo(void* dst, HWND hwnd, int w, int h) {
  m_frameInfo.hwnd = (unsigned __int64)hwnd;
  m_frameInfo.frameId++;
  m_frameInfo.time = ::GetTickCount();
  m_frameInfo.width = w;
  m_frameInfo.height = h;
  m_frameInfo.fmtChk();
  memcpy(dst, &m_frameInfo, sizeof(m_frameInfo));
}
