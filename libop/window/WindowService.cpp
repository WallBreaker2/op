// #include "stdafx.h"
#include "WindowService.h"

#include <utility>

namespace op {

namespace {
class WindowTextValue {
  public:
    WindowTextValue() = default;
    explicit WindowTextValue(std::wstring value) : value_(std::move(value)) {
    }

    operator const wchar_t *() const noexcept {
        return value_.c_str();
    }

  private:
    std::wstring value_;
};

WindowTextValue WindowClassNameText(HWND hwnd) {
    std::vector<wchar_t> buffer(256, L'\0');
    for (;;) {
        const int copied = ::GetClassNameW(hwnd, buffer.data(), static_cast<int>(buffer.size()));
        if (copied <= 0)
            return WindowTextValue();
        if (static_cast<size_t>(copied) < buffer.size() - 1)
            return WindowTextValue(std::wstring(buffer.data(), static_cast<size_t>(copied)));
        buffer.assign(buffer.size() * 2, L'\0');
    }
}

WindowTextValue WindowTitleText(HWND hwnd) {
    const int length = ::GetWindowTextLengthW(hwnd);
    if (length <= 0)
        return WindowTextValue();

    std::vector<wchar_t> buffer(static_cast<size_t>(length) + 1, L'\0');
    const int copied = ::GetWindowTextW(hwnd, buffer.data(), static_cast<int>(buffer.size()));
    if (copied <= 0)
        return WindowTextValue();

    return WindowTextValue(std::wstring(buffer.data(), static_cast<size_t>(copied)));
}

void AppendHwndText(std::wstring *retstring, int &retstringlen, HWND hwnd) {
    if (!retstring)
        return;

    const auto value = static_cast<unsigned long long>(reinterpret_cast<ULONG_PTR>(hwnd));
    if (!retstring->empty())
        retstring->push_back(L',');
    *retstring += std::to_wstring(value);
    retstringlen = static_cast<int>(retstring->size());
}

} // namespace

WindowService::WindowService(void) {
    retstringlen = 0;
    window_version = 0;
    enum_process_success_count = 0;
    npid.clear();
}

WindowService::~WindowService(void) {
}

HWND WindowService::FindChildWnd(HWND child_hwnd, const wchar_t *title, const wchar_t *classname, std::wstring *retstring,
                          bool isGW_OWNER, bool isVisible, const wchar_t *process_name) {
    child_hwnd = ::GetWindow(child_hwnd, GW_HWNDFIRST);
    while (child_hwnd != NULL) {
        if (isGW_OWNER) // 判断是否要匹配所有者窗口为0的窗口,即顶级窗口
            if (::GetWindow(child_hwnd, GW_OWNER) != 0) {
                child_hwnd = ::GetWindow(child_hwnd, GW_HWNDNEXT); // 获取下一个窗口
                continue;
            }

        if (isVisible) // 判断是否匹配可视窗口
            if (::IsWindowVisible(child_hwnd) == false) {
                child_hwnd = ::GetWindow(child_hwnd, GW_HWNDNEXT); // 获取下一个窗口
                continue;
            }
        if (title == NULL && classname == NULL) {
            if (process_name) {
                DWORD pid = 0;
                GetWindowThreadProcessId(child_hwnd, &pid);
                if (EnumProcessbyName(pid, process_name)) {
                    if (retstring)
                        AppendHwndText(retstring, retstringlen, child_hwnd);
                    else
                        return child_hwnd;
                }
            } else {
                if (retstring)
                    AppendHwndText(retstring, retstringlen, child_hwnd);
                else
                    return child_hwnd;
            }
        } else if (title != NULL && classname != NULL) {
            auto WindowClassName = WindowClassNameText(child_hwnd);
            auto WindowTitle = WindowTitleText(child_hwnd);
            if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                const wchar_t *strfindclass = wcsstr(WindowClassName, classname); // 模糊匹配
                const wchar_t *strfindtitle = wcsstr(WindowTitle, title);         // 模糊匹配
                if (strfindclass && strfindtitle) {
                    if (process_name) // EnumWindowByProcess
                    {
                        DWORD pid = 0;
                        GetWindowThreadProcessId(child_hwnd, &pid);
                        if (EnumProcessbyName(pid, process_name)) {
                            if (retstring)
                                AppendHwndText(retstring, retstringlen, child_hwnd);
                            else
                                return child_hwnd;
                        }
                    } else {
                        if (retstring)
                            AppendHwndText(retstring, retstringlen, child_hwnd);
                        else
                            return child_hwnd;
                    }
                }
            }

        } else if (title != NULL) {
            auto WindowTitle = WindowTitleText(child_hwnd);
            if (wcslen(WindowTitle) > 1) {
                const wchar_t *strfind = wcsstr(WindowTitle, title); // 模糊匹配
                if (strfind) {
                    if (process_name) // EnumWindowByProcess
                    {
                        DWORD pid = 0;
                        GetWindowThreadProcessId(child_hwnd, &pid);
                        if (EnumProcessbyName(pid, process_name)) {
                            if (retstring)
                                AppendHwndText(retstring, retstringlen, child_hwnd);
                            else
                                return child_hwnd;
                        }
                    } else {
                        if (retstring)
                            AppendHwndText(retstring, retstringlen, child_hwnd);
                        else
                            return child_hwnd;
                    }
                }
            }

        } else if (classname != NULL) {
            auto WindowClassName = WindowClassNameText(child_hwnd);
            if (wcslen(WindowClassName) > 1) {
                const wchar_t *strfind = wcsstr(WindowClassName, classname); // 模糊匹配
                if (strfind) {
                    if (process_name) // EnumWindowByProcess
                    {
                        DWORD pid = 0;
                        GetWindowThreadProcessId(child_hwnd, &pid);
                        if (EnumProcessbyName(pid, process_name)) {
                            if (retstring)
                                AppendHwndText(retstring, retstringlen, child_hwnd);
                            else
                                return child_hwnd;
                        }
                    } else {
                        if (retstring)
                            AppendHwndText(retstring, retstringlen, child_hwnd);
                        else
                            return child_hwnd;
                    }
                }
            }
        }

        HWND grandchild_hwnd = ::GetWindow(child_hwnd, GW_CHILD);
        if (grandchild_hwnd != NULL) {
            HWND dret = FindChildWnd(grandchild_hwnd, title, classname, retstring, isGW_OWNER, isVisible, process_name);
            if (dret != nullptr)
                break;
        }

        child_hwnd = ::GetWindow(child_hwnd, GW_HWNDNEXT); // 获取下一个窗口
    }
    return nullptr;
}

