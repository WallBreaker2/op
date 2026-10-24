#pragma once
#ifndef OP_CAPTURE_BACKENDS_HOOK_CAPTURE_H_
#define OP_CAPTURE_BACKENDS_HOOK_CAPTURE_H_

#include "../ICaptureBackend.h"
namespace op {
struct Image;
}

namespace op::capture {

class HookCapture : public ICaptureBackend {
  public:
    HookCapture();
    ~HookCapture();
    // 1
    long BindEx(HWND hwnd, long render_type) override;

    /*long UnBind(HWND hwnd);*/

    long UnBindEx() override;

    virtual bool requestCapture(int x1, int y1, int w, int h, Image &img) override;
    void waitForBindReady() override;

    // nox mode
    long BindNox(HWND hwnd, long render_type);
    //
    long UnBindNox();

  private:
    bool ensureSharedFrameCapacity(int width, int height);
    // 轮询等注入侧写出第一帧，true = 等到了。抽出来是因为 bind 后的 waitForBindReady
    // 和 requestCapture 的首帧重试都要用同一套判定。
    bool waitForFrameReady(DWORD timeout_ms);
    // 注入式后端的像素帧要写进命名共享段，容量必须按窗口面积预留。
    bool RequiresSharedPixelBuffer() const override {
        return true;
    }
    // blackbone::Process _process;
    std::wstring m_opPath;
};

} // namespace op::capture

#endif // OP_CAPTURE_BACKENDS_HOOK_CAPTURE_H_
