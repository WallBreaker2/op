

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0622 */
/* at Tue Jan 19 11:14:07 2038
 */
/* Compiler settings for op.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.01.0622 
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
            BSTR dll_name,
            LONG *ret) = 0;
        
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
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
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
            /* [out] */ VARIANT *data,
            /* [retval][out] */ LONG *ret) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetScreenDataBmp( 
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [out] */ VARIANT *data,
            /* [out] */ VARIANT *size,
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
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetPath )( 
            IOpInterface * This,
            /* [retval][out] */ BSTR *path);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetBasePath )( 
            IOpInterface * This,
            /* [retval][out] */ BSTR *path);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetID )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetLastError )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetShowErrorMsg )( 
            IOpInterface * This,
            /* [in] */ LONG show_type,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Sleep )( 
            IOpInterface * This,
            /* [in] */ LONG millseconds,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *InjectDll )( 
            IOpInterface * This,
            /* [in] */ BSTR process_name,
            BSTR dll_name,
            LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *EnablePicCache )( 
            IOpInterface * This,
            /* [in] */ LONG enable,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CapturePre )( 
            IOpInterface * This,
            /* [in] */ BSTR file_name,
            /* [retval][out] */ LONG *ret);
        
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
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *EnumWindow )( 
            IOpInterface * This,
            /* [in] */ LONG parent,
            /* [in] */ BSTR title,
            /* [in] */ BSTR class_name,
            /* [in] */ LONG filter,
            /* [retval][out] */ BSTR *retstr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *EnumWindowByProcess )( 
            IOpInterface * This,
            /* [in] */ BSTR process_name,
            /* [in] */ BSTR title,
            /* [in] */ BSTR class_name,
            /* [in] */ LONG filter,
            /* [retval][out] */ BSTR *retstring);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *EnumProcess )( 
            IOpInterface * This,
            /* [in] */ BSTR name,
            /* [retval][out] */ BSTR *retstring);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ClientToScreen )( 
            IOpInterface * This,
            /* [in] */ LONG ClientToScreen,
            /* [out][in] */ VARIANT *x,
            /* [out][in] */ VARIANT *y,
            /* [retval][out] */ LONG *bret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindWindow )( 
            IOpInterface * This,
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ LONG *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindWindowByProcess )( 
            IOpInterface * This,
            /* [in] */ BSTR process_name,
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ LONG *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindWindowByProcessId )( 
            IOpInterface * This,
            /* [in] */ LONG process_id,
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ LONG *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FindWindowEx )( 
            IOpInterface * This,
            /* [in] */ LONG parent,
            /* [in] */ BSTR class_name,
            /* [in] */ BSTR title,
            /* [retval][out] */ LONG *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetClientRect )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [out] */ VARIANT *x1,
            /* [out] */ VARIANT *y1,
            /* [out] */ VARIANT *x2,
            /* [out] */ VARIANT *y2,
            /* [retval][out] */ LONG *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetClientSize )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [out] */ VARIANT *width,
            /* [out] */ VARIANT *height,
            /* [retval][out] */ LONG *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetForegroundFocus )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetForegroundWindow )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetMousePointWindow )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetPointWindow )( 
            IOpInterface * This,
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [retval][out] */ LONG *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetProcessInfo )( 
            IOpInterface * This,
            /* [in] */ LONG pid,
            /* [retval][out] */ BSTR *retstring);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetSpecialWindow )( 
            IOpInterface * This,
            /* [in] */ LONG flag,
            /* [retval][out] */ LONG *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindow )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ LONG flag,
            /* [retval][out] */ LONG *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindowClass )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [retval][out] */ BSTR *retstring);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindowProcessId )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [retval][out] */ LONG *nretpid);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindowProcessPath )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [retval][out] */ BSTR *retstring);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindowRect )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [out] */ VARIANT *x1,
            /* [out] */ VARIANT *y1,
            /* [out] */ VARIANT *x2,
            /* [out] */ VARIANT *y2,
            /* [retval][out] */ LONG *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindowState )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ LONG flag,
            /* [retval][out] */ LONG *rethwnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetWindowTitle )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [retval][out] */ BSTR *rettitle);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MoveWindow )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [retval][out] */ LONG *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ScreenToClient )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ LONG *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SendPaste )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [retval][out] */ LONG *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetClientSize )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ LONG width,
            /* [in] */ LONG hight,
            /* [retval][out] */ LONG *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetWindowState )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ LONG flag,
            /* [retval][out] */ LONG *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetWindowSize )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ LONG width,
            /* [in] */ LONG height,
            /* [retval][out] */ LONG *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetWindowText )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ BSTR title,
            /* [retval][out] */ LONG *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetWindowTransparent )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ LONG trans,
            /* [retval][out] */ LONG *nret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SendString )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ BSTR str,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SendStringIme )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ BSTR str,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RunApp )( 
            IOpInterface * This,
            /* [in] */ BSTR cmdline,
            /* [in] */ LONG mode,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *WinExec )( 
            IOpInterface * This,
            /* [in] */ BSTR cmdline,
            /* [in] */ LONG cmdshow,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetCmdStr )( 
            IOpInterface * This,
            /* [in] */ BSTR cmd,
            /* [in] */ LONG millseconds,
            /* [retval][out] */ BSTR *retstr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *BindWindow )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ BSTR display,
            /* [in] */ BSTR mouse,
            /* [in] */ BSTR keypad,
            /* [in] */ LONG mode,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *UnBindWindow )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetCursorPos )( 
            IOpInterface * This,
            /* [out] */ VARIANT *x,
            /* [out] */ VARIANT *y,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MoveR )( 
            IOpInterface * This,
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MoveTo )( 
            IOpInterface * This,
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MoveToEx )( 
            IOpInterface * This,
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [in] */ LONG w,
            /* [in] */ long h,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LeftClick )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LeftDoubleClick )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LeftDown )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LeftUp )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MiddleClick )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MiddleDown )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MiddleUp )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RightClick )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RightDown )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RightUp )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *WheelDown )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *WheelUp )( 
            IOpInterface * This,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetKeyState )( 
            IOpInterface * This,
            /* [in] */ LONG vk_code,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *KeyDown )( 
            IOpInterface * This,
            /* [in] */ LONG vk_code,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *KeyDownChar )( 
            IOpInterface * This,
            /* [in] */ BSTR vk_code,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *KeyUp )( 
            IOpInterface * This,
            /* [in] */ LONG vk_code,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *KeyUpChar )( 
            IOpInterface * This,
            /* [in] */ BSTR vk_code,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *WaitKey )( 
            IOpInterface * This,
            /* [in] */ LONG vk_code,
            /* [in] */ LONG time_out,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *KeyPress )( 
            IOpInterface * This,
            /* [in] */ LONG vk_code,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *KeyPressChar )( 
            IOpInterface * This,
            /* [in] */ BSTR vk_code,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Capture )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR file_name,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CmpColor )( 
            IOpInterface * This,
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ LONG *ret);
        
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
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetColor )( 
            IOpInterface * This,
            /* [in] */ LONG x,
            /* [in] */ LONG y,
            /* [retval][out] */ BSTR *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetDisplayInput )( 
            IOpInterface * This,
            /* [in] */ BSTR method,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *LoadPic )( 
            IOpInterface * This,
            /* [in] */ BSTR pic_name,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *FreePic )( 
            IOpInterface * This,
            /* [in] */ BSTR Pic_name,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetScreenData )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [out] */ VARIANT *data,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetScreenDataBmp )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [out] */ VARIANT *data,
            /* [out] */ VARIANT *size,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetDict )( 
            IOpInterface * This,
            /* [in] */ LONG idx,
            /* [in] */ BSTR file_name,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *UseDict )( 
            IOpInterface * This,
            /* [in] */ LONG idx,
            /* [retval][out] */ LONG *ret);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Ocr )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ BSTR *ret_str);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *OcrEx )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ BSTR color,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ BSTR *ret_str);
        
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
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *OcrAuto )( 
            IOpInterface * This,
            /* [in] */ LONG x1,
            /* [in] */ LONG y1,
            /* [in] */ LONG x2,
            /* [in] */ LONG y2,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ BSTR *retstr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *OcrFromFile )( 
            IOpInterface * This,
            /* [in] */ BSTR file_name,
            /* [in] */ BSTR color_format,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ BSTR *retstr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *OcrAutoFromFile )( 
            IOpInterface * This,
            /* [in] */ BSTR file_name,
            /* [in] */ DOUBLE sim,
            /* [retval][out] */ BSTR *retstr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *WriteData )( 
            IOpInterface * This,
            /* [in] */ LONG hwnd,
            /* [in] */ BSTR address,
            /* [in] */ BSTR data,
            /* [in] */ LONG size,
            /* [retval][out] */ LONG *ret);
        
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

