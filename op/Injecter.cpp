#include "stdafx.h"
#include "Injecter.h"
#include "Common.h"

Injecter::Injecter()
{
}


Injecter::~Injecter()
{
}

BOOL Injecter::EnablePrivilege(BOOL enable)
{
	// 得到令牌句柄
	HANDLE hToken = NULL;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_READ, &hToken))
		return FALSE;

	// 得到特权值
	LUID luid;
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
		return FALSE;

	// 提升令牌句柄权限
	TOKEN_PRIVILEGES tp = {};
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;
	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL))
		return FALSE;

	// 关闭令牌句柄
	CloseHandle(hToken);

	return TRUE;
}


HMODULE Injecter::InjectDll(LPCTSTR commandLine, LPCTSTR dllPath/*, DWORD* pid, HANDLE* process*/)
{
	TCHAR* commandLineCopy = new TCHAR[32768]; // CreateProcess可能修改这个
	_tcscpy_s(commandLineCopy, 32768, commandLine);
	int cdSize = _tcsrchr(commandLine, _T('\\')) - commandLine + 1;
	TCHAR* cd = new TCHAR[cdSize];
	_tcsnccpy_s(cd, cdSize, commandLine, cdSize - 1);
	// 创建进程并暂停
	STARTUPINFO startInfo = {};
	PROCESS_INFORMATION processInfo = {};
	if (!CreateProcess(NULL, commandLineCopy, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, cd, &startInfo, &processInfo))
	{
		setlog(L"CreateProcess %s False,code=%d",commandLineCopy, GetLastError());
		delete commandLineCopy;
		delete cd;
		return 0;
	}
	delete commandLineCopy;
	delete cd;

	/**pid = processInfo.dwProcessId;
	*process = processInfo.hProcess;*/

	DWORD dllPathSize = ((DWORD)_tcslen(dllPath) + 1) * sizeof(TCHAR);

	// 申请内存用来存放DLL路径
	void* remoteMemory = VirtualAllocEx(processInfo.hProcess, NULL, dllPathSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (remoteMemory == NULL)
	{
		setlog(L"申请内存失败，错误代码：%u\n", GetLastError());
		return 0;
	}

	// 写入DLL路径
	if (!WriteProcessMemory(processInfo.hProcess, remoteMemory, dllPath, dllPathSize, NULL))
	{
		setlog(L"写入内存失败，错误代码：%u\n", GetLastError());
		return 0;
	}

	// 创建远线程调用LoadLibrary
	HANDLE remoteThread = CreateRemoteThread(processInfo.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibrary, remoteMemory, 0, NULL);
	if (remoteThread == NULL)
	{
		setlog(L"创建远线程失败，错误代码：%u\n", GetLastError());
		return NULL;
	}
	// 等待远线程结束
	WaitForSingleObject(remoteThread, INFINITE);
	// 取DLL在目标进程的句柄
	DWORD remoteModule;
	GetExitCodeThread(remoteThread, &remoteModule);

	// 恢复线程
	ResumeThread(processInfo.hThread);

	// 释放
	CloseHandle(remoteThread);
	VirtualFreeEx(processInfo.hProcess, remoteMemory, dllPathSize, MEM_DECOMMIT);

	return (HMODULE)remoteModule;
}