// EnumWindow: filter 为位掩码，各参数可以使用加法或按位或组合：
//
// 1  : 匹配窗口标题，使用参数 title 进行模糊匹配。
// 2  : 匹配窗口类名，使用参数 class_name 进行模糊匹配。
// 4  : 只匹配指定 parent 的第一层子窗口，不递归匹配后代窗口。
// 8  : 只匹配所有者窗口为 0 的窗口，即顶级窗口。
// 16 : 只匹配可见窗口。
// 32 : 按窗口 Z 序返回匹配结果，即按照窗口打开顺序排列。
//
// 例如：1 + 2 同时匹配窗口标题和类名；8 + 16 + 32 匹配可见的顶级窗口并按顺序返回。
bool WindowService::EnumWindow(HWND parent, const wchar_t *title, const wchar_t *class_name, LONG filter,
                               std::wstring &retstring, const wchar_t *process_name) {
    retstring.clear();
    return EnumWindowInternal(parent, title, class_name, filter, &retstring, process_name);
}

bool WindowService::EnumWindowInternal(HWND parent, const wchar_t *title, const wchar_t *class_name, LONG filter,
                                       std::wstring *retstring, const wchar_t *process_name) {
    constexpr LONG kMatchTitle = 1;
    constexpr LONG kMatchClass = 2;
    constexpr LONG kDirectChildrenOnly = 4;
    constexpr LONG kTopLevelOnly = 8;
    constexpr LONG kVisibleOnly = 16;
    constexpr LONG kZOrder = 32;
    constexpr LONG kSupportedFilters =
        kMatchTitle | kMatchClass | kDirectChildrenOnly | kTopLevelOnly | kVisibleOnly | kZOrder;

    // 过滤位只能使用已定义的选项，避免未知参数产生不明确的结果。
    if (filter < 0 || (filter & ~kSupportedFilters) != 0)
        return false;

    // 将位掩码拆成谓词，后续对所有窗口使用同一套匹配逻辑。
    const bool match_title = (filter & kMatchTitle) != 0;
    const bool match_class = (filter & kMatchClass) != 0;
    const bool direct_children_only = (filter & kDirectChildrenOnly) != 0;
    const bool top_level_only = (filter & kTopLevelOnly) != 0;
    const bool visible_only = (filter & kVisibleOnly) != 0;
    const wchar_t *title_filter = title ? title : L"";
    const wchar_t *class_filter = class_name ? class_name : L"";

    // 启用标题或类名筛选时，必须提供对应的非空参数。
    if ((match_title && wcslen(title_filter) < 1) || (match_class && wcslen(class_filter) < 1))
        return false;

    if (!parent)
        parent = ::GetDesktopWindow();

    // 进程名筛选前先收集匹配的进程 ID，后续窗口统一按进程判断。
    if (process_name) {
        if (wcslen(process_name) < 1)
            return false;
        npid.clear();
        enum_process_success_count = 0;
        if (!EnumProcessbyName(0, process_name))
            return false;
    }

    retstringlen = 0;
    bool found = false;

    // 统一判断窗口属性，避免为每一种筛选位组合复制遍历分支。
    const auto matches = [this, match_title, match_class, top_level_only, visible_only, title_filter, class_filter,
                          process_name](HWND hwnd) {
        if (top_level_only && ::GetWindow(hwnd, GW_OWNER) != nullptr)
            return false;
        if (visible_only && !::IsWindowVisible(hwnd))
            return false;

        if (match_title) {
            const auto window_title = WindowTitleText(hwnd);
            if (wcslen(window_title) <= 1 || !wcsstr(window_title, title_filter))
                return false;
        }

        if (match_class) {
            const auto window_class = WindowClassNameText(hwnd);
            if (wcslen(window_class) <= 1 || !wcsstr(window_class, class_filter))
                return false;
        }

        if (!process_name)
            return true;

        DWORD pid = 0;
        ::GetWindowThreadProcessId(hwnd, &pid);
        return EnumProcessbyName(pid, process_name) != FALSE;
    };

    // 按 Z 序遍历当前窗口的所有子窗口；未启用直接子窗口位时递归遍历后代。
    const auto visit_children = [&](auto &&self, HWND ancestor) -> void {
        HWND child = ::GetWindow(ancestor, GW_CHILD);
        if (!child)
            return;

        for (child = ::GetWindow(child, GW_HWNDFIRST); child; child = ::GetWindow(child, GW_HWNDNEXT)) {
            if (matches(child)) {
                AppendHwndText(retstring, retstringlen, child);
                found = true;
            }
            if (!direct_children_only)
                self(self, child);
        }
    };

    // GW_HWNDFIRST/GW_HWNDNEXT 提供筛选位 32 要求的窗口顺序。
    visit_children(visit_children, parent);
    return found;
}

