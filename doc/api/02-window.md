# 窗口与进程

`op::Op` 的这一组接口共 **47** 个。返回索引：[doc/api.md](../api.md)

> 通用约定（`long *ret` 的含义、坐标语义、线程安全、C API 命名映射）见 [索引页](../api.md#通用约定)。

## windows api（41）

- **`void EnumWindow(_In_ LONG_PTR parent, _In_ const wchar_t *title, _In_ const wchar_t *class_name, _In_ long filter, _Out_ std::wstring &ret)`** → `void`
  根据指定条件,枚举系统中符合条件的窗口

- **`void EnumWindowByProcess(_In_ const wchar_t *process_name, _In_ const wchar_t *title, _In_ const wchar_t *class_name, _In_ long filter, _Out_ std::wstring &ret)`** → `void`
  根据指定进程以及其它条件,枚举系统中符合条件的窗口

- **`void EnumProcess(_In_ const wchar_t *name, _Out_ std::wstring &ret)`** → `void`
  根据指定进程名,枚举系统中符合条件的进程PID

- **`void ClientToScreen(_In_ LONG_PTR hwnd, _Inout_ long *x, _Inout_ long *y, _Out_ long *bret)`** → `void`
  把窗口坐标转换为屏幕坐标

- **`void FindWindow(_In_ const wchar_t *class_name, _In_ const wchar_t *title, _Out_ LONG_PTR *ret)`** → `void`
  查找符合类名或者标题名的顶层可见窗口

- **`void FindWindowByProcess(_In_ const wchar_t *process_name, _In_ const wchar_t *class_name, _In_ const wchar_t *title, _Out_ LONG_PTR *ret)`** → `void`
  根据指定的进程名字，来查找可见窗口

- **`void FindWindowByProcessId(_In_ long process_id, _In_ const wchar_t *class_name, _In_ const wchar_t *title, _Out_ LONG_PTR *ret)`** → `void`
  根据指定的进程Id，来查找可见窗口

- **`void FindWindowEx(_In_ LONG_PTR parent, _In_ const wchar_t *class_name, _In_ const wchar_t *title, _Out_ LONG_PTR *ret)`** → `void`
  查找符合类名或者标题名的顶层可见窗口,如果指定了parent,则在parent的第一层子窗口中查找

- **`void GetClientRect(_In_ LONG_PTR hwnd, _Out_ long *x1, _Out_ long *y1, _Out_ long *x2, _Out_ long *y2, _Out_ long *ret)`** → `void`
  获取窗口客户区域在屏幕上的位置

- **`void GetClientSize(_In_ LONG_PTR hwnd, _Out_ long *width, _Out_ long *height, _Out_ long *ret)`** → `void`
  获取窗口客户区域的宽度和高度

- **`void GetForegroundFocus(_Out_ LONG_PTR *ret)`** → `void`
  获取顶层活动窗口中具有输入焦点的窗口句柄

- **`void GetForegroundWindow(_Out_ LONG_PTR *ret)`** → `void`
  获取顶层活动窗口,可以获取到按键自带插件无法获取到的句柄

- **`void GetMousePointWindow(_Out_ LONG_PTR *ret)`** → `void`
  获取鼠标指向的可见窗口句柄

- **`void GetPointWindow(_In_ long x, _In_ long y, _Out_ LONG_PTR *ret)`** → `void`
  获取给定坐标的可见窗口句柄

- **`void GetProcessInfo(_In_ long pid, _Out_ std::wstring &ret)`** → `void`
  根据指定的pid获取进程详细信息

- **`void GetSpecialWindow(_In_ long flag, _Out_ LONG_PTR *ret)`** → `void`
  获取特殊窗口

- **`void GetWindow(_In_ LONG_PTR hwnd, _In_ long flag, _Out_ LONG_PTR *ret)`** → `void`
  获取给定窗口相关的窗口句柄

- **`void GetWindowClass(_In_ LONG_PTR hwnd, _Out_ std::wstring &ret)`** → `void`
  获取窗口的类名

- **`void GetWindowProcessId(_In_ LONG_PTR hwnd, _Out_ long *ret)`** → `void`
  获取指定窗口所在的进程ID

- **`void GetWindowProcessPath(_In_ LONG_PTR hwnd, _Out_ std::wstring &ret)`** → `void`
  获取指定窗口所在的进程的exe文件全路径

- **`void GetWindowRect(_In_ LONG_PTR hwnd, _Out_ long *x1, _Out_ long *y1, _Out_ long *x2, _Out_ long *y2, _Out_ long *ret)`** → `void`
  获取窗口在屏幕上的位置

- **`void GetWindowState(_In_ LONG_PTR hwnd, _In_ long flag, _Out_ long *ret)`** → `void`
  获取指定窗口的一些属性

- **`void GetWindowTitle(_In_ LONG_PTR hwnd, _Out_ std::wstring &rettitle)`** → `void`
  获取窗口的标题

- **`void MoveWindow(_In_ LONG_PTR hwnd, _In_ long x, _In_ long y, _Out_ long *ret)`** → `void`
  移动指定窗口到指定位置

- **`void ScreenToClient(_In_ LONG_PTR hwnd, _Inout_ long *x, _Inout_ long *y, _Out_ long *ret)`** → `void`
  把屏幕坐标转换为窗口坐标

- **`void SendPaste(_In_ LONG_PTR hwnd, _Out_ long *ret)`** → `void`
  向指定窗口发送粘贴命令

- **`void SetClientSize(_In_ LONG_PTR hwnd, _In_ long width, _In_ long hight, _Out_ long *ret)`** → `void`
  设置窗口客户区域的宽度和高度

- **`void SetWindowState(_In_ LONG_PTR hwnd, _In_ long flag, _Out_ long *ret)`** → `void`
  设置窗口的状态

- **`void SetWindowSize(_In_ LONG_PTR hwnd, _In_ long width, _In_ long height, _Out_ long *ret)`** → `void`
  设置窗口的大小

- **`void LayoutWindows(_In_ const wchar_t *hwnds, _In_ long layout_type, _In_ long columns, _In_ long start_x, _In_ long start_y, _In_ long gap_x, _In_ long gap_y, _In_ long size_mode, _In_ long window_width, _In_ long window_height, _In_ long anchor_mode, _Out_ long *ret)`** → `void`
  按指定布局批量排列多个窗口。hwnds 格式例如 "123|456|789"。

- **`void SetWindowText(_In_ LONG_PTR hwnd, _In_ const wchar_t *title, _Out_ long *ret)`** → `void`
  设置窗口的标题

- **`void SetWindowTransparent(_In_ LONG_PTR hwnd, _In_ long trans, _Out_ long *ret)`** → `void`
  设置窗口的透明度

- **`void SendString(_In_ LONG_PTR hwnd, _In_ const wchar_t *str, _Out_ long *ret)`** → `void`
  向指定窗口发送文本数据

- **`void SendStringIme(_In_ LONG_PTR hwnd, _In_ const wchar_t *str, _Out_ long *ret)`** → `void`
  向指定窗口发送文本数据-输入法

- **`void RunApp(_In_ const wchar_t *cmdline, _In_ long mode, _Out_ unsigned long *pid, _Out_ long *ret)`** → `void`
  运行可执行文件,可指定模式

- **`void WinExec(_In_ const wchar_t *cmdline, _In_ long cmdshow, _Out_ long *ret)`** → `void`
  运行可执行文件，可指定显示模式

- **`void GetCmdStr(_In_ const wchar_t *cmd, _In_ long millseconds, _Out_ std::wstring &retstr)`** → `void`
  运行命令行并返回结果

- **`void SetClipboard(_In_ const wchar_t *str, _Out_ long *ret)`** → `void`
  设置剪贴板数据

- **`void GetClipboard(_Out_ std::wstring &ret)`** → `void`
  获取剪贴板数据

- **`void Delay(_In_ long mis, _Out_ long *ret)`** → `void`
  延时指定的毫秒,过程中不阻塞UI操作

- **`void Delays(_In_ long mis_min, _In_ long mis_max, _Out_ long *ret)`** → `void`
  延时指定范围内随机毫秒,过程中不阻塞UI操作

## Background（6）

- **`void BindWindow(_In_ LONG_PTR hwnd, _In_ const wchar_t *display, _In_ const wchar_t *mouse, _In_ const wchar_t *keypad, _In_ long mode, _Out_ long *ret)`** → `void`
  兼容旧接口的单句柄绑定。显示和输入都使用同一个 hwnd。

- **`void BindWindowEx(_In_ LONG_PTR display_hwnd, _In_ LONG_PTR input_hwnd, _In_ const wchar_t *display, _In_ const wchar_t *mouse, _In_ const wchar_t *keypad, _In_ long mode, _Out_ long *ret)`** → `void`
  扩展绑定接口。显示截图使用 display_hwnd，鼠标和键盘输入使用 input_hwnd。

- **`void UnBindWindow(_Out_ long *ret)`** → `void`
  解绑窗口

- **`void LockInput(_In_ long lock, _Out_ long *ret)`** → `void`
  临时锁定目标窗口的外部输入。只对 dx 鼠标、dx 键盘有效。

- **`void GetBindWindow(_Out_ LONG_PTR *ret)`** → `void`
  获取当前对象已经绑定的显示窗口句柄. 无绑定返回0

- **`void IsBind(_Out_ long *ret)`** → `void`
  判定当前对象是否已绑定窗口.

