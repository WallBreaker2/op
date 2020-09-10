#include "stdafx.h"
#include "helpfunc.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include "globalVar.h"
std::wstring _s2wstring(const std::string&s) {
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

std::string _ws2string(const std::wstring&ws) {
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

long Path2GlobalPath(const std::wstring&file, const std::wstring& curr_path, std::wstring& out) {
	if (::PathFileExistsW(file.c_str())) {
		out = file;
		return 1;
	}
	out.clear();
	out = curr_path + L"\\" + file;
	if (::PathFileExistsW(out.c_str())) {
		return 1;
	}
	return 0;
}

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
	if (gShowError == 1) {
		MessageBoxW(NULL, buf, L"error", MB_ICONERROR);
	}
	else if (gShowError == 2) {
		wchar_t dll_path[MAX_PATH];
		::GetModuleFileNameW(gInstance, dll_path, MAX_PATH);
		wstring fname = dll_path;
		fname = fname.substr(0, fname.rfind(L'\\'));
		fname += L"\\op.log";
		std::wfstream file;
		file.open(fname, std::ios::app | std::ios::out);
		if (!file.is_open())
			return 0;
		file << tm << buf << std::endl;
		file.close();
	}
	else if (gShowError == 3) {
		std::wcout << tm << buf << std::endl;
	}
	
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
	if (gShowError == 1) {
		MessageBoxA(NULL, buf, "error", MB_ICONERROR);
	}
	else if (gShowError == 2) {
		wchar_t dll_path[MAX_PATH];
		::GetModuleFileNameW(gInstance, dll_path, MAX_PATH);
		wstring fname = dll_path;
		fname = fname.substr(0, fname.rfind(L'\\'));
		fname += L"\\op.log";
		std::fstream file;
		file.open(fname, std::ios::app | std::ios::out);
		if (!file.is_open())
			return 0;
		file << tm << buf << std::endl;
		file.close();
	}
	else if(gShowError==3){
		std::cout << tm << buf << std::endl;
	}
	return 1;
}

void split(const std::wstring& s, std::vector<std::wstring>& v, const std::wstring& c)
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

void split(const std::string& s, std::vector<std::string>& v, const std::string& c)
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

void wstring2upper(std::wstring& s) {
	std::transform(s.begin(), s.end(),s.begin(), towupper);
}

void string2upper(std::string& s) {
	std::transform(s.begin(), s.end(), s.begin(), toupper);
}

void wstring2lower(std::wstring& s) {
	std::transform(s.begin(), s.end(), s.begin(), towlower);
}

void string2lower(std::string& s) {
	std::transform(s.begin(), s.end(), s.begin(), tolower);
}

void replacea(string& str, const string&oldval, const string& newval) {
	size_t x0 = 0, dx = newval.length() - oldval.length() + 1;
	size_t idx = str.find(oldval, x0);
	while (idx != -1 && x0 >= 0) {
		str.replace(idx, oldval.length(), newval);
		x0 = idx + dx;
		idx = str.find(oldval, x0);
	}
}

void replacew(wstring& str, const wstring&oldval, const wstring& newval) {
	size_t x0 = 0, dx = newval.length() - oldval.length() + 1;
	size_t idx = str.find(oldval, x0);
	while (idx != -1 && x0 >= 0) {
		str.replace(idx, oldval.length(), newval);
		x0 = idx + dx;
		idx = str.find(oldval, x0);
	}
}