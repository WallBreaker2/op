#include "stdafx.h"
#include "Common.h"
#include <time.h>

HINSTANCE gInstance;

long setlog(const wchar_t* format, ...) {
	va_list args;
	wchar_t buf[512];
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	wchar_t tm[128];
	wsprintf(tm, L"[%4d/%02d/%02d %02d:%02d:%02d.%03d]",
		sys.wYear, sys.wMonth, sys.wDay,
		sys.wHour, sys.wMinute, sys.wSecond,
		sys.wMilliseconds);
	va_start(args, format);
	vswprintf(buf, format, args);
	va_end(args);
	std::wfstream file;
	file.open(L"op.log", std::ios::app | std::ios::out);
	if (!file.is_open())
		return 0;
	file << tm << buf << std::endl;
	file.close();
	return 1;
}

long setlog(const char* format, ...) {
	va_list args;
	char buf[512];
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	char tm[128];
	sprintf(tm, "[%4d/%02d/%02d %02d:%02d:%02d.%03d]",
		sys.wYear, sys.wMonth, sys.wDay,
		sys.wHour, sys.wMinute, sys.wSecond,
		sys.wMilliseconds);
	va_start(args, format);
	vsprintf(buf, format, args);
	va_end(args);
	std::fstream file;
	file.open("op.log", std::ios::app | std::ios::out);
	if (!file.is_open())
		return 0;
	file << tm << buf << std::endl;
	file.close();
	return 1;
}

void split(const std::wstring& s, std::vector<std::wstring>& v, const std::wstring& c)
{
	std::wstring::size_type pos1, pos2;
	size_t len = s.length();
	pos2 = s.find(c);
	pos1 = 0;
	while (std::wstring::npos != pos2)
	{
		v.emplace_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != len)
		v.emplace_back(s.substr(pos1));
}