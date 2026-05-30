// #include "stdafx.h"
#include "winkeypad.h"
#include "./core/globalVar.h"
#include "./core/helpfunc.h"
#include <string>

static uint oem_code(uint key) {
    short code[256] = {0};
    code['q'] = 0x10;
    code['a'] = 0x1e;
    code['w'] = 0x11;
    code['s'] = 0x1f;
    code['e'] = 0x12;
    code['d'] = 0x20;
    code['r'] = 0x13;
    code['f'] = 0x21;
    code['t'] = 0x14;
    code['g'] = 0x22;
    code['y'] = 0x15;
    code['h'] = 0x23;
    code['u'] = 0x16;
    code['j'] = 0x24;
    code['i'] = 0x17;
    code['k'] = 0x25;
    code['o'] = 0x18;
    code['l'] = 0x26;
    code['p'] = 0x19;
    code[':'] = 0x27;
    code[';'] = 0x27;

    code['z'] = 0x2c;
    code['x'] = 0x2d;
    code['c'] = 0x2e;
    code['v'] = 0x2f;
    code['b'] = 0x30;
    code['n'] = 0x31;
    code['m'] = 0x32;
    return code[key & 0xffu];
}

static long normalize_vk_code(long vk_code) {
    if ((vk_code >= 'a' && vk_code <= 'z') || (vk_code >= 'A' && vk_code <= 'Z'))
        return toupper(vk_code);
    return vk_code;
}

static bool is_alt_vk(long vk_code) {
    return vk_code == VK_MENU || vk_code == VK_LMENU || vk_code == VK_RMENU;
}

static bool is_ctrl_vk(long vk_code) {
    return vk_code == VK_CONTROL || vk_code == VK_LCONTROL || vk_code == VK_RCONTROL;
}

static bool is_shift_vk(long vk_code) {
    return vk_code == VK_SHIFT || vk_code == VK_LSHIFT || vk_code == VK_RSHIFT;
}

static bool is_extended_vk(long vk_code) {
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

// 按键消息的 lParam 需要带扫描码、扩展键标志和按下/抬起状态。
static LPARAM build_windows_key_lparam(long vk_code, bool key_up, bool extended, bool alt_context) {
    DWORD data = 1;
    const WORD scan_code = static_cast<WORD>(::MapVirtualKey(vk_code, MAPVK_VK_TO_VSC));
    data |= static_cast<DWORD>(scan_code) << 16;
    if (extended)
        data |= 1u << 24;
    if (alt_context)
        data |= 1u << 29;
    if (key_up)
        data |= 3u << 30;
    return static_cast<LPARAM>(data);
}

// 按当前修饰键状态把虚拟键翻译成字符，供 WM_CHAR / WM_SYSCHAR 使用。
static bool translate_vk_to_text(long vk_code, bool shift_down, bool ctrl_down, bool alt_down, std::wstring &text) {
    BYTE key_state[256] = {0};
    if (shift_down)
        key_state[VK_SHIFT] = 0x80;
    if (ctrl_down)
        key_state[VK_CONTROL] = 0x80;
    if (alt_down)
        key_state[VK_MENU] = 0x80;

    wchar_t chars[8] = {0};
    const UINT scan_code = static_cast<UINT>(::MapVirtualKey(vk_code, MAPVK_VK_TO_VSC));
    const int count = ::ToUnicodeEx(vk_code, scan_code, key_state, chars, 8, 0, ::GetKeyboardLayout(0));
    if (count <= 0)
        return false;

    text.assign(chars, chars + count);
    return true;
}

winkeypad::winkeypad() : bkkeypad() {
}

winkeypad::~winkeypad() {
    // UnBind();
}

long winkeypad::Bind(HWND hwnd, long mode) {
    if (!::IsWindow(hwnd))
        return 0;
    _hwnd = hwnd;
    _mode = mode;
    return 1;
}

long winkeypad::UnBind() {
    _hwnd = NULL;
    _mode = 0;
    _shift_down = false;
    _ctrl_down = false;
    _alt_down = false;
    return 1;
}

long winkeypad::GetKeyState(long vk_code) {
    const long normalized_vk = normalize_vk_code(vk_code);
    return (::GetAsyncKeyState(normalized_vk) & 0x8000) ? 1 : 0;
}

long winkeypad::KeyDown(long vk_code) {
    long ret = 0;
    // vk_code = toupper(vk_code);

    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {

        INPUT Input = {0};
        Input.type = INPUT_KEYBOARD;
        Input.ki.wVk = (WORD)vk_code;
        Input.ki.wScan = 0;
        Input.ki.dwFlags = 0;

        /*The function returns the number of events that it successfully inserted into the keyboard or mouse input
        stream. If the function returns zero, the input was already blocked by another thread. To get extended error
        information, call GetLastError. This function fails when it is blocked by UIPI. Note that neither GetLastError
        nor the return value will indicate the failure was caused by UIPI blocking.
        */
        ret = ::SendInput(1, &Input, sizeof(INPUT));
        break;
    }
    case INPUT_TYPE::IN_NORMAL2: {
        INPUT Input = {0};
        Input.type = INPUT_KEYBOARD;
        Input.ki.wVk = 0;
        Input.ki.wScan = MapVirtualKey(vk_code, MAPVK_VK_TO_VSC);
        Input.ki.dwFlags = KEYEVENTF_SCANCODE;
        // 扫描码模式下，扩展键还需要补扩展键标志。
        if (is_extended_vk(vk_code))
            Input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        ret = ::SendInput(1, &Input, sizeof(INPUT));
        if (ret == 0)
            setlog("op:IN_NORMAL2 erro code:%s", GetLastErrorAsString().c_str());
        break;
    }
    case INPUT_TYPE::IN_WINDOWS: {
        // windows 模式下补扩展键标志，并在 Alt 参与时切到系统键消息。
        const bool extended = is_extended_vk(vk_code);
        const bool use_system_message = is_alt_vk(vk_code) || _alt_down;
        const bool alt_context = _alt_down && !is_alt_vk(vk_code);
        const UINT message = use_system_message ? WM_SYSKEYDOWN : WM_KEYDOWN;
        const LPARAM lparam = build_windows_key_lparam(vk_code, false, extended, alt_context);

        ret = ::SendMessageTimeout(_hwnd, message, vk_code, lparam, SMTO_BLOCK, 2000, nullptr);
        if (ret == 0)
            setlog("error code=%d", GetLastError());
        else {
            if (is_shift_vk(vk_code))
                _shift_down = true;
            else if (is_ctrl_vk(vk_code))
                _ctrl_down = true;
            else if (is_alt_vk(vk_code))
                _alt_down = true;
        }
        break;
    }
    }

    return ret;
}

long winkeypad::KeyUp(long vk_code) {
    long ret = 0;
    // vk_code = toupper(vk_code);
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {

        INPUT Input = {0};
        Input.type = INPUT_KEYBOARD;
        Input.ki.wVk = vk_code;
        Input.ki.wScan = 0;
        Input.ki.dwFlags = KEYEVENTF_KEYUP;

        /*The function returns the number of events that it successfully inserted into the keyboard or mouse input
        stream. If the function returns zero, the input was already blocked by another thread. To get extended error
        information, call GetLastError. This function fails when it is blocked by UIPI. Note that neither GetLastError
        nor the return value will indicate the failure was caused by UIPI blocking.
        */
        ret = ::SendInput(1, &Input, sizeof(INPUT));
        break;
    }
    case INPUT_TYPE::IN_NORMAL2: {
        INPUT Input = {0};
        Input.type = INPUT_KEYBOARD;
        Input.ki.wVk = 0;
        Input.ki.wScan = MapVirtualKey(vk_code, MAPVK_VK_TO_VSC);
        Input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
        // 抬键时保持和按下阶段一致的扩展键标志。
        if (is_extended_vk(vk_code))
            Input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        ret = ::SendInput(1, &Input, sizeof(INPUT));
        if (ret == 0)
            setlog("op:IN_NORMAL2 erro code:%s", GetLastErrorAsString().c_str());
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        // 抬键时保持和按下阶段一致的消息类型与 lParam 位语义。
        const bool extended = is_extended_vk(vk_code);
        const bool use_system_message = is_alt_vk(vk_code) || _alt_down;
        const bool alt_context = _alt_down && !is_alt_vk(vk_code);
        const UINT message = use_system_message ? WM_SYSKEYUP : WM_KEYUP;
        const LPARAM lparam = build_windows_key_lparam(vk_code, true, extended, alt_context);

        ret = ::SendMessageTimeout(_hwnd, message, vk_code, lparam, SMTO_BLOCK, 2000, nullptr);
        if (ret == 0)
            setlog("error code=%d", GetLastError());
        else {
            if (is_shift_vk(vk_code))
                _shift_down = false;
            else if (is_ctrl_vk(vk_code))
                _ctrl_down = false;
            else if (is_alt_vk(vk_code))
                _alt_down = false;
        }
        break;
    }
    }

    return ret;
}

long winkeypad::WaitKey(long vk_code, unsigned long time_out) {
    auto deadline = ::GetTickCount64() + time_out;
    do {
        if (vk_code == 0) {
            for (int i = 1; i < 255; ++i) {
                if (::GetAsyncKeyState(i) & 0x8000)
                    return i;
            }
            if (time_out == 0) return 0;
        } else {
            if (GetKeyState(vk_code))
                return vk_code;
            if (time_out == 0) return 0;
        }
        if (time_out > 0) ::Sleep(1);
    } while (::GetTickCount64() < deadline);
    return 0;
}

long winkeypad::KeyPress(long vk_code) {
    KeyDown(vk_code);
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        ::Delay(KEYPAD_NORMAL_DELAY);
        break;
    }
    case INPUT_TYPE::IN_NORMAL2: {
        ::Delay(KEYPAD_NORMAL2_DELAY);
        break;
    }
    case INPUT_TYPE::IN_WINDOWS: {
        // 对文本控件补字符消息，便于标准编辑控件直接落字。
        std::wstring text;
        if (translate_vk_to_text(vk_code, _shift_down, _ctrl_down, _alt_down, text)) {
            const UINT char_message = _alt_down ? WM_SYSCHAR : WM_CHAR;
            for (wchar_t ch : text)
                ::SendMessageTimeout(_hwnd, char_message, ch, 1, SMTO_BLOCK, 2000, nullptr);
        }
        ::Delay(KEYPAD_WINDOWS_DELAY);
        break;
    }
    }
    return KeyUp(vk_code);
}
