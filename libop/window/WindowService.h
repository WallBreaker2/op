#pragma once
#ifndef OP_WINDOW_WINDOW_SERVICE_H_
#define OP_WINDOW_WINDOW_SERVICE_H_
#include "../runtime/Types.h"
#undef FindWindow
#undef FindWindowEx
namespace op {

class WindowService {
  public:
    WindowService(void);
    ~WindowService(void);

  public:
    int retstringlen;
    DWORD window_version;
    int enum_process_success_count;
    std::vector<DWORD> npid;
    bool EnumWindow(HWND parent, const wchar_t *title, const wchar_t *class_name, LONG filter, std::wstring &retstring,
                    const wchar_t *process_name = NULL);
    bool EnumProcess(const wchar_t *name, std::wstring &retstring);
    bool ClientToScreen(HWND hwnd, LONG &x, LONG &y);
    HWND FindWindow(const wchar_t *class_name, const wchar_t *title);
    HWND FindWindowEx(HWND parent, const wchar_t *class_name, const wchar_t *title);
    bool FindWindowByProcess(const wchar_t *class_name, const wchar_t *titl, HWND &rethwnd,
                             const wchar_t *process_name = NULL, DWORD Pid = 0);
    bool GetClientRect(HWND hwnd, LONG &x, LONG &y, LONG &x1, LONG &y1);
    bool GetClientSize(HWND hwnd, LONG &width, LONG &height);
    bool GetMousePointWindow(HWND &rethwnd, LONG x = -1, LONG y = -1);
    bool GetProcessInfo(LONG pid, std::wstring &retstring);
    bool GetWindow(HWND hwnd, LONG flag, HWND &rethwnd);
    bool GetProcesspath(DWORD ProcessID, std::wstring &process_path);
    bool GetWindowState(HWND hwnd, LONG flag);
    bool SendPaste(HWND hwnd);
    bool SetWindowSize(HWND hwnd, LONG width, LONG height, int type = 0);
    bool SetWindowState(HWND hwnd, LONG flag, HWND rethwnd = nullptr);
    bool SetWindowTransparent(HWND hwnd, LONG trans);
    bool SetClipboard(const wchar_t *values);
    bool GetClipboard(std::wstring &retstr);
    // 2019.1
    long SendString(HWND hwnd, const std::wstring &str);
    long SendStringIme(HWND hwnd, const std::wstring &str);
    // 2019.3
    long RunApp(const std::wstring &cmd, long mode, DWORD *pid);
    static HWND GetTopWindowSp(HWND hwnd);

  private:
    bool EnumWindowInternal(HWND parent, const wchar_t *title, const wchar_t *class_name, LONG filter,
                            std::wstring *retstring, const wchar_t *process_name = NULL);
    HWND FindChildWnd(HWND child_hwnd, const wchar_t *title, const wchar_t *classname, std::wstring *retstring,
                      bool isGW_OWNER = false, bool isVisible = false, const wchar_t *process_name = NULL);
    BOOL EnumProcessbyName(DWORD dwPID, LPCWSTR ExeName, LONG type = 0);
    int GetProcessNumber(); // 获取CPU个数
    // 时间格式转换
    __int64 FileTimeToInt64(const FILETIME &time);
    double get_cpu_usage(DWORD ProcessID); // 获取指定进程CPU使用率
    DWORD GetMemoryInfo(DWORD ProcessID);  // 或者指定进程内存使用率
};


} // namespace op

#endif // OP_WINDOW_WINDOW_SERVICE_H_
