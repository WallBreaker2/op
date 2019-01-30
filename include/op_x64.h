

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0622 */
/* at Tue Jan 19 11:14:07 2038
 */
/* Compiler settings for C:\Users\wall\AppData\Local\Temp\IDL87BA.tmp:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0622 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */


#ifndef __op_x64_h__
#define __op_x64_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IComponentRegistrar_FWD_DEFINED__
#define __IComponentRegistrar_FWD_DEFINED__
typedef interface IComponentRegistrar IComponentRegistrar;

#endif 	/* __IComponentRegistrar_FWD_DEFINED__ */


#ifndef __IOpInterface_FWD_DEFINED__
#define __IOpInterface_FWD_DEFINED__
typedef interface IOpInterface IOpInterface;

#endif 	/* __IOpInterface_FWD_DEFINED__ */


#ifndef __CompReg_FWD_DEFINED__
#define __CompReg_FWD_DEFINED__

#ifdef __cplusplus
typedef class CompReg CompReg;
#else
typedef struct CompReg CompReg;
#endif /* __cplusplus */

#endif 	/* __CompReg_FWD_DEFINED__ */


#ifndef __OpInterface_FWD_DEFINED__
#define __OpInterface_FWD_DEFINED__

#ifdef __cplusplus
typedef class OpInterface OpInterface;
#else
typedef struct OpInterface OpInterface;
#endif /* __cplusplus */

#endif 	/* __OpInterface_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __opLib_LIBRARY_DEFINED__
#define __opLib_LIBRARY_DEFINED__

/* library opLib */
/* [custom][custom][custom][custom][version][uuid] */ 




EXTERN_C const IID LIBID_opLib;

#ifndef __IComponentRegistrar_INTERFACE_DEFINED__
#define __IComponentRegistrar_INTERFACE_DEFINED__

/* interface IComponentRegistrar */
/* [object][oleautomation][dual][uuid] */ 


EXTERN_C const IID IID_IComponentRegistrar;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A817E7A2-43FA-11D0-9E44-00AA00B6770A")
    IComponentRegistrar : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Attach( 
            /* [in] */ BSTR bstrPath) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RegisterAll( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE UnregisterAll( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetComponents( 
            /* [out] */ SAFEARRAY * *pbstrCLSIDs,
            /* [out] */ SAFEARRAY * *pbstrDescriptions) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RegisterComponent( 
            /* [in] */ BSTR bstrCLSID) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE UnregisterComponent( 
            /* [in] */ BSTR bstrCLSID) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IComponentRegistrarVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComponentRegistrar * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComponentRegistrar * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComponentRegistrar * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IComponentRegistrar * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IComponentRegistrar * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IComponentRegistrar * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IComponentRegistrar * This,
            /* [annotation][in] */ 
            _In_  DISPID dispIdMember,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][in] */ 
            _In_  LCID lcid,
            /* [annotation][in] */ 
            _In_  WORD wFlags,
            /* [annotation][out][in] */ 
            _In_  DISPPARAMS *pDispParams,
            /* [annotation][out] */ 
            _Out_opt_  VARIANT *pVarResult,
            /* [annotation][out] */ 
            _Out_opt_  EXCEPINFO *pExcepInfo,
            /* [annotation][out] */ 
            _Out_opt_  UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Attach )( 
            IComponentRegistrar * This,
            /* [in] */ BSTR bstrPath);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RegisterAll )( 
            IComponentRegistrar * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *UnregisterAll )( 
            IComponentRegistrar * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetComponents )( 
            IComponentRegistrar * This,
            /* [out] */ SAFEARRAY * *pbstrCLSIDs,
            /* [out] */ SAFEARRAY * *pbstrDescriptions);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RegisterComponent )( 
            IComponentRegistrar * This,
            /* [in] */ BSTR bstrCLSID);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *UnregisterComponent )( 
            IComponentRegistrar * This,
            /* [in] */ BSTR bstrCLSID);
        
        END_INTERFACE
    } IComponentRegistrarVtbl;

    interface IComponentRegistrar
    {
        CONST_VTBL struct IComponentRegistrarVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComponentRegistrar_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IComponentRegistrar_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IComponentRegistrar_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IComponentRegistrar_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IComponentRegistrar_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IComponentRegistrar_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IComponentRegistrar_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IComponentRegistrar_Attach(This,bstrPath)	\
    ( (This)->lpVtbl -> Attach(This,bstrPath) ) 

#define IComponentRegistrar_RegisterAll(This)	\
    ( (This)->lpVtbl -> RegisterAll(This) ) 

#define IComponentRegistrar_UnregisterAll(This)	\
    ( (This)->lpVtbl -> UnregisterAll(This) ) 

#define IComponentRegistrar_GetComponents(This,pbstrCLSIDs,pbstrDescriptions)	\
    ( (This)->lpVtbl -> GetComponents(This,pbstrCLSIDs,pbstrDescriptions) ) 

#define IComponentRegistrar_RegisterComponent(This,bstrCLSID)	\
    ( (This)->lpVtbl -> RegisterComponent(This,bstrCLSID) ) 

#define IComponentRegistrar_UnregisterComponent(This,bstrCLSID)	\
    ( (This)->lpVtbl -> UnregisterComponent(This,bstrCLSID) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IComponentRegistrar_INTERFACE_DEFINED__ */


#ifndef __IOpInterface_INTERFACE_DEFINED__
#define __IOpInterface_INTERFACE_DEFINED__

/* interface IOpInterface */
/* [object][oleautomation][nonextensible][dual][uuid] */ 


EXTERN_C const IID IID_IOpInterface;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("51E59A6F-85F4-4DA0-A01E-C9B6B3B8B8A7")
    IOpInterface : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Ver( 
            /* [retval][out] */ BSTR *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetPath( 
            /* [in] */ BSTR path,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetPath( 
            /* [retval][out] */ BSTR *path) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Sleep( 
            /* [in] */ long millseconds,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE InjectDll( 
            BSTR process_name,
            BSTR dll_name,
            long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE EnumWindow( 
            /* [in] */ long parent,
            /* [in] */ BSTR title,
            /* [in] */ BSTR class_name,
            /* [in] */ long filter,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE EnumWindowByProcess( 
            /* [in] */ BSTR process_name,
            /* [in] */ BSTR title,
            /* [in] */ BSTR class_name,
            /* [in] */ long filter,
            /* [retval][out] */ BSTR *retstring) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE EnumProcess( 
            /* [in] */ BSTR name,
            /* [retval][out] */ BSTR *retstring) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ClientToScreen( 
            /* [in] */ long ClientToScreen,
            /* [out][in] */ VARIANT *x,
            /* [out][in] */ VARIANT *y,
            /* [retval][out] */ long *bret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindWindow( 
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ long *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindWindowByProcess( 
            /* [in] */ BSTR process_name,
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ long *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindWindowByProcessId( 
            /* [in] */ long process_id,
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ long *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindWindowEx( 
            /* [in] */ long parent,
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ long *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetClientRect( 
            /* [in] */ long hwnd,
            /* [out] */ VARIANT *x1,
            /* [out] */ VARIANT *y1,
            /* [out] */ VARIANT *x2,
            /* [out] */ VARIANT *y2,
            /* [retval][out] */ long *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetClientSize( 
            /* [in] */ long hwnd,
            /* [out] */ VARIANT *width,
            /* [out] */ VARIANT *height,
            /* [retval][out] */ long *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetForegroundFocus( 
            /* [retval][out] */ long *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetForegroundWindow( 
            /* [retval][out] */ long *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetMousePointWindow( 
            /* [retval][out] */ long *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetPointWindow( 
            /* [in] */ long x,
            /* [in] */ long y,
            /* [retval][out] */ long *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetProcessInfo( 
            /* [in] */ long pid,
            /* [retval][out] */ BSTR *retstring) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetSpecialWindow( 
            /* [in] */ long flag,
            /* [retval][out] */ long *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetWindow( 
            /* [in] */ long hwnd,
            /* [in] */ long flag,
            /* [retval][out] */ long *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetWindowClass( 
            /* [in] */ long hwnd,
            /* [retval][out] */ BSTR *retstring) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetWindowProcessId( 
            /* [in] */ long hwnd,
            /* [retval][out] */ long *nretpid) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetWindowProcessPath( 
            /* [in] */ long hwnd,
            /* [retval][out] */ BSTR *retstring) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetWindowRect( 
            /* [in] */ long hwnd,
            /* [out] */ VARIANT *x1,
            /* [out] */ VARIANT *y1,
            /* [out] */ VARIANT *x2,
            /* [out] */ VARIANT *y2,
            /* [retval][out] */ long *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetWindowState( 
            /* [in] */ long hwnd,
            /* [in] */ long flag,
            /* [retval][out] */ long *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetWindowTitle( 
            /* [in] */ long hwnd,
            /* [retval][out] */ BSTR *rettitle) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MoveWindow( 
            /* [in] */ long hwnd,
            /* [in] */ long x,
            /* [in] */ long y,
            /* [retval][out] */ long *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ScreenToClient( 
            /* [in] */ long hwnd,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ long *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SendPaste( 
            /* [in] */ long hwnd,
            /* [retval][out] */ long *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetClientSize( 
            /* [in] */ long hwnd,
            /* [in] */ long width,
            /* [in] */ long hight,
            /* [retval][out] */ long *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetWindowState( 
            /* [in] */ long hwnd,
            /* [in] */ long flag,
            /* [retval][out] */ long *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetWindowSize( 
            /* [in] */ long hwnd,
            /* [in] */ long width,
            /* [in] */ long height,
            /* [retval][out] */ long *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetWindowText( 
            /* [in] */ long hwnd,
            /* [in] */ BSTR title,
            /* [retval][out] */ long *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetWindowTransparent( 
            /* [in] */ long hwnd,
            /* [in] */ long trans,
            /* [retval][out] */ long *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ExcuteCmd( 
            /* [in] */ BSTR cmd,
            /* [in] */ long millseconds,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE BindWindow( 
            /* [in] */ long hwnd,
            /* [in] */ BSTR display,
            /* [in] */ BSTR mouse,
            /* [in] */ BSTR keypad,
            /* [in] */ long mode,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE UnBind( 
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetCursorPos( 
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MoveR( 
            /* [in] */ long x,
            /* [in] */ long y,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MoveTo( 
            /* [in] */ long x,
            /* [in] */ long y,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MoveToEx( 
            /* [in] */ long x,
            /* [in] */ long y,
            /* [in] */ long w,
            /* [in] */ long h,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LeftClick( 
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LeftDoubleClick( 
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LeftDown( 
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LeftUp( 
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MiddleClick( 
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MiddleDown( 
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MiddleUp( 
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RightClick( 
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RightDown( 
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RightUp( 
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE WheelDown( 
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE WheelUp( 
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetKeyState( 
            /* [in] */ long vk_code,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE KeyDown( 
            /* [in] */ long vk_code,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE KeyDownChar( 
            /* [in] */ BSTR vk_code,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE KeyUp( 
            /* [in] */ long vk_code,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE KeyUpChar( 
            /* [in] */ BSTR vk_code,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE WaitKey( 
            /* [in] */ long vk_code,
            /* [in] */ long time_out,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Capture( 
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR file_name,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CmpColor( 
            /* [in] */ long x,
            /* [in] */ long y,
            /* [in] */ BSTR color,
            /* [in] */ double sim,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindColor( 
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR color,
            /* [in] */ double sim,
            /* [in] */ long dir,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindColorEx( 
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR color,
            /* [in] */ double sim,
            /* [in] */ long dir,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindMultiColor( 
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR first_color,
            /* [in] */ BSTR offset_color,
            /* [in] */ double sim,
            /* [in] */ long dir,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindMultiColorEx( 
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR first_color,
            /* [in] */ BSTR offset_color,
            /* [in] */ double sim,
            /* [in] */ long dir,
            /* [retval][out] */ BSTR *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindPic( 
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR files,
            /* [in] */ BSTR delta_color,
            /* [in] */ double sim,
            /* [in] */ long dir,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindPicEx( 
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR files,
            /* [in] */ BSTR delta_color,
            /* [in] */ double sim,
            /* [in] */ long dir,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetColor( 
            /* [in] */ long x,
            /* [in] */ long y,
            /* [retval][out] */ BSTR *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetDict( 
            /* [in] */ long idx,
            /* [in] */ BSTR file_name,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE UseDict( 
            /* [in] */ long idx,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Ocr( 
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR color,
            /* [in] */ double sim,
            /* [retval][out] */ BSTR *ret_str) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE OcrEx( 
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR color,
            /* [in] */ double sim,
            /* [retval][out] */ BSTR *ret_str) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindStr( 
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR str,
            /* [in] */ BSTR color,
            /* [in] */ double sim,
            /* [out] */ VARIANT *retx,
            /* [out] */ VARIANT *rety,
            /* [retval][out] */ long *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindStrEx( 
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR str,
            /* [in] */ BSTR color,
            /* [in] */ double sim,
            /* [retval][out] */ BSTR *ret_str) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IOpInterfaceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IOpInterface * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IOpInterface * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IOpInterface * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IOpInterface * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IOpInterface * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IOpInterface * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IOpInterface * This,
            /* [annotation][in] */ 
            _In_  DISPID dispIdMember,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][in] */ 
            _In_  LCID lcid,
            /* [annotation][in] */ 
            _In_  WORD wFlags,
            /* [annotation][out][in] */ 
            _In_  DISPPARAMS *pDispParams,
            /* [annotation][out] */ 
            _Out_opt_  VARIANT *pVarResult,
            /* [annotation][out] */ 
            _Out_opt_  EXCEPINFO *pExcepInfo,
            /* [annotation][out] */ 
            _Out_opt_  UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Ver )( 
            IOpInterface * This,
            /* [retval][out] */ BSTR *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetPath )( 
            IOpInterface * This,
            /* [in] */ BSTR path,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetPath )( 
            IOpInterface * This,
            /* [retval][out] */ BSTR *path);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Sleep )( 
            IOpInterface * This,
            /* [in] */ long millseconds,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *InjectDll )( 
            IOpInterface * This,
            BSTR process_name,
            BSTR dll_name,
            long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *EnumWindow )( 
            IOpInterface * This,
            /* [in] */ long parent,
            /* [in] */ BSTR title,
            /* [in] */ BSTR class_name,
            /* [in] */ long filter,
            /* [retval][out] */ BSTR *retstr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *EnumWindowByProcess )( 
            IOpInterface * This,
            /* [in] */ BSTR process_name,
            /* [in] */ BSTR title,
            /* [in] */ BSTR class_name,
            /* [in] */ long filter,
            /* [retval][out] */ BSTR *retstring);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *EnumProcess )( 
            IOpInterface * This,
            /* [in] */ BSTR name,
            /* [retval][out] */ BSTR *retstring);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ClientToScreen )( 
            IOpInterface * This,
            /* [in] */ long ClientToScreen,
            /* [out][in] */ VARIANT *x,
            /* [out][in] */ VARIANT *y,
            /* [retval][out] */ long *bret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindWindow )( 
            IOpInterface * This,
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ long *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindWindowByProcess )( 
            IOpInterface * This,
            /* [in] */ BSTR process_name,
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ long *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindWindowByProcessId )( 
            IOpInterface * This,
            /* [in] */ long process_id,
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ long *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindWindowEx )( 
            IOpInterface * This,
            /* [in] */ long parent,
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ long *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetClientRect )( 
            IOpInterface * This,
            /* [in] */ long hwnd,
            /* [out] */ VARIANT *x1,
            /* [out] */ VARIANT *y1,
            /* [out] */ VARIANT *x2,
            /* [out] */ VARIANT *y2,
            /* [retval][out] */ long *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetClientSize )( 
            IOpInterface * This,
            /* [in] */ long hwnd,
            /* [out] */ VARIANT *width,
            /* [out] */ VARIANT *height,
            /* [retval][out] */ long *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetForegroundFocus )( 
            IOpInterface * This,
            /* [retval][out] */ long *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetForegroundWindow )( 
            IOpInterface * This,
            /* [retval][out] */ long *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetMousePointWindow )( 
            IOpInterface * This,
            /* [retval][out] */ long *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetPointWindow )( 
            IOpInterface * This,
            /* [in] */ long x,
            /* [in] */ long y,
            /* [retval][out] */ long *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetProcessInfo )( 
            IOpInterface * This,
            /* [in] */ long pid,
            /* [retval][out] */ BSTR *retstring);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetSpecialWindow )( 
            IOpInterface * This,
            /* [in] */ long flag,
            /* [retval][out] */ long *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindow )( 
            IOpInterface * This,
            /* [in] */ long hwnd,
            /* [in] */ long flag,
            /* [retval][out] */ long *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindowClass )( 
            IOpInterface * This,
            /* [in] */ long hwnd,
            /* [retval][out] */ BSTR *retstring);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindowProcessId )( 
            IOpInterface * This,
            /* [in] */ long hwnd,
            /* [retval][out] */ long *nretpid);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindowProcessPath )( 
            IOpInterface * This,
            /* [in] */ long hwnd,
            /* [retval][out] */ BSTR *retstring);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindowRect )( 
            IOpInterface * This,
            /* [in] */ long hwnd,
            /* [out] */ VARIANT *x1,
            /* [out] */ VARIANT *y1,
            /* [out] */ VARIANT *x2,
            /* [out] */ VARIANT *y2,
            /* [retval][out] */ long *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindowState )( 
            IOpInterface * This,
            /* [in] */ long hwnd,
            /* [in] */ long flag,
            /* [retval][out] */ long *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindowTitle )( 
            IOpInterface * This,
            /* [in] */ long hwnd,
            /* [retval][out] */ BSTR *rettitle);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MoveWindow )( 
            IOpInterface * This,
            /* [in] */ long hwnd,
            /* [in] */ long x,
            /* [in] */ long y,
            /* [retval][out] */ long *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ScreenToClient )( 
            IOpInterface * This,
            /* [in] */ long hwnd,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ long *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SendPaste )( 
            IOpInterface * This,
            /* [in] */ long hwnd,
            /* [retval][out] */ long *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetClientSize )( 
            IOpInterface * This,
            /* [in] */ long hwnd,
            /* [in] */ long width,
            /* [in] */ long hight,
            /* [retval][out] */ long *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetWindowState )( 
            IOpInterface * This,
            /* [in] */ long hwnd,
            /* [in] */ long flag,
            /* [retval][out] */ long *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetWindowSize )( 
            IOpInterface * This,
            /* [in] */ long hwnd,
            /* [in] */ long width,
            /* [in] */ long height,
            /* [retval][out] */ long *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetWindowText )( 
            IOpInterface * This,
            /* [in] */ long hwnd,
            /* [in] */ BSTR title,
            /* [retval][out] */ long *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetWindowTransparent )( 
            IOpInterface * This,
            /* [in] */ long hwnd,
            /* [in] */ long trans,
            /* [retval][out] */ long *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ExcuteCmd )( 
            IOpInterface * This,
            /* [in] */ BSTR cmd,
            /* [in] */ long millseconds,
            /* [retval][out] */ BSTR *retstr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *BindWindow )( 
            IOpInterface * This,
            /* [in] */ long hwnd,
            /* [in] */ BSTR display,
            /* [in] */ BSTR mouse,
            /* [in] */ BSTR keypad,
            /* [in] */ long mode,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *UnBind )( 
            IOpInterface * This,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetCursorPos )( 
            IOpInterface * This,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MoveR )( 
            IOpInterface * This,
            /* [in] */ long x,
            /* [in] */ long y,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MoveTo )( 
            IOpInterface * This,
            /* [in] */ long x,
            /* [in] */ long y,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MoveToEx )( 
            IOpInterface * This,
            /* [in] */ long x,
            /* [in] */ long y,
            /* [in] */ long w,
            /* [in] */ long h,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LeftClick )( 
            IOpInterface * This,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LeftDoubleClick )( 
            IOpInterface * This,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LeftDown )( 
            IOpInterface * This,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LeftUp )( 
            IOpInterface * This,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MiddleClick )( 
            IOpInterface * This,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MiddleDown )( 
            IOpInterface * This,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MiddleUp )( 
            IOpInterface * This,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RightClick )( 
            IOpInterface * This,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RightDown )( 
            IOpInterface * This,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RightUp )( 
            IOpInterface * This,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *WheelDown )( 
            IOpInterface * This,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *WheelUp )( 
            IOpInterface * This,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetKeyState )( 
            IOpInterface * This,
            /* [in] */ long vk_code,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *KeyDown )( 
            IOpInterface * This,
            /* [in] */ long vk_code,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *KeyDownChar )( 
            IOpInterface * This,
            /* [in] */ BSTR vk_code,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *KeyUp )( 
            IOpInterface * This,
            /* [in] */ long vk_code,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *KeyUpChar )( 
            IOpInterface * This,
            /* [in] */ BSTR vk_code,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *WaitKey )( 
            IOpInterface * This,
            /* [in] */ long vk_code,
            /* [in] */ long time_out,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Capture )( 
            IOpInterface * This,
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR file_name,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CmpColor )( 
            IOpInterface * This,
            /* [in] */ long x,
            /* [in] */ long y,
            /* [in] */ BSTR color,
            /* [in] */ double sim,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindColor )( 
            IOpInterface * This,
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR color,
            /* [in] */ double sim,
            /* [in] */ long dir,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindColorEx )( 
            IOpInterface * This,
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR color,
            /* [in] */ double sim,
            /* [in] */ long dir,
            /* [retval][out] */ BSTR *retstr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindMultiColor )( 
            IOpInterface * This,
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR first_color,
            /* [in] */ BSTR offset_color,
            /* [in] */ double sim,
            /* [in] */ long dir,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindMultiColorEx )( 
            IOpInterface * This,
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR first_color,
            /* [in] */ BSTR offset_color,
            /* [in] */ double sim,
            /* [in] */ long dir,
            /* [retval][out] */ BSTR *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindPic )( 
            IOpInterface * This,
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR files,
            /* [in] */ BSTR delta_color,
            /* [in] */ double sim,
            /* [in] */ long dir,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindPicEx )( 
            IOpInterface * This,
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR files,
            /* [in] */ BSTR delta_color,
            /* [in] */ double sim,
            /* [in] */ long dir,
            /* [retval][out] */ BSTR *retstr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetColor )( 
            IOpInterface * This,
            /* [in] */ long x,
            /* [in] */ long y,
            /* [retval][out] */ BSTR *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetDict )( 
            IOpInterface * This,
            /* [in] */ long idx,
            /* [in] */ BSTR file_name,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *UseDict )( 
            IOpInterface * This,
            /* [in] */ long idx,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Ocr )( 
            IOpInterface * This,
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR color,
            /* [in] */ double sim,
            /* [retval][out] */ BSTR *ret_str);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *OcrEx )( 
            IOpInterface * This,
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR color,
            /* [in] */ double sim,
            /* [retval][out] */ BSTR *ret_str);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindStr )( 
            IOpInterface * This,
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR str,
            /* [in] */ BSTR color,
            /* [in] */ double sim,
            /* [out] */ VARIANT *retx,
            /* [out] */ VARIANT *rety,
            /* [retval][out] */ long *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindStrEx )( 
            IOpInterface * This,
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ BSTR str,
            /* [in] */ BSTR color,
            /* [in] */ double sim,
            /* [retval][out] */ BSTR *ret_str);
        
        END_INTERFACE
    } IOpInterfaceVtbl;

    interface IOpInterface
    {
        CONST_VTBL struct IOpInterfaceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IOpInterface_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IOpInterface_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IOpInterface_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IOpInterface_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IOpInterface_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IOpInterface_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IOpInterface_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IOpInterface_Ver(This,ret)	\
    ( (This)->lpVtbl -> Ver(This,ret) ) 

#define IOpInterface_SetPath(This,path,ret)	\
    ( (This)->lpVtbl -> SetPath(This,path,ret) ) 

#define IOpInterface_GetPath(This,path)	\
    ( (This)->lpVtbl -> GetPath(This,path) ) 

#define IOpInterface_Sleep(This,millseconds,ret)	\
    ( (This)->lpVtbl -> Sleep(This,millseconds,ret) ) 

#define IOpInterface_InjectDll(This,process_name,dll_name,ret)	\
    ( (This)->lpVtbl -> InjectDll(This,process_name,dll_name,ret) ) 

#define IOpInterface_EnumWindow(This,parent,title,class_name,filter,retstr)	\
    ( (This)->lpVtbl -> EnumWindow(This,parent,title,class_name,filter,retstr) ) 

#define IOpInterface_EnumWindowByProcess(This,process_name,title,class_name,filter,retstring)	\
    ( (This)->lpVtbl -> EnumWindowByProcess(This,process_name,title,class_name,filter,retstring) ) 

#define IOpInterface_EnumProcess(This,name,retstring)	\
    ( (This)->lpVtbl -> EnumProcess(This,name,retstring) ) 

#define IOpInterface_ClientToScreen(This,ClientToScreen,x,y,bret)	\
    ( (This)->lpVtbl -> ClientToScreen(This,ClientToScreen,x,y,bret) ) 

#define IOpInterface_FindWindow(This,class_name,title,rethwnd)	\
    ( (This)->lpVtbl -> FindWindow(This,class_name,title,rethwnd) ) 

#define IOpInterface_FindWindowByProcess(This,process_name,class_name,title,rethwnd)	\
    ( (This)->lpVtbl -> FindWindowByProcess(This,process_name,class_name,title,rethwnd) ) 

#define IOpInterface_FindWindowByProcessId(This,process_id,class_name,title,rethwnd)	\
    ( (This)->lpVtbl -> FindWindowByProcessId(This,process_id,class_name,title,rethwnd) ) 

#define IOpInterface_FindWindowEx(This,parent,class_name,title,rethwnd)	\
    ( (This)->lpVtbl -> FindWindowEx(This,parent,class_name,title,rethwnd) ) 

#define IOpInterface_GetClientRect(This,hwnd,x1,y1,x2,y2,nret)	\
    ( (This)->lpVtbl -> GetClientRect(This,hwnd,x1,y1,x2,y2,nret) ) 

#define IOpInterface_GetClientSize(This,hwnd,width,height,nret)	\
    ( (This)->lpVtbl -> GetClientSize(This,hwnd,width,height,nret) ) 

#define IOpInterface_GetForegroundFocus(This,rethwnd)	\
    ( (This)->lpVtbl -> GetForegroundFocus(This,rethwnd) ) 

#define IOpInterface_GetForegroundWindow(This,rethwnd)	\
    ( (This)->lpVtbl -> GetForegroundWindow(This,rethwnd) ) 

#define IOpInterface_GetMousePointWindow(This,rethwnd)	\
    ( (This)->lpVtbl -> GetMousePointWindow(This,rethwnd) ) 

#define IOpInterface_GetPointWindow(This,x,y,rethwnd)	\
    ( (This)->lpVtbl -> GetPointWindow(This,x,y,rethwnd) ) 

#define IOpInterface_GetProcessInfo(This,pid,retstring)	\
    ( (This)->lpVtbl -> GetProcessInfo(This,pid,retstring) ) 

#define IOpInterface_GetSpecialWindow(This,flag,rethwnd)	\
    ( (This)->lpVtbl -> GetSpecialWindow(This,flag,rethwnd) ) 

#define IOpInterface_GetWindow(This,hwnd,flag,nret)	\
    ( (This)->lpVtbl -> GetWindow(This,hwnd,flag,nret) ) 

#define IOpInterface_GetWindowClass(This,hwnd,retstring)	\
    ( (This)->lpVtbl -> GetWindowClass(This,hwnd,retstring) ) 

#define IOpInterface_GetWindowProcessId(This,hwnd,nretpid)	\
    ( (This)->lpVtbl -> GetWindowProcessId(This,hwnd,nretpid) ) 

#define IOpInterface_GetWindowProcessPath(This,hwnd,retstring)	\
    ( (This)->lpVtbl -> GetWindowProcessPath(This,hwnd,retstring) ) 

#define IOpInterface_GetWindowRect(This,hwnd,x1,y1,x2,y2,nret)	\
    ( (This)->lpVtbl -> GetWindowRect(This,hwnd,x1,y1,x2,y2,nret) ) 

#define IOpInterface_GetWindowState(This,hwnd,flag,rethwnd)	\
    ( (This)->lpVtbl -> GetWindowState(This,hwnd,flag,rethwnd) ) 

#define IOpInterface_GetWindowTitle(This,hwnd,rettitle)	\
    ( (This)->lpVtbl -> GetWindowTitle(This,hwnd,rettitle) ) 

#define IOpInterface_MoveWindow(This,hwnd,x,y,nret)	\
    ( (This)->lpVtbl -> MoveWindow(This,hwnd,x,y,nret) ) 

#define IOpInterface_ScreenToClient(This,hwnd,x,y,nret)	\
    ( (This)->lpVtbl -> ScreenToClient(This,hwnd,x,y,nret) ) 

#define IOpInterface_SendPaste(This,hwnd,nret)	\
    ( (This)->lpVtbl -> SendPaste(This,hwnd,nret) ) 

#define IOpInterface_SetClientSize(This,hwnd,width,hight,nret)	\
    ( (This)->lpVtbl -> SetClientSize(This,hwnd,width,hight,nret) ) 

#define IOpInterface_SetWindowState(This,hwnd,flag,nret)	\
    ( (This)->lpVtbl -> SetWindowState(This,hwnd,flag,nret) ) 

#define IOpInterface_SetWindowSize(This,hwnd,width,height,nret)	\
    ( (This)->lpVtbl -> SetWindowSize(This,hwnd,width,height,nret) ) 

#define IOpInterface_SetWindowText(This,hwnd,title,nret)	\
    ( (This)->lpVtbl -> SetWindowText(This,hwnd,title,nret) ) 

#define IOpInterface_SetWindowTransparent(This,hwnd,trans,nret)	\
    ( (This)->lpVtbl -> SetWindowTransparent(This,hwnd,trans,nret) ) 

#define IOpInterface_ExcuteCmd(This,cmd,millseconds,retstr)	\
    ( (This)->lpVtbl -> ExcuteCmd(This,cmd,millseconds,retstr) ) 

#define IOpInterface_BindWindow(This,hwnd,display,mouse,keypad,mode,ret)	\
    ( (This)->lpVtbl -> BindWindow(This,hwnd,display,mouse,keypad,mode,ret) ) 

#define IOpInterface_UnBind(This,ret)	\
    ( (This)->lpVtbl -> UnBind(This,ret) ) 

#define IOpInterface_GetCursorPos(This,x,y,ret)	\
    ( (This)->lpVtbl -> GetCursorPos(This,x,y,ret) ) 

#define IOpInterface_MoveR(This,x,y,ret)	\
    ( (This)->lpVtbl -> MoveR(This,x,y,ret) ) 

#define IOpInterface_MoveTo(This,x,y,ret)	\
    ( (This)->lpVtbl -> MoveTo(This,x,y,ret) ) 

#define IOpInterface_MoveToEx(This,x,y,w,h,ret)	\
    ( (This)->lpVtbl -> MoveToEx(This,x,y,w,h,ret) ) 

#define IOpInterface_LeftClick(This,ret)	\
    ( (This)->lpVtbl -> LeftClick(This,ret) ) 

#define IOpInterface_LeftDoubleClick(This,ret)	\
    ( (This)->lpVtbl -> LeftDoubleClick(This,ret) ) 

#define IOpInterface_LeftDown(This,ret)	\
    ( (This)->lpVtbl -> LeftDown(This,ret) ) 

#define IOpInterface_LeftUp(This,ret)	\
    ( (This)->lpVtbl -> LeftUp(This,ret) ) 

#define IOpInterface_MiddleClick(This,ret)	\
    ( (This)->lpVtbl -> MiddleClick(This,ret) ) 

#define IOpInterface_MiddleDown(This,ret)	\
    ( (This)->lpVtbl -> MiddleDown(This,ret) ) 

#define IOpInterface_MiddleUp(This,ret)	\
    ( (This)->lpVtbl -> MiddleUp(This,ret) ) 

#define IOpInterface_RightClick(This,ret)	\
    ( (This)->lpVtbl -> RightClick(This,ret) ) 

#define IOpInterface_RightDown(This,ret)	\
    ( (This)->lpVtbl -> RightDown(This,ret) ) 

#define IOpInterface_RightUp(This,ret)	\
    ( (This)->lpVtbl -> RightUp(This,ret) ) 

#define IOpInterface_WheelDown(This,ret)	\
    ( (This)->lpVtbl -> WheelDown(This,ret) ) 

#define IOpInterface_WheelUp(This,ret)	\
    ( (This)->lpVtbl -> WheelUp(This,ret) ) 

#define IOpInterface_GetKeyState(This,vk_code,ret)	\
    ( (This)->lpVtbl -> GetKeyState(This,vk_code,ret) ) 

#define IOpInterface_KeyDown(This,vk_code,ret)	\
    ( (This)->lpVtbl -> KeyDown(This,vk_code,ret) ) 

#define IOpInterface_KeyDownChar(This,vk_code,ret)	\
    ( (This)->lpVtbl -> KeyDownChar(This,vk_code,ret) ) 

#define IOpInterface_KeyUp(This,vk_code,ret)	\
    ( (This)->lpVtbl -> KeyUp(This,vk_code,ret) ) 

#define IOpInterface_KeyUpChar(This,vk_code,ret)	\
    ( (This)->lpVtbl -> KeyUpChar(This,vk_code,ret) ) 

#define IOpInterface_WaitKey(This,vk_code,time_out,ret)	\
    ( (This)->lpVtbl -> WaitKey(This,vk_code,time_out,ret) ) 

#define IOpInterface_Capture(This,x1,y1,x2,y2,file_name,ret)	\
    ( (This)->lpVtbl -> Capture(This,x1,y1,x2,y2,file_name,ret) ) 

#define IOpInterface_CmpColor(This,x,y,color,sim,ret)	\
    ( (This)->lpVtbl -> CmpColor(This,x,y,color,sim,ret) ) 

#define IOpInterface_FindColor(This,x1,y1,x2,y2,color,sim,dir,x,y,ret)	\
    ( (This)->lpVtbl -> FindColor(This,x1,y1,x2,y2,color,sim,dir,x,y,ret) ) 

#define IOpInterface_FindColorEx(This,x1,y1,x2,y2,color,sim,dir,retstr)	\
    ( (This)->lpVtbl -> FindColorEx(This,x1,y1,x2,y2,color,sim,dir,retstr) ) 

#define IOpInterface_FindMultiColor(This,x1,y1,x2,y2,first_color,offset_color,sim,dir,x,y,ret)	\
    ( (This)->lpVtbl -> FindMultiColor(This,x1,y1,x2,y2,first_color,offset_color,sim,dir,x,y,ret) ) 

#define IOpInterface_FindMultiColorEx(This,x1,y1,x2,y2,first_color,offset_color,sim,dir,ret)	\
    ( (This)->lpVtbl -> FindMultiColorEx(This,x1,y1,x2,y2,first_color,offset_color,sim,dir,ret) ) 

#define IOpInterface_FindPic(This,x1,y1,x2,y2,files,delta_color,sim,dir,x,y,ret)	\
    ( (This)->lpVtbl -> FindPic(This,x1,y1,x2,y2,files,delta_color,sim,dir,x,y,ret) ) 

#define IOpInterface_FindPicEx(This,x1,y1,x2,y2,files,delta_color,sim,dir,retstr)	\
    ( (This)->lpVtbl -> FindPicEx(This,x1,y1,x2,y2,files,delta_color,sim,dir,retstr) ) 

#define IOpInterface_GetColor(This,x,y,ret)	\
    ( (This)->lpVtbl -> GetColor(This,x,y,ret) ) 

#define IOpInterface_SetDict(This,idx,file_name,ret)	\
    ( (This)->lpVtbl -> SetDict(This,idx,file_name,ret) ) 

#define IOpInterface_UseDict(This,idx,ret)	\
    ( (This)->lpVtbl -> UseDict(This,idx,ret) ) 

#define IOpInterface_Ocr(This,x1,y1,x2,y2,color,sim,ret_str)	\
    ( (This)->lpVtbl -> Ocr(This,x1,y1,x2,y2,color,sim,ret_str) ) 

#define IOpInterface_OcrEx(This,x1,y1,x2,y2,color,sim,ret_str)	\
    ( (This)->lpVtbl -> OcrEx(This,x1,y1,x2,y2,color,sim,ret_str) ) 

#define IOpInterface_FindStr(This,x1,y1,x2,y2,str,color,sim,retx,rety,ret)	\
    ( (This)->lpVtbl -> FindStr(This,x1,y1,x2,y2,str,color,sim,retx,rety,ret) ) 

#define IOpInterface_FindStrEx(This,x1,y1,x2,y2,str,color,sim,ret_str)	\
    ( (This)->lpVtbl -> FindStrEx(This,x1,y1,x2,y2,str,color,sim,ret_str) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IOpInterface_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_CompReg;

#ifdef __cplusplus

class DECLSPEC_UUID("54CA0535-FB49-4D91-8709-786FB8725132")
CompReg;
#endif

EXTERN_C const CLSID CLSID_OpInterface;

#ifdef __cplusplus

class DECLSPEC_UUID("12BEC402-A06E-4FAD-A7D4-830F967374C6")
OpInterface;
#endif
#endif /* __opLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


