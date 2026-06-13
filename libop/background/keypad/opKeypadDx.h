#pragma once

#include "Bkkeypad.h"
#include <array>

class opKeypadDx : public bkkeypad {
  public:
    opKeypadDx();
    ~opKeypadDx() override;

    long Bind(HWND hwnd, long mode) override;
    long UnBind() override;
    long GetKeyState(long vk_code) override;
    long KeyDown(long vk_code) override;
    long KeyUp(long vk_code) override;
    long WaitKey(long vk_code, unsigned long time_out) override;
    long KeyPress(long vk_code) override;

  private:
    std::array<BYTE, 256> _keys{};
};
