#include "WindowService.h"

#include <cstring>
#include <string>

namespace op {

namespace {

class ClipboardGuard {
  public:
    ClipboardGuard() : opened_(::OpenClipboard(nullptr) != 0) {
    }

    ~ClipboardGuard() {
        if (opened_) {
            ::CloseClipboard();
        }
    }

    ClipboardGuard(const ClipboardGuard &) = delete;
    ClipboardGuard &operator=(const ClipboardGuard &) = delete;

    explicit operator bool() const noexcept {
        return opened_;
    }

  private:
    bool opened_ = false;
};

class GlobalLockGuard {
  public:
    explicit GlobalLockGuard(HGLOBAL handle) : handle_(handle), data_(handle ? ::GlobalLock(handle) : nullptr) {
    }

    ~GlobalLockGuard() {
        if (data_) {
            ::GlobalUnlock(handle_);
        }
    }

    GlobalLockGuard(const GlobalLockGuard &) = delete;
    GlobalLockGuard &operator=(const GlobalLockGuard &) = delete;

    void *get() const noexcept {
        return data_;
    }

  private:
    HGLOBAL handle_ = nullptr;
    void *data_ = nullptr;
};

class GlobalMemory {
  public:
    explicit GlobalMemory(SIZE_T size) : handle_(::GlobalAlloc(GHND | GMEM_SHARE, size)) {
    }

    ~GlobalMemory() {
        if (handle_) {
            ::GlobalFree(handle_);
        }
    }

    GlobalMemory(const GlobalMemory &) = delete;
    GlobalMemory &operator=(const GlobalMemory &) = delete;

    HGLOBAL get() const noexcept {
        return handle_;
    }

    HGLOBAL release() noexcept {
        HGLOBAL handle = handle_;
        handle_ = nullptr;
        return handle;
    }

    explicit operator bool() const noexcept {
        return handle_ != nullptr;
    }

  private:
    HGLOBAL handle_ = nullptr;
};

std::wstring ansi_to_wide(const std::string &text) {
    if (text.empty())
        return L"";

    const int length = ::MultiByteToWideChar(CP_ACP, 0, text.c_str(), -1, nullptr, 0);
    if (length <= 0)
        return L"";

    std::wstring out(static_cast<size_t>(length), L'\0');
    ::MultiByteToWideChar(CP_ACP, 0, text.c_str(), -1, out.data(), length);
    if (!out.empty() && out.back() == L'\0')
        out.pop_back();
    return out;
}

std::string wide_to_ansi(const wchar_t *text) {
    if (text == nullptr || text[0] == L'\0')
        return "";

    const int length = ::WideCharToMultiByte(CP_ACP, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (length <= 0)
        return "";

    std::string out(static_cast<size_t>(length), '\0');
    ::WideCharToMultiByte(CP_ACP, 0, text, -1, out.data(), length, nullptr, nullptr);
    if (!out.empty() && out.back() == '\0')
        out.pop_back();
    return out;
}

bool read_clipboard_ansi(std::string &text) {
    text.clear();
    ClipboardGuard clipboard;
    if (!clipboard)
        return false;

    auto *handle = static_cast<HGLOBAL>(::GetClipboardData(CF_TEXT));
    GlobalLockGuard lock(handle);
    const auto *buffer = static_cast<const char *>(lock.get());
    if (buffer == nullptr)
        return false;

    text = buffer;
    return true;
}

HWND ResolveInputTargetWindow(HWND hwnd) {
    if (!::IsWindow(hwnd))
        return nullptr;

    DWORD target_thread = ::GetWindowThreadProcessId(hwnd, nullptr);
    if (target_thread != 0) {
        GUITHREADINFO gti = {0};
        gti.cbSize = sizeof(gti);
        if (::GetGUIThreadInfo(target_thread, &gti)) {
            if (::IsWindow(gti.hwndFocus) && (gti.hwndFocus == hwnd || ::IsChild(hwnd, gti.hwndFocus)))
                return gti.hwndFocus;
            if (::IsWindow(gti.hwndActive) && (gti.hwndActive == hwnd || ::IsChild(hwnd, gti.hwndActive)))
                return gti.hwndActive;
        }
    }

    return hwnd;
}

bool SendCharMessage(HWND hwnd, UINT msg, wchar_t ch) {
    DWORD_PTR result = 0;
    LRESULT ret = ::SendMessageTimeoutW(hwnd, msg, (WPARAM)ch, (LPARAM)1, SMTO_BLOCK | SMTO_ABORTIFHUNG, 200, &result);
    if (ret != 0)
        return true;

    return ::PostMessageW(hwnd, msg, (WPARAM)ch, 0) != 0;
}

} // namespace

bool WindowService::SendPaste(HWND hwnd) {
    bool bret = true;
    std::string clipboard_text;
    if (!read_clipboard_ansi(clipboard_text))
        return false;

    const std::wstring wword = ansi_to_wide(clipboard_text);
    int len = static_cast<int>(wword.length());
    // MessageBoxA(NULL,tts,tts,NULL);
    for (int i = 0; i < len; i++) {
        ::SendMessage(hwnd, WM_CHAR, (WPARAM)wword[i], (LPARAM)1);
        Sleep(10);
    }

    return bret;
}

bool WindowService::SetClipboard(const wchar_t *values) {
    const std::string content = wide_to_ansi(values);
    ClipboardGuard clipboard;
    if (!clipboard)
        return false;

    ::EmptyClipboard();

    const SIZE_T length = content.size() + 1;
    GlobalMemory memory(length);
    if (!memory)
        return false;

    {
        GlobalLockGuard lock(memory.get());
        auto *buffer = static_cast<char *>(lock.get());
        if (buffer == nullptr)
            return false;
        memcpy(buffer, content.c_str(), length);
    }

    if (::SetClipboardData(CF_TEXT, memory.get()) == nullptr)
        return false;

    memory.release();
    return true;
}

bool WindowService::GetClipboard(std::wstring &retstr) {
    std::string clipboard_text;
    if (!read_clipboard_ansi(clipboard_text))
        return false;

    retstr = ansi_to_wide(clipboard_text);
    return true;
}

long WindowService::SendString(HWND hwnd, const std::wstring &s) {
    if (!::IsWindow(hwnd))
        return 0;

    HWND target = ResolveInputTargetWindow(hwnd);
    if (!::IsWindow(target))
        return 0;

    auto p = s.data();
    for (size_t i = 0; i < s.length(); ++i) {
        bool delivered = SendCharMessage(target, WM_CHAR, p[i]);
        if (!delivered && p[i] > 0x7f)
            delivered = SendCharMessage(target, WM_IME_CHAR, p[i]);
        if (!delivered)
            return 0;
        ::Sleep(5);
    }
    return 1;
}

long WindowService::SendStringIme(HWND hwnd, const std::wstring &s) {
    if (!::IsWindow(hwnd))
        return 0;

    HWND target = ResolveInputTargetWindow(hwnd);
    if (!::IsWindow(target))
        return 0;

    auto p = s.data();
    for (size_t i = 0; i < s.length(); ++i) {
        bool delivered = SendCharMessage(target, WM_CHAR, p[i]);
        if (!delivered)
            delivered = SendCharMessage(target, WM_IME_CHAR, p[i]);
        if (!delivered)
            return 0;
        ::Sleep(5);
    }
    return 1;
}

} // namespace op
