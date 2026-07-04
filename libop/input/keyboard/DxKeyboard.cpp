#include "DxKeyboard.h"
#include "KeyMessageUtils.h"
#include "../../hook/HookProtocol.h"
#include "../../hook/InputHookClient.h"
#include "../../runtime/AutomationModes.h"
#include "../../runtime/RuntimeUtils.h"

namespace input_hook_client = op::hook::input_hook_client;

namespace {

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

    const long ret = message::SendTimeout(_hwnd, OP_WM_KEYDOWN, static_cast<WPARAM>(vk_code),
                                          key_message::BuildKeyLParam(static_cast<UINT>(vk_code), false));
    if (ret == 1)
        _keys[static_cast<size_t>(vk_code)] = 0x80;
    return ret;
}

long DxKeyboard::KeyUp(long vk_code) {
    if (!valid_vk(vk_code))
        return 0;

    const long ret = message::SendTimeout(_hwnd, OP_WM_KEYUP, static_cast<WPARAM>(vk_code),
                                          key_message::BuildKeyLParam(static_cast<UINT>(vk_code), true));
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

long DxKeyboard::InputChar(wchar_t ch) {
    return message::SendTimeout(_hwnd, OP_WM_CHAR, static_cast<WPARAM>(ch), 1);
}

} // namespace op::input
