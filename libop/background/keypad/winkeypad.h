#pragma once
#ifndef _WIN_KEYPAD_H_
#define _WIN_KEYPAD_H_
#include "Bkkeypad.h"
class winkeypad : public bkkeypad {
  public:
    winkeypad();

    virtual ~winkeypad();

    virtual long Bind(HWND hwnd, long mode);

    virtual long UnBind();

    virtual long GetKeyState(long vk_code);

    virtual long KeyDown(long vk_code);

    // virtual long GetKeyState(long vk_code);

    virtual long KeyUp(long vk_code);

    virtual long WaitKey(long vk_code, unsigned long time_out);

    virtual long KeyPress(long vk_code);

  private:
    bool _shift_down = false;
    bool _ctrl_down = false;
    bool _alt_down = false;
};
#endif // !_WIN_KEYPAD_H_
