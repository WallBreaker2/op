#include "OpContext.h"
#include "OpResult.h"

#include "base/AutomationModes.h"
#include "base/Utils.h"

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
    // 旧实现失败时直接 return 0：已经按下的修饰键不会再抬起来，
    // 它们会留在系统输入状态里，后续所有按键都被 Shift/Ctrl/Alt 污染。
    size_t pressed = 0;
    for (long modifier : combo.modifiers) {
        if (keypad->KeyDown(modifier) != 1)
            break;
        ++pressed;
    }

    const bool modifiers_ok = pressed == combo.modifiers.size();
    const bool key_ok = modifiers_ok && keypad->KeyDown(combo.vk) == 1;
    if (key_ok)
        return 1;

    // 回滚：先抬主键（若已按下），再把已按下的修饰键逆序抬起，失败也继续尝试。
    if (modifiers_ok)
        keypad->KeyUp(combo.vk);
    for (size_t i = pressed; i > 0; --i)
        keypad->KeyUp(combo.modifiers[i - 1]);
    return 0;
}

long key_combo_up(KeyboardBackend *keypad, const key_combo_t &combo) {
    // 主键抬起失败时也要把修饰键放掉，否则同样会卡住 Shift/Ctrl/Alt。
    long ret = keypad->KeyUp(combo.vk);

    for (auto it = combo.modifiers.rbegin(); it != combo.modifiers.rend(); ++it) {
        if (keypad->KeyUp(*it) != 1)
            ret = 0;
    }
    return ret == 1 ? 1 : 0;
}

long key_combo_press(KeyboardBackend *keypad, const key_combo_t &combo) {
    if (combo.modifiers.empty())
        return keypad->KeyPress(combo.vk);

    if (key_combo_down(keypad, combo) != 1)
        return 0;
    return key_combo_up(keypad, combo);
}

} // namespace

void op::Op::BindWindow(LONG_PTR hwnd, const wchar_t *display, const wchar_t *mouse, const wchar_t *keypad,
                            long mode, long *ret) {
    BindWindowEx(hwnd, hwnd, display, mouse, keypad, mode, ret);
}

void op::Op::BindWindowEx(LONG_PTR display_hwnd, LONG_PTR input_hwnd, const wchar_t *display,
                              const wchar_t *mouse, const wchar_t *keypad, long mode, long *ret) {
    if (m_context->bkproc.IsBind())
        m_context->bkproc.UnBindWindow();
    internal::set_result(ret, m_context->bkproc.BindWindowEx(display_hwnd, input_hwnd, display, mouse, keypad, mode));
}

void op::Op::UnBindWindow(long *ret) {
    internal::set_result(ret, m_context->bkproc.UnBindWindow());
}

void op::Op::LockInput(long lock, long *ret) {
    internal::set_result(ret, m_context->bkproc.LockInput(lock));
}

void op::Op::GetBindWindow(LONG_PTR *ret) {
    internal::set_result(ret, m_context->bkproc.GetBindWindow());
}

void op::Op::IsBind(long *ret) {
    internal::set_result(ret, m_context->bkproc.IsBind());
}

void op::Op::GetCursorPos(long *x, long *y, long *ret) {
    long cursor_x = 0;
    long cursor_y = 0;
    internal::set_result(ret, m_context->bkproc._mouse->GetCursorPos(cursor_x, cursor_y));
    internal::set_result(x, cursor_x);
    internal::set_result(y, cursor_y);
}

void op::Op::GetCursorShape(std::wstring &ret) {
    m_context->bkproc._mouse->GetCursorShape(ret);
}

void op::Op::MoveR(long x, long y, long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->MoveR(x, y));
}

void op::Op::MoveTo(long x, long y, long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->MoveTo(x, y));
}

void op::Op::MoveToEx(long x, long y, long w, long h, std::wstring &ret) {
    int dst_x = x;
    int dst_y = y;
    if (m_context->bkproc._mouse->MoveToEx(x, y, w, h, dst_x, dst_y)) {
        ret = std::to_wstring(dst_x) + L"," + std::to_wstring(dst_y);
    } else {
        ret.clear();
    }
}

void op::Op::MoveToSmooth(long x, long y, long duration, long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->MoveToSmooth(x, y, duration));
}

void op::Op::MoveToExSmooth(long x, long y, long w, long h, long duration, std::wstring &ret) {
    int dst_x = x;
    int dst_y = y;
    if (m_context->bkproc._mouse->MoveToExSmooth(x, y, w, h, duration, dst_x, dst_y)) {
        ret = std::to_wstring(dst_x) + L"," + std::to_wstring(dst_y);
    } else {
        ret.clear();
    }
}

void op::Op::MovePath(const wchar_t *path, long duration, long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->MovePath(path ? path : L"", duration));
}

void op::Op::DragPath(const wchar_t *path, long duration, long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->DragPath(path ? path : L"", duration));
}

void op::Op::SetMouseTrajectory(long mode, long min_duration, long max_duration, long jitter, long start_delay,
                                long end_delay, long *ret) {
    const long result = m_context->bkproc._mouse->SetMouseTrajectory(
        static_cast<int>(mode), static_cast<int>(min_duration), static_cast<int>(max_duration),
        static_cast<int>(jitter), static_cast<int>(start_delay), static_cast<int>(end_delay));
    internal::set_result(ret, result);
}

void op::Op::LeftClick(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->LeftClick());
}

void op::Op::LeftDoubleClick(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->LeftDoubleClick());
}

void op::Op::LeftDown(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->LeftDown());
}

