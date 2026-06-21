#include "ClientContext.h"
#include "ClientResult.h"

#include "runtime/AutomationModes.h"
#include "runtime/RuntimeUtils.h"

#include <libop.h>

#include <Windows.h>
#include <cwchar>
#include <map>
#include <string>
#include <vector>

namespace {

using op::input::KeyboardBackend;

struct key_combo_t {
    long vk = 0;
    std::vector<long> modifiers;
};

bool is_named_vk(const std::map<std::wstring, long> &vkmap, const wchar_t *text, long &vk) {
    if (text == nullptr || text[0] == L'\0')
        return false;

    std::wstring key = text;
    wstring2lower(key);
    auto it = vkmap.find(key);
    if (it == vkmap.end())
        return false;

    vk = it->second;
    return true;
}

bool resolve_char_key_combo(const wchar_t ch, key_combo_t &combo) {
    const SHORT mapped = ::VkKeyScanW(ch);
    if (mapped == -1)
        return false;

    combo = {};
    combo.vk = LOBYTE(mapped);

    const BYTE shift_state = HIBYTE(mapped);
    if (shift_state & 1)
        combo.modifiers.push_back(VK_SHIFT);
    if (shift_state & 2)
        combo.modifiers.push_back(VK_CONTROL);
    if (shift_state & 4)
        combo.modifiers.push_back(VK_MENU);

    return combo.vk != 0;
}

bool resolve_text_key_combo(const std::map<std::wstring, long> &vkmap, const wchar_t *text, key_combo_t &combo) {
    long named_vk = 0;
    if (is_named_vk(vkmap, text, named_vk)) {
        combo = {};
        combo.vk = named_vk;
        return true;
    }

    if (text == nullptr || text[0] == L'\0' || text[1] != L'\0')
        return false;

    return resolve_char_key_combo(text[0], combo);
}

long key_combo_down(KeyboardBackend *keypad, const key_combo_t &combo) {
    for (long modifier : combo.modifiers) {
        if (keypad->KeyDown(modifier) != 1)
            return 0;
    }
    return keypad->KeyDown(combo.vk);
}

long key_combo_up(KeyboardBackend *keypad, const key_combo_t &combo) {
    long ret = keypad->KeyUp(combo.vk);
    if (ret != 1)
        return ret;

    for (auto it = combo.modifiers.rbegin(); it != combo.modifiers.rend(); ++it) {
        if (keypad->KeyUp(*it) != 1)
            return 0;
    }
    return 1;
}

long key_combo_press(KeyboardBackend *keypad, const key_combo_t &combo) {
    if (combo.modifiers.empty())
        return keypad->KeyPress(combo.vk);

    if (key_combo_down(keypad, combo) != 1)
        return 0;
    return key_combo_up(keypad, combo);
}

} // namespace

void op::Client::BindWindow(LONG_PTR hwnd, const wchar_t *display, const wchar_t *mouse, const wchar_t *keypad,
                            long mode, long *ret) {
    BindWindowEx(hwnd, hwnd, display, mouse, keypad, mode, ret);
}

void op::Client::BindWindowEx(LONG_PTR display_hwnd, LONG_PTR input_hwnd, const wchar_t *display,
                              const wchar_t *mouse, const wchar_t *keypad, long mode, long *ret) {
    if (m_context->bkproc.IsBind())
        m_context->bkproc.UnBindWindow();
    internal::set_result(ret, m_context->bkproc.BindWindowEx(display_hwnd, input_hwnd, display, mouse, keypad, mode));
}

void op::Client::UnBindWindow(long *ret) {
    internal::set_result(ret, m_context->bkproc.UnBindWindow());
}

void op::Client::GetBindWindow(LONG_PTR *ret) {
    internal::set_result(ret, m_context->bkproc.GetBindWindow());
}

void op::Client::IsBind(long *ret) {
    internal::set_result(ret, m_context->bkproc.IsBind());
}

void op::Client::GetCursorPos(long *x, long *y, long *ret) {
    long cursor_x = 0;
    long cursor_y = 0;
    internal::set_result(ret, m_context->bkproc._mouse->GetCursorPos(cursor_x, cursor_y));
    internal::set_result(x, cursor_x);
    internal::set_result(y, cursor_y);
}

void op::Client::GetCursorShape(std::wstring &ret) {
    m_context->bkproc._mouse->GetCursorShape(ret);
}

void op::Client::MoveR(long x, long y, long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->MoveR(x, y));
}

void op::Client::MoveTo(long x, long y, long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->MoveTo(x, y));
}

void op::Client::MoveToEx(long x, long y, long w, long h, std::wstring &ret) {
    int dst_x = x;
    int dst_y = y;
    if (m_context->bkproc._mouse->MoveToEx(x, y, w, h, dst_x, dst_y)) {
        ret = std::to_wstring(dst_x) + L"," + std::to_wstring(dst_y);
    } else {
        ret.clear();
    }
}

void op::Client::LeftClick(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->LeftClick());
}

void op::Client::LeftDoubleClick(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->LeftDoubleClick());
}

void op::Client::LeftDown(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->LeftDown());
}

void op::Client::LeftUp(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->LeftUp());
}

void op::Client::MiddleClick(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->MiddleClick());
}

void op::Client::MiddleDown(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->MiddleDown());
}

void op::Client::MiddleUp(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->MiddleUp());
}

void op::Client::RightClick(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->RightClick());
}

void op::Client::RightDown(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->RightDown());
}

void op::Client::RightUp(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->RightUp());
}

void op::Client::WheelDown(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->WheelDown());
}

void op::Client::WheelUp(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->WheelUp());
}

void op::Client::SetMouseDelay(const wchar_t *type, long delay, long *ret) {
    internal::set_result(ret, 0L);
    if (!type || delay < 0)
        return;
    internal::set_result(ret, 1L);
    if (wcscmp(type, L"normal") == 0)
        MOUSE_NORMAL_DELAY = delay;
    else if (wcscmp(type, L"windows") == 0)
        MOUSE_WINDOWS_DELAY = delay;
    else if (wcscmp(type, L"dx") == 0)
        MOUSE_DX_DELAY = delay;
    else
        internal::set_result(ret, 0L);
}

void op::Client::GetKeyState(long vk_code, long *ret) {
    internal::set_result(ret, m_context->bkproc._keyboard->GetKeyState(vk_code));
}

void op::Client::KeyDown(long vk_code, long *ret) {
    internal::set_result(ret, m_context->bkproc._keyboard->KeyDown(vk_code));
}

void op::Client::KeyDownChar(const wchar_t *vk_code, long *ret) {
    internal::set_result(ret, 0L);
    key_combo_t combo;
    if (resolve_text_key_combo(m_context->vkmap, vk_code, combo))
        internal::set_result(ret, key_combo_down(m_context->bkproc._keyboard.get(), combo));
}

void op::Client::KeyUp(long vk_code, long *ret) {
    internal::set_result(ret, m_context->bkproc._keyboard->KeyUp(vk_code));
}

void op::Client::KeyUpChar(const wchar_t *vk_code, long *ret) {
    internal::set_result(ret, 0L);
    key_combo_t combo;
    if (resolve_text_key_combo(m_context->vkmap, vk_code, combo))
        internal::set_result(ret, key_combo_up(m_context->bkproc._keyboard.get(), combo));
}

void op::Client::WaitKey(long vk_code, long time_out, long *ret) {
    unsigned long t = time_out < 0 ? 0xffffffffu : static_cast<unsigned long>(time_out);
    internal::set_result(ret, m_context->bkproc._keyboard->WaitKey(vk_code, t));
}

void op::Client::KeyPress(long vk_code, long *ret) {
    internal::set_result(ret, m_context->bkproc._keyboard->KeyPress(vk_code));
}

void op::Client::KeyPressChar(const wchar_t *vk_code, long *ret) {
    internal::set_result(ret, 0L);
    key_combo_t combo;
    if (resolve_text_key_combo(m_context->vkmap, vk_code, combo))
        internal::set_result(ret, key_combo_press(m_context->bkproc._keyboard.get(), combo));
}

void op::Client::SetKeypadDelay(const wchar_t *type, long delay, long *ret) {
    internal::set_result(ret, 0L);
    if (!type || delay < 0)
        return;
    internal::set_result(ret, 1L);
    if (wcscmp(type, L"normal") == 0)
        KEYPAD_NORMAL_DELAY = delay;
    else if (wcscmp(type, L"normal.hd") == 0)
        KEYPAD_NORMAL2_DELAY = delay;
    else if (wcscmp(type, L"windows") == 0)
        KEYPAD_WINDOWS_DELAY = delay;
    else if (wcscmp(type, L"dx") == 0)
        KEYPAD_DX_DELAY = delay;
    else
        internal::set_result(ret, 0L);
}

void op::Client::KeyPressStr(const wchar_t *key_str, long delay, long *ret) {
    internal::set_result(ret, 0L);
    if (!key_str)
        return;
    auto nlen = wcslen(key_str);
    for (size_t i = 0; i < nlen; ++i) {
        key_combo_t combo;
        if (!resolve_char_key_combo(key_str[i], combo))
            return;

        const long key_ret = key_combo_press(m_context->bkproc._keyboard.get(), combo);
        internal::set_result(ret, key_ret);
        if (key_ret == 0)
            return;
        ::Delay(delay > 0 ? delay : 1);
    }
}
