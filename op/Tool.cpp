#include "stdafx.h"
#include "Tool.h"
#include <algorithm>
std::wstring _sto_wstring(const std::string&s) {
	std::string strLocale = setlocale(LC_ALL, "");
	const char* chSrc = s.c_str();
	size_t nDestSize = mbstowcs(NULL, chSrc, 0) + 1;
	wchar_t* wchDest = new wchar_t[nDestSize];
	wmemset(wchDest, 0, nDestSize);
	mbstowcs(wchDest, chSrc, nDestSize);
	std::wstring wstrResult = wchDest;
	delete[]wchDest;
	setlocale(LC_ALL, strLocale.c_str());
	return wstrResult;
}

std::string _wsto_string(const std::wstring&ws) {
	std::string strLocale = setlocale(LC_ALL, "");
	const wchar_t* wchSrc = ws.c_str();
	size_t nDestSize = wcstombs(NULL, wchSrc, 0) + 1;
	char *chDest = new char[nDestSize];
	memset(chDest, 0, nDestSize);
	wcstombs(chDest, wchSrc, nDestSize);
	std::string strResult = chDest;
	delete[]chDest;
	setlocale(LC_ALL, strLocale.c_str());
	return strResult;
}

long Tool::setlog(const wchar_t* format, ...) {
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

long Tool::setlog(const char* format, ...) {
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

void Tool::split(const std::wstring& s, std::vector<std::wstring>& v, const std::wstring& c)
{
	std::wstring::size_type pos1, pos2;
	size_t len = s.length();
	pos2 = s.find(c);
	pos1 = 0;
	v.clear();
	while (std::wstring::npos != pos2)
	{
		v.emplace_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != len)
		v.emplace_back(s.substr(pos1));
}

void Tool::split(const std::string& s, std::vector<std::string>& v, const std::string& c)
{
	std::string::size_type pos1, pos2;
	size_t len = s.length();
	pos2 = s.find(c);
	pos1 = 0;
	v.clear();
	while (std::string::npos != pos2)
	{
		v.emplace_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != len)
		v.emplace_back(s.substr(pos1));
}

void Tool::wstring2upper(std::wstring& s) {
	std::transform(s.begin(), s.end(),s.begin(), towupper);
}

void Tool::string2upper(std::string& s) {
	std::transform(s.begin(), s.end(), s.begin(), toupper);
}