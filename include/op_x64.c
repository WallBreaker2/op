

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 8.01.0622 */
/* at Tue Jan 19 11:14:07 2038
 */
/* Compiler settings for C:\Users\wall\AppData\Local\Temp\IDLC5FD.tmp:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0622 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        EXTERN_C __declspec(selectany) const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif // !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, LIBID_opLib,0x66B9C175,0x82F2,0x45E9,0xAF,0x86,0x58,0xAD,0x5D,0xED,0x5A,0xDC);


MIDL_DEFINE_GUID(IID, IID_IComponentRegistrar,0xA817E7A2,0x43FA,0x11D0,0x9E,0x44,0x00,0xAA,0x00,0xB6,0x77,0x0A);


MIDL_DEFINE_GUID(IID, IID_IOpInterface,0x51E59A6F,0x85F4,0x4DA0,0xA0,0x1E,0xC9,0xB6,0xB3,0xB8,0xB8,0xA7);


MIDL_DEFINE_GUID(CLSID, CLSID_CompReg,0x54CA0535,0xFB49,0x4D91,0x87,0x09,0x78,0x6F,0xB8,0x72,0x51,0x32);


MIDL_DEFINE_GUID(CLSID, CLSID_OpInterface,0x12BEC402,0xA06E,0x4FAD,0xA7,0xD4,0x83,0x0F,0x96,0x73,0x74,0xC6);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



