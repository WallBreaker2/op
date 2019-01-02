// 从类型库向导中用“添加类”创建的计算机生成的 IDispatch 包装器类

#import "E:\\project\\op\\x64\\Release\\op_x64.dll" no_namespace
// COpInterface 包装器类

class COpInterface : public COleDispatchDriver
{
public:
	COpInterface() {} // 调用 COleDispatchDriver 默认构造函数
	COpInterface(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	COpInterface(const COpInterface& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

	// 特性
public:

	// 操作
public:


	// IOpInterface 方法
public:
	CString Ver()
	{
		CString result;
		InvokeHelper(0x1, DISPATCH_METHOD, VT_BSTR, (void*)&result, nullptr);
		return result;
	}
	long SetPath(LPCTSTR path)
	{
		long result;
		static BYTE parms[] = VTS_BSTR;
		InvokeHelper(0x2, DISPATCH_METHOD, VT_I4, (void*)&result, parms, path);
		return result;
	}
	CString GetPath()
	{
		CString result;
		InvokeHelper(0x3, DISPATCH_METHOD, VT_BSTR, (void*)&result, nullptr);
		return result;
	}
	long Sleep(long millseconds)
	{
		long result;
		static BYTE parms[] = VTS_I4;
		InvokeHelper(0x4, DISPATCH_METHOD, VT_I4, (void*)&result, parms, millseconds);
		return result;
	}
	CString EnumWindow(long parent, LPCTSTR title, LPCTSTR class_name, long filter)
	{
		CString result;
		static BYTE parms[] = VTS_I4 VTS_BSTR VTS_BSTR VTS_I4;
		InvokeHelper(0x3a, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, parent, title, class_name, filter);
		return result;
	}
	CString EnumWindowByProcess(LPCTSTR process_name, LPCTSTR title, LPCTSTR class_name, long filter)
	{
		CString result;
		static BYTE parms[] = VTS_BSTR VTS_BSTR VTS_BSTR VTS_I4;
		InvokeHelper(0x3b, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, process_name, title, class_name, filter);
		return result;
	}
	CString EnumProcess(LPCTSTR name)
	{
		CString result;
		static BYTE parms[] = VTS_BSTR;
		InvokeHelper(0x3c, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, name);
		return result;
	}
	long ClientToScreen(long ClientToScreen, VARIANT * x, VARIANT * y)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_PVARIANT VTS_PVARIANT;
		InvokeHelper(0x3d, DISPATCH_METHOD, VT_I4, (void*)&result, parms, ClientToScreen, x, y);
		return result;
	}
	long FindWindow(LPCTSTR class_name, LPCTSTR title)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_BSTR;
		InvokeHelper(0x3e, DISPATCH_METHOD, VT_I4, (void*)&result, parms, class_name, title);
		return result;
	}
	long FindWindowByProcess(LPCTSTR process_name, LPCTSTR class_name, LPCTSTR title)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_BSTR VTS_BSTR;
		InvokeHelper(0x3f, DISPATCH_METHOD, VT_I4, (void*)&result, parms, process_name, class_name, title);
		return result;
	}
	long FindWindowByProcessId(long process_id, LPCTSTR class_name, LPCTSTR title)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_BSTR VTS_BSTR;
		InvokeHelper(0x40, DISPATCH_METHOD, VT_I4, (void*)&result, parms, process_id, class_name, title);
		return result;
	}
	long FindWindowEx(long parent, LPCTSTR class_name, LPCTSTR title)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_BSTR VTS_BSTR;
		InvokeHelper(0x41, DISPATCH_METHOD, VT_I4, (void*)&result, parms, parent, class_name, title);
		return result;
	}
	long GetClientRect(long hwnd, VARIANT * x1, VARIANT * y1, VARIANT * x2, VARIANT * y2)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT;
		InvokeHelper(0x42, DISPATCH_METHOD, VT_I4, (void*)&result, parms, hwnd, x1, y1, x2, y2);
		return result;
	}
	long GetClientSize(long hwnd, VARIANT * width, VARIANT * height)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_PVARIANT VTS_PVARIANT;
		InvokeHelper(0x43, DISPATCH_METHOD, VT_I4, (void*)&result, parms, hwnd, width, height);
		return result;
	}
	long GetForegroundFocus()
	{
		long result;
		InvokeHelper(0x44, DISPATCH_METHOD, VT_I4, (void*)&result, nullptr);
		return result;
	}
	long GetForegroundWindow()
	{
		long result;
		InvokeHelper(0x45, DISPATCH_METHOD, VT_I4, (void*)&result, nullptr);
		return result;
	}
	long GetMousePointWindow()
	{
		long result;
		InvokeHelper(0x46, DISPATCH_METHOD, VT_I4, (void*)&result, nullptr);
		return result;
	}
	long GetPointWindow(long x, long y)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_I4;
		InvokeHelper(0x47, DISPATCH_METHOD, VT_I4, (void*)&result, parms, x, y);
		return result;
	}
	CString GetProcessInfo(long pid)
	{
		CString result;
		static BYTE parms[] = VTS_I4;
		InvokeHelper(0x48, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, pid);
		return result;
	}
	long GetSpecialWindow(long flag)
	{
		long result;
		static BYTE parms[] = VTS_I4;
		InvokeHelper(0x49, DISPATCH_METHOD, VT_I4, (void*)&result, parms, flag);
		return result;
	}
	long GetWindow(long hwnd, long flag)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_I4;
		InvokeHelper(0x4a, DISPATCH_METHOD, VT_I4, (void*)&result, parms, hwnd, flag);
		return result;
	}
	CString GetWindowClass(long hwnd)
	{
		CString result;
		static BYTE parms[] = VTS_I4;
		InvokeHelper(0x4b, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, hwnd);
		return result;
	}
	long GetWindowProcessId(long hwnd)
	{
		long result;
		static BYTE parms[] = VTS_I4;
		InvokeHelper(0x4c, DISPATCH_METHOD, VT_I4, (void*)&result, parms, hwnd);
		return result;
	}
	CString GetWindowProcessPath(long hwnd)
	{
		CString result;
		static BYTE parms[] = VTS_I4;
		InvokeHelper(0x4d, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, hwnd);
		return result;
	}
	long GetWindowRect(long hwnd, VARIANT * x1, VARIANT * y1, VARIANT * x2, VARIANT * y2)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT;
		InvokeHelper(0x4e, DISPATCH_METHOD, VT_I4, (void*)&result, parms, hwnd, x1, y1, x2, y2);
		return result;
	}
	long GetWindowState(long hwnd, long flag)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_I4;
		InvokeHelper(0x4f, DISPATCH_METHOD, VT_I4, (void*)&result, parms, hwnd, flag);
		return result;
	}
	CString GetWindowTitle(long hwnd)
	{
		CString result;
		static BYTE parms[] = VTS_I4;
		InvokeHelper(0x50, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, hwnd);
		return result;
	}
	long MoveWindow(long hwnd, long x, long y)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_I4 VTS_I4;
		InvokeHelper(0x51, DISPATCH_METHOD, VT_I4, (void*)&result, parms, hwnd, x, y);
		return result;
	}
	long ScreenToClient(long hwnd, VARIANT * x, VARIANT * y)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_PVARIANT VTS_PVARIANT;
		InvokeHelper(0x52, DISPATCH_METHOD, VT_I4, (void*)&result, parms, hwnd, x, y);
		return result;
	}
	long SendPaste(long hwnd)
	{
		long result;
		static BYTE parms[] = VTS_I4;
		InvokeHelper(0x53, DISPATCH_METHOD, VT_I4, (void*)&result, parms, hwnd);
		return result;
	}
	long SetClientSize(long hwnd, long width, long hight)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_I4 VTS_I4;
		InvokeHelper(0x54, DISPATCH_METHOD, VT_I4, (void*)&result, parms, hwnd, width, hight);
		return result;
	}
	long SetWindowState(long hwnd, long flag)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_I4;
		InvokeHelper(0x55, DISPATCH_METHOD, VT_I4, (void*)&result, parms, hwnd, flag);
		return result;
	}
	long SetWindowSize(long hwnd, long width, long height)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_I4 VTS_I4;
		InvokeHelper(0x56, DISPATCH_METHOD, VT_I4, (void*)&result, parms, hwnd, width, height);
		return result;
	}
	long SetWindowText(long hwnd, LPCTSTR title)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_BSTR;
		InvokeHelper(0x57, DISPATCH_METHOD, VT_I4, (void*)&result, parms, hwnd, title);
		return result;
	}
	long SetWindowTransparent(long hwnd, long trans)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_I4;
		InvokeHelper(0x58, DISPATCH_METHOD, VT_I4, (void*)&result, parms, hwnd, trans);
		return result;
	}
	CString ExcuteCmd(LPCTSTR cmd, long millseconds)
	{
		CString result;
		static BYTE parms[] = VTS_BSTR VTS_I4;
		InvokeHelper(0x59, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, cmd, millseconds);
		return result;
	}
	long MoveTo(long x, long y)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_I4;
		InvokeHelper(0x5a, DISPATCH_METHOD, VT_I4, (void*)&result, parms, x, y);
		return result;
	}
	long LeftClick()
	{
		long result;
		InvokeHelper(0x5b, DISPATCH_METHOD, VT_I4, (void*)&result, nullptr);
		return result;
	}
	long BindWindow(long hwnd, long display, long mouse, long keypad, long mode)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_I4;
		InvokeHelper(0x5c, DISPATCH_METHOD, VT_I4, (void*)&result, parms, hwnd, display, mouse, keypad, mode);
		return result;
	}
	long Capture(LPCTSTR file_name)
	{
		long result;
		static BYTE parms[] = VTS_BSTR;
		InvokeHelper(0x5d, DISPATCH_METHOD, VT_I4, (void*)&result, parms, file_name);
		return result;
	}
	long UnBind()
	{
		long result;
		InvokeHelper(0x5e, DISPATCH_METHOD, VT_I4, (void*)&result, nullptr);
		return result;
	}

	// IOpInterface 属性
public:

};
