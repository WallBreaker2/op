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

void replacea(string& str, const string&oldval, const string& newval);
void replacew(wstring& str, const wstring&oldval, const wstring& newval);


//for debug
long setlog(const wchar_t* format, ...);
//
long setlog(const char* format, ...);

int inline hex2bin(int c) {
	return c <= L'9' ? c - L'0' : c - L'A' + 10;
};

int inline bin2hex(int c) {
	int ans = 0;
	int c1 = c >> 4 & 0xf;
	int c2 = c & 0xf;
	ans |= c1 <= 9 ? c1 + L'0' : c1 + 'A' - 10;
	ans |= c2 <= 9 ? c2 + L'0' : c2 + 'A' - 10;
	return ans;
};

#endif // !__TOOL_H_



