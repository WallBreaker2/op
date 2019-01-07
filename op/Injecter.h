#pragma once
class Injecter
{
public:
	Injecter();
	~Injecter();
	static BOOL EnablePrivilege(BOOL enable);
	// 程序运行时注入DLL，返回模块句柄（64位程序只能返回低32位）
	static HMODULE InjectDll(LPCTSTR commandLine, LPCTSTR dllPath/*, DWORD* pid, HANDLE* process*/);
};

