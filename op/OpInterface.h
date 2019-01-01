// OpInterface.h: OpInterface 的声明

#pragma once
#include "resource.h"       // 主符号



#include "op_i.h"
#include "Type.h"
#include "WinApi.h"
#include "BackGround.h"

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
	//
	Background _background;

public:
	//---------------方法-------------------

	//1.版本号Version
	STDMETHOD(Ver)(long* ret);

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

	//Background 
	STDMETHOD(MoveTo)(LONG x, LONG y, LONG* ret);
	STDMETHOD(LeftClick)(LONG* ret);
	STDMETHOD(BindWindow)(LONG hwnd, LONG display, LONG mouse, LONG keypad, LONG mode,LONG *ret);


};

OBJECT_ENTRY_AUTO(__uuidof(OpInterface), OpInterface)

