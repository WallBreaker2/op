#pragma once
#include "../../runtime/Types.h"

namespace op::input {

class KeyboardBackend {
  public:
    KeyboardBackend();

    virtual ~KeyboardBackend();

    virtual long Bind(HWND hwnd, long mode) = 0;

    virtual long UnBind();

    virtual long GetKeyState(long vk_code) = 0;

    virtual long KeyDown(long vk_code) = 0;

    // virtual long GetKeyState(long vk_code);

    virtual long KeyUp(long vk_code) = 0;

    virtual long WaitKey(long vk_code, unsigned long time_out) = 0;

    virtual long KeyPress(long vk_code) = 0;

  protected:
    HWND _hwnd;
    int _mode;
};

} // namespace op::input
