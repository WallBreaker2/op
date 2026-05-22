

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0628 */
/* at Tue Jan 19 11:14:07 2038
 */
/* Compiler settings for D:/code/op/libop/com/op.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.01.0628 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __op_i_h__
#define __op_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef DECLSPEC_XFGVIRT
#if defined(_CONTROL_FLOW_GUARD_XFG)
#define DECLSPEC_XFGVIRT(base, func) __declspec(xfg_virtual(base, func))
#else
#define DECLSPEC_XFGVIRT(base, func)
#endif
#endif

/* Forward Declarations */ 

#ifndef __IOpInterface_FWD_DEFINED__
#define __IOpInterface_FWD_DEFINED__
typedef interface IOpInterface IOpInterface;

#endif 	/* __IOpInterface_FWD_DEFINED__ */


#ifndef __OpInterface_FWD_DEFINED__
#define __OpInterface_FWD_DEFINED__

#ifdef __cplusplus
typedef class OpInterface OpInterface;
#else
typedef struct OpInterface OpInterface;
#endif /* __cplusplus */

#endif 	/* __OpInterface_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "shobjidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IOpInterface_INTERFACE_DEFINED__
#define __IOpInterface_INTERFACE_DEFINED__

/* interface IOpInterface */
/* [unique][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IOpInterface;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("51e59a6f-85f4-4da0-a01e-c9b6b3b8b8a7")
    IOpInterface : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Ver( 
            /* [retval][out] */ BSTR *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetPath( 
            /* [in] */ BSTR path,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetPath( 
            /* [retval][out] */ BSTR *path) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetBasePath( 
            /* [retval][out] */ BSTR *path) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetID( 
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetLastError( 
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetShowErrorMsg( 
            /* [in] */ LONG show_type,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Sleep( 
            /* [in] */ LONG millseconds,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE InjectDll( 
            /* [in] */ BSTR process_name,
            /* [in] */ BSTR dll_name,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE EnablePicCache( 
            /* [in] */ LONG enable,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CapturePre( 
            /* [in] */ BSTR file_name,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE AStarFindPath( 
            /* [in] */ LONG mapWidth,
            /* [in] */ LONG mapHeight,
            /* [in] */ BSTR disable_points,
            /* [in] */ LONG beginX,
            /* [in] */ LONG beginY,
            /* [in] */ LONG endX,
            /* [in] */ LONG endY,
            /* [retval][out] */ BSTR *path) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindNearestPos( 
            /* [in] */ BSTR all_pos,
            /* [in] */ LONG type,
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE EnumWindow( 
            /* [in] */ LONG parent,
            /* [in] */ BSTR title,
            /* [in] */ BSTR class_name,
            /* [in] */ LONG filter,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE EnumWindowByProcess( 
            /* [in] */ BSTR process_name,
            /* [in] */ BSTR title,
            /* [in] */ BSTR class_name,
            /* [in] */ LONG filter,
            /* [retval][out] */ BSTR *retstring) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE EnumProcess( 
            /* [in] */ BSTR name,
            /* [retval][out] */ BSTR *retstring) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ClientToScreen( 
            /* [in] */ LONG ClientToScreen,
            /* [out][in] */ VARIANT *x,
            /* [out][in] */ VARIANT *y,
            /* [retval][out] */ LONG *bret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindWindow( 
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ LONG *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindWindowByProcess( 
            /* [in] */ BSTR process_name,
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ LONG *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindWindowByProcessId( 
            /* [in] */ LONG process_id,
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ LONG *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindWindowEx( 
            /* [in] */ LONG parent,
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ LONG *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetClientRect( 
            /* [in] */ LONG hwnd,
            /* [out] */ VARIANT *x1,
            /* [out] */ VARIANT *y1,
            /* [out] */ VARIANT *x2,
            /* [out] */ VARIANT *y2,
            /* [retval][out] */ LONG *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetClientSize( 
            /* [in] */ LONG hwnd,
            /* [out] */ VARIANT *width,
            /* [out] */ VARIANT *height,
            /* [retval][out] */ LONG *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetForegroundFocus( 
            /* [retval][out] */ LONG *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetForegroundWindow( 
            /* [retval][out] */ LONG *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetMousePointWindow( 
            /* [retval][out] */ LONG *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetPointWindow( 
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [retval][out] */ LONG *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetProcessInfo( 
            /* [in] */ LONG pid,
            /* [retval][out] */ BSTR *retstring) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetSpecialWindow( 
            /* [in] */ LONG flag,
            /* [retval][out] */ LONG *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetWindow( 
            /* [in] */ LONG hwnd,
            /* [in] */ LONG flag,
            /* [retval][out] */ LONG *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetWindowClass( 
            /* [in] */ LONG hwnd,
            /* [retval][out] */ BSTR *retstring) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetWindowProcessId( 
            /* [in] */ LONG hwnd,
            /* [retval][out] */ LONG *nretpid) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetWindowProcessPath( 
            /* [in] */ LONG hwnd,
            /* [retval][out] */ BSTR *retstring) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetWindowRect( 
            /* [in] */ LONG hwnd,
            /* [out] */ VARIANT *x1,
            /* [out] */ VARIANT *y1,
            /* [out] */ VARIANT *x2,
            /* [out] */ VARIANT *y2,
            /* [retval][out] */ LONG *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetWindowState( 
            /* [in] */ LONG hwnd,
            /* [in] */ LONG flag,
            /* [retval][out] */ LONG *rethwnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetWindowTitle( 
            /* [in] */ LONG hwnd,
            /* [retval][out] */ BSTR *rettitle) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MoveWindow( 
            /* [in] */ LONG hwnd,
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [retval][out] */ LONG *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ScreenToClient( 
            /* [in] */ LONG hwnd,
            /* [out][in] */ VARIANT *x,
            /* [out][in] */ VARIANT *y,
            /* [retval][out] */ LONG *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SendPaste( 
            /* [in] */ LONG hwnd,
            /* [retval][out] */ LONG *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetClientSize( 
            /* [in] */ LONG hwnd,
            /* [in] */ LONG width,
            /* [in] */ LONG hight,
            /* [retval][out] */ LONG *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetWindowState( 
            /* [in] */ LONG hwnd,
            /* [in] */ LONG flag,
            /* [retval][out] */ LONG *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetWindowSize( 
            /* [in] */ LONG hwnd,
            /* [in] */ LONG width,
            /* [in] */ LONG height,
            /* [retval][out] */ LONG *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetWindowText( 
            /* [in] */ LONG hwnd,
            /* [in] */ BSTR title,
            /* [retval][out] */ LONG *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetWindowTransparent( 
            /* [in] */ LONG hwnd,
            /* [in] */ LONG trans,
            /* [retval][out] */ LONG *nret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SendString( 
            /* [in] */ LONG hwnd,
            /* [in] */ BSTR str,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SendStringIme( 
            /* [in] */ LONG hwnd,
            /* [in] */ BSTR str,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RunApp( 
            /* [in] */ BSTR cmdline,
            /* [in] */ LONG mode,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE WinExec( 
            /* [in] */ BSTR cmdline,
            /* [in] */ LONG cmdshow,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetCmdStr( 
            /* [in] */ BSTR cmd,
            /* [in] */ LONG millseconds,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE BindWindow( 
            /* [in] */ LONG hwnd,
            /* [in] */ BSTR display,
            /* [in] */ BSTR mouse,
            /* [in] */ BSTR keypad,
            /* [in] */ LONG mode,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE UnBindWindow( 
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetBindWindow( 
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE IsBind( 
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetCursorPos( 
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MoveR( 
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MoveTo( 
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MoveToEx( 
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [in] */ LONG w,
            /* [in] */ long h,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LeftClick( 
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LeftDoubleClick( 
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LeftDown( 
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LeftUp( 
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MiddleClick( 
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MiddleDown( 
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MiddleUp( 
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RightClick( 
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RightDown( 
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RightUp( 
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE WheelDown( 
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE WheelUp( 
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetKeyState( 
            /* [in] */ LONG vk_code,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE KeyDown( 
            /* [in] */ LONG vk_code,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE KeyDownChar( 
            /* [in] */ BSTR vk_code,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE KeyUp( 
            /* [in] */ LONG vk_code,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE KeyUpChar( 
            /* [in] */ BSTR vk_code,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE WaitKey( 
            /* [in] */ LONG vk_code,
            /* [in] */ LONG time_out,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE KeyPress( 
            /* [in] */ LONG vk_code,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE KeyPressChar( 
            /* [in] */ BSTR vk_code,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Capture( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR file_name,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CmpColor( 
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindColor( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [in] */ LONG dir,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindColorEx( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [in] */ LONG dir,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindMultiColor( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR first_color,
            /* [in] */ BSTR offset_color,
            /* [in] */ DOUBLE sim,
            /* [in] */ LONG dir,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindMultiColorEx( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR first_color,
            /* [in] */ BSTR offset_color,
            /* [in] */ DOUBLE sim,
            /* [in] */ LONG dir,
            /* [retval][out] */ BSTR *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindPic( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR files,
            /* [in] */ BSTR delta_color,
            /* [in] */ DOUBLE sim,
            /* [in] */ LONG dir,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindPicEx( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR files,
            /* [in] */ BSTR delta_color,
            /* [in] */ DOUBLE sim,
            /* [in] */ LONG dir,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindPicExS( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR files,
            /* [in] */ BSTR delta_color,
            /* [in] */ DOUBLE sim,
            /* [in] */ LONG dir,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindColorBlock( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [in] */ LONG count,
            /* [in] */ LONG height,
            /* [in] */ LONG width,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindColorBlockEx( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [in] */ LONG count,
            /* [in] */ LONG height,
            /* [in] */ LONG width,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetResultCount( 
            /* [in] */ BSTR str,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetResultPos( 
            /* [in] */ BSTR str,
            /* [in] */ LONG index,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetColor( 
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [retval][out] */ BSTR *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetDisplayInput( 
            /* [in] */ BSTR method,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LoadPic( 
            /* [in] */ BSTR pic_name,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FreePic( 
            /* [in] */ BSTR Pic_name,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetScreenData( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetScreenDataBmp( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [out] */ VARIANT *data,
            /* [out] */ VARIANT *size,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MatchPicName( 
            /* [in] */ BSTR pic_name,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE LoadMemPic( 
            /* [in] */ BSTR pic_name,
            /* [in] */ long long data,
            /* [in] */ LONG size,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetMouseDelay( 
            /* [in] */ BSTR type,
            /* [in] */ LONG delay,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetKeypadDelay( 
            /* [in] */ BSTR type,
            /* [in] */ LONG delay,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetClipboard( 
            /* [in] */ BSTR str,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetClipboard( 
            /* [retval][out] */ BSTR *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Delay( 
            /* [in] */ LONG mis,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Delays( 
            /* [in] */ LONG mis_min,
            /* [in] */ LONG mis_max,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE KeyPressStr( 
            /* [in] */ BSTR key_str,
            /* [in] */ LONG delay,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetColorNum( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetPicSize( 
            /* [in] */ BSTR pic_name,
            /* [out] */ VARIANT *width,
            /* [out] */ VARIANT *height,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetDict( 
            /* [in] */ LONG idx,
            /* [in] */ BSTR file_name,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE UseDict( 
            /* [in] */ LONG idx,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Ocr( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ BSTR *ret_str) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE OcrEx( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ BSTR *ret_str) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindStr( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR str,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [out] */ VARIANT *retx,
            /* [out] */ VARIANT *rety,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindStrEx( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR str,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ BSTR *ret_str) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE OcrAuto( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE OcrFromFile( 
            /* [in] */ BSTR file_name,
            /* [in] */ BSTR color_format,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE OcrAutoFromFile( 
            /* [in] */ BSTR file_name,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FindLine( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetMemDict( 
            /* [in] */ LONG idx,
            /* [in] */ BSTR data,
            /* [in] */ LONG size,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetDict( 
            /* [in] */ LONG idx,
            /* [in] */ LONG font_index,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddDict( 
            /* [in] */ LONG idx,
            /* [in] */ BSTR dict_info,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SaveDict( 
            /* [in] */ LONG idx,
            /* [in] */ BSTR file_name,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ClearDict( 
            /* [in] */ LONG idx,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetDictCount( 
            /* [in] */ LONG idx,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetNowDict( 
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FetchWord( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ BSTR word,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetWordsNoDict( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetWordResultCount( 
            /* [in] */ BSTR result,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetWordResultPos( 
            /* [in] */ BSTR result,
            /* [in] */ LONG index,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetWordResultStr( 
            /* [in] */ BSTR result,
            /* [in] */ LONG index,
            /* [retval][out] */ BSTR *retstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE WriteData( 
            /* [in] */ LONG hwnd,
            /* [in] */ BSTR address,
            /* [in] */ BSTR data,
            /* [in] */ LONG size,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ReadData( 
            /* [in] */ LONG hwnd,
            /* [in] */ BSTR address,
            /* [in] */ LONG size,
            /* [retval][out] */ BSTR *retstr) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IOpInterfaceVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IOpInterface * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IOpInterface * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IOpInterface * This);
        
        DECLSPEC_XFGVIRT(IDispatch, GetTypeInfoCount)
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IOpInterface * This,
            /* [out] */ UINT *pctinfo);
        
        DECLSPEC_XFGVIRT(IDispatch, GetTypeInfo)
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IOpInterface * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        DECLSPEC_XFGVIRT(IDispatch, GetIDsOfNames)
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IOpInterface * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        DECLSPEC_XFGVIRT(IDispatch, Invoke)
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
        
        DECLSPEC_XFGVIRT(IOpInterface, Ver)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Ver )( 
            IOpInterface * This,
            /* [retval][out] */ BSTR *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, SetPath)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetPath )( 
            IOpInterface * This,
            /* [in] */ BSTR path,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetPath)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetPath )( 
            IOpInterface * This,
            /* [retval][out] */ BSTR *path);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetBasePath)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetBasePath )( 
            IOpInterface * This,
            /* [retval][out] */ BSTR *path);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetID)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetID )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetLastError)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetLastError )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, SetShowErrorMsg)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetShowErrorMsg )( 
            IOpInterface * This,
            /* [in] */ LONG show_type,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, Sleep)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Sleep )( 
            IOpInterface * This,
            /* [in] */ LONG millseconds,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, InjectDll)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *InjectDll )( 
            IOpInterface * This,
            /* [in] */ BSTR process_name,
            /* [in] */ BSTR dll_name,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, EnablePicCache)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *EnablePicCache )( 
            IOpInterface * This,
            /* [in] */ LONG enable,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, CapturePre)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CapturePre )( 
            IOpInterface * This,
            /* [in] */ BSTR file_name,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, AStarFindPath)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *AStarFindPath )( 
            IOpInterface * This,
            /* [in] */ LONG mapWidth,
            /* [in] */ LONG mapHeight,
            /* [in] */ BSTR disable_points,
            /* [in] */ LONG beginX,
            /* [in] */ LONG beginY,
            /* [in] */ LONG endX,
            /* [in] */ LONG endY,
            /* [retval][out] */ BSTR *path);
        
        DECLSPEC_XFGVIRT(IOpInterface, FindNearestPos)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindNearestPos )( 
            IOpInterface * This,
            /* [in] */ BSTR all_pos,
            /* [in] */ LONG type,
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [retval][out] */ BSTR *retstr);
        
        DECLSPEC_XFGVIRT(IOpInterface, EnumWindow)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *EnumWindow )( 
            IOpInterface * This,
            /* [in] */ LONG parent,
            /* [in] */ BSTR title,
            /* [in] */ BSTR class_name,
            /* [in] */ LONG filter,
            /* [retval][out] */ BSTR *retstr);
        
        DECLSPEC_XFGVIRT(IOpInterface, EnumWindowByProcess)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *EnumWindowByProcess )( 
            IOpInterface * This,
            /* [in] */ BSTR process_name,
            /* [in] */ BSTR title,
            /* [in] */ BSTR class_name,
            /* [in] */ LONG filter,
            /* [retval][out] */ BSTR *retstring);
        
        DECLSPEC_XFGVIRT(IOpInterface, EnumProcess)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *EnumProcess )( 
            IOpInterface * This,
            /* [in] */ BSTR name,
            /* [retval][out] */ BSTR *retstring);
        
        DECLSPEC_XFGVIRT(IOpInterface, ClientToScreen)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ClientToScreen )( 
            IOpInterface * This,
            /* [in] */ LONG ClientToScreen,
            /* [out][in] */ VARIANT *x,
            /* [out][in] */ VARIANT *y,
            /* [retval][out] */ LONG *bret);
        
        DECLSPEC_XFGVIRT(IOpInterface, FindWindow)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindWindow )( 
            IOpInterface * This,
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ LONG *rethwnd);
        
        DECLSPEC_XFGVIRT(IOpInterface, FindWindowByProcess)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindWindowByProcess )( 
            IOpInterface * This,
            /* [in] */ BSTR process_name,
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ LONG *rethwnd);
        
        DECLSPEC_XFGVIRT(IOpInterface, FindWindowByProcessId)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindWindowByProcessId )( 
            IOpInterface * This,
            /* [in] */ LONG process_id,
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ LONG *rethwnd);
        
        DECLSPEC_XFGVIRT(IOpInterface, FindWindowEx)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindWindowEx )( 
            IOpInterface * This,
            /* [in] */ LONG parent,
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ LONG *rethwnd);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetClientRect)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetClientRect )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [out] */ VARIANT *x1,
            /* [out] */ VARIANT *y1,
            /* [out] */ VARIANT *x2,
            /* [out] */ VARIANT *y2,
            /* [retval][out] */ LONG *nret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetClientSize)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetClientSize )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [out] */ VARIANT *width,
            /* [out] */ VARIANT *height,
            /* [retval][out] */ LONG *nret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetForegroundFocus)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetForegroundFocus )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *rethwnd);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetForegroundWindow)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetForegroundWindow )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *rethwnd);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetMousePointWindow)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetMousePointWindow )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *rethwnd);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetPointWindow)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetPointWindow )( 
            IOpInterface * This,
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [retval][out] */ LONG *rethwnd);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetProcessInfo)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetProcessInfo )( 
            IOpInterface * This,
            /* [in] */ LONG pid,
            /* [retval][out] */ BSTR *retstring);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetSpecialWindow)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetSpecialWindow )( 
            IOpInterface * This,
            /* [in] */ LONG flag,
            /* [retval][out] */ LONG *rethwnd);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetWindow)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindow )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ LONG flag,
            /* [retval][out] */ LONG *nret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetWindowClass)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindowClass )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [retval][out] */ BSTR *retstring);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetWindowProcessId)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindowProcessId )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [retval][out] */ LONG *nretpid);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetWindowProcessPath)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindowProcessPath )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [retval][out] */ BSTR *retstring);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetWindowRect)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindowRect )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [out] */ VARIANT *x1,
            /* [out] */ VARIANT *y1,
            /* [out] */ VARIANT *x2,
            /* [out] */ VARIANT *y2,
            /* [retval][out] */ LONG *nret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetWindowState)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindowState )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ LONG flag,
            /* [retval][out] */ LONG *rethwnd);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetWindowTitle)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindowTitle )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [retval][out] */ BSTR *rettitle);
        
        DECLSPEC_XFGVIRT(IOpInterface, MoveWindow)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MoveWindow )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [retval][out] */ LONG *nret);
        
        DECLSPEC_XFGVIRT(IOpInterface, ScreenToClient)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ScreenToClient )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [out][in] */ VARIANT *x,
            /* [out][in] */ VARIANT *y,
            /* [retval][out] */ LONG *nret);
        
        DECLSPEC_XFGVIRT(IOpInterface, SendPaste)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SendPaste )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [retval][out] */ LONG *nret);
        
        DECLSPEC_XFGVIRT(IOpInterface, SetClientSize)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetClientSize )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ LONG width,
            /* [in] */ LONG hight,
            /* [retval][out] */ LONG *nret);
        
        DECLSPEC_XFGVIRT(IOpInterface, SetWindowState)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetWindowState )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ LONG flag,
            /* [retval][out] */ LONG *nret);
        
        DECLSPEC_XFGVIRT(IOpInterface, SetWindowSize)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetWindowSize )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ LONG width,
            /* [in] */ LONG height,
            /* [retval][out] */ LONG *nret);
        
        DECLSPEC_XFGVIRT(IOpInterface, SetWindowText)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetWindowText )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ BSTR title,
            /* [retval][out] */ LONG *nret);
        
        DECLSPEC_XFGVIRT(IOpInterface, SetWindowTransparent)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetWindowTransparent )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ LONG trans,
            /* [retval][out] */ LONG *nret);
        
        DECLSPEC_XFGVIRT(IOpInterface, SendString)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SendString )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ BSTR str,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, SendStringIme)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SendStringIme )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ BSTR str,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, RunApp)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RunApp )( 
            IOpInterface * This,
            /* [in] */ BSTR cmdline,
            /* [in] */ LONG mode,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, WinExec)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *WinExec )( 
            IOpInterface * This,
            /* [in] */ BSTR cmdline,
            /* [in] */ LONG cmdshow,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetCmdStr)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetCmdStr )( 
            IOpInterface * This,
            /* [in] */ BSTR cmd,
            /* [in] */ LONG millseconds,
            /* [retval][out] */ BSTR *retstr);
        
        DECLSPEC_XFGVIRT(IOpInterface, BindWindow)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *BindWindow )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ BSTR display,
            /* [in] */ BSTR mouse,
            /* [in] */ BSTR keypad,
            /* [in] */ LONG mode,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, UnBindWindow)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *UnBindWindow )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetBindWindow)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetBindWindow )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, IsBind)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *IsBind )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetCursorPos)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetCursorPos )( 
            IOpInterface * This,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, MoveR)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MoveR )( 
            IOpInterface * This,
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, MoveTo)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MoveTo )( 
            IOpInterface * This,
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, MoveToEx)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MoveToEx )( 
            IOpInterface * This,
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [in] */ LONG w,
            /* [in] */ long h,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, LeftClick)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LeftClick )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, LeftDoubleClick)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LeftDoubleClick )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, LeftDown)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LeftDown )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, LeftUp)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LeftUp )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, MiddleClick)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MiddleClick )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, MiddleDown)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MiddleDown )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, MiddleUp)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MiddleUp )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, RightClick)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RightClick )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, RightDown)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RightDown )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, RightUp)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RightUp )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, WheelDown)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *WheelDown )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, WheelUp)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *WheelUp )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetKeyState)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetKeyState )( 
            IOpInterface * This,
            /* [in] */ LONG vk_code,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, KeyDown)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *KeyDown )( 
            IOpInterface * This,
            /* [in] */ LONG vk_code,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, KeyDownChar)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *KeyDownChar )( 
            IOpInterface * This,
            /* [in] */ BSTR vk_code,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, KeyUp)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *KeyUp )( 
            IOpInterface * This,
            /* [in] */ LONG vk_code,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, KeyUpChar)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *KeyUpChar )( 
            IOpInterface * This,
            /* [in] */ BSTR vk_code,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, WaitKey)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *WaitKey )( 
            IOpInterface * This,
            /* [in] */ LONG vk_code,
            /* [in] */ LONG time_out,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, KeyPress)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *KeyPress )( 
            IOpInterface * This,
            /* [in] */ LONG vk_code,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, KeyPressChar)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *KeyPressChar )( 
            IOpInterface * This,
            /* [in] */ BSTR vk_code,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, Capture)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Capture )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR file_name,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, CmpColor)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CmpColor )( 
            IOpInterface * This,
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, FindColor)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindColor )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [in] */ LONG dir,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, FindColorEx)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindColorEx )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [in] */ LONG dir,
            /* [retval][out] */ BSTR *retstr);
        
        DECLSPEC_XFGVIRT(IOpInterface, FindMultiColor)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindMultiColor )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR first_color,
            /* [in] */ BSTR offset_color,
            /* [in] */ DOUBLE sim,
            /* [in] */ LONG dir,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, FindMultiColorEx)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindMultiColorEx )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR first_color,
            /* [in] */ BSTR offset_color,
            /* [in] */ DOUBLE sim,
            /* [in] */ LONG dir,
            /* [retval][out] */ BSTR *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, FindPic)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindPic )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR files,
            /* [in] */ BSTR delta_color,
            /* [in] */ DOUBLE sim,
            /* [in] */ LONG dir,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, FindPicEx)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindPicEx )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR files,
            /* [in] */ BSTR delta_color,
            /* [in] */ DOUBLE sim,
            /* [in] */ LONG dir,
            /* [retval][out] */ BSTR *retstr);
        
        DECLSPEC_XFGVIRT(IOpInterface, FindPicExS)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindPicExS )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR files,
            /* [in] */ BSTR delta_color,
            /* [in] */ DOUBLE sim,
            /* [in] */ LONG dir,
            /* [retval][out] */ BSTR *retstr);
        
        DECLSPEC_XFGVIRT(IOpInterface, FindColorBlock)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindColorBlock )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [in] */ LONG count,
            /* [in] */ LONG height,
            /* [in] */ LONG width,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, FindColorBlockEx)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindColorBlockEx )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [in] */ LONG count,
            /* [in] */ LONG height,
            /* [in] */ LONG width,
            /* [retval][out] */ BSTR *retstr);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetResultCount)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetResultCount )( 
            IOpInterface * This,
            /* [in] */ BSTR str,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetResultPos)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetResultPos )( 
            IOpInterface * This,
            /* [in] */ BSTR str,
            /* [in] */ LONG index,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetColor)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetColor )( 
            IOpInterface * This,
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [retval][out] */ BSTR *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, SetDisplayInput)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetDisplayInput )( 
            IOpInterface * This,
            /* [in] */ BSTR method,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, LoadPic)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LoadPic )( 
            IOpInterface * This,
            /* [in] */ BSTR pic_name,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, FreePic)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FreePic )( 
            IOpInterface * This,
            /* [in] */ BSTR Pic_name,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetScreenData)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetScreenData )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetScreenDataBmp)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetScreenDataBmp )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [out] */ VARIANT *data,
            /* [out] */ VARIANT *size,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, MatchPicName)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MatchPicName )( 
            IOpInterface * This,
            /* [in] */ BSTR pic_name,
            /* [retval][out] */ BSTR *retstr);
        
        DECLSPEC_XFGVIRT(IOpInterface, LoadMemPic)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LoadMemPic )( 
            IOpInterface * This,
            /* [in] */ BSTR pic_name,
            /* [in] */ long long data,
            /* [in] */ LONG size,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, SetMouseDelay)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetMouseDelay )( 
            IOpInterface * This,
            /* [in] */ BSTR type,
            /* [in] */ LONG delay,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, SetKeypadDelay)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetKeypadDelay )( 
            IOpInterface * This,
            /* [in] */ BSTR type,
            /* [in] */ LONG delay,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, SetClipboard)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetClipboard )( 
            IOpInterface * This,
            /* [in] */ BSTR str,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetClipboard)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetClipboard )( 
            IOpInterface * This,
            /* [retval][out] */ BSTR *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, Delay)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Delay )( 
            IOpInterface * This,
            /* [in] */ LONG mis,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, Delays)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Delays )( 
            IOpInterface * This,
            /* [in] */ LONG mis_min,
            /* [in] */ LONG mis_max,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, KeyPressStr)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *KeyPressStr )( 
            IOpInterface * This,
            /* [in] */ BSTR key_str,
            /* [in] */ LONG delay,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetColorNum)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetColorNum )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetPicSize)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetPicSize )( 
            IOpInterface * This,
            /* [in] */ BSTR pic_name,
            /* [out] */ VARIANT *width,
            /* [out] */ VARIANT *height,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, SetDict)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetDict )( 
            IOpInterface * This,
            /* [in] */ LONG idx,
            /* [in] */ BSTR file_name,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, UseDict)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *UseDict )( 
            IOpInterface * This,
            /* [in] */ LONG idx,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, Ocr)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Ocr )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ BSTR *ret_str);
        
        DECLSPEC_XFGVIRT(IOpInterface, OcrEx)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *OcrEx )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ BSTR *ret_str);
        
        DECLSPEC_XFGVIRT(IOpInterface, FindStr)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindStr )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR str,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [out] */ VARIANT *retx,
            /* [out] */ VARIANT *rety,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, FindStrEx)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindStrEx )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR str,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ BSTR *ret_str);
        
        DECLSPEC_XFGVIRT(IOpInterface, OcrAuto)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *OcrAuto )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ BSTR *retstr);
        
        DECLSPEC_XFGVIRT(IOpInterface, OcrFromFile)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *OcrFromFile )( 
            IOpInterface * This,
            /* [in] */ BSTR file_name,
            /* [in] */ BSTR color_format,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ BSTR *retstr);
        
        DECLSPEC_XFGVIRT(IOpInterface, OcrAutoFromFile)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *OcrAutoFromFile )( 
            IOpInterface * This,
            /* [in] */ BSTR file_name,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ BSTR *retstr);
        
        DECLSPEC_XFGVIRT(IOpInterface, FindLine)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindLine )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ BSTR *retstr);
        
        DECLSPEC_XFGVIRT(IOpInterface, SetMemDict)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetMemDict )( 
            IOpInterface * This,
            /* [in] */ LONG idx,
            /* [in] */ BSTR data,
            /* [in] */ LONG size,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetDict)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetDict )( 
            IOpInterface * This,
            /* [in] */ LONG idx,
            /* [in] */ LONG font_index,
            /* [retval][out] */ BSTR *retstr);
        
        DECLSPEC_XFGVIRT(IOpInterface, AddDict)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *AddDict )( 
            IOpInterface * This,
            /* [in] */ LONG idx,
            /* [in] */ BSTR dict_info,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, SaveDict)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SaveDict )( 
            IOpInterface * This,
            /* [in] */ LONG idx,
            /* [in] */ BSTR file_name,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, ClearDict)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ClearDict )( 
            IOpInterface * This,
            /* [in] */ LONG idx,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetDictCount)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetDictCount )( 
            IOpInterface * This,
            /* [in] */ LONG idx,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetNowDict)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetNowDict )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, FetchWord)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FetchWord )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ BSTR word,
            /* [retval][out] */ BSTR *retstr);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetWordsNoDict)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWordsNoDict )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [retval][out] */ BSTR *retstr);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetWordResultCount)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWordResultCount )( 
            IOpInterface * This,
            /* [in] */ BSTR result,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetWordResultPos)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWordResultPos )( 
            IOpInterface * This,
            /* [in] */ BSTR result,
            /* [in] */ LONG index,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, GetWordResultStr)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWordResultStr )( 
            IOpInterface * This,
            /* [in] */ BSTR result,
            /* [in] */ LONG index,
            /* [retval][out] */ BSTR *retstr);
        
        DECLSPEC_XFGVIRT(IOpInterface, WriteData)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *WriteData )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ BSTR address,
            /* [in] */ BSTR data,
            /* [in] */ LONG size,
            /* [retval][out] */ LONG *ret);
        
        DECLSPEC_XFGVIRT(IOpInterface, ReadData)
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ReadData )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ BSTR address,
            /* [in] */ LONG size,
            /* [retval][out] */ BSTR *retstr);
        
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

