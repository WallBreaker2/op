#ifndef OP_CAPTURE_ICAPTURE_BACKEND_H_
#define OP_CAPTURE_ICAPTURE_BACKEND_H_
#include "../ipc/ProcessMutex.h"
#include "../ipc/SharedMemory.h"
#include "FrameInfo.h"
#include <exception>
#include <memory>
#include <string>

namespace op {
struct Image;
}

namespace op::capture {

class ICaptureBackend : public std::enable_shared_from_this<ICaptureBackend> {
  public:
    ICaptureBackend();
    virtual ~ICaptureBackend();
    // bind window
    long Bind(HWND hwnd, long flag);
    // unbind window
    long UnBind();
    // unbind window
    // virtual long UnBind(HWND hwnd) = 0;
    virtual bool requestCapture(int x1, int y1, int w, int h, Image &img) = 0;
    // 截图前刷新动态尺寸，默认后端尺寸稳定，不需要处理。
    virtual void refreshMetrics() {
    }
    // 绑定后等待后端就绪，默认后端同步完成，不需要处理。
    virtual void waitForBindReady() {
    }

    ProcessMutex *get_mutex() {
        return _pmutex;
    }

    long get_height() {
        return _height;
    }

    long get_width() {
        return _width;
    }

    int get_client_x() const {
        return _client_x;
    }

    int get_client_y() const {
        return _client_y;
    }

    void getFrameInfo(FrameInfo &info);

  private:
    long bind_init();

    long bind_release();

    long _bind_state;

  protected:
    virtual long BindEx(HWND hwnd, long flag) = 0;
    virtual long UnBindEx() = 0;
    virtual bool DeferBindReleaseAfterUnBind() const {
        return false;
    }

    HWND _hwnd;

    SharedMemory *_shmem;

    ProcessMutex *_pmutex;

    std::wstring _shared_res_name;

    std::wstring _mutex_name;

    //
    int _render_type;

    long _width;
    long _height;

    int _client_x, _client_y;
    // need capture rect
    // RECT rect;
};

} // namespace op::capture

#endif // OP_CAPTURE_ICAPTURE_BACKEND_H_
