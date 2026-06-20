#pragma once
#ifndef OP_INPUT_KEYBOARD_WIN_KEYBOARD_H_
#define OP_INPUT_KEYBOARD_WIN_KEYBOARD_H_
#include "KeyboardBackend.h"

namespace op::input {

class WinKeyboard : public KeyboardBackend {
  public:
    WinKeyboard();

    virtual ~WinKeyboard();

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

} // namespace op::input

#endif // OP_INPUT_KEYBOARD_WIN_KEYBOARD_H_