#define IOpInterface_GetBasePath(This,path)	\
    ( (This)->lpVtbl -> GetBasePath(This,path) ) 

#define IOpInterface_GetID(This,ret)	\
    ( (This)->lpVtbl -> GetID(This,ret) ) 

#define IOpInterface_GetLastError(This,ret)	\
    ( (This)->lpVtbl -> GetLastError(This,ret) ) 

#define IOpInterface_SetShowErrorMsg(This,show_type,ret)	\
    ( (This)->lpVtbl -> SetShowErrorMsg(This,show_type,ret) ) 

#define IOpInterface_Sleep(This,millseconds,ret)	\
    ( (This)->lpVtbl -> Sleep(This,millseconds,ret) ) 

#define IOpInterface_InjectDll(This,process_name,dll_name,ret)	\
    ( (This)->lpVtbl -> InjectDll(This,process_name,dll_name,ret) ) 

#define IOpInterface_EnablePicCache(This,enable,ret)	\
    ( (This)->lpVtbl -> EnablePicCache(This,enable,ret) ) 

#define IOpInterface_CapturePre(This,file_name,ret)	\
    ( (This)->lpVtbl -> CapturePre(This,file_name,ret) ) 

#define IOpInterface_AStarFindPath(This,mapWidth,mapHeight,disable_points,beginX,beginY,endX,endY,path)	\
    ( (This)->lpVtbl -> AStarFindPath(This,mapWidth,mapHeight,disable_points,beginX,beginY,endX,endY,path) ) 

