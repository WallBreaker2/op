#pragma once
#ifndef __COMMON_H_
#define __COMMON_H_
#include <fstream>



#define SAFE_CLOSE(h)if(h) CloseHandle(h);h=NULL;

#define SAFE_DELETE(ptr) if(ptr)delete ptr;ptr=nullptr

#define SAFE_DELETE_ARRAY(ptr) if(ptr)delete[] ptr;ptr=nullptr

#include <string>
#include <boost/locale.hpp>
#include <boost/algorithm/string.hpp>
#define _sto_wstring(s) boost::locale::conv::to_utf<wchar_t>(s, "GBK")
#define _wsto_string(s)  boost::locale::conv::from_utf(s,"GBK")

#define DLL_API extern "C" _declspec(dllexport)
enum BACKTYPE { NORMAL,WINDOWS, GDI, DX, OPENGL };

const size_t MAX_IMAGE_WIDTH = 1<<11;
const size_t SHARED_MEMORY_SIZE = 1080 * 1928 * 4;

extern HINSTANCE gInstance;
//for debug
long setlog(const wchar_t* format, ...);
//
long setlog(const char* format, ...);

void split(const std::wstring& s, std::vector<std::wstring>& v, const std::wstring& c);
#endif