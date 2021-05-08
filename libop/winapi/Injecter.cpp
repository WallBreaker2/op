//#include "stdafx.h"
#include "Injecter.h"


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
	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL))
		return FALSE;

	// 关闭令牌句柄
	CloseHandle(hToken);

	return TRUE;
}


long Injecter::InjectDll(DWORD pid, LPCTSTR dllPath,long& error_code)
{
	
	auto jhandle=::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	/**pid = processInfo.dwProcessId;
	*process = processInfo.hProcess;*/
	if (!jhandle) {
		error_code = ::GetLastError();
		return -1;
	}
	DWORD dllPathSize = ((DWORD)wcslen(dllPath) + 1) * sizeof(TCHAR);

	// 申请内存用来存放DLL路径
	void* remoteMemory = VirtualAllocEx(jhandle, NULL, dllPathSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (remoteMemory == NULL)
	{
		//setlog(L"申请内存失败，错误代码：%u\n", GetLastError());
		error_code = ::GetLastError();
		return -2;
	}

	// 写入DLL路径
	if (!WriteProcessMemory(jhandle, remoteMemory, dllPath, dllPathSize, NULL))
	{
		//setlog(L"写入内存失败，错误代码：%u\n", GetLastError());
		error_code = ::GetLastError();
		return -3;
	}

	// 创建远线程调用LoadLibrary
	auto lpfn=GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
	if (!lpfn) {
		error_code = ::GetLastError();
		return -4;
	}
	HANDLE remoteThread = CreateRemoteThread(jhandle, NULL, 0, (LPTHREAD_START_ROUTINE)lpfn, remoteMemory, 0, NULL);
	if (remoteThread == NULL)
	{
		//setlog(L"创建远线程失败，错误代码：%u\n", GetLastError());
		error_code = ::GetLastError();
		return -5;
	}
	// 等待远线程结束
	WaitForSingleObject(remoteThread, INFINITE);
	// 取DLL在目标进程的句柄
	DWORD remoteModule;
	GetExitCodeThread(remoteThread, &remoteModule);

	// 恢复线程
	//ResumeThread(processInfo.hThread);

	// 释放
	CloseHandle(remoteThread);
	VirtualFreeEx(jhandle, remoteMemory, dllPathSize, MEM_DECOMMIT);
	CloseHandle(jhandle);
	error_code = 0;
	return 1;
}