#define IOpInterface_GetColor(This,x,y,ret)	\
    ( (This)->lpVtbl -> GetColor(This,x,y,ret) ) 

#define IOpInterface_SetDisplayInput(This,method,ret)	\
    ( (This)->lpVtbl -> SetDisplayInput(This,method,ret) ) 

#define IOpInterface_LoadPic(This,pic_name,ret)	\
    ( (This)->lpVtbl -> LoadPic(This,pic_name,ret) ) 

#define IOpInterface_FreePic(This,Pic_name,ret)	\
    ( (This)->lpVtbl -> FreePic(This,Pic_name,ret) ) 

#define IOpInterface_GetScreenData(This,x1,y1,x2,y2,data,ret)	\
    ( (This)->lpVtbl -> GetScreenData(This,x1,y1,x2,y2,data,ret) ) 

#define IOpInterface_GetScreenDataBmp(This,x1,y1,x2,y2,data,size,ret)	\
    ( (This)->lpVtbl -> GetScreenDataBmp(This,x1,y1,x2,y2,data,size,ret) ) 

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

#define IOpInterface_WriteData(This,hwnd,address,data,size,ret)	\
    ( (This)->lpVtbl -> WriteData(This,hwnd,address,data,size,ret) ) 

#define IOpInterface_ReadData(This,hwnd,address,size,retstr)	\
    ( (This)->lpVtbl -> ReadData(This,hwnd,address,size,retstr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




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


