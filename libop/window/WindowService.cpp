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

void ClearHwndText(std::wstring *retstring, int &retstringlen) {
    if (retstring)
        retstring->clear();
    retstringlen = 0;
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

// TSEnumWindow:filter整形数: 取值定义如下
//
// 1 : 匹配窗口标题,参数title有效
//
// 2 : 匹配窗口类名,参数class_name有效.
//
// 4 : 只匹配指定父窗口的第一层孩子窗口
//
// 8 : 匹配所有者窗口为0的窗口,即顶级窗口
//
// 16 : 匹配可见的窗口
//
// 32 : 匹配出的窗口按照窗口打开顺序依次排列
bool WindowService::EnumWindow(HWND parent, const wchar_t *title, const wchar_t *class_name, LONG filter,
                               std::wstring &retstring, const wchar_t *process_name) {
    retstring.clear();
    return EnumWindowInternal(parent, title, class_name, filter, &retstring, process_name);
}

bool WindowService::EnumWindowInternal(HWND parent, const wchar_t *title, const wchar_t *class_name, LONG filter,
                                       std::wstring *retstring, const wchar_t *process_name) {
    bool bret = false;
    bool bZwindow = false; // 匹配出的窗口按照窗口打开顺序依次排列
    if (parent == 0) {
        parent = GetDesktopWindow();
    }
    if (filter > 32) {
        bZwindow = true; // 说明要排序窗口句柄
        filter = filter - 32;
    }

    int indexpid = 0;
    if (process_name) // EnumWindowByProcess
    {
        if (wcslen(process_name) < 1)
            return false;
        npid.clear();
        enum_process_success_count = 0;
        if (EnumProcessbyName(0, process_name) == false)
            return false;
    }

    DWORD processpid = 0;
    retstringlen = 0;
    switch (filter) {
    case 0: // 所有模式
    {
        if (process_name) // EnumWindowByProcess
        {
            return false;
        }

        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        if (p == NULL)
            return false;
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            AppendHwndText(retstring, retstringlen, p);
            bret = true;
            HWND child_hwnd = ::GetWindow(p, GW_CHILD);
            if (child_hwnd != NULL) {
                FindChildWnd(child_hwnd, NULL, NULL, retstring);
            }

            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 1: // 1 : 匹配窗口标题,参数title有效
    {
        if (wcslen(title) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        if (p == NULL)
            return false;
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            auto WindowTitle = WindowTitleText(p);
            if (wcslen(WindowTitle) > 1) {
                const wchar_t *strfind = wcsstr(WindowTitle, title); // 模糊匹配
                if (strfind) {
                    if (process_name) // EnumWindowByProcess
                    {
                        DWORD pid = 0;
                        GetWindowThreadProcessId(p, &pid);
                        if (EnumProcessbyName(pid, process_name)) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    } else {
                        AppendHwndText(retstring, retstringlen, p);
                        bret = true;
                        HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                        if (child_hwnd != NULL) {
                            FindChildWnd(child_hwnd, title, NULL, retstring);
                        }
                    }
                }
            }

            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 2: // 2 : 匹配窗口类名,参数class_name有效.
    {
        if (wcslen(class_name) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        if (p == NULL)
            return false;
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            auto WindowClassName = WindowClassNameText(p);
            if (wcslen(WindowClassName) > 1) {
                const wchar_t *strfind = wcsstr(WindowClassName, class_name); // 模糊匹配
                if (strfind) {
                    if (process_name) // EnumWindowByProcess
                    {
                        DWORD pid = 0;
                        GetWindowThreadProcessId(p, &pid);
                        if (EnumProcessbyName(pid, process_name)) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    } else {
                        AppendHwndText(retstring, retstringlen, p);
                        bret = true;
                        HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                        if (child_hwnd != NULL) {
                            FindChildWnd(child_hwnd, NULL, class_name, retstring);
                        }
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 3: // 1.窗口标题+2.窗口类名
    {
        if (wcslen(class_name) < 1 && wcslen(title) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        if (p == NULL)
            return false;
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            auto WindowClassName = WindowClassNameText(p);
            auto WindowTitle = WindowTitleText(p);
            if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                const wchar_t *strfindclass = wcsstr(WindowClassName, class_name); // 模糊匹配
                const wchar_t *strfindtitle = wcsstr(WindowTitle, title);          // 模糊匹配
                if (strfindclass && strfindtitle) {
                    if (process_name) // EnumWindowByProcess
                    {
                        DWORD pid = 0;
                        GetWindowThreadProcessId(p, &pid);
                        if (EnumProcessbyName(pid, process_name)) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    } else {
                        AppendHwndText(retstring, retstringlen, p);
                        bret = true;
                        HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                        if (child_hwnd != NULL) {
                            FindChildWnd(child_hwnd, title, class_name, retstring);
                        }
                    }
                }
            }

            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 4: // 4 : 只匹配指定父窗口的第一层孩子窗口
    {
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        if (p == NULL)
            return false;
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (process_name) // EnumWindowByProcess
            {
                DWORD pid = 0;
                GetWindowThreadProcessId(p, &pid);
                if (EnumProcessbyName(pid, process_name)) {
                    if (processpid != pid) // 只匹配指定映像的所对应的第一个进程.
                                           // 可能有很多同映像名的进程，只匹配第一个进程的.
                    {
                        if (indexpid < enum_process_success_count) {
                            indexpid++;
                            processpid = pid;
                            ClearHwndText(retstring, retstringlen); // 清空返回字符串
                        }
                    }
                    if (processpid == pid) {
                        AppendHwndText(retstring, retstringlen, p);
                        bret = true;
                        HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                        if (child_hwnd != NULL) {
                            FindChildWnd(child_hwnd, NULL, NULL, retstring, false, false, process_name);
                        }
                    }
                }
            } else {
                AppendHwndText(retstring, retstringlen, p);
                bret = true;
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 5: // 1.匹配窗口标题+//4 : 只匹配指定父窗口的第一层孩子窗口
    {
        if (wcslen(title) < 1)
            return false;

        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        if (p == NULL)
            return false;
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (process_name) // EnumWindowByProcess
            {
                DWORD pid = 0;
                GetWindowThreadProcessId(p, &pid);
                if (EnumProcessbyName(pid, process_name)) {
                    if (processpid != pid) // 只匹配指定映像的所对应的第一个进程.
                                           // 可能有很多同映像名的进程，只匹配第一个进程的.
                    {
                        if (indexpid < enum_process_success_count) {
                            indexpid++;
                            processpid = pid;
                            ClearHwndText(retstring, retstringlen); // 清空返回字符串
                        }
                    }
                    if (processpid == pid) {
                        auto WindowTitle = WindowTitleText(p);
                        if (wcslen(WindowTitle) > 1) {
                            if (wcsstr(WindowTitle, title)) {
                                AppendHwndText(retstring, retstringlen, p);
                                bret = true;
                            }
                        }
                        HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                        if (child_hwnd != NULL) {
                            FindChildWnd(child_hwnd, title, NULL, retstring, false, false, process_name);
                        }
                    }
                }

            } else {
                auto WindowTitle = WindowTitleText(p);
                if (wcslen(WindowTitle) > 1) {
                    if (wcsstr(WindowTitle, title)) {
                        AppendHwndText(retstring, retstringlen, p);
                        bret = true;
                    }
                }
            }

            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 6: // 2 : 匹配窗口类名+4 : 只匹配指定父窗口的第一层孩子窗口
    {
        if (wcslen(class_name) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        if (p == NULL)
            return false;
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (process_name) // EnumWindowByProcess
            {
                DWORD pid = 0;
                GetWindowThreadProcessId(p, &pid);
                if (EnumProcessbyName(pid, process_name)) {
                    if (indexpid < enum_process_success_count) {
                        indexpid++;
                        processpid = pid;
                        ClearHwndText(retstring, retstringlen); // 清空返回字符串
                    }
                }
                if (processpid == pid) {
                    auto WindowClassName = WindowClassNameText(p);
                    if (wcslen(WindowClassName) > 1) {
                        if (wcsstr(WindowClassName, class_name)) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }
                    HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                    if (child_hwnd != NULL) {
                        FindChildWnd(child_hwnd, NULL, class_name, retstring, false, false, process_name);
                    }
                }
            } else {
                auto WindowClassName = WindowClassNameText(p);
                if (wcslen(WindowClassName) > 1) {
                    if (wcsstr(WindowClassName, class_name)) {
                        AppendHwndText(retstring, retstringlen, p);
                        bret = true;
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 7: // 1.窗口标题+2.窗口类名+4 : 只匹配指定父窗口的第一层孩子窗口
    {
        if (wcslen(class_name) < 1 && wcslen(title) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        if (p == NULL)
            return false;
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (process_name) // EnumWindowByProcess
            {
                DWORD pid = 0;
                GetWindowThreadProcessId(p, &pid);
                if (EnumProcessbyName(pid, process_name)) {
                    if (indexpid < enum_process_success_count) {
                        indexpid++;
                        processpid = pid;
                        ClearHwndText(retstring, retstringlen); // 清空返回字符串
                    }
                }
                if (processpid == pid) {
                    auto WindowClassName = WindowClassNameText(p);
                    auto WindowTitle = WindowTitleText(p);
                    if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                        const wchar_t *strfindclass = wcsstr(WindowClassName, class_name); // 模糊匹配
                        const wchar_t *strfindtitle = wcsstr(WindowTitle, title);          // 模糊匹配
                        if (strfindclass && strfindtitle) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }
                    HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                    if (child_hwnd != NULL) {
                        FindChildWnd(child_hwnd, title, class_name, retstring, false, false, process_name);
                    }
                }
            } else {
                auto WindowClassName = WindowClassNameText(p);
                auto WindowTitle = WindowTitleText(p);
                if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                    const wchar_t *strfindclass = wcsstr(WindowClassName, class_name); // 模糊匹配
                    const wchar_t *strfindtitle = wcsstr(WindowTitle, title);          // 模糊匹配
                    if (strfindclass && strfindtitle) {
                        AppendHwndText(retstring, retstringlen, p);
                        bret = true;
                    }
                }
            }

            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 8: // 8 : 匹配所有者窗口为0的窗口,即顶级窗口
    {
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        if (p == NULL)
            return false;
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::GetWindow(p, GW_OWNER) == 0) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                        AppendHwndText(retstring, retstringlen, p);
                        bret = true;
                        HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                        if (child_hwnd != NULL) {
                            FindChildWnd(child_hwnd, NULL, NULL, retstring, true, false, process_name);
                        }
                    }
                } else {
                    AppendHwndText(retstring, retstringlen, p);
                    bret = true;
                    HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                    if (child_hwnd != NULL) {
                        FindChildWnd(child_hwnd, NULL, NULL, retstring, true);
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 9: // 1.窗口标题+8 : 匹配所有者窗口为0的窗口,即顶级窗口
    {
        if (wcslen(title) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        if (p == NULL)
            return false;
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::GetWindow(p, GW_OWNER) == 0) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                        auto WindowTitle = WindowTitleText(p);
                        if (wcslen(WindowTitle) > 1) {
                            const wchar_t *strfind = wcsstr(WindowTitle, title); // 模糊匹配
                            if (strfind) {
                                AppendHwndText(retstring, retstringlen, p);
                                bret = true;
                            }
                        }
                        HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                        if (child_hwnd != NULL) {
                            FindChildWnd(child_hwnd, title, NULL, retstring, true, false, process_name);
                        }
                    }
                } else {
                    auto WindowTitle = WindowTitleText(p);
                    if (wcslen(WindowTitle) > 1) {
                        const wchar_t *strfind = wcsstr(WindowTitle, title); // 模糊匹配
                        if (strfind) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }
                    HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                    if (child_hwnd != NULL) {
                        FindChildWnd(child_hwnd, title, NULL, retstring, true);
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 10: // 2.窗口类名+8 : 匹配所有者窗口为0的窗口,即顶级窗口
    {
        if (wcslen(class_name) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        if (p == NULL)
            return false;
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::GetWindow(p, GW_OWNER) == 0) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                        auto WindowClassName = WindowClassNameText(p);
                        if (wcslen(WindowClassName) > 1) {
                            const wchar_t *strfind = wcsstr(WindowClassName, class_name); // 模糊匹配
                            if (strfind) {
                                AppendHwndText(retstring, retstringlen, p);
                                bret = true;
                            }
                        }
                        HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                        if (child_hwnd != NULL) {
                            FindChildWnd(child_hwnd, NULL, class_name, retstring, true, false, process_name);
                        }
                    }
                } else {
                    auto WindowClassName = WindowClassNameText(p);
                    if (wcslen(WindowClassName) > 1) {
                        const wchar_t *strfind = wcsstr(WindowClassName, class_name); // 模糊匹配
                        if (strfind) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }

                    HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                    if (child_hwnd != NULL) {
                        FindChildWnd(child_hwnd, NULL, class_name, retstring, true);
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 11: ////1.窗口标题+2.窗口类名+8 : 匹配所有者窗口为0的窗口,即顶级窗口
    {
        if (wcslen(class_name) < 1 && wcslen(title) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        if (p == NULL)
            return false;
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::GetWindow(p, GW_OWNER) == 0) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                        auto WindowClassName = WindowClassNameText(p);
                        auto WindowTitle = WindowTitleText(p);
                        if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                            const wchar_t *strfindclass = wcsstr(WindowClassName, class_name); // 模糊匹配
                            const wchar_t *strfindtitle = wcsstr(WindowTitle, title);          // 模糊匹配
                            if (strfindclass && strfindtitle) {
                                AppendHwndText(retstring, retstringlen, p);
                                bret = true;
                            }
                        }
                        HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                        if (child_hwnd != NULL) {
                            FindChildWnd(child_hwnd, title, class_name, retstring, true, false, process_name);
                        }
                    }
                } else {
                    auto WindowClassName = WindowClassNameText(p);
                    auto WindowTitle = WindowTitleText(p);
                    if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                        const wchar_t *strfindclass = wcsstr(WindowClassName, class_name); // 模糊匹配
                        const wchar_t *strfindtitle = wcsstr(WindowTitle, title);          // 模糊匹配
                        if (strfindclass && strfindtitle) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }
                    HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                    if (child_hwnd != NULL) {
                        FindChildWnd(child_hwnd, title, class_name, retstring, true);
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 12: // 4 : 只匹配指定父窗口的第一层孩子窗口+8 :
             // 匹配所有者窗口为0的窗口,即顶级窗口
    {
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::GetWindow(p, GW_OWNER) == 0) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                    if (processpid != pid) // 只匹配指定映像的所对应的第一个进程.
                                               // 可能有很多同映像名的进程，只匹配第一个进程的.
                        {
                            if (indexpid < enum_process_success_count) {
                                indexpid++;
                                processpid = pid;
                                ClearHwndText(retstring, retstringlen); // 清空返回字符串
                            }
                        }
                        if (processpid == pid) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                            HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                            if (child_hwnd != NULL) {
                                FindChildWnd(child_hwnd, NULL, NULL, retstring, true, false, process_name);
                            }
                        }
                    }
                } else {
                    AppendHwndText(retstring, retstringlen, p);
                    bret = true;
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 13: // 1.窗口标题+4 : 只匹配指定父窗口的第一层孩子窗口+8 :
             // 匹配所有者窗口为0的窗口,即顶级窗口
    {
        if (wcslen(title) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::GetWindow(p, GW_OWNER) == 0) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                    if (processpid != pid) // 只匹配指定映像的所对应的第一个进程.
                                               // 可能有很多同映像名的进程，只匹配第一个进程的.
                        {
                            if (indexpid < enum_process_success_count) {
                                indexpid++;
                                processpid = pid;
                                ClearHwndText(retstring, retstringlen); // 清空返回字符串
                            }
                        }
                        if (processpid == pid) {
                            auto WindowTitle = WindowTitleText(p);
                            if (wcslen(WindowTitle) > 1) {
                                const wchar_t *strfind = wcsstr(WindowTitle, title); // 模糊匹配
                                if (strfind) {
                                    AppendHwndText(retstring, retstringlen, p);
                                    bret = true;
                                }
                            }
                            HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                            if (child_hwnd != NULL) {
                                FindChildWnd(child_hwnd, title, NULL, retstring, true, false, process_name);
                            }
                        }
                    }
                } else {
                    auto WindowTitle = WindowTitleText(p);
                    if (wcslen(WindowTitle) > 1) {
                        const wchar_t *strfind = wcsstr(WindowTitle, title); // 模糊匹配
                        if (strfind) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 14: // 2.窗口类名+4 : 只匹配指定父窗口的第一层孩子窗口+8 :
             // 匹配所有者窗口为0的窗口,即顶级窗口
    {
        if (wcslen(class_name) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::GetWindow(p, GW_OWNER) == 0) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                    if (processpid != pid) // 只匹配指定映像的所对应的第一个进程.
                                               // 可能有很多同映像名的进程，只匹配第一个进程的.
                        {
                            if (indexpid < enum_process_success_count) {
                                indexpid++;
                                processpid = pid;
                                ClearHwndText(retstring, retstringlen); // 清空返回字符串
                            }
                        }
                        if (processpid == pid) {
                            auto WindowClassName = WindowClassNameText(p);
                            if (wcslen(WindowClassName) > 1) {
                                const wchar_t *strfind = wcsstr(WindowClassName, class_name); // 模糊匹配
                                if (strfind) {
                                    AppendHwndText(retstring, retstringlen, p);
                                    bret = true;
                                }
                            }
                            HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                            if (child_hwnd != NULL) {
                                FindChildWnd(child_hwnd, NULL, class_name, retstring, true, false, process_name);
                            }
                        }
                    }
                } else {
                    auto WindowClassName = WindowClassNameText(p);
                    if (wcslen(WindowClassName) > 1) {
                        const wchar_t *strfind = wcsstr(WindowClassName, class_name); // 模糊匹配
                        if (strfind) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 15: ////1.窗口标题+2.窗口类名+4 : 只匹配指定父窗口的第一层孩子窗口+8 :
             // 匹配所有者窗口为0的窗口,即顶级窗口
    {
        if (wcslen(class_name) < 1 && wcslen(title) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::GetWindow(p, GW_OWNER) == 0) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                    if (processpid != pid) // 只匹配指定映像的所对应的第一个进程.
                                               // 可能有很多同映像名的进程，只匹配第一个进程的.
                        {
                            if (indexpid < enum_process_success_count) {
                                indexpid++;
                                processpid = pid;
                                ClearHwndText(retstring, retstringlen); // 清空返回字符串
                            }
                        }
                        if (processpid == pid) {
                            auto WindowClassName = WindowClassNameText(p);
                            auto WindowTitle = WindowTitleText(p);
                            if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                                const wchar_t *strfindclass = wcsstr(WindowClassName, class_name); // 模糊匹配
                                const wchar_t *strfindtitle = wcsstr(WindowTitle, title);          // 模糊匹配
                                if (strfindclass && strfindtitle) {
                                    AppendHwndText(retstring, retstringlen, p);
                                    bret = true;
                                }
                            }
                            HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                            if (child_hwnd != NULL) {
                                FindChildWnd(child_hwnd, title, class_name, retstring, true, false, process_name);
                            }
                        }
                    }
                } else {
                    auto WindowClassName = WindowClassNameText(p);
                    auto WindowTitle = WindowTitleText(p);
                    if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                        const wchar_t *strfindclass = wcsstr(WindowClassName, class_name); // 模糊匹配
                        const wchar_t *strfindtitle = wcsstr(WindowTitle, title);          // 模糊匹配
                        if (strfindclass && strfindtitle) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 16: // 匹配可见的窗口
    {
        parent = GetDesktopWindow();
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::IsWindowVisible(p)) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                        AppendHwndText(retstring, retstringlen, p);
                        bret = true;
                        HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                        if (child_hwnd != NULL) {
                            FindChildWnd(child_hwnd, NULL, NULL, retstring, false, true, process_name);
                        }
                    }
                } else {
                    AppendHwndText(retstring, retstringlen, p);
                    bret = true;
                    HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                    if (child_hwnd != NULL) {
                        FindChildWnd(child_hwnd, NULL, NULL, retstring, false, true);
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 17: // 1.窗口标题+//匹配可见的窗口
    {
        if (wcslen(title) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::IsWindowVisible(p)) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                        auto WindowTitle = WindowTitleText(p);
                        if (wcslen(WindowTitle) > 1) {
                            const wchar_t *strfind = wcsstr(WindowTitle, title); // 模糊匹配
                            if (strfind) {
                                AppendHwndText(retstring, retstringlen, p);
                                bret = true;
                            }
                        }
                        HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                        if (child_hwnd != NULL) {
                            FindChildWnd(child_hwnd, title, NULL, retstring, false, true, process_name);
                        }
                    }
                } else {
                    auto WindowTitle = WindowTitleText(p);
                    if (wcslen(WindowTitle) > 1) {
                        const wchar_t *strfind = wcsstr(WindowTitle, title); // 模糊匹配
                        if (strfind) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }
                    HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                    if (child_hwnd != NULL) {
                        FindChildWnd(child_hwnd, title, NULL, retstring, false, true);
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 18: // 2.窗口类名+//匹配可见的窗口
    {
        if (wcslen(class_name) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::IsWindowVisible(p)) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                        auto WindowClassName = WindowClassNameText(p);
                        if (wcslen(WindowClassName) > 1) {
                            const wchar_t *strfind = wcsstr(WindowClassName, class_name); // 模糊匹配
                            if (strfind) {
                                AppendHwndText(retstring, retstringlen, p);
                                bret = true;
                            }
                        }
                        HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                        if (child_hwnd != NULL) {
                            FindChildWnd(child_hwnd, NULL, class_name, retstring, false, true, process_name);
                        }
                    }
                } else {
                    auto WindowClassName = WindowClassNameText(p);
                    if (wcslen(WindowClassName) > 1) {
                        const wchar_t *strfind = wcsstr(WindowClassName, class_name); // 模糊匹配
                        if (strfind) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }
                    HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                    if (child_hwnd != NULL) {
                        FindChildWnd(child_hwnd, NULL, class_name, retstring, false, true);
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 19: ////1.窗口标题+2.窗口类名+匹配可见的窗口
    {
        if (wcslen(class_name) < 1 && wcslen(title) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::IsWindowVisible(p)) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                        auto WindowClassName = WindowClassNameText(p);
                        auto WindowTitle = WindowTitleText(p);
                        if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                            const wchar_t *strfindclass = wcsstr(WindowClassName, class_name); // 模糊匹配
                            const wchar_t *strfindtitle = wcsstr(WindowTitle, title);          // 模糊匹配
                            if (strfindclass && strfindtitle) {
                                AppendHwndText(retstring, retstringlen, p);
                                bret = true;
                            }
                        }
                        HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                        if (child_hwnd != NULL) {
                            FindChildWnd(child_hwnd, title, class_name, retstring, false, true, process_name);
                        }
                    }
                } else {
                    auto WindowClassName = WindowClassNameText(p);
                    auto WindowTitle = WindowTitleText(p);
                    if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                        const wchar_t *strfindclass = wcsstr(WindowClassName, class_name); // 模糊匹配
                        const wchar_t *strfindtitle = wcsstr(WindowTitle, title);          // 模糊匹配
                        if (strfindclass && strfindtitle) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }
                    HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                    if (child_hwnd != NULL) {
                        FindChildWnd(child_hwnd, title, class_name, retstring, false, true);
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 20: // 4 : 只匹配指定父窗口的第一层孩子窗口+匹配可见的窗口
    {
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::IsWindowVisible(p)) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                    if (processpid != pid) // 只匹配指定映像的所对应的第一个进程.
                                               // 可能有很多同映像名的进程，只匹配第一个进程的.
                        {
                            if (indexpid < enum_process_success_count) {
                                indexpid++;
                                processpid = pid;
                                ClearHwndText(retstring, retstringlen); // 清空返回字符串
                            }
                        }
                        if (processpid == pid) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                            HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                            if (child_hwnd != NULL) {
                                FindChildWnd(child_hwnd, NULL, NULL, retstring, false, true, process_name);
                            }
                        }
                    }
                } else {
                    AppendHwndText(retstring, retstringlen, p);
                    bret = true;
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 21: // 1.窗口标题+4 : 只匹配指定父窗口的第一层孩子窗口+匹配可见的窗口
    {
        if (wcslen(title) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::IsWindowVisible(p)) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                    if (processpid != pid) // 只匹配指定映像的所对应的第一个进程.
                                               // 可能有很多同映像名的进程，只匹配第一个进程的.
                        {
                            if (indexpid < enum_process_success_count) {
                                indexpid++;
                                processpid = pid;
                                ClearHwndText(retstring, retstringlen); // 清空返回字符串
                            }
                        }
                        if (processpid == pid) {
                            auto WindowTitle = WindowTitleText(p);
                            if (wcslen(WindowTitle) > 1) {
                                const wchar_t *strfind = wcsstr(WindowTitle, title); // 模糊匹配
                                if (strfind) {
                                    AppendHwndText(retstring, retstringlen, p);
                                    bret = true;
                                }
                            }
                            HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                            if (child_hwnd != NULL) {
                                FindChildWnd(child_hwnd, title, NULL, retstring, false, true, process_name);
                            }
                        }
                    }
                } else {
                    auto WindowTitle = WindowTitleText(p);
                    if (wcslen(WindowTitle) > 1) {
                        const wchar_t *strfind = wcsstr(WindowTitle, title); // 模糊匹配
                        if (strfind) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 22: // 2.窗口类名+4 : 只匹配指定父窗口的第一层孩子窗口+匹配可见的窗口
    {
        if (wcslen(class_name) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::IsWindowVisible(p)) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                    if (processpid != pid) // 只匹配指定映像的所对应的第一个进程.
                                               // 可能有很多同映像名的进程，只匹配第一个进程的.
                        {
                            if (indexpid < enum_process_success_count) {
                                indexpid++;
                                processpid = pid;
                                ClearHwndText(retstring, retstringlen); // 清空返回字符串
                            }
                        }
                        if (processpid == pid) {
                            auto WindowClassName = WindowClassNameText(p);
                            if (wcslen(WindowClassName) > 1) {
                                const wchar_t *strfind = wcsstr(WindowClassName, class_name); // 模糊匹配
                                if (strfind) {
                                    AppendHwndText(retstring, retstringlen, p);
                                    bret = true;
                                }
                            }
                            HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                            if (child_hwnd != NULL) {
                                FindChildWnd(child_hwnd, NULL, class_name, retstring, false, true, process_name);
                            }
                        }
                    }
                } else {
                    auto WindowClassName = WindowClassNameText(p);
                    if (wcslen(WindowClassName) > 1) {
                        const wchar_t *strfind = wcsstr(WindowClassName, class_name); // 模糊匹配
                        if (strfind) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 23: // 1.窗口标题+2.窗口类名+4 :
             // 只匹配指定父窗口的第一层孩子窗口+16:匹配可见的窗口
    {
        if (wcslen(class_name) < 1 && wcslen(title) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::IsWindowVisible(p)) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                    if (processpid != pid) // 只匹配指定映像的所对应的第一个进程.
                                               // 可能有很多同映像名的进程，只匹配第一个进程的.
                        {
                            if (indexpid < enum_process_success_count) {
                                indexpid++;
                                processpid = pid;
                                ClearHwndText(retstring, retstringlen); // 清空返回字符串
                            }
                        }
                        if (processpid == pid) {
                            auto WindowClassName = WindowClassNameText(p);
                            auto WindowTitle = WindowTitleText(p);
                            if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                                const wchar_t *strfindclass = wcsstr(WindowClassName, class_name); // 模糊匹配
                                const wchar_t *strfindtitle = wcsstr(WindowTitle, title);          // 模糊匹配
                                if (strfindclass && strfindtitle) {
                                    AppendHwndText(retstring, retstringlen, p);
                                    bret = true;
                                }
                            }
                            HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                            if (child_hwnd != NULL) {
                                FindChildWnd(child_hwnd, title, class_name, retstring, false, true, process_name);
                            }
                        }
                    }
                } else {
                    auto WindowClassName = WindowClassNameText(p);
                    auto WindowTitle = WindowTitleText(p);
                    if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                        const wchar_t *strfindclass = wcsstr(WindowClassName, class_name); // 模糊匹配
                        const wchar_t *strfindtitle = wcsstr(WindowTitle, title);          // 模糊匹配
                        if (strfindclass && strfindtitle) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 24: // 8 : 匹配所有者窗口为0的窗口,即顶级窗口+16.匹配可见的窗口
    {
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                        AppendHwndText(retstring, retstringlen, p);
                        bret = true;
                        HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                        if (child_hwnd != NULL) {
                            FindChildWnd(child_hwnd, NULL, NULL, retstring, true, true, process_name);
                        }
                    }
                } else {
                    AppendHwndText(retstring, retstringlen, p);
                    bret = true;

                    HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                    if (child_hwnd != NULL) {
                        FindChildWnd(child_hwnd, NULL, NULL, retstring, true, true);
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 25: // 1.窗口标题+
             // 8:匹配所有者窗口为0的窗口,即顶级窗口+16:匹配可见的窗口
    {
        if (wcslen(title) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                        auto WindowTitle = WindowTitleText(p);
                        if (wcslen(WindowTitle) > 1) {
                            const wchar_t *strfind = wcsstr(WindowTitle, title); // 模糊匹配
                            if (strfind) {
                                AppendHwndText(retstring, retstringlen, p);
                                bret = true;
                            }
                        }
                        HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                        if (child_hwnd != NULL) {
                            FindChildWnd(child_hwnd, title, NULL, retstring, true, true, process_name);
                        }
                    }
                } else {
                    auto WindowTitle = WindowTitleText(p);
                    if (wcslen(WindowTitle) > 1) {
                        const wchar_t *strfind = wcsstr(WindowTitle, title); // 模糊匹配
                        if (strfind) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }
                    HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                    if (child_hwnd != NULL) {
                        FindChildWnd(child_hwnd, title, NULL, retstring, true, true);
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 26: // 2.窗口类名+
             // 8:匹配所有者窗口为0的窗口,即顶级窗口+16:匹配可见的窗口
    {
        if (wcslen(class_name) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                        auto WindowClassName = WindowClassNameText(p);
                        if (wcslen(WindowClassName) > 1) {
                            const wchar_t *strfind = wcsstr(WindowClassName, class_name); // 模糊匹配
                            if (strfind) {
                                AppendHwndText(retstring, retstringlen, p);
                                bret = true;
                            }
                        }
                        HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                        if (child_hwnd != NULL) {
                            FindChildWnd(child_hwnd, NULL, class_name, retstring, true, true, process_name);
                        }
                    }
                } else {
                    auto WindowClassName = WindowClassNameText(p);
                    if (wcslen(WindowClassName) > 1) {
                        const wchar_t *strfind = wcsstr(WindowClassName, class_name); // 模糊匹配
                        if (strfind) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }
                    HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                    if (child_hwnd != NULL) {
                        FindChildWnd(child_hwnd, NULL, class_name, retstring, true, true);
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 27: // 1.窗口标题+2.窗口类名+8:匹配所有者窗口为0的窗口,即顶级窗口+16.匹配可见的窗口
    {
        if (wcslen(class_name) < 1 && wcslen(title) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                        auto WindowClassName = WindowClassNameText(p);
                        auto WindowTitle = WindowTitleText(p);
                        if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                            const wchar_t *strfindclass = wcsstr(WindowClassName, class_name); // 模糊匹配
                            const wchar_t *strfindtitle = wcsstr(WindowTitle, title);          // 模糊匹配
                            if (strfindclass && strfindtitle) {
                                AppendHwndText(retstring, retstringlen, p);
                                bret = true;
                            }
                        }
                        HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                        if (child_hwnd != NULL) {
                            FindChildWnd(child_hwnd, title, class_name, retstring, true, true, process_name);
                        }
                    }
                } else {
                    auto WindowClassName = WindowClassNameText(p);
                    auto WindowTitle = WindowTitleText(p);
                    if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                        const wchar_t *strfindclass = wcsstr(WindowClassName, class_name); // 模糊匹配
                        const wchar_t *strfindtitle = wcsstr(WindowTitle, title);          // 模糊匹配
                        if (strfindclass && strfindtitle) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }
                    HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                    if (child_hwnd != NULL) {
                        FindChildWnd(child_hwnd, title, class_name, retstring, true, true);
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 28: // 4 :
             // 只匹配指定父窗口的第一层孩子窗口+8:匹配所有者窗口为0的窗口,即顶级窗口+16:匹配可见的窗口
    {
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                    if (processpid != pid) // 只匹配指定映像的所对应的第一个进程.
                                               // 可能有很多同映像名的进程，只匹配第一个进程的.
                        {
                            if (indexpid < enum_process_success_count) {
                                indexpid++;
                                processpid = pid;
                                ClearHwndText(retstring, retstringlen); // 清空返回字符串
                            }
                        }
                        if (processpid == pid) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                            HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                            if (child_hwnd != NULL) {
                                FindChildWnd(child_hwnd, NULL, NULL, retstring, true, true, process_name);
                            }
                        }
                    }
                } else {
                    AppendHwndText(retstring, retstringlen, p);
                    bret = true;
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 29: ////1.窗口标题+4 :
             // 只匹配指定父窗口的第一层孩子窗口+8:匹配所有者窗口为0的窗口,即顶级窗口+16:匹配可见的窗口
    {
        if (wcslen(title) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                    if (processpid != pid) // 只匹配指定映像的所对应的第一个进程.
                                               // 可能有很多同映像名的进程，只匹配第一个进程的.
                        {
                            if (indexpid < enum_process_success_count) {
                                indexpid++;
                                processpid = pid;
                                ClearHwndText(retstring, retstringlen); // 清空返回字符串
                            }
                        }
                        if (processpid == pid) {
                            auto WindowTitle = WindowTitleText(p);
                            if (wcslen(WindowTitle) > 1) {
                                const wchar_t *strfind = wcsstr(WindowTitle, title); // 模糊匹配
                                if (strfind) {
                                    AppendHwndText(retstring, retstringlen, p);
                                    bret = true;
                                }
                            }
                            HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                            if (child_hwnd != NULL) {
                                FindChildWnd(child_hwnd, title, NULL, retstring, true, true, process_name);
                            }
                        }
                    }
                } else {
                    auto WindowTitle = WindowTitleText(p);
                    if (wcslen(WindowTitle) > 1) {
                        const wchar_t *strfind = wcsstr(WindowTitle, title); // 模糊匹配
                        if (strfind) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 30: // 2.窗口类名+4 :
             // 只匹配指定父窗口的第一层孩子窗口+8:匹配所有者窗口为0的窗口,即顶级窗口+16:匹配可见的窗口
    {
        if (wcslen(class_name) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                    if (processpid != pid) // 只匹配指定映像的所对应的第一个进程.
                                               // 可能有很多同映像名的进程，只匹配第一个进程的.
                        {
                            if (indexpid < enum_process_success_count) {
                                indexpid++;
                                processpid = pid;
                                ClearHwndText(retstring, retstringlen); // 清空返回字符串
                            }
                        }
                        if (processpid == pid) {
                            auto WindowClassName = WindowClassNameText(p);
                            if (wcslen(WindowClassName) > 1) {
                                const wchar_t *strfind = wcsstr(WindowClassName, class_name); // 模糊匹配
                                if (strfind) {
                                    AppendHwndText(retstring, retstringlen, p);
                                    bret = true;
                                }
                            }
                            HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                            if (child_hwnd != NULL) {
                                FindChildWnd(child_hwnd, NULL, class_name, retstring, true, true, process_name);
                            }
                        }
                    }
                } else {
                    auto WindowClassName = WindowClassNameText(p);
                    if (wcslen(WindowClassName) > 1) {
                        const wchar_t *strfind = wcsstr(WindowClassName, class_name); // 模糊匹配
                        if (strfind) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    case 31: // 1.窗口标题+2.窗口类名+4 :
             // 只匹配指定父窗口的第一层孩子窗口+8:匹配所有者窗口为0的窗口,即顶级窗口+16.匹配可见的窗口
    {
        if (wcslen(class_name) < 1 && wcslen(title) < 1)
            return false;
        HWND p = ::GetWindow(parent, GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
                if (process_name) // EnumWindowByProcess
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(p, &pid);
                    if (EnumProcessbyName(pid, process_name)) {
                    if (processpid != pid) // 只匹配指定映像的所对应的第一个进程.
                                               // 可能有很多同映像名的进程，只匹配第一个进程的.
                        {
                            if (indexpid < enum_process_success_count) {
                                indexpid++;
                                processpid = pid;
                                ClearHwndText(retstring, retstringlen); // 清空返回字符串
                            }
                        }
                        if (processpid == pid) {
                            auto WindowClassName = WindowClassNameText(p);
                            auto WindowTitle = WindowTitleText(p);
                            if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                                const wchar_t *strfindclass = wcsstr(WindowClassName, class_name); // 模糊匹配
                                const wchar_t *strfindtitle = wcsstr(WindowTitle, title);          // 模糊匹配
                                if (strfindclass && strfindtitle) {
                                    AppendHwndText(retstring, retstringlen, p);
                                    bret = true;
                                }
                            }
                            HWND child_hwnd = ::GetWindow(p, GW_CHILD);
                            if (child_hwnd != NULL) {
                                FindChildWnd(child_hwnd, title, class_name, retstring, true, true, process_name);
                            }
                        }
                    }
                } else {
                    auto WindowClassName = WindowClassNameText(p);
                    auto WindowTitle = WindowTitleText(p);
                    if (wcslen(WindowClassName) > 1 && wcslen(WindowTitle) > 1) {
                        const wchar_t *strfindclass = wcsstr(WindowClassName, class_name); // 模糊匹配
                        const wchar_t *strfindtitle = wcsstr(WindowTitle, title);          // 模糊匹配
                        if (strfindclass && strfindtitle) {
                            AppendHwndText(retstring, retstringlen, p);
                            bret = true;
                        }
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
        break;
    }
    default:
        return bret;
    }

    return bret;
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
