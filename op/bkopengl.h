#pragma once
#include "3rd_party/include/BlackBone/Process/Process.h"
//#include <BlackBone/Patterns/PatternSearch.h>
#include "3rd_party/include/BlackBone/Process/RPC/RemoteFunction.hpp"
#pragma comment(lib,"./3rd_party/lib/x86/BlackBone.lib")
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