#define IOpInterface_FindNearestPos(This,all_pos,type,x,y,retstr)	\
    ( (This)->lpVtbl -> FindNearestPos(This,all_pos,type,x,y,retstr) ) 

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

#define IOpInterface_SendString(This,hwnd,str,ret)	\
    ( (This)->lpVtbl -> SendString(This,hwnd,str,ret) ) 

#define IOpInterface_SendStringIme(This,hwnd,str,ret)	\
    ( (This)->lpVtbl -> SendStringIme(This,hwnd,str,ret) ) 

#define IOpInterface_RunApp(This,cmdline,mode,ret)	\
    ( (This)->lpVtbl -> RunApp(This,cmdline,mode,ret) ) 

#define IOpInterface_WinExec(This,cmdline,cmdshow,ret)	\
    ( (This)->lpVtbl -> WinExec(This,cmdline,cmdshow,ret) ) 

#define IOpInterface_GetCmdStr(This,cmd,millseconds,retstr)	\
    ( (This)->lpVtbl -> GetCmdStr(This,cmd,millseconds,retstr) ) 

#define IOpInterface_BindWindow(This,hwnd,display,mouse,keypad,mode,ret)	\
    ( (This)->lpVtbl -> BindWindow(This,hwnd,display,mouse,keypad,mode,ret) ) 

#define IOpInterface_UnBindWindow(This,ret)	\
    ( (This)->lpVtbl -> UnBindWindow(This,ret) ) 

#define IOpInterface_GetBindWindow(This,ret)	\
    ( (This)->lpVtbl -> GetBindWindow(This,ret) ) 

#define IOpInterface_IsBind(This,ret)	\
    ( (This)->lpVtbl -> IsBind(This,ret) ) 

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

#define IOpInterface_KeyPress(This,vk_code,ret)	\
    ( (This)->lpVtbl -> KeyPress(This,vk_code,ret) ) 

#define IOpInterface_KeyPressChar(This,vk_code,ret)	\
    ( (This)->lpVtbl -> KeyPressChar(This,vk_code,ret) ) 

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

#define IOpInterface_FindPicExS(This,x1,y1,x2,y2,files,delta_color,sim,dir,retstr)	\
    ( (This)->lpVtbl -> FindPicExS(This,x1,y1,x2,y2,files,delta_color,sim,dir,retstr) ) 

#define IOpInterface_FindColorBlock(This,x1,y1,x2,y2,color,sim,count,height,width,x,y,ret)	\
    ( (This)->lpVtbl -> FindColorBlock(This,x1,y1,x2,y2,color,sim,count,height,width,x,y,ret) ) 

#define IOpInterface_FindColorBlockEx(This,x1,y1,x2,y2,color,sim,count,height,width,retstr)	\
    ( (This)->lpVtbl -> FindColorBlockEx(This,x1,y1,x2,y2,color,sim,count,height,width,retstr) ) 

#define IOpInterface_GetResultCount(This,str,ret)	\
    ( (This)->lpVtbl -> GetResultCount(This,str,ret) ) 

#define IOpInterface_GetResultPos(This,str,index,x,y,ret)	\
    ( (This)->lpVtbl -> GetResultPos(This,str,index,x,y,ret) ) 

#define IOpInterface_GetColor(This,x,y,ret)	\
    ( (This)->lpVtbl -> GetColor(This,x,y,ret) ) 

#define IOpInterface_SetDisplayInput(This,method,ret)	\
    ( (This)->lpVtbl -> SetDisplayInput(This,method,ret) ) 

#define IOpInterface_LoadPic(This,pic_name,ret)	\
    ( (This)->lpVtbl -> LoadPic(This,pic_name,ret) ) 

#define IOpInterface_FreePic(This,Pic_name,ret)	\
    ( (This)->lpVtbl -> FreePic(This,Pic_name,ret) ) 

#define IOpInterface_GetScreenData(This,x1,y1,x2,y2,ret)	\
    ( (This)->lpVtbl -> GetScreenData(This,x1,y1,x2,y2,ret) ) 

