#include "WindowService.h"
#include "../runtime/WindowsHandle.h"

namespace op {

namespace {

std::wstring window_class_name(HWND hwnd) {
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

} // namespace

bool WindowService::GetClientRect(HWND hwnd, LONG &x, LONG &y, LONG &x1, LONG &y1) {
    bool bret = false;
    RECT clientrect;
    if (IsWindow(hwnd)) {
        ::GetClientRect(hwnd, &clientrect);
        POINT point;
        point.x = clientrect.left;
        point.y = clientrect.top;
        ::ClientToScreen(hwnd, &point);
        x = point.x;
        y = point.y;
        point.x = clientrect.right;
        point.y = clientrect.bottom;
        ::ClientToScreen(hwnd, &point);
        x1 = point.x;
        y1 = point.y;
        bret = true;
    }

    return bret;
}

bool WindowService::GetClientSize(HWND hwnd, LONG &width, LONG &height) {
    bool bret = false;
    RECT clientrect;
    if (IsWindow(hwnd)) {
        ::GetClientRect(hwnd, &clientrect);
        width = clientrect.right - clientrect.left;
        height = clientrect.bottom - clientrect.top;
        bret = true;
    }
    return bret;
}

bool WindowService::GetMousePointWindow(HWND &rethwnd, LONG x, LONG y) {
    bool bret = false;
    rethwnd = nullptr;
    POINT point;
    if ((x != -1 && y != -1)) {
        point.x = x;
        point.y = y;
    } else {
        ::GetCursorPos(&point);
    }
    rethwnd = ::WindowFromPoint(point);
    if (rethwnd == NULL) {
        HWND p = ::GetWindow(GetDesktopWindow(), GW_CHILD); // 获取桌面窗口的子窗口
        p = ::GetWindow(p, GW_HWNDFIRST);
        while (p != NULL) {
            if (::IsWindowVisible(p) && ::GetWindow(p, GW_OWNER) == 0) {
                RECT rc;
                ::GetWindowRect(p, &rc);
                if ((rc.top <= point.y) && (rc.left <= point.x) && (rc.right >= (point.x - rc.left)) &&
                    (rc.bottom >= (point.y - rc.top))) {
                    std::wstring WindowClass = window_class_name(p);
                    // if((windowpoint.x==0||windowpoint.x<rc.left)&&wcscmp(WindowClass,L"CabinetWClass")!=0)
                    // //IE框窗体排除在外
                    if (WindowClass != L"CabinetWClass") // IE框窗体排除在外
                    {
                        rethwnd = p;
                        bret = true;
                        break;
                    }
                }
            }
            p = ::GetWindow(p, GW_HWNDNEXT); // 获取下一个窗口
        }
    } else
        bret = true;

    return bret;
}

bool WindowService::GetWindow(HWND hwnd, LONG flag, HWND &rethwnd) {
    bool bret = false;
    rethwnd = nullptr;
    HWND wnd = hwnd;
    if (IsWindow(wnd) == false)
        return bret;
    DWORD type = -1;
    if (flag == 0) // 0:获取父窗口
        rethwnd = ::GetParent(wnd);
    else if (flag == 1) // 获取第一个儿子窗口
        type = GW_CHILD;
    else if (flag == 2) // 获取First 窗口
        type = GW_HWNDFIRST;
    else if (flag == 3) // 获取Last窗口
        type = GW_HWNDLAST;
    else if (flag == 4) // 获取下一个窗口
        type = GW_HWNDNEXT;
    else if (flag == 5) // 获取上一个窗口
        type = GW_HWNDPREV;
    else if (flag == 6) // 获取拥有者窗口
        type = GW_OWNER;
    else if (flag == 7) // 获取顶层窗口
    {
        // rethwnd = (LONG)::GetForegroundWindow();
        HWND next = NULL, current = hwnd;
        while (next = ::GetParent(current))
            current = next;
        rethwnd = current;
        return ::IsWindow(current);
    }

    if (type != -1)
        rethwnd = ::GetWindow(wnd, (UINT)type);

    if (rethwnd != nullptr)
        bret = true;

    return bret;
}

bool WindowService::GetWindowState(HWND hwnd, LONG flag) {
    bool bret = false;
    HWND wnd = hwnd;
    if (flag == 0) // 0://判断窗口是否存在
        bret = ::IsWindow(wnd);
    else if (flag == 1) // 判断窗口是否处于激活
    {
        if (::GetActiveWindow() == wnd)
            bret = true;
    } else if (flag == 2) // 2 : 判断窗口是否可见
        bret = ::IsWindowVisible(wnd);
    else if (flag == 3) // 3 : 判断窗口是否最小化
        bret = ::IsIconic(wnd);
    else if (flag == 4) // 4 : 判断窗口是否最大化
        bret = ::IsZoomed(wnd);
    else if (flag == 5) // 5 : 判断窗口是否置顶
    {
        if (::GetForegroundWindow() == wnd)
            bret = true;
    } else if (flag == 6) // 6 : 判断窗口是否无响应
        bret = ::IsHungAppWindow(wnd);
    else if (flag == 7) // 判断窗口是否可用(灰色为不可用)
        bret = ::IsWindowEnabled(wnd);

    return bret;
}

bool WindowService::SetWindowSize(HWND hwnd, LONG width, LONG height, int type) {
    bool bret = false;
    if (type == 0) // SetClientSize
    {
        RECT rectProgram, rectClient;
        HWND hWnd = hwnd;
        ::GetWindowRect(hWnd, &rectProgram); // 获得程序窗口位于屏幕坐标
        ::GetClientRect(hWnd, &rectClient);  // 获得客户区坐标
        // 非客户区宽,高
        int nWidth = rectProgram.right - rectProgram.left - (rectClient.right - rectClient.left);
        int adjusted_height = rectProgram.bottom - rectProgram.top - (rectClient.bottom - rectClient.top);
        nWidth += width;
        adjusted_height += height;
        rectProgram.right = nWidth;
        rectProgram.bottom = adjusted_height;
        int showToScreenx = GetSystemMetrics(SM_CXSCREEN) / 2 - nWidth / 2; // 居中处理
        int showToScreeny = GetSystemMetrics(SM_CYSCREEN) / 2 - adjusted_height / 2;
        bret = ::MoveWindow(hWnd, showToScreenx, showToScreeny, rectProgram.right, rectProgram.bottom, false);
    } else // SetWindowSize
    {
        RECT rectClient;
        HWND hWnd = hwnd;
        ::GetWindowRect(hWnd, &rectClient); // 获得程序窗口位于屏幕坐标
        bret = ::MoveWindow(hWnd, rectClient.left, rectClient.top, width, height, false);
    }
    return bret;
}

bool WindowService::SetWindowState(HWND hwnd, LONG flag, HWND rethwnd) {
    bool bret = false;
    HWND hWnd = hwnd;
    if (IsWindow(hWnd) == false)
        return bret;
    int type = -1;
    type = flag;
    if (flag == 0) // 关闭指定窗口
        ::SendMessage(hWnd, WM_CLOSE, 0, 0);
    else if (flag == 1) // 激活指定窗口
    {
        ::ShowWindow(hWnd, SW_SHOW);
        ::SetForegroundWindow(hWnd);
    } else if (flag == 2) // 最小化指定窗口,但不激活
        ::ShowWindow(hWnd, SW_SHOWMINNOACTIVE);
    else if (flag == 3) // 最小化指定窗口,并释放内存,但同时也会激活窗口
        ::ShowWindow(hWnd, SW_SHOWMINIMIZED);
    else if (flag == 4) // 最大化指定窗口,同时激活窗口.
        ::ShowWindow(hWnd, SW_SHOWMAXIMIZED);
    else if (flag == 5) // 恢复指定窗口 ,但不激活
        ::ShowWindow(hWnd, SW_SHOWNOACTIVATE);
    else if (flag == 6) // 隐藏指定窗口
        ::ShowWindow(hWnd, SW_HIDE);
    else if (flag == 7) // 显示指定窗口
    {
        ::ShowWindow(hWnd, SW_SHOW);
        ::SetForegroundWindow(hWnd);
    } else if (flag == 8) // 置顶指定窗口
        ::SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                       SWP_NOMOVE | SWP_NOSIZE);
    else if (flag == 9) // 9 : 取消置顶指定窗口
        ::SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    else if (flag == 10) // 禁止指定窗口
        ::EnableWindow(hWnd, false);
    else if (flag == 11) // 取消禁止指定窗口
        ::EnableWindow(hWnd, true);
    else if (flag == 12) // 12 : 恢复并激活指定窗口
        ::ShowWindow(hWnd, SW_RESTORE);
    else if (flag == 13) // 13 : 强制结束窗口所在进程.
    {
        DWORD pid = 0;
        ::GetWindowThreadProcessId(hWnd, &pid);
        // TSRuntime::EnablePrivilege(L"SeDebugPrivilege", true);
        op::win32::unique_handle process(::OpenProcess(PROCESS_TERMINATE, false, pid));
        if (!process)
            return false;
        return ::TerminateProcess(process.get(), 0) != 0;
    } else if (flag == 14) // 14 : 闪烁指定的窗口
    {
        FLASHWINFO fInfo;
        fInfo.cbSize = sizeof(FLASHWINFO);
        // 这里是闪动窗标题和任务栏按钮,直到用户激活窗体
        fInfo.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
        fInfo.dwTimeout = 0;
        fInfo.hwnd = hWnd;
        fInfo.uCount = 0xffffff;
        FlashWindowEx(&fInfo);
    } else if (flag == 15) // 使指定的窗口获取输入焦点
    {
        ::ShowWindow(hWnd, SW_SHOW);
        ::SetFocus(hWnd);
    }