void op::Op::LeftUp(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->LeftUp());
}

void op::Op::MiddleClick(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->MiddleClick());
}

void op::Op::MiddleDoubleClick(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->MiddleDoubleClick());
}

void op::Op::MiddleDown(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->MiddleDown());
}

void op::Op::MiddleUp(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->MiddleUp());
}

void op::Op::RightClick(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->RightClick());
}

void op::Op::RightDoubleClick(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->RightDoubleClick());
}

void op::Op::RightDown(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->RightDown());
}

void op::Op::RightUp(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->RightUp());
}

void op::Op::XButton1Click(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->XButton1Click());
}

void op::Op::XButton1DoubleClick(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->XButton1DoubleClick());
}

void op::Op::XButton1Down(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->XButton1Down());
}

void op::Op::XButton1Up(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->XButton1Up());
}

void op::Op::XButton2Click(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->XButton2Click());
}

void op::Op::XButton2DoubleClick(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->XButton2DoubleClick());
}

void op::Op::XButton2Down(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->XButton2Down());
}

void op::Op::XButton2Up(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->XButton2Up());
}

void op::Op::Wheel(long delta, long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->Wheel(delta));
}

void op::Op::HWheel(long delta, long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->HWheel(delta));
}

void op::Op::WheelDown(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->WheelDown());
}

void op::Op::WheelUp(long *ret) {
    internal::set_result(ret, m_context->bkproc._mouse->WheelUp());
}

void op::Op::SetMouseDelay(const wchar_t *type, long delay, long *ret) {
    internal::set_result(ret, 0L);
    if (!type || delay < 0)
        return;

    // 模式名不区分大小写：同族的 KeyDownChar/KeyPressChar 收键名时会先转小写，
    // 这里旧实现用 wcscmp，传 "Normal"/"DX" 会静默失败（返回 0 且什么都没设）。
    std::wstring mode = type;
    wstring2lower(mode);

    if (mode == L"normal")
        MOUSE_NORMAL_DELAY = delay;
    else if (mode == L"windows")
        MOUSE_WINDOWS_DELAY = delay;
    else if (mode == L"dx")
        MOUSE_DX_DELAY = delay;
    else
        return;
    internal::set_result(ret, 1L);
}

void op::Op::GetKeyState(long vk_code, long *ret) {
    internal::set_result(ret, m_context->bkproc._keyboard->GetKeyState(vk_code));
}

void op::Op::KeyDown(long vk_code, long *ret) {
    internal::set_result(ret, m_context->bkproc._keyboard->KeyDown(vk_code));
}

void op::Op::KeyDownChar(const wchar_t *vk_code, long *ret) {
    internal::set_result(ret, 0L);
    key_combo_t combo;
    if (resolve_text_key_combo(m_context->vkmap, vk_code, combo))
        internal::set_result(ret, key_combo_down(m_context->bkproc._keyboard.get(), combo));
}

void op::Op::KeyUp(long vk_code, long *ret) {
    internal::set_result(ret, m_context->bkproc._keyboard->KeyUp(vk_code));
}

void op::Op::KeyUpChar(const wchar_t *vk_code, long *ret) {
    internal::set_result(ret, 0L);
    key_combo_t combo;
    if (resolve_text_key_combo(m_context->vkmap, vk_code, combo))
        internal::set_result(ret, key_combo_up(m_context->bkproc._keyboard.get(), combo));
}

void op::Op::WaitKey(long vk_code, long time_out, long *ret) {
    unsigned long t = time_out < 0 ? 0xffffffffu : static_cast<unsigned long>(time_out);
    internal::set_result(ret, m_context->bkproc._keyboard->WaitKey(vk_code, t));
}

void op::Op::KeyPress(long vk_code, long *ret) {
    internal::set_result(ret, m_context->bkproc._keyboard->KeyPress(vk_code));
}

void op::Op::KeyPressChar(const wchar_t *vk_code, long *ret) {
    internal::set_result(ret, 0L);
    key_combo_t combo;
    if (resolve_text_key_combo(m_context->vkmap, vk_code, combo))
        internal::set_result(ret, key_combo_press(m_context->bkproc._keyboard.get(), combo));
}

void op::Op::SetKeypadDelay(const wchar_t *type, long delay, long *ret) {
    internal::set_result(ret, 0L);
    if (!type || delay < 0)
        return;

    std::wstring mode = type; // 理由同 SetMouseDelay
    wstring2lower(mode);

    if (mode == L"normal")
        KEYPAD_NORMAL_DELAY = delay;
    else if (mode == L"normal.hd")
        KEYPAD_NORMAL2_DELAY = delay;
    else if (mode == L"windows")
        KEYPAD_WINDOWS_DELAY = delay;
    else if (mode == L"dx")
        KEYPAD_DX_DELAY = delay;
    else
        return;
    internal::set_result(ret, 1L);
}

void op::Op::KeyPressStr(const wchar_t *key_str, long delay, long *ret) {
    internal::set_result(ret, 0L);
    if (!key_str)
        return;
    auto nlen = wcslen(key_str);
    if (nlen == 0) {
        // 空串是"没有要按的键"，不是失败：旧实现循环一次都不进，ret 停在 0。
        internal::set_result(ret, 1L);
        return;
    }
    for (size_t i = 0; i < nlen; ++i) {
        const long key_ret = m_context->bkproc._keyboard->InputChar(key_str[i]);
        internal::set_result(ret, key_ret);
        if (key_ret == 0)
            return;
        ::Delay(delay > 0 ? delay : 1);
    }
}
