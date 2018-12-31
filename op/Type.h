#pragma once
#ifndef __TYPE_H_
#define __TYPE_H_
//version 主 副 修订 发布
#define MAKE_VERSION(a,b,c,d) (a<<24)|(b<<16)|(c<<8)|d

#define SAFE_CLOSE(h)if(h) CloseHandle(h);h=NULL;

#define SAFE_DELETE(ptr) if(ptr)delete ptr;ptr=nullptr

#define SAFE_DELETE_ARRAY(ptr) if(ptr)delete[] ptr;ptr=nullptr

#include <string>
#include <boost/locale.hpp>
#define _sto_wstring(s) boost::locale::conv::to_utf<wchar_t>(s, "GBK")
#define _wsto_string(s)  boost::locale::conv::from_utf(s,"GBK")
#endif