#include "stdafx.h"
#include "Common.h"



long setlog(const wchar_t* format, ...) {
	va_list args;
	wchar_t buf[256];
	va_start(args, format);
	vswprintf(buf, format, args);
	va_end(args);
	std::wfstream file;
	file.open(L"op.log", std::ios::app | std::ios::out);
	if (!file.is_open())
		return 0;
	file << buf << std::endl;
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