#define IOpInterface_GetScreenDataBmp(This,x1,y1,x2,y2,data,size,ret)	\
    ( (This)->lpVtbl -> GetScreenDataBmp(This,x1,y1,x2,y2,data,size,ret) ) 

#define IOpInterface_MatchPicName(This,pic_name,retstr)	\
    ( (This)->lpVtbl -> MatchPicName(This,pic_name,retstr) ) 

#define IOpInterface_LoadMemPic(This,pic_name,data,size,ret)	\
    ( (This)->lpVtbl -> LoadMemPic(This,pic_name,data,size,ret) ) 

#define IOpInterface_SetMouseDelay(This,type,delay,ret)	\
    ( (This)->lpVtbl -> SetMouseDelay(This,type,delay,ret) ) 

#define IOpInterface_SetKeypadDelay(This,type,delay,ret)	\
    ( (This)->lpVtbl -> SetKeypadDelay(This,type,delay,ret) ) 

#define IOpInterface_SetClipboard(This,str,ret)	\
    ( (This)->lpVtbl -> SetClipboard(This,str,ret) ) 

#define IOpInterface_GetClipboard(This,ret)	\
    ( (This)->lpVtbl -> GetClipboard(This,ret) ) 

#define IOpInterface_Delay(This,mis,ret)	\
    ( (This)->lpVtbl -> Delay(This,mis,ret) ) 

