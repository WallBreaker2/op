// EasyCom.cpp : 定义 DLL 的导出函数。
//
#include "framework.h"
#include "EasyCom.h"

#include <stdio.h>
#include "../libop/com/op_i.h"
#include "MinHook.h"
#include <iostream>
#include <comdef.h>
#include <fstream>
#pragma warning(disable:4996)

char dllpathA[512] = "op_x86.dll";
wchar_t dllpathW[512] = L"op_x86.dll";
HRESULT(__stdcall* oCLSIDFromProgID)(
	_In_ LPCOLESTR lpszProgID,
	_Out_ LPCLSID lpclsid
	);

HRESULT(__stdcall* oCoCreateInstance)(REFCLSID  rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext,
	REFIID  riid, LPVOID  FAR* ppv);

HRESULT(__stdcall*
	oCoGetClassObject)(
		_In_ REFCLSID rclsid,
		_In_ DWORD dwClsContext,
		_In_opt_ LPVOID pvReserved,
		_In_ REFIID riid,
		_Outptr_ LPVOID  FAR* ppv
	);

HRESULT __stdcall myCLSIDFromProgID(
	_In_ LPCOLESTR lpszProgID,
	_Out_ LPCLSID lpclsid
) {
	//MessageBoxW(NULL, L"myCLSIDFromProgID",lpszProgID, 0);
	//printf("myCLSIDFromProgID\n");
	if (wcscmp(lpszProgID, L"op.opsoft") == 0) {
		memcpy(lpclsid, &CLSID_OpInterface, sizeof(CLSID_OpInterface));
		//MessageBoxW(NULL, L"memcpy", lpszProgID, 0);
		return S_OK;
	}
	else {
		//printf("oCLSIDFromProgID\n");
		//MessageBoxW(NULL, L"oCLSIDFromProgID", lpszProgID, 0);
		return oCLSIDFromProgID(lpszProgID, lpclsid);
	}
}



HRESULT __stdcall
myCoCreateInstance(
	_In_ REFCLSID rclsid,
	_In_opt_ LPUNKNOWN pUnkOuter,
	_In_ DWORD dwClsContext,
	_In_ REFIID riid,
	_COM_Outptr_ _At_(*ppv, _Post_readable_size_(_Inexpressible_(varies))) LPVOID  FAR* ppv
) {
	/*if (IsEqualCLSID(rclsid, CLSID_OpInterface))printf("CLSID_OpInterface\n");
	else printf("unknown clsid\n");
	
	if (IsEqualCLSID(riid, IID_IDispatch))
		printf("IID_IDispatch\n");
	else if (IsEqualCLSID(riid, IID_IUnknown))
		printf("IID_IUnknown\n");
	else 
		printf("unknown riid\n");
	*/
	//return oCoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
	if (memcmp(&rclsid, &CLSID_OpInterface, sizeof(CLSID_OpInterface)) != 0
		/*|| memcmp(riid, &IID_IOpInterface, sizeof(IID_IOpInterface)) != 0*/) {

		//MessageBoxA(NULL, "oCoCreateInstance", "", 0);
		return oCoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);

	}
	//return -1;
	*ppv = 0;
	printf("myCoCreateInstance\n");

	HMODULE hdll = LoadLibraryA(dllpathA);
	if (!hdll) {
		printf("LoadLibraryA false!\n");
		MessageBoxW(NULL, L"LoadLibraryA", NULL, 0);
		return -1;
	}
	//printf("LoadLibraryA !\n");
	typedef HRESULT(__stdcall* DllGetClassObject_t)(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* ppv);
	DllGetClassObject_t dynamiDllGetClassObject = (DllGetClassObject_t)GetProcAddress(hdll, "DllGetClassObject");
	if (!dynamiDllGetClassObject) {
		//printf("GetProcAddress false!\n");
		MessageBoxW(NULL, L"GetProcAddress", NULL, 0);
		return -2;
	}
	//printf("GetProcAddress !\n");
	IClassFactory* fac = 0;

	dynamiDllGetClassObject(CLSID_OpInterface, IID_IClassFactory, (void**)&fac);
	
	//IClassFa
	//pfun1(&CLSID_OpInterface, riid, (void**)&fac);
	if (!fac) {
		//printf("DllGetClassObject false!\n");
		MessageBoxW(NULL, L"DllGetClassObject", NULL, 0);
		return -3;
	}
	//printf("DllGetClassObject !\n");
	IOpInterface* op;
	//fac->lpVtbl->CreateInstance(fac, NULL, &IID_IOpInterface, (void**)&op);
	fac->CreateInstance(NULL, riid, (void**)&op);
	//fac->lpVtbl->QueryInterface()
	if (!op) {
		//printf("CoCreateInstance false!\n");
		MessageBoxW(NULL, L"CoCreateInstance", NULL, 0);
		return -4;
	}
	//printf("CreateInstance !\n");
	*ppv = op;
	return S_OK;
}

HRESULT __stdcall
myCoGetClassObject(
	_In_ REFCLSID rclsid,
	_In_ DWORD dwClsContext,
	_In_opt_ LPVOID pvReserved,
	_In_ REFIID riid,
	_Outptr_ LPVOID  FAR* ppv
) {
	//printf("myCoGetClassObject\n");
	return oCoGetClassObject(rclsid, dwClsContext, pvReserved, riid, ppv);
}

HRESULT(__stdcall* oLoadRegTypeLib)(REFGUID rguid, WORD wVerMajor, WORD wVerMinor,
	LCID lcid, ITypeLib** pptlib);
HRESULT __stdcall mLoadRegTypeLib(REFGUID rguid, WORD wVerMajor, WORD wVerMinor,
	LCID lcid, ITypeLib** pptlib) {
	//printf("mLoadRegTypeLib\n");
	/*return memcmp(rguid, &LIBID_opLib, sizeof(LIBID_opLib)) == 0 ? LoadTypeLib(dllpath, pptlib) :
		oLoadRegTypeLib(rguid, wVerMajor, wVerMinor, lcid, pptlib);*/
	HRESULT hr = memcmp(&rguid, &LIBID_opLib, sizeof(LIBID_opLib)) == 0 ? LoadTypeLib(dllpathW, pptlib) :
		oLoadRegTypeLib(rguid, wVerMajor, wVerMinor, lcid, pptlib);
	return hr;
	//return oLoadRegTypeLib(rguid, wVerMajor, wVerMinor, lcid, pptlib);
}



int sethook() {
	//MessageBoxA(NULL, "sethook", "", 0);
	//printf("sethook\n");
	if (MH_Initialize() != MH_OK) {
		printf("MH_Initialize false\n");
		return -1;
	}
	if (MH_CreateHook(&CoCreateInstance, &myCoCreateInstance, (void**)&oCoCreateInstance) != MH_OK) {
		printf("MH_CreateHook false \n");
		return -2;
	}
	if (MH_CreateHook(&CLSIDFromProgID, &myCLSIDFromProgID, (void**)&oCLSIDFromProgID) != MH_OK) {
		printf("MH_CreateHook false \n");
		return -3;
	}
	if (MH_CreateHook(&CoGetClassObject, &myCoGetClassObject, (void**)&oCoGetClassObject) != MH_OK) {
		printf("MH_CreateHook false \n");
		return -4;
	}
	//LoadRegTypeLib
	if (MH_CreateHook(&LoadRegTypeLib, &mLoadRegTypeLib, (void**)&oLoadRegTypeLib) != MH_OK) {
		printf("MH_CreateHook false \n");
		return -5;
	}
	if (MH_EnableHook(NULL) != MH_OK) {
		//printf("MH_EnableHook false\n");
		return -6;
	}
	return 1;

}

// 这是导出变量的一个示例
EASYCOM_API int nEasyCom = 0;

// 这是导出函数的一个示例。
EASYCOM_API int __stdcall setupA(const char* path)
{
	std::ifstream fin(path);
	
	if (!fin) {
		return 0;//file not exist
	}
	strcpy(dllpathA, path);
	wchar_t* m_wchar;
	int nlen = strlen(dllpathA);
	int len = MultiByteToWideChar(CP_ACP, 0, dllpathA,nlen , NULL, 0);
	m_wchar = new wchar_t[len + 1];
	MultiByteToWideChar(CP_ACP, 0, dllpathA, nlen, m_wchar, len);
	m_wchar[len] = '\0';
	memcpy(dllpathW, m_wchar, sizeof(wchar_t) * (len + 1));
	delete[]m_wchar;

	
	return sethook();


}

EASYCOM_API int __stdcall setupW(const wchar_t* path) {
	std::ifstream fin(path);

	if (!fin) {
		return 0;//file not exist
	}
	int nlen = wcslen(path);
	wcscpy(dllpathW, path);
	char* m_char;
	int len = WideCharToMultiByte(CP_ACP, 0, path, nlen, NULL, 0, NULL, NULL);
	m_char = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, path, nlen, m_char, len, NULL, NULL);
	m_char[len] = '\0';
	strcpy(dllpathA, m_char);
	delete[] m_char;

	return sethook();

}
