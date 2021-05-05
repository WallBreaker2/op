#pragma once
#ifndef __TEST_H_
#define __TEST_H_
#include <iostream>

#include "../libop/libop.h"
#include <windows.h>
//#ifdef _M_X64
//#pragma comment(lib,"../bin/x86/op_x64.lib")
//#else
//#pragma comment(lib,"../bin/x86/op_x86.lib")
//#endif
using namespace std;
class test {
public:
	test() :m_op(new libop(nullptr)){
		
	}
	virtual ~test() {
		delete m_op;
		m_op = nullptr;
	}

	int BindNox(libop* op) {
		long hwnd = 0, subhwnd = 0, ret = 0;
		op->FindWindow(L"Qt5QWindowIcon", L"Ò¹ÉñÄ£ÄâÆ÷", &hwnd);
		if (!hwnd) {
			std::cout << "FindWindow of Ò¹ÉñÄ£ÄâÆ÷ false!\n";

			return -1;
		}
		op->FindWindowEx(hwnd, L"Qt5QWindowIcon", L"centralWidgetWindow", &subhwnd);
		if (!subhwnd) {
			std::cout << "FindWindow of centralWidgetWindow false!\n";
			return -2;
		}
		printf("find ok ,hwnd is %08x\n", subhwnd);
		op->BindWindow(subhwnd, L"opengl.nox", L"windows", L"windows", 0, &ret);
		if (!ret) {
			std::cout << "BindWindow Nox false!\n";
			return -2;
		}
		//todo..
		do_some_test(op);

		op->UnBindWindow(&ret);
		return ret;
	}
	int BindLDPlayer(libop* op) {
		long hwnd = 0, subhwnd = 0, ret = 0;
		op->FindWindow(L"LDPlayerMainFrame", L"À×µçÄ£ÄâÆ÷", &hwnd);
		if (!hwnd) {
			std::cout << "FindWindow of À×µçÄ£ÄâÆ÷ false!\n";
			return -1;
		}
		op->FindWindowEx(hwnd, L"RenderWindow", L"TheRender", &subhwnd);
		if (!subhwnd) {
			std::cout << "FindWindow of TheRender false!\n";
			return -2;
		}
		printf("find ok ,hwnd is %08x\n", subhwnd);
		op->BindWindow(subhwnd, L"opengl", L"windows", L"windows", 0, &ret);
		if (!ret) {
			std::cout << "BindWindow Nox false!\n";
			return -2;
		}
		//todo..
		do_some_test(op);

		op->UnBindWindow(&ret);
		return ret;
	}

	int BindMine(libop* op) {
		long hwnd = 0;
		long ret = 0;
		op->FindWindow(L"É¨À×", L"É¨À×", &hwnd);
		//hwnd = 0x004608BC;
		if (hwnd) {
			op->BindWindow(hwnd, L"dx.d3d9", L"normal", L"normal", 0, &ret);
			if (ret) {
				do_some_test(op);
			}
			op->UnBindWindow(&ret);
		}
		return 0;
	}
	int do_some_test(libop* op) {
		printf("bind ok\n");
		long ret = 0;
		std::wstring ver;
		m_op->Capture(10, 10, 50, 50, L"xxx.bmp", &ret);
	
		for (int i = 0; i < 10; i++) {
			Sleep(200);
			long frame_id, time;
			m_op->GetScreenFrameInfo(&frame_id, &time);
			printf("frame id=%d time=%.3lfs\n", frame_id, time / 1000.);
		}
		void* data; long size;
		m_op->GetScreenDataBmp(0, 0, 200, 200, &data, &size, &ret);
		std::ofstream os("GetScreenDataBmp.bmp",std::ios::binary);
		os.write((char*)data, size);
		os.close();
		m_op->FindLine(50, 50, 400, 400, L"", 0.95, ver);
		std::wcout << "FindLine result:" << ver << std::endl;
		return 0;
	}
	void do_test() {
		wstring ver;
		long lret = 0;
		m_op->SetShowErrorMsg(3,&lret);
		m_op->MatchPicName(L"s*.bmp", ver);
		std::wcout << "MatchPicName:" << ver << std::endl;
		m_op->FindNearestPos(L"1,2|3,4|5,6|7,8",1,3,0,ver);
		std::wcout << "FindNearestPos:" << ver << std::endl;
		m_op->Ver(ver);
		wprintf(L"ver:%s\n", ver.data());
		m_op->GetBasePath(ver);
		wprintf(L"GetBasePath:%s\n", ver.data());

		BindNox(m_op);
		BindLDPlayer(m_op);
		BindMine(m_op);
	}
protected:
	libop* m_op;
};
#endif // !__TEST_H_
