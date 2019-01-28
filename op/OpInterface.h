// OpInterface.h: OpInterface 的声明

#pragma once
#include "resource.h"       // 主符号



#include "op_i.h"
#include "Common.h"
#include "WinApi.h"
#include "BKbase.h"
#include "ImageProc.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Windows CE 平台(如不提供完全 DCOM 支持的 Windows Mobile 平台)上无法正确支持单线程 COM 对象。定义 _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA 可强制 ATL 支持创建单线程 COM 对象实现并允许使用其单线程 COM 对象实现。rgs 文件中的线程模型已被设置为“Free”，原因是该模型是非 DCOM Windows CE 平台支持的唯一线程模型。"
#endif

using namespace ATL;


// OpInterface

class ATL_NO_VTABLE OpInterface :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<OpInterface, &CLSID_OpInterface>,
	public IDispatchImpl<IOpInterface, &IID_IOpInterface, &LIBID_opLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	OpInterface()
	{
	}

DECLARE_REGISTRY_RESOURCEID(106)


BEGIN_COM_MAP(OpInterface)
	COM_INTERFACE_ENTRY(IOpInterface)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()



	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}
private:
	//一些共用变量

	//1. Windows API
	WinApi _winapi;
	// background module
	Bkbase _bkproc;
	// work path
	std::wstring _curr_path;
	//image process
	ImageProc _image_proc;
public:
	//---------------方法-------------------

	//1.版本号Version
	STDMETHOD(Ver)(BSTR* ret);
	//set curr path
	STDMETHOD(SetPath)(BSTR path, LONG* ret);
	//get curr path
	STDMETHOD(GetPath)(BSTR*path);
	//sleep
	STDMETHOD(Sleep)(LONG millseconds, LONG* ret);
	//Process
	//inject dll
	STDMETHOD(InjectDll)(BSTR process_name,BSTR dll_name, LONG* ret);
	//WIN API
	STDMETHOD(EnumWindow)(LONG parent, BSTR title, BSTR class_name, LONG filter, BSTR* retstr);
	STDMETHOD(EnumWindowByProcess)(BSTR process_name, BSTR title, BSTR class_name, LONG filter, BSTR* retstring);
	STDMETHOD(EnumProcess)(BSTR name, BSTR* retstring);
	STDMETHOD(ClientToScreen)(LONG ClientToScreen, VARIANT* x, VARIANT* y, LONG* bret);
	STDMETHOD(FindWindow)(BSTR class_name, BSTR title, LONG* rethwnd);
	STDMETHOD(FindWindowByProcess)(BSTR process_name, BSTR class_name, BSTR title, LONG* rethwnd);
	STDMETHOD(FindWindowByProcessId)(LONG process_id, BSTR class_name, BSTR title, LONG* rethwnd);
	STDMETHOD(FindWindowEx)(LONG parent, BSTR class_name, BSTR title, LONG* rethwnd);
	STDMETHOD(GetClientRect)(LONG hwnd, VARIANT* x1, VARIANT* y1, VARIANT* x2, VARIANT* y2, LONG* nret);
	STDMETHOD(GetClientSize)(LONG hwnd, VARIANT* width, VARIANT* height, LONG* nret);
	STDMETHOD(GetForegroundFocus)(LONG* rethwnd);
	STDMETHOD(GetForegroundWindow)(LONG* rethwnd);
	STDMETHOD(GetMousePointWindow)(LONG* rethwnd);
	STDMETHOD(GetPointWindow)(LONG x, LONG y, LONG* rethwnd);
	STDMETHOD(GetProcessInfo)(LONG pid, BSTR* retstring);
	STDMETHOD(GetSpecialWindow)(LONG flag, LONG* rethwnd);
	STDMETHOD(GetWindow)(LONG hwnd, LONG flag, LONG* nret);
	STDMETHOD(GetWindowClass)(LONG hwnd, BSTR* retstring);
	STDMETHOD(GetWindowProcessId)(LONG hwnd, LONG* nretpid);
	STDMETHOD(GetWindowProcessPath)(LONG hwnd, BSTR* retstring);
	STDMETHOD(GetWindowRect)(LONG hwnd, VARIANT* x1, VARIANT* y1, VARIANT* x2, VARIANT* y2, LONG* nret);
	STDMETHOD(GetWindowState)(LONG hwnd, LONG flag, LONG* rethwnd);
	STDMETHOD(GetWindowTitle)(LONG hwnd, BSTR* rettitle);
	STDMETHOD(MoveWindow)(LONG hwnd, LONG x, LONG y, LONG* nret);
	STDMETHOD(ScreenToClient)(LONG hwnd, VARIANT* x, VARIANT* y, LONG* nret);
	STDMETHOD(SendPaste)(LONG hwnd, LONG* nret);
	STDMETHOD(SetClientSize)(LONG hwnd, LONG width, LONG hight, LONG* nret);
	STDMETHOD(SetWindowState)(LONG hwnd, LONG flag, LONG* nret);
	STDMETHOD(SetWindowSize)(LONG hwnd, LONG width, LONG height, LONG* nret);
	STDMETHOD(SetWindowText)(LONG hwnd, BSTR title, LONG* nret);
	STDMETHOD(SetWindowTransparent)(LONG hwnd, LONG trans, LONG* nret);

	//cmd
	STDMETHOD(ExcuteCmd)(BSTR cmd,LONG millseconds, BSTR* retstr);

	//--------------------Background -----------------------
	//bind window and beign capture screen
	STDMETHOD(BindWindow)(LONG hwnd, BSTR display, BSTR mouse, BSTR keypad, LONG mode,LONG *ret);
	//
	STDMETHOD(UnBind)(LONG* ret);
	//--------------------mouse & keyboard------------------
	//获取鼠标位置.
	STDMETHOD(GetCursorPos)(VARIANT* x, VARIANT* y, LONG* ret);
	//鼠标相对于上次的位置移动rx,ry.
	STDMETHOD(MoveR)(LONG x, LONG y, LONG* ret);
	//把鼠标移动到目的点(x,y)
	STDMETHOD(MoveTo)(LONG x, LONG y, LONG* ret);
	//把鼠标移动到目的范围内的任意一点
	STDMETHOD(MoveToEx)(LONG x, LONG y,LONG w,LONG h, LONG* ret);
	//按下鼠标左键
	STDMETHOD(LeftClick)(LONG* ret);
	//双击鼠标左键
	STDMETHOD(LeftDoubleClick)(LONG* ret);
	//按住鼠标左键
	STDMETHOD(LeftDown)(LONG* ret);
	//弹起鼠标左键
	STDMETHOD(LeftUp)(LONG* ret);
	//按下鼠标中键
	STDMETHOD(MiddleClick)(LONG* ret);
	//按住鼠标中键
	STDMETHOD(MiddleDown)(LONG* ret);
	//弹起鼠标中键
	STDMETHOD(MiddleUp)(LONG* ret);
	//按下鼠标右键
	STDMETHOD(RightClick)(LONG* ret);
	//按住鼠标右键
	STDMETHOD(RightDown)(LONG* ret);
	//弹起鼠标右键
	STDMETHOD(RightUp)(LONG* ret);
	//滚轮向下滚
	STDMETHOD(WheelDown)(LONG* ret);
	//滚轮向上滚
	STDMETHOD(WheelUp)(LONG* ret);
	//获取指定的按键状态.(前台信息,不是后台)
	STDMETHOD(GetKeyState)(LONG vk_code, LONG* ret);
	//按住指定的虚拟键码
	STDMETHOD(KeyDown)(LONG vk_code, LONG* ret);
	//按住指定的虚拟键码
	STDMETHOD(KeyDownChar)(BSTR vk_code, LONG* ret);
	//弹起来虚拟键vk_code
	STDMETHOD(KeyUp)(LONG vk_code, LONG* ret);
	//弹起来虚拟键vk_code
	STDMETHOD(KeyUpChar)(BSTR vk_code, LONG* ret);
	//等待指定的按键按下 (前台,不是后台)
	STDMETHOD(WaitKey)(LONG vk_code,LONG time_out, LONG* ret);

	//--------------------image and color-----------------------
	STDMETHOD(Capture)(BSTR file_name, LONG* ret);
	//
	STDMETHOD(FindPic)(LONG x1,LONG y1,LONG x2,LONG y2,BSTR files,DOUBLE sim,VARIANT* x,VARIANT* y,LONG* ret);

	STDMETHOD(SetDict)(LONG idx, BSTR file_name, LONG* ret);

	STDMETHOD(Ocr)(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim,BSTR* ret_str);
	STDMETHOD(FindColor)(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, VARIANT* x, VARIANT* y, LONG* ret);
	STDMETHOD(GetColor)(LONG x, LONG y, BSTR* ret);

};

OBJECT_ENTRY_AUTO(__uuidof(OpInterface), OpInterface)

