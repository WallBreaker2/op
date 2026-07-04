#pragma once

#include "../InputMessageUtils.h"
#include "../../base/Types.h"

namespace op::input::key_message {

inline bool IsExtendedVirtualKey(UINT vk_code) {
    switch (vk_code) {
    case VK_RMENU:
    case VK_RCONTROL:
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:
    case VK_NUMLOCK:
    case VK_SNAPSHOT:
    case VK_DIVIDE:
    case VK_APPS:
    case VK_LWIN:
    case VK_RWIN:
        return true;
    default:
        return false;
    }
}

inline WORD ScanCode(UINT vk_code) {
    return static_cast<WORD>((::MapVirtualKey(vk_code, MAPVK_VK_TO_VSC)) & 0xff);
}

// Windows 键盘消息的 lParam 需要同时带扫描码、扩展键和按下/抬起状态。
inline LPARAM BuildKeyLParam(UINT vk_code, bool key_up, bool alt_context = false) {
    DWORD data = 1;
    data |= static_cast<DWORD>(ScanCode(vk_code)) << 16;
    if (IsExtendedVirtualKey(vk_code))
        data |= 1u << 24;
    if (alt_context)
        data |= 1u << 29;
    if (key_up)
        data |= 3u << 30;
    return static_cast<LPARAM>(data);
}

// DirectInput 的扫描码和普通窗口消息略有差异，少数特殊键统一在这里处理。
inline BYTE DirectInputScanCode(UINT vk_code) {
    switch (vk_code) {
    case VK_NUMLOCK:
        return 0x45;
    case VK_PAUSE:
        return 0xC5;
    case VK_SNAPSHOT:
        return 0xB7;
    default:
        break;
    }

    BYTE scan = static_cast<BYTE>(::MapVirtualKey(vk_code, MAPVK_VK_TO_VSC) & 0xff);
    if (scan == 0)
        return 0;
    return IsExtendedVirtualKey(vk_code) ? static_cast<BYTE>(scan | 0x80) : scan;
}

} // namespace op::input::key_message
