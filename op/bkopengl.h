#pragma once
#include <BlackBone/Process/Process.h>
//#include <BlackBone/Patterns/PatternSearch.h>
#include <BlackBone/Process/RPC/RemoteFunction.hpp>
#include "bkdisplay.h"
class bkopengl :public bkdisplay
{
public:
	bkopengl();
	~bkopengl();

	long Bind(HWND hwnd, long flag);

	long UnBind();


	//╫ьм╪жанд╪Ч
	long capture(const std::wstring& file_name);
private:
	blackbone::Process _process;



	std::wstring _dllname;

	DWORD _process_id;
};

