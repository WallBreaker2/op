#pragma once
#ifndef __TOOL_H_
#define __TOOL_H_
#include "Common.h"
namespace Tool {
	//for debug
	long setlog(const wchar_t* format, ...);
	//
	long setlog(const char* format, ...);

	void split(const std::wstring& s, std::vector<std::wstring>& v, const std::wstring& c);
	void split(const std::string& s, std::vector<std::string>& v, const std::string& c);
};
#endif // !__TOOL_H_



