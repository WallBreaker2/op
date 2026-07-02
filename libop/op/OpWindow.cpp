#include "OpContext.h"
#include "OpResult.h"

#include "runtime/RuntimeUtils.h"
#include "window/WindowLayout.h"

#include <libop.h>

#include <cwchar>
#include <string>
#include <vector>

#undef FindWindow
#undef FindWindowEx
#undef SetWindowText

namespace {

bool parse_layout_type(long value, op::window_layout::Type &type) {
    switch (value) {
    case 0:
        type = op::window_layout::Type::Grid;
        return true;
    case 1:
        type = op::window_layout::Type::Diagonal;
        return true;
    default:
        return false;
    }
}

bool parse_size_mode(long value, op::window_layout::SizeMode &mode) {
    switch (value) {
    case 0:
        mode = op::window_layout::SizeMode::Keep;
        return true;
    case 1:
        mode = op::window_layout::SizeMode::Uniform;
        return true;
    default:
        return false;
    }
}

bool parse_anchor_mode(long value, op::window_layout::AnchorMode &mode) {
    switch (value) {
    case 0:
        mode = op::window_layout::AnchorMode::Window;
        return true;
    case 1:
        mode = op::window_layout::AnchorMode::Client;
        return true;
    default:
        return false;
    }
}

bool parse_window_list(const wchar_t *hwnds, std::vector<HWND> &windows) {
    if (hwnds == nullptr || hwnds[0] == L'\0')
        return false;

    std::vector<std::wstring> items;
    split(hwnds, items, L"|");
    if (items.empty())
        return false;

    windows.clear();
    windows.reserve(items.size());
    for (const auto &item : items) {
        wchar_t *end = nullptr;
        const auto value = _wcstoi64(item.c_str(), &end, 0);
        if (end == item.c_str() || (end && *end != L'\0'))
            return false;
        windows.push_back(reinterpret_cast<HWND>(static_cast<LONG_PTR>(value)));
    }

    return !windows.empty();
}

std::wstring get_window_class_name(HWND hwnd) {
    std::vector<wchar_t> buffer(256, L'\0');
    for (;;) {
        const int copied = ::GetClassNameW(hwnd, buffer.data(), static_cast<int>(buffer.size()));
        if (copied <= 0)
            return L"";
        if (static_cast<size_t>(copied) < buffer.size() - 1)
            return std::wstring(buffer.data(), static_cast<size_t>(copied));
        buffer.assign(buffer.size() * 2, L'\0');
    }
}

std::wstring get_window_title(HWND hwnd) {
    const int length = ::GetWindowTextLengthW(hwnd);
    if (length <= 0)
        return L"";

    std::vector<wchar_t> buffer(static_cast<size_t>(length) + 1, L'\0');
    const int copied = ::GetWindowTextW(hwnd, buffer.data(), static_cast<int>(buffer.size()));
    if (copied <= 0)
        return L"";
    return std::wstring(buffer.data(), static_cast<size_t>(copied));
}

LONG_PTR hwnd_result(HWND hwnd) {
    return reinterpret_cast<LONG_PTR>(hwnd);
}

} // namespace

void op::Op::EnumWindow(LONG_PTR parent, const wchar_t *title, const wchar_t *class_name, long filter,
                            std::wstring &retstr) {
    m_context->window_service.EnumWindow(reinterpret_cast<HWND>(parent), title, class_name, filter, retstr);
}

void op::Op::EnumWindowByProcess(const wchar_t *process_name, const wchar_t *title, const wchar_t *class_name,
                                     long filter, std::wstring &retstring) {
    m_context->window_service.EnumWindow(nullptr, title, class_name, filter, retstring, process_name);
}

void op::Op::ClientToScreen(LONG_PTR hwnd, long *x, long *y, long *bret) {
    internal::set_result(bret, m_context->window_service.ClientToScreen(reinterpret_cast<HWND>(hwnd), *x, *y));
}

void op::Op::FindWindow(const wchar_t *class_name, const wchar_t *title, LONG_PTR *rethwnd) {
    internal::set_result(rethwnd, hwnd_result(m_context->window_service.FindWindow(class_name, title)));
}

void op::Op::FindWindowByProcess(const wchar_t *process_name, const wchar_t *class_name, const wchar_t *title,
                                     LONG_PTR *rethwnd) {

    HWND hwnd = nullptr;
    m_context->window_service.FindWindowByProcess(class_name, title, hwnd, process_name);
    internal::set_result(rethwnd, hwnd_result(hwnd));
}

void op::Op::FindWindowByProcessId(long process_id, const wchar_t *class_name, const wchar_t *title,
                                       LONG_PTR *rethwnd) {
    HWND hwnd = nullptr;
    m_context->window_service.FindWindowByProcess(class_name, title, hwnd, NULL, process_id);
    internal::set_result(rethwnd, hwnd_result(hwnd));
}

void op::Op::FindWindowEx(LONG_PTR parent, const wchar_t *class_name, const wchar_t *title, LONG_PTR *rethwnd) {
    internal::set_result(
        rethwnd, hwnd_result(m_context->window_service.FindWindowEx(reinterpret_cast<HWND>(parent), class_name, title)));
}

void op::Op::GetClientRect(LONG_PTR hwnd, long *x1, long *y1, long *x2, long *y2, long *nret) {
    long left = 0;
    long top = 0;
    long right = 0;
    long bottom = 0;
    internal::set_result(nret,
                         m_context->window_service.GetClientRect(reinterpret_cast<HWND>(hwnd), left, top, right, bottom));
    internal::set_result(x1, left);
    internal::set_result(y1, top);
    internal::set_result(x2, right);
    internal::set_result(y2, bottom);
}

