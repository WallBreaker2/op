// ConsoleTest.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <Windows.h>

#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
//#include "optest.hpp"
#include <time.h>

#include <chrono>
#include <filesystem>

#include "../libop/com/op_i.h"
#include "../libop/imageProc/compute/ThreadPool.h"
#include "test.h"
#include "../libop/core/optype.h"
#pragma warning(disable : 4996)

using namespace std;
bool gRun = false;
mutex gMutex;

void proc_shared(libop *sharedOp) {
  void *data = 0;
  long size, ret;
  while (gRun) {
    this_thread::sleep_for(chrono::microseconds(100));
    lock_guard<std::mutex> lock(gMutex);
    cout << "thread:" << this_thread::get_id() << endl;
    sharedOp->GetScreenDataBmp(0, 0, 50, 50, &data, &size, &ret);
  }
}

void proc_unique(long hwnd) {
  void *data = 0;
  long size, ret;
  libop *uniqueOp = new libop();
  uniqueOp->BindWindow(hwnd, L"normal", L"normal", L"normal", 0, &ret);
  while (gRun && ret) {
    // this_thread::sleep_for(chrono::microseconds(100));
    // lock_guard lock(gMutex);
    cout << "thread:" << this_thread::get_id() << endl;

    uniqueOp->GetScreenDataBmp(0, 0, 50, 50, &data, &size, &ret);
  }
  delete uniqueOp;
}

void test_shared() {
  libop *sharedOp = new libop();
  gRun = true;
  thread t1(proc_shared, sharedOp);
  thread t2(proc_shared, sharedOp);
  cin.get();
  gRun = false;
  t1.join();
  t2.join();

  delete sharedOp;
}

void test_unique() {
  gRun = true;
  long hwnd[2] = {0x001407EE, 0x00010472};
  thread t1(proc_unique, hwnd[0]);
  thread t2(proc_unique, hwnd[1]);
  cin.get();
  gRun = false;
  t1.join();
  t2.join();
}

int test_com() {
#ifdef _WIN64
  HMODULE hdll = LoadLibraryA("op_x64.dll");
#else
  HMODULE hdll = LoadLibraryA("op_x86.dll");
#endif
  if (!hdll) {
    printf("LoadLibraryA false!\n");
    return -1;
  }
  typedef HRESULT(__stdcall * DllGetClassObject_t)(
      _In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID FAR * ppv);
  DllGetClassObject_t pfun1 =
      (DllGetClassObject_t)GetProcAddress(hdll, "DllGetClassObject");
  if (!pfun1) {
    printf("GetProcAddress false!\n");
    return -2;
  }
  IClassFactory *fac = 0;
  pfun1(CLSID_OpInterface, IID_IClassFactory, (void **)&fac);
  if (!fac) {
    printf("DllGetClassObject false!\n");
    return -3;
  }
  IOpInterface *op;
  fac->CreateInstance(NULL, IID_IOpInterface, (void **)&op);

  if (!op) {
    printf("CoCreateInstance false!\n");
    return -4;
  }

  std::cout << "ok\n";
  long hwnd = 0;
  op->GetPointWindow(100, 100, &hwnd);
  long ret = 0;
  op->GetWindowState(hwnd, 2, &ret);
  std::cout << "----ret----:" << ret << std::endl;
  return 0;
}

void printHR(HRESULT hr) {
  if (hr == S_OK) {
    printf("\n");
  } else if (hr == E_OUTOFMEMORY) {
    printf("E_OUTOFMEMORY\n");
  } else if (hr == DISP_E_UNKNOWNNAME) {
    printf("DISP_E_UNKNOWNNAME\n");
  } else if (hr == DISP_E_UNKNOWNLCID) {
    printf("DISP_E_UNKNOWNLCID\n");
  } else {
    printf("other error code=%08X\n", hr);
  }
}

