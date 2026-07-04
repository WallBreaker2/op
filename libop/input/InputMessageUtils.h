#pragma once

#include "../runtime/Types.h"

namespace op::input::message {

// 键鼠的窗口消息都用同一套超时发送，避免目标窗口卡住时脚本一起挂住。
inline long SendTimeout(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    return ::SendMessageTimeout(hwnd, message, wparam, lparam, SMTO_BLOCK, 2000, nullptr) ? 1L : 0L;
}

} // namespace op::input::message
