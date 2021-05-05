//#include "stdafx.h"
#include "core/optype.h"
#include "query_api.h"


void* query_api(const char* mod_name, const char* func_name) {
	auto hdll = ::GetModuleHandleA(mod_name);
	if (!hdll) {
		//_error_code = -1;
		return NULL;
	}
	void* paddress = (void*)::GetProcAddress(hdll, func_name);
	if (!paddress) {
		//_error_code = -2;
		return NULL;
	}
	//_error_code = 0;
	return paddress;

}


