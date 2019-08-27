#pragma once
#ifndef __HELPFUCN_H_
#define __HELPFUNC_H_
#include "optype.h"
std::wstring _s2wstring(const std::string&s);
std::string _ws2string(const std::wstring&s);
//将路径转化为全局路径
long Path2GlobalPath(const std::wstring&file, const std::wstring& curr_path, std::wstring& out);

void split(const std::wstring& s, std::vector<std::wstring>& v, const std::wstring& c);
void split(const std::string& s, std::vector<std::string>& v, const std::string& c);

void wstring2upper(std::wstring& s);
void string2upper(std::string& s);


//for debug
long setlog(const wchar_t* format, ...);
//
long setlog(const char* format, ...);



#endif // !__TOOL_H_



