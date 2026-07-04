#pragma once
#ifndef OP_BINDING_BINDING_SESSION_H_
#define OP_BINDING_BINDING_SESSION_H_
#include "../image/Image.h"
#include <memory>
#include <string>
#include <utility>

#include "../capture/ICaptureBackend.h"

#include "../input/keyboard/KeyboardBackend.h"
#include "../input/mouse/WinMouse.h"

namespace op::binding {

class BindingSession {
  public:
    BindingSession();
    ~BindingSession();

  public:
    // 扩展绑定接口。显示截图绑定到 display_hwnd，鼠标和键盘输入绑定到 input_hwnd。
    virtual long BindWindowEx(LONG_PTR display_hwnd, LONG_PTR input_hwnd, const wstring &sdisplay, const wstring &smouse,
                              const wstring &skeypad, long mode);
    // 兼容旧接口的单句柄绑定，内部等价于显示和输入都绑定到同一个 hwnd。
    virtual long BindWindow(LONG_PTR hwnd, const wstring &sdisplay, const wstring &smouse, const wstring &skeypad,
                            long mode);
    virtual long UnBindWindow();
    virtual long LockInput(long lock);
    // 返回当前绑定的显示窗口句柄。使用 BindWindowEx 时，输入句柄可能不同于这个句柄。
    virtual LONG_PTR GetBindWindow();
    virtual long IsBind();
    // virtual long GetCursorPos(int& x, int& y);

    long GetDisplay();
    /*byte* GetScreenData();*/
    void lock_data();
    void unlock_data();
    long get_height();
    long get_width();
    long RectConvert(long &x1, long &y1, long &x2, long &y2);
    // 0:normal;-1 reserve 1 need cut
    long get_image_type();

    bool check_bind();
    const std::pair<wstring, wstring> &get_display_method() const;
    long set_display_method(const wstring &method);

    bool requestCapture(int x1, int y1, int w, int h, Image &img);

  private:
    HWND _display_hwnd;
    HWND _input_hwnd;
    int _is_bind;
    int _display;
    int _mode;
    int _mouse_mode;
    int _keypad_mode;
    std::pair<wstring, wstring> _display_method;
    Image _pic;

    long reset_bind_state(bool restore_default_input);
    std::shared_ptr<op::capture::ICaptureBackend> createDisplay(int mode);
    std::unique_ptr<op::input::WinMouse> createMouse(int mode);
    std::unique_ptr<op::input::KeyboardBackend> createKeypad(int mode);

  public:
    std::shared_ptr<op::capture::ICaptureBackend> _capture;
    std::unique_ptr<op::input::WinMouse> _mouse;
    std::unique_ptr<op::input::KeyboardBackend> _keyboard;
    wstring _curr_path;
};

} // namespace op::binding

#endif // OP_BINDING_BINDING_SESSION_H_
