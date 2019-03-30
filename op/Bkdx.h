#pragma once
#ifndef __DXBACKGROUND_H_
#define __DXBACKGROUND_H_
#include "3rd_party/include/BlackBone/Process/Process.h"
//#include <BlackBone/Patterns/PatternSearch.h>
#include "3rd_party/include/BlackBone/Process/RPC/RemoteFunction.hpp"
//#include <BlackBone/Syscalls/Syscall.h>
#include "Common.h"

#include "bkdisplay.h"

using std::wstring;
class Bkdx:public bkdisplay
{
public:
	Bkdx();
	~Bkdx();
	//1
	long Bind(HWND hwnd,long flag);

	long UnBind();

	
	//╫ьм╪жанд╪Ч
	long capture(const std::wstring& file_name);
private:
	blackbone::Process _process;

	

	std::wstring _dllname;
	
	DWORD _process_id;
};

#endif

