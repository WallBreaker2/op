//#include "stdafx.h"
#include "helpfunc.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <shlwapi.h>
#include "globalVar.h"
#include "opEnv.h"
//#define USE_BOOST_STACK_TRACE
#ifdef USE_BOOST_STACK_TRACE
#include <boost/stacktrace.hpp>
#endif

std::wstring _s2wstring(const std::string&s) {
	size_t nlen = s.length();

	wchar_t* m_char;
	int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), nlen, NULL, 0);
	m_char = new wchar_t[len + 1];
	MultiByteToWideChar(CP_ACP, 0, s.data(), nlen, m_char, len);
	m_char[len] = '\0';
	std::wstring ws(m_char);
	delete[] m_char;
	return ws;
}

std::string _ws2string(const std::wstring&ws) {
	// std::string strLocale = setlocale(LC_ALL, "");
	// const wchar_t* wchSrc = ws.c_str();
	// size_t nDestSize = wcstombs(NULL, wchSrc, 0) + 1;
	// char *chDest = new char[nDestSize];
	// memset(chDest, 0, nDestSize);
	// wcstombs(chDest, wchSrc, nDestSize);
	// std::string strResult = chDest;
	// delete[]chDest;
	// setlocale(LC_ALL, strLocale.c_str());
	//return strResult;
	int nlen = ws.length();

	char* m_char;
	int len = WideCharToMultiByte(CP_ACP, 0, ws.data(), nlen, NULL, 0, NULL, NULL);
	m_char = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, ws.data(), nlen, m_char, len, NULL, NULL);
	m_char[len] = '\0';
	std::string s(m_char);
	delete[] m_char;
	return s;
}

string utf8_to_ansi(string strUTF8) {
	UINT nLen = MultiByteToWideChar(CP_UTF8, NULL, strUTF8.c_str(), -1, NULL, NULL);
	WCHAR* wszBuffer = new WCHAR[nLen + 1];
	nLen = MultiByteToWideChar(CP_UTF8, NULL, strUTF8.c_str(), -1, wszBuffer, nLen);
	wszBuffer[nLen] = 0;
	nLen = WideCharToMultiByte(936, NULL, wszBuffer, -1, NULL, NULL, NULL, NULL);
	CHAR* szBuffer = new CHAR[nLen + 1];
	nLen = WideCharToMultiByte(936, NULL, wszBuffer, -1, szBuffer, nLen, NULL, NULL);
	szBuffer[nLen] = 0;
	strUTF8 = szBuffer;
	delete[]szBuffer;
	delete[]wszBuffer;
	return strUTF8;
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
	va_start(args, format);
	vswprintf(buf, format, args);
	va_end(args);
	wstring tmpw = buf;
	string tmps = _ws2string(tmpw);
	
	return setlog(tmps.data());
}

long setlog(const char* format, ...) {
	std::stringstream ss(std::wstringstream::in | std::wstringstream::out);
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
	ss << tm << (OP64 == 1 ? "x64" : "x32") << "info: " << buf << std::endl;
#ifdef USE_BOOST_STACK_TRACE
	ss << "<stack>\n"
		<< boost::stacktrace::stacktrace() << std::endl;
#endif // USE_BOOST_STACK_TRACE

	
	string s = ss.str();
	if (opEnv::m_showErrorMsg == 1) {
		MessageBoxA(NULL, s.data(), "error", MB_ICONERROR);
	}
	else if (opEnv::m_showErrorMsg == 2) {
		/*	wchar_t dll_path[MAX_PATH];
			::GetModuleFileNameW(gInstance, dll_path, MAX_PATH);
			wstring fname = dll_path;
			fname = fname.substr(0, fname.rfind(L'\\'));
			fname += L"\\op.log";*/
		std::fstream file;
		file.open("__op.log", std::ios::app | std::ios::out);
		if (!file.is_open())
			return 0;
		file << s << std::endl;
		file.close();
	}
	else if (opEnv::m_showErrorMsg == 3) {
		std::cout << s << std::endl;
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

std::ostream& operator<<(std::ostream& o, point_t const& rhs) {
	o << rhs.x << "," << rhs.y;
	return o;
}

std::wostream& operator<<(std::wostream& o, point_t const& rhs) {
	o << rhs.x << L"," << rhs.y;
	return o;
}

std::ostream& operator<<(std::ostream& o, FrameInfo const& rhs) {
	o << "hwnd:" << rhs.hwnd << std::endl
		<< "frameId:" << rhs.frameId << std::endl
		<< "time:" << rhs.time << std::endl
		<< "height" << rhs.height << std::endl
		<< "width:" << rhs.width << std::endl;
	return o;
}
std::wostream& operator<<(std::wostream& o, FrameInfo const& rhs) {
	o << L"hwnd:" << rhs.hwnd << std::endl
		<< L"frameId:" << rhs.frameId << std::endl
		<< L"time:" << rhs.time << std::endl
		<< L"height" << rhs.height << std::endl
		<< L"width:" << rhs.width << std::endl;
	return o;
}

//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetLastErrorAsString()
{
	//Get the error message ID, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0) {
		return std::string(); //No error message has been recorded
	}

	LPSTR messageBuffer = nullptr;

	//Ask Win32 to give us the string version of that message ID.
	//The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	//Copy the error message into a std::string.
	std::string message(messageBuffer, size);

	//Free the Win32's string's buffer.
	LocalFree(messageBuffer);

	return message;
}

bool Delay(long mis)
{
  MSG msg = {};
  auto deadline = ::GetTickCount64() + mis;
  while (::GetTickCount64() < deadline)
  {
		// 除收到'WM_QUIT'消息，结果始终都是大于0的
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
  }
  return true;
}

bool Delays(long mis_min, long mis_max)
{
  if (mis_min <= 0 || mis_max <= 0)
    return false;
  long mis = mis_min + rand() % mis_max;
  return Delay(mis);
}




