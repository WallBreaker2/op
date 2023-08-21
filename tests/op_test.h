#pragma once
#ifndef __OP_TEST_H_
#define __OP_TEST_H_
#include <windows.h>

#include <iostream>

#include "../libop/libop.h"
#define op_check(name, express)                                        \
  std::cout << "check interface:'" << #name << "' condition:" << #express \
            << ((express) ? "\npass\n" : "\nfailed\n");++total_cnt;pass_cnt+=express
using namespace std;
class test {
 public:
  test() : m_op(new libop()) {}
  virtual ~test() {
    delete m_op;
    m_op = nullptr;
  }

  int TestForeground() {
      long ret = 0;
      m_op->BindWindow(0, L"normal", L"normal", L"normal", 0, &ret);
      if (ret == 1) {
          std::cout << "bind normal normal normal ok\n";
          do_some_test(m_op);
          m_op->UnBindWindow(&ret);
      }
      m_op->BindWindow(0, L"normal.dxgi", L"normal", L"normal", 0, &ret);
      if (ret == 1) {
          std::cout << "bind normal.dxgi normal normal ok\n";
          do_some_test(m_op);
          m_op->UnBindWindow(&ret);
      }
      return 0;
  }

  int TestNox(libop *op) {
    long hwnd = 0, subhwnd = 0, ret = 0;
    op->FindWindow(L"Qt5QWindowIcon", L"夜神模拟器", &hwnd);
    if (!hwnd) {
      std::wcout << L"FindWindow of 夜神模拟器 false!\n";

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
    // todo..
    do_some_test(op);

    op->UnBindWindow(&ret);
    return ret;
  }
  int TestLD(libop *op) {
    long hwnd = 0, subhwnd = 0, ret = 0;
    op->FindWindow(L"LDPlayerMainFrame", L"雷电模拟器", &hwnd);
    if (!hwnd) {
      std::cout << "FindWindow of LDPlayerMainFrame false!\n";
      return -1;
    }
    op->FindWindowEx(hwnd, L"RenderWindow", L"TheRender", &subhwnd);
    if (!subhwnd) {
      std::cout << "FindWindow of TheRender false!\n";
      return -2;
    }
    printf("find LD ok ,hwnd is %08x\n", subhwnd);
    op->BindWindow(subhwnd, L"opengl", L"windows", L"windows", 0, &ret);
    if (!ret) {
      std::cout << "BindWindow Nox false!\n";
      return -2;
    }
    // todo..
    do_some_test(op);

    op->UnBindWindow(&ret);
    return ret;
  }

  int BindMine(libop *op) {
    long hwnd = 0;
    long ret = 0;
    op->FindWindow(L"扫雷", L"扫雷", &hwnd);
    // hwnd = 0x004608BC;
    if (hwnd) {
      op->BindWindow(hwnd, L"dx.d3d9", L"normal", L"normal", 0, &ret);
      if (ret) {
        do_some_test(op);
      }
      op->UnBindWindow(&ret);
    }
    return 0;
  }
  int do_some_test(libop *op) {
    printf("******************** do_some_test *************************** \n");
    wstring ver, str;
    long lret = 0;
    m_op->Capture(10, 10, 200, 200, L"xxx.bmp", &lret);

    for (int i = 0; i < 10; i++) {
      Sleep(20);
      long frame_id, time;
      m_op->GetScreenFrameInfo(&frame_id, &time);
      printf("frame id=%d time=%.3lfs\n", frame_id, time / 1000.);
    }
    void *data=nullptr;
    long size;
    m_op->GetScreenDataBmp(0, 0, 200, 200, (size_t*)&data, &size, &lret);
    if (data) {
        std::ofstream os("GetScreenDataBmp.bmp", std::ios::binary);
        os.write((char*)data, size);
        os.close();
    }
    m_op->FindLine(50, 50, 400, 400, L"", 0.95, ver);
    std::wcout << "FindLine result:" << ver << std::endl;

    std::wcout.imbue(locale("chs"));
    
    int pass_cnt = 0;
    int total_cnt = 0;
    // ***************** check base function ************************
    ver = m_op->Ver();
    op_check(Ver, ver == L"0.4.2.0");

    m_op->SetShowErrorMsg(3, &lret);
    op_check(SetShowErrorMsg, lret);

    m_op->MatchPicName(L"s*.bmp", str);
    std::wcout << "MatchPicName:" << str << std::endl;

    m_op->FindNearestPos(L"1,2|3,4|5,6|7,8", 1, 3, 0, str);
    op_check(FindNearestPos, str == L"1,2");


    ver = m_op->Ver();
    wprintf(L"ver:%s\n", ver.data());

    m_op->GetBasePath(str);
    wprintf(L"GetBasePath:%s\n", str.data());
    // op_check(GetBasePath, str == L"E:\\project\\op\\bin\\x86");

    // ***************** check ocr function ******************

    //m_op->SetDict(0, L"st10.dict", &lret);
    //op_check(SetDict, lret == 1);
    m_op->Ocr(0, 0, 2000, 2000, L"000000", 0.8, str);
    std::wcout << L"ocr:" << str << std::endl;
    op_check(Ocr, str.length() > 0);
    m_op->GetWindowState((long)(::GetDesktopWindow()), 2, &lret);
    op_check(GetWindowState, lret == 1);

    // ***************** check color function ******************

    std::wstring color;
    m_op->GetColor(20, 20, color);
    std::wcout << L"color in 20,20: " << std::endl;
    std::wcout << color << std::endl;
    op_check(GetColor, color.length() == 6);

    m_op->CmpColor(20, 20, color.data(), 1.0, &lret);
    op_check(CmpColor, lret == 1);

    long lx, ly;
    for(int dir=0;dir<4;++dir){
        m_op->FindColor(0, 0, 2000, 2000, color.data(), 1.0, dir, &lx, &ly, &lret);
        std::cout <<"dir:"<< dir<<" ret:" << lret << "," << lx << "," << ly << std::endl;
        op_check(FindColor, lret == 1 && 0 <= lx && lx <= 2000);
    }
    wstring colors[3];
    m_op->GetColor(20, 20, colors[0]);
    m_op->GetColor(21, 20, colors[1]);
    m_op->GetColor(20, 21, colors[2]);
    wstring offcolor = L"1|0|" + colors[1] + L",0|1|" + colors[2];
    for(int dir=0;dir<4;dir++){
      m_op->FindMultiColor(0, 0, 2000, 2000, colors[0].data(), offcolor.data(), 1.0, dir, &lx, &ly, &lret);
      std::cout <<"FindMultiColor dir:"<< dir<<" ret:" << lret << "," << lx << "," << ly << std::endl;
    op_check(FindMultiColor, lret == 1);
    }
    

    m_op->FindMultiColorEx(0, 0, 2000, 2000, colors[0].data(), offcolor.data(), 1.0, 0, str);
    op_check(FindMultiColorEx, str.length() > 0);

    // ***********************8 check screen capture *************8
    m_op->SetScreenDataMode(0, &lret);
    m_op->GetScreenDataBmp(0, 0, 200, 200, (size_t*) & data, &size, &lret);
    op_check(GetScreenDataBmp, lret == 1);

    m_op->FindColorBlock(0, 0, 2000, 2000, L"000000", 1.0, 200, 20, 20, &lx, &ly, &lret);
    op_check(FindColorBlock, lret == 1);

    m_op->FindColorBlockEx(0, 0, 2000, 2000, L"000000", 1.0, 200, 20, 20, str);
    //std::wcout << str << std::endl;
    op_check(FindColorBlockEx, str.length() > 0);

    m_op->Capture(60, 60, 100, 100, L"test.bmp", &lret);
    op_check(Capture, lret >= 0);

    for (int dir = 0; dir < 4; ++dir) {
		m_op->FindPic(0, 0, 2000, 2000, L"test.bmp", L"000000", 1.0, dir, &lx, &ly, &lret);
		std::cout <<"dir:"<< dir<<" ret:" << lret << "," << lx << "," << ly << std::endl;
		op_check(FindPic, lret >= 0);
	}
    for (int dir = 0; dir < 4; ++dir) {
        m_op->FindPicEx(0, 0, 2000, 2000, L"test.bmp", L"000000", 1.0, dir, str);
        std::wcout << "FindPicEx:" << str << std::endl;
        op_check(FindPicEx, str.length() > 0);
    }
    
    
    std::cout << "--------------total_test-----------:\n" 
        << total_cnt << " passed:" << pass_cnt << std::endl;
    return 0;
  }
  void do_test() {
    
    // m_op->FindWindow()
    TestForeground();

    TestNox(m_op);
    TestLD(m_op);
    BindMine(m_op);
  }

 protected:
  libop *m_op;
};
#endif  // !__TEST_H_