void op::Op::GetClientSize(LONG_PTR hwnd, long *width, long *height, long *nret) {
    long client_width = 0;
    long client_height = 0;
    internal::set_result(
        nret, m_context->window_service.GetClientSize(reinterpret_cast<HWND>(hwnd), client_width, client_height));
    internal::set_result(width, client_width);
    internal::set_result(height, client_height);
}

void op::Op::GetForegroundFocus(LONG_PTR *rethwnd) {
    internal::set_result(rethwnd, hwnd_result(::GetFocus()));
}

void op::Op::GetForegroundWindow(LONG_PTR *rethwnd) {
    internal::set_result(rethwnd, hwnd_result(::GetForegroundWindow()));
}

void op::Op::GetMousePointWindow(LONG_PTR *rethwnd) {
    //::Sleep(2000);
    HWND hwnd = nullptr;
    m_context->window_service.GetMousePointWindow(hwnd);
    internal::set_result(rethwnd, hwnd_result(hwnd));
}

void op::Op::GetPointWindow(long x, long y, LONG_PTR *rethwnd) {
    HWND hwnd = nullptr;
    m_context->window_service.GetMousePointWindow(hwnd, x, y);
    internal::set_result(rethwnd, hwnd_result(hwnd));
}

void op::Op::GetSpecialWindow(long flag, LONG_PTR *rethwnd) {
    internal::set_result(rethwnd, 0);
    if (flag == 0)
        internal::set_result(rethwnd, hwnd_result(GetDesktopWindow()));
    else if (flag == 1) {
        internal::set_result(rethwnd, hwnd_result(::FindWindowW(L"Shell_TrayWnd", NULL)));
    }
}

void op::Op::GetWindow(LONG_PTR hwnd, long flag, LONG_PTR *nret) {
    HWND target = nullptr;
    m_context->window_service.GetWindow(reinterpret_cast<HWND>(hwnd), flag, target);
    internal::set_result(nret, hwnd_result(target));
}

void op::Op::GetWindowClass(LONG_PTR hwnd, std::wstring &retstring) {
    retstring = get_window_class_name(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)));
}

void op::Op::GetWindowRect(LONG_PTR hwnd, long *x1, long *y1, long *x2, long *y2, long *nret) {
    RECT winrect = {};
    internal::set_result(nret, ::GetWindowRect(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), &winrect));
    internal::set_result(x1, winrect.left);
    internal::set_result(y1, winrect.top);
    internal::set_result(x2, winrect.right);
    internal::set_result(y2, winrect.bottom);
}

void op::Op::GetWindowState(LONG_PTR hwnd, long flag, long *rethwnd) {
    internal::set_result(rethwnd,
                         m_context->window_service.GetWindowState(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), flag));
}

void op::Op::GetWindowTitle(LONG_PTR hwnd, std::wstring &rettitle) {
    rettitle = get_window_title(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)));
}

void op::Op::MoveWindow(LONG_PTR hwnd, long x, long y, long *nret) {
    RECT winrect;
    HWND target = reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd));
    ::GetWindowRect(target, &winrect);
    int width = winrect.right - winrect.left;
    int height = winrect.bottom - winrect.top;
    internal::set_result(nret, ::MoveWindow(target, x, y, width, height, false));
}

void op::Op::ScreenToClient(LONG_PTR hwnd, long *x, long *y, long *nret) {
    POINT point;
    point.x = x ? *x : 0;
    point.y = y ? *y : 0;
    internal::set_result(nret, ::ScreenToClient(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), &point));
    internal::set_result(x, point.x);
    internal::set_result(y, point.y);
}

void op::Op::SetClientSize(LONG_PTR hwnd, long width, long height, long *nret) {
    internal::set_result(nret, m_context->window_service.SetWindowSize(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)),
                                                                       width, height));
}

void op::Op::SetWindowState(LONG_PTR hwnd, long flag, long *nret) {
    internal::set_result(nret,
                         m_context->window_service.SetWindowState(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), flag));
}

void op::Op::SetWindowSize(LONG_PTR hwnd, long width, long height, long *nret) {
    internal::set_result(nret, m_context->window_service.SetWindowSize(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)),
                                                                       width, height, 1));
}

void op::Op::LayoutWindows(const wchar_t *hwnds, long layout_type, long columns, long start_x, long start_y,
                                long gap_x, long gap_y, long size_mode, long window_width, long window_height,
                                long anchor_mode, long *ret) {
    internal::set_result(ret, 0L);

    std::vector<HWND> windows;
    if (!parse_window_list(hwnds, windows))
        return;

    op::window_layout::Options options;
    if (!parse_layout_type(layout_type, options.type))
        return;
    if (!parse_size_mode(size_mode, options.size_mode))
        return;
    if (!parse_anchor_mode(anchor_mode, options.anchor_mode))
        return;

    options.columns = columns;
    options.start_x = start_x;
    options.start_y = start_y;
    options.gap_x = gap_x;
    options.gap_y = gap_y;
    options.window_width = window_width;
    options.window_height = window_height;

    internal::set_result(ret, op::window_layout::Layout(windows, options));
}

void op::Op::SetWindowText(LONG_PTR hwnd, const wchar_t *title, long *nret) {
    internal::set_result(nret, ::SetWindowTextW(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), title));
}

void op::Op::SetWindowTransparent(LONG_PTR hwnd, long trans, long *nret) {
    internal::set_result(nret,
                         m_context->window_service.SetWindowTransparent(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)),
                                                                        trans));
}

void op::Op::SendString(LONG_PTR hwnd, const wchar_t *str, long *ret) {
    internal::set_result(ret, m_context->window_service.SendString(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), str));
}

void op::Op::SendStringIme(LONG_PTR hwnd, const wchar_t *str, long *ret) {
    internal::set_result(ret,
                         m_context->window_service.SendStringIme(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), str));
}
