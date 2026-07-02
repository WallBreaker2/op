#pragma once

#include "KeyboardBackend.h"
#include <array>

namespace op::input {

class DxKeyboard : public KeyboardBackend {
  public:
    DxKeyboard();
    ~DxKeyboard() override;

    long Bind(HWND hwnd, long mode) override;
    long UnBind() override;
    long GetKeyState(long vk_code) override;
    long KeyDown(long vk_code) override;
    long KeyUp(long vk_code) override;
    long WaitKey(long vk_code, unsigned long time_out) override;
    long KeyPress(long vk_code) override;
    long InputChar(wchar_t ch) override;

  private:
    std::array<BYTE, 256> _keys{};
};

} // namespace op::input