bool WindowService::ClientToScreen(HWND hwnd, LONG &x, LONG &y) {
    POINT point;

    point.x = x;
    point.y = y;
    ::ClientToScreen(hwnd, &point);
    x = point.x;
    y = point.y;

    return true;
}
HWND WindowService::FindWindow(const wchar_t *class_name, const wchar_t *title) {
    if (class_name[0] == L'\0')
        class_name = nullptr;
    if (title[0] == L'\0')
        title = nullptr;
    return ::FindWindowW(class_name, title);
}

HWND WindowService::FindWindowEx(HWND parent, const wchar_t *class_name, const wchar_t *title) {
    if (class_name[0] == L'\0')
        class_name = nullptr;
    if (title[0] == L'\0')
        title = nullptr;
    return ::FindWindowExW(parent, NULL, class_name, title);
}

bool WindowService::FindWindowByProcess(const wchar_t *class_name, const wchar_t *title, HWND &rethwnd,
                                 const wchar_t *process_name, DWORD Pid) {
    bool bret = false;
    rethwnd = nullptr;
    if (process_name) {
        if (wcslen(process_name) < 1)
            return false;
        npid.clear();
        enum_process_success_count = 0;
        if (EnumProcessbyName(0, process_name) == false)
            return false;

        HWND p = ::GetWindow(GetDesktopWindow(), GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
                DWORD pid = 0;
                GetWindowThreadProcessId(p, &pid);
                if (EnumProcessbyName(pid, process_name)) {
                    if (wcslen(class_name) < 1 && wcslen(title) < 1) {
                        rethwnd = p;
                        bret = true;
                        break;
                    } else {
                        auto WindowClassName = WindowClassNameText(p);
                        auto WindowTitle = WindowTitleText(p);
                        if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                            const wchar_t *strfindclass = wcsstr(WindowClassName, class_name); // 模糊匹配
                            const wchar_t *strfindtitle = wcsstr(WindowTitle, title);          // 模糊匹配
                            if ((wcslen(class_name) >= 1 && strfindclass) || (wcslen(title) >= 1 && strfindtitle)) {
                                rethwnd = p;
                                bret = true;
                                break;
                            }
                        }

                        HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                        if (child_hwnd != NULL) {
                            const wchar_t *classname = NULL;
                            const wchar_t *titles = NULL;
                            if (wcslen(class_name) > 0)
                                classname = class_name;
                            if (wcslen(title) > 0)
                                titles = titles;
                            HWND dret = FindChildWnd(child_hwnd, titles, classname, NULL, false, false, process_name);
                            if (dret != nullptr) {
                                rethwnd = dret;
                                bret = true;
                                break;
                            }
                        }
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
    } else if (Pid > 0) {
        HWND p = ::GetWindow(GetDesktopWindow(), GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
                DWORD npid = 0;
                GetWindowThreadProcessId(p, &npid);
                if (Pid == npid) {
                    if (wcslen(class_name) < 1 && wcslen(title) < 1) {
                        rethwnd = p;
                        bret = true;
                        break;
                    } else {
                        auto WindowClassName = WindowClassNameText(p);
                        auto WindowTitle = WindowTitleText(p);
                        if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                            const wchar_t *strfindclass = wcsstr(WindowClassName, class_name); // 模糊匹配
                            const wchar_t *strfindtitle = wcsstr(WindowTitle, title);          // 模糊匹配
                            if ((wcslen(class_name) >= 1 && strfindclass) || (wcslen(title) >= 1 && strfindtitle)) {
                                rethwnd = p;
                                bret = true;
                                break;
                            }
                        }
                        HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                        if (child_hwnd != NULL) {
                            const wchar_t *classname = NULL;
                            const wchar_t *titles = NULL;
                            if (wcslen(class_name) > 0)
                                classname = class_name;
                            if (wcslen(title) > 0)
                                titles = titles;
                            HWND dret = FindChildWnd(child_hwnd, titles, classname, NULL, false, false, process_name);
                            if (dret != nullptr) {
                                rethwnd = dret;
                                bret = true;
                                break;
                            }
                        }
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
    }

    return bret;
}

} // namespace op