#define IOpInterface_Delays(This,mis_min,mis_max,ret)	\
    ( (This)->lpVtbl -> Delays(This,mis_min,mis_max,ret) ) 

#define IOpInterface_KeyPressStr(This,key_str,delay,ret)	\
    ( (This)->lpVtbl -> KeyPressStr(This,key_str,delay,ret) ) 

#define IOpInterface_GetColorNum(This,x1,y1,x2,y2,color,sim,ret)	\
    ( (This)->lpVtbl -> GetColorNum(This,x1,y1,x2,y2,color,sim,ret) ) 

#define IOpInterface_GetPicSize(This,pic_name,width,height,ret)	\
    ( (This)->lpVtbl -> GetPicSize(This,pic_name,width,height,ret) ) 

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

#define IOpInterface_OcrAuto(This,x1,y1,x2,y2,sim,retstr)	\
    ( (This)->lpVtbl -> OcrAuto(This,x1,y1,x2,y2,sim,retstr) ) 

#define IOpInterface_OcrFromFile(This,file_name,color_format,sim,retstr)	\
    ( (This)->lpVtbl -> OcrFromFile(This,file_name,color_format,sim,retstr) ) 

#define IOpInterface_OcrAutoFromFile(This,file_name,sim,retstr)	\
    ( (This)->lpVtbl -> OcrAutoFromFile(This,file_name,sim,retstr) ) 

#define IOpInterface_FindLine(This,x1,y1,x2,y2,color,sim,retstr)	\
    ( (This)->lpVtbl -> FindLine(This,x1,y1,x2,y2,color,sim,retstr) ) 

#define IOpInterface_SetMemDict(This,idx,data,size,ret)	\
    ( (This)->lpVtbl -> SetMemDict(This,idx,data,size,ret) ) 

