#include "DxKeyboard.h"
#include "../../hook/HookProtocol.h"
#include "../../hook/InputHookClient.h"
#include "../../runtime/AutomationModes.h"
#include "../../runtime/RuntimeUtils.h"

namespace input_hook_client = op::hook::input_hook_client;

namespace {

long send_input_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    return ::SendMessageTimeout(hwnd, message, wparam, lparam, SMTO_BLOCK, 2000, nullptr) ? 1L : 0L;
}

bool is_extended_vk(long vk_code) {
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

LPARAM key_lparam(long vk_code, bool key_up) {
    DWORD value = 1;
    const WORD scan_code = static_cast<WORD>(::MapVirtualKey(vk_code, MAPVK_VK_TO_VSC));
    value |= static_cast<DWORD>(scan_code) << 16;
    if (is_extended_vk(vk_code))
        value |= 1u << 24;
    if (key_up)
        value |= 3u << 30;
    return static_cast<LPARAM>(value);
}

bool valid_vk(long vk_code) {
    return 0 <= vk_code && vk_code < 256;
}

} // namespace

namespace op::input {

DxKeyboard::DxKeyboard() = default;

DxKeyboard::~DxKeyboard() {
    UnBind();
}

long DxKeyboard::Bind(HWND hwnd, long mode) {
    if (_hwnd == hwnd && _mode == mode)
        return 1;

    UnBind();
    if (input_hook_client::Bind(hwnd, static_cast<int>(mode)) != 1)
        return 0;
    _hwnd = hwnd;
    _mode = static_cast<int>(mode);
    _keys.fill(0);
    return 1;
}

long DxKeyboard::UnBind() {
    input_hook_client::UnBind(_hwnd);
    _keys.fill(0);
    return KeyboardBackend::UnBind();
}

long DxKeyboard::GetKeyState(long vk_code) {
    return valid_vk(vk_code) && _keys[static_cast<size_t>(vk_code)] ? 1 : 0;
}

long DxKeyboard::KeyDown(long vk_code) {
    if (!valid_vk(vk_code))
        return 0;

    const long ret = send_input_message(_hwnd, OP_WM_KEYDOWN, static_cast<WPARAM>(vk_code), key_lparam(vk_code, false));
    if (ret == 1)
        _keys[static_cast<size_t>(vk_code)] = 0x80;
    return ret;
}

long DxKeyboard::KeyUp(long vk_code) {
    if (!valid_vk(vk_code))
        return 0;

    const long ret = send_input_message(_hwnd, OP_WM_KEYUP, static_cast<WPARAM>(vk_code), key_lparam(vk_code, true));
    if (ret == 1)
        _keys[static_cast<size_t>(vk_code)] = 0;
    return ret;
}

long DxKeyboard::WaitKey(long vk_code, unsigned long time_out) {
    const auto deadline = ::GetTickCount64() + time_out;
    do {
        if (vk_code == 0) {
            for (long key = 1; key < 256; ++key) {
                if (GetKeyState(key))
                    return key;
            }
        } else if (GetKeyState(vk_code)) {
            return vk_code;
        }

        if (time_out == 0)
            return 0;
        ::Sleep(1);
    } while (::GetTickCount64() < deadline);
    return 0;
}

long DxKeyboard::KeyPress(long vk_code) {
    if (KeyDown(vk_code) != 1)
        return 0;
    ::Delay(KEYPAD_DX_DELAY);
    return KeyUp(vk_code);
}

} // namespace op::input
