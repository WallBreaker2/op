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


enum BACKTYPE { NORMAL,WINDOWS, GDI, DX, OPENGL };

const size_t MAX_IMAGE_WIDTH = 1<<11;


long setlog(const wchar_t* format, ...);

void split(const std::wstring& s, std::vector<std::wstring>& v, const std::wstring& c);
#endif