    if (type >= 0 && type < 16)
        bret = true;

    return bret;
}

bool WindowService::SetWindowTransparent(HWND hwnd, LONG trans) {
    bool bret = false;

    COLORREF crKey = NULL;
    DWORD dwFlags = 0;
    BYTE bAlpha = 0;
    if (trans < 0)
        trans = 0;
    if (trans > 255)
        trans = 255;
    //...
    /*typedef bool(__stdcall  *  mySetLayeredWindowAttributes)(
            HWND hwnd,
            COLORREF pcrKey,
            BYTE pbAlpha,
            DWORD pdwFlags);
    mySetLayeredWindowAttributes obj_SetLayeredWindowAttributes = NULL;
    HINSTANCE hlibrary;
    hlibrary = LoadLibrary(_T("user32.dll"));
    obj_SetLayeredWindowAttributes =
    (mySetLayeredWindowAttributes)GetProcAddress(hlibrary,
    "SetLayeredWindowAttributes");*/

    SetWindowLong(hwnd, GWL_EXSTYLE, 0x80001);
    bret = SetLayeredWindowAttributes(hwnd, crKey, static_cast<BYTE>(trans), 2);

    return bret;
}

HWND WindowService::GetTopWindowSp(HWND hwnd) {
    HWND i = hwnd, temp;

    while (GetWindowLongA(i, GWL_STYLE) > 0) {
        temp = GetParent(i);
        if (!temp)
            break;
    }
    return i;
}

} // namespace op