// int test_invoke() {
//	IOpInterface* op;
//
//	//CoCreateInstance(&CLSID_OpInterface, NULL, CLSCTX_INPROC,
//&IID_IOpInterface, &op);
//	/*if (op) {
//		printf("use free com ok!\n");
//		BSTR ver;
//		op->lpVtbl->Ver(op, &ver);
//		wprintf(L"op ver=%s", ver);
//	}*/
//	IDispatch* dispatch;
//	HRESULT hr = 0;
//	hr = CoCreateInstance(CLSID_OpInterface, NULL, CLSCTX_INPROC,
//IID_IDispatch, (void**)&dispatch); 	wchar_t*  names  = new wchar_t [256] ;
//	memcpy(names, L"Ver", 8);
//	if (dispatch == 0) {
//		printf("error:CoCreateInstance\n");
//		printHR(hr);
//		return 0;
//	}
//
//	DISPID id;
//	VARIANT ret;
//	hr = dispatch->GetIDsOfNames(IID_NULL, &names, 1, LOCALE_SYSTEM_DEFAULT,
//&id); 	if (hr < 0) { 		printf("error:GetIDsOfNames\n"); 		printHR(hr); 		return 0;
//	}
//	DISPPARAMS parms = {
//		NULL,
//		0,
//		0
//	};
//
//	hr = dispatch->Invoke(id, IID_NULL, LOCALE_SYSTEM_DEFAULT,
//DISPATCH_METHOD, 		&parms, &ret, NULL, NULL); 	if (hr < 0) {
//		printf("error:Invoke\n");
//		printHR(hr);
//		return 0;
//	}
//
//	if (ret.vt == VT_BSTR) {
//		wprintf(ret.bstrVal);
//		wprintf(L"\n");
//	}
//	else {
//		printf("error:ret.vt != VT_BSTR\n");
//	}
//	return 0;
//}

int add(int a, int b) { return a + b; }

struct SimpleTest {
  char a, b, c, d;
};
union Var {
  SimpleTest sp;
  int val;
};

void test_fs();

void test_threadPool();

void test_rect();

int main(int argc, char *argv[]) {
  int a[2] = {0, 1};
  int b = *a + 1;
  int c = *(a + 1);
  std::cout << "b:" << b << ", c:" << c << std::endl;
  Var v = {};
  v.sp.a = 1;
  std::cout << "v:" << v.val << std::endl;
  std::cout << add(1, 2);
  // test_shared();
  CoInitialize(NULL);
  // test_unique();
  // test_invoke();
  test_com();
  test *ptest = new test;
  ptest->do_test();
  delete ptest;
  ptest = nullptr;
  test_fs();

  test_threadPool();

  test_rect();

  return 0;
}

void test_fs() {
  namespace fs = std::filesystem;
  std::cout << "current path:" << fs::current_path() << std::endl;
  ;
  std::error_code e;
  std::cout << "absolute: " << fs::absolute("../", e) << std::endl;
  std::cout << "err code:" << e << std::endl;
  std::cout << "absolute: " << fs::absolute("../e", e) << std::endl;
  std::cout << "err code:" << e << std::endl;
}

void test_threadPool(){
	size_t maxThread = std::thread::hardware_concurrency();
	std::cout<<"max support thread num : "<<maxThread<<std::endl;
	ThreadPool pool(4);

	std::vector<std::future<int>> results;

	for(int i = 0; i < 8; ++i) {
        results.emplace_back(
            pool.enqueue([i] {
                std::cout << "hello " << i << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout << "world " << i << std::endl;
                return i*i;
            })
        );
    }

    for(auto && result: results)
        std::cout << result.get() << ' ';
    std::cout << std::endl;
    
    
}

void test_rect(){
	rect_t rc(0,0,100,100);
	std::vector<rect_t> blocks;

	rc.divideBlock(3,false, blocks);
	for(auto& r : blocks){
		std::cout<<r.x1<<","<<r.y1<<","<<r.x2<<","<<r.y2<<std::endl;
	}

	rc.divideBlock(3,true, blocks);
	for(auto& r : blocks){
		std::cout<<r.x1<<","<<r.y1<<","<<r.x2<<","<<r.y2<<std::endl;
	}

}
