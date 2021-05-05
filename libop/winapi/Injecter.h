#pragma once
#include "core/optype.h"
class Injecter
{
public:
	Injecter();
	~Injecter();
	static BOOL EnablePrivilege(BOOL enable);
	// 
	static long InjectDll(DWORD pid, LPCTSTR dllPath,long& error_code);
};