#define IOpInterface_GetDict(This,idx,font_index,retstr)	\
    ( (This)->lpVtbl -> GetDict(This,idx,font_index,retstr) ) 

#define IOpInterface_AddDict(This,idx,dict_info,ret)	\
    ( (This)->lpVtbl -> AddDict(This,idx,dict_info,ret) ) 

#define IOpInterface_SaveDict(This,idx,file_name,ret)	\
    ( (This)->lpVtbl -> SaveDict(This,idx,file_name,ret) ) 

#define IOpInterface_ClearDict(This,idx,ret)	\
    ( (This)->lpVtbl -> ClearDict(This,idx,ret) ) 

#define IOpInterface_GetDictCount(This,idx,ret)	\
    ( (This)->lpVtbl -> GetDictCount(This,idx,ret) ) 

#define IOpInterface_GetNowDict(This,ret)	\
    ( (This)->lpVtbl -> GetNowDict(This,ret) ) 

#define IOpInterface_FetchWord(This,x1,y1,x2,y2,color,word,retstr)	\
    ( (This)->lpVtbl -> FetchWord(This,x1,y1,x2,y2,color,word,retstr) ) 

#define IOpInterface_GetWordsNoDict(This,x1,y1,x2,y2,color,retstr)	\
    ( (This)->lpVtbl -> GetWordsNoDict(This,x1,y1,x2,y2,color,retstr) ) 

#define IOpInterface_GetWordResultCount(This,result,ret)	\
    ( (This)->lpVtbl -> GetWordResultCount(This,result,ret) ) 

#define IOpInterface_GetWordResultPos(This,result,index,x,y,ret)	\
    ( (This)->lpVtbl -> GetWordResultPos(This,result,index,x,y,ret) ) 

#define IOpInterface_GetWordResultStr(This,result,index,retstr)	\
    ( (This)->lpVtbl -> GetWordResultStr(This,result,index,retstr) ) 

#define IOpInterface_WriteData(This,hwnd,address,data,size,ret)	\
    ( (This)->lpVtbl -> WriteData(This,hwnd,address,data,size,ret) ) 

#define IOpInterface_ReadData(This,hwnd,address,size,retstr)	\
    ( (This)->lpVtbl -> ReadData(This,hwnd,address,size,retstr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IOpInterface_ClearDict_Proxy( 
    IOpInterface * This,
    /* [in] */ LONG idx,
    /* [retval][out] */ LONG *ret);


void __RPC_STUB IOpInterface_ClearDict_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IOpInterface_GetDictCount_Proxy( 
    IOpInterface * This,
    /* [in] */ LONG idx,
    /* [retval][out] */ LONG *ret);


void __RPC_STUB IOpInterface_GetDictCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IOpInterface_GetNowDict_Proxy( 
    IOpInterface * This,
    /* [retval][out] */ LONG *ret);


void __RPC_STUB IOpInterface_GetNowDict_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IOpInterface_FetchWord_Proxy( 
    IOpInterface * This,
    /* [in] */ LONG x1,
    /* [in] */ LONG y1,
    /* [in] */ LONG x2,
    /* [in] */ LONG y2,
    /* [in] */ BSTR color,
    /* [in] */ BSTR word,
    /* [retval][out] */ BSTR *retstr);


void __RPC_STUB IOpInterface_FetchWord_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IOpInterface_GetWordsNoDict_Proxy( 
    IOpInterface * This,
    /* [in] */ LONG x1,
    /* [in] */ LONG y1,
    /* [in] */ LONG x2,
    /* [in] */ LONG y2,
    /* [in] */ BSTR color,
    /* [retval][out] */ BSTR *retstr);


void __RPC_STUB IOpInterface_GetWordsNoDict_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IOpInterface_GetWordResultCount_Proxy( 
    IOpInterface * This,
    /* [in] */ BSTR result,
    /* [retval][out] */ LONG *ret);


void __RPC_STUB IOpInterface_GetWordResultCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IOpInterface_GetWordResultPos_Proxy( 
    IOpInterface * This,
    /* [in] */ BSTR result,
    /* [in] */ LONG index,
    /* [out] */ VARIANT *x,
    /* [out] */ VARIANT *y,
    /* [retval][out] */ LONG *ret);


void __RPC_STUB IOpInterface_GetWordResultPos_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IOpInterface_GetWordResultStr_Proxy( 
    IOpInterface * This,
    /* [in] */ BSTR result,
    /* [in] */ LONG index,
    /* [retval][out] */ BSTR *retstr);


void __RPC_STUB IOpInterface_GetWordResultStr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IOpInterface_WriteData_Proxy( 
    IOpInterface * This,
    /* [in] */ LONG hwnd,
    /* [in] */ BSTR address,
    /* [in] */ BSTR data,
    /* [in] */ LONG size,
    /* [retval][out] */ LONG *ret);


void __RPC_STUB IOpInterface_WriteData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IOpInterface_ReadData_Proxy( 
    IOpInterface * This,
    /* [in] */ LONG hwnd,
    /* [in] */ BSTR address,
    /* [in] */ LONG size,
    /* [retval][out] */ BSTR *retstr);


void __RPC_STUB IOpInterface_ReadData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IOpInterface_INTERFACE_DEFINED__ */



#ifndef __opLib_LIBRARY_DEFINED__
#define __opLib_LIBRARY_DEFINED__

/* library opLib */
/* [custom][version][uuid] */ 


EXTERN_C const IID LIBID_opLib;

EXTERN_C const CLSID CLSID_OpInterface;

#ifdef __cplusplus

class DECLSPEC_UUID("12bec402-a06e-4fad-a7d4-830f967374c6")
OpInterface;
#endif
#endif /* __opLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

unsigned long             __RPC_USER  BSTR_UserSize64(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal64(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal64(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree64(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize64(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal64(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal64(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree64(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


