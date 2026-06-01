#pragma once
#ifndef __BACKBASE_H_
#define __BACKBASE_H_
#include "include/Image.hpp"
#include <string>

#include "./display/IDisplay.h"

#include "./keypad/Bkkeypad.h"
#include "./mouse/opMouseWin.h"

using std::wstring;

class opBackground {
  public:
    opBackground();
    ~opBackground();

  public:
    // 扩展绑定接口。显示截图绑定到 display_hwnd，鼠标和键盘输入绑定到 input_hwnd。
    virtual long BindWindowEx(LONG_PTR display_hwnd, LONG_PTR input_hwnd, const wstring &sdisplay, const wstring &smouse,
                              const wstring &skeypad, long mode);
    // 兼容旧接口的单句柄绑定，内部等价于显示和输入都绑定到同一个 hwnd。
    virtual long BindWindow(LONG_PTR hwnd, const wstring &sdisplay, const wstring &smouse, const wstring &skeypad,
                            long mode);
    virtual long UnBindWindow();
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
    std::pair<wstring, wstring> _display_method;
    Image _pic;

    long reset_bind_state(bool restore_default_input);
    IDisplay *createDisplay(int mode);
    opMouseWin *createMouse(int mode);
    bkkeypad *createKeypad(int mode);

  public:
    IDisplay *_pbkdisplay;
    opMouseWin *_bkmouse;
    bkkeypad *_keypad;
    wstring _curr_path;
};
#endif
