// libop的声明
/*
所有op的开放接口都从此cpp类衍生而出
*/
#pragma once

#include <string>
#include<map>
#include<vector>
//forward declare
class WinApi;
class bkbase;
class ImageProc;

using bytearray = std::vector<unsigned char>;
#if defined(OP_EXPORTS)
#define OP_API __declspec(dllexport)
#else
#define OP_API __declspec(dllimport)
#endif
// libop

class OP_API libop{
public:
	libop();
	~libop();

private:
	//一些共用变量

	//1. Windows API
	WinApi* _winapi;
	// background module
	bkbase* _bkproc;
	//image process
	ImageProc* _image_proc;
	// work path
	std::wstring _curr_path;
	
	std::map<std::wstring, long> _vkmap;
	bytearray _screenData;
	bytearray _screenDataBmp;
public:
	//---------------基本设置/属性-------------------

	//1.版本号Version
	long Ver(std::wstring& ret);
	//设置目录
	long SetPath(const wchar_t* path, long* ret);
	//获取目录
	long GetPath(std::wstring&path);
	//获取插件目录
	long GetBasePath(std::wstring& path);
	//返回当前对象的ID值，这个值对于每个对象是唯一存在的。可以用来判定两个对象是否一致
	long GetID(long* ret);
	//
	long GetLastError(long* ret);
	//设置是否弹出错误信息,默认是打开 0:关闭，1:显示为信息框，2:保存到文件,3:输出到标准输出
	long SetShowErrorMsg(long show_type, long* ret);
	
	//sleep
	long Sleep(long millseconds, long* ret);
	//Process
	//inject dll
	long InjectDll(const wchar_t* process_name,const wchar_t* dll_name, long* ret);
	//设置是否开启或者关闭插件内部的图片缓存机制
	long EnablePicCache(long enable, long* ret);
	//取上次操作的图色区域，保存为file(24位位图)
	long CapturePre(const wchar_t* file_name, long* ret);
	//---------------------algorithm-------------------------------
	//A星算法
	long AStarFindPath(long mapWidth,long mapHeight,const wchar_t* disable_points,long beginX,long beginY, long endX,long endY,std::wstring& path);
	//--------------------windows api------------------------------
	//根据指定条件,枚举系统中符合条件的窗口
	long EnumWindow(long parent, const wchar_t* title, const wchar_t* class_name, long filter, std::wstring& retstr);
	//根据指定进程以及其它条件,枚举系统中符合条件的窗口
	long EnumWindowByProcess(const wchar_t* process_name, const wchar_t* title, const wchar_t* class_name, long filter, std::wstring& retstring);
	//根据指定进程名,枚举系统中符合条件的进程PID
	long EnumProcess(const wchar_t* name, std::wstring& retstring);
	//把窗口坐标转换为屏幕坐标
	long ClientToScreen(long ClientToScreen, long* x, long* y, long* bret);
	//查找符合类名或者标题名的顶层可见窗口
	long FindWindow(const wchar_t* class_name, const wchar_t* title, long* rethwnd);
	//根据指定的进程名字，来查找可见窗口
	long FindWindowByProcess(const wchar_t* process_name, const wchar_t* class_name, const wchar_t* title, long* rethwnd);
	//根据指定的进程Id，来查找可见窗口
	long FindWindowByProcessId(long process_id, const wchar_t* class_name, const wchar_t* title, long* rethwnd);
	//查找符合类名或者标题名的顶层可见窗口,如果指定了parent,则在parent的第一层子窗口中查找
	long FindWindowEx(long parent, const wchar_t* class_name, const wchar_t* title, long* rethwnd);
	//获取窗口客户区域在屏幕上的位置
	long GetClientRect(long hwnd, long* x1, long* y1, long* x2, long* y2, long* nret);
	//获取窗口客户区域的宽度和高度
	long GetClientSize(long hwnd, long* width, long* height, long* nret);
	//获取顶层活动窗口中具有输入焦点的窗口句柄
	long GetForegroundFocus(long* rethwnd);
	//获取顶层活动窗口,可以获取到按键自带插件无法获取到的句柄
	long GetForegroundWindow(long* rethwnd);
	//获取鼠标指向的可见窗口句柄
	long GetMousePointWindow(long* rethwnd);
	//获取给定坐标的可见窗口句柄
	long GetPointWindow(long x, long y, long* rethwnd);
	//根据指定的pid获取进程详细信息
	long GetProcessInfo(long pid, std::wstring& retstring);
	//获取特殊窗口
	long GetSpecialWindow(long flag, long* rethwnd);
	//获取给定窗口相关的窗口句柄
	long GetWindow(long hwnd, long flag, long* nret);
	//获取窗口的类名
	long GetWindowClass(long hwnd, std::wstring& retstring);
	//获取指定窗口所在的进程ID
	long GetWindowProcessId(long hwnd, long* nretpid);
	//获取指定窗口所在的进程的exe文件全路径
	long GetWindowProcessPath(long hwnd, std::wstring& retstring);
	//获取窗口在屏幕上的位置
	long GetWindowRect(long hwnd, long* x1, long* y1, long* x2, long* y2, long* nret);
	//获取指定窗口的一些属性
	long GetWindowState(long hwnd, long flag, long* rethwnd);
	//获取窗口的标题
	long GetWindowTitle(long hwnd, std::wstring& rettitle);
	//移动指定窗口到指定位置
	long MoveWindow(long hwnd, long x, long y, long* nret);
	//把屏幕坐标转换为窗口坐标
	long ScreenToClient(long hwnd, long* x, long* y, long* nret);
	//向指定窗口发送粘贴命令
	long SendPaste(long hwnd, long* nret);
	//设置窗口客户区域的宽度和高度
	long SetClientSize(long hwnd, long width, long hight, long* nret);
	//设置窗口的状态
	long SetWindowState(long hwnd, long flag, long* nret);
	//设置窗口的大小
	long SetWindowSize(long hwnd, long width, long height, long* nret);
	//设置窗口的标题
	long SetWindowText(long hwnd, const wchar_t* title, long* nret);
	//设置窗口的透明度
	long SetWindowTransparent(long hwnd, long trans, long* nret);
	//向指定窗口发送文本数据
	long SendString(long hwnd, const wchar_t* str, long* ret);
	//向指定窗口发送文本数据-输入法
	long SendStringIme(long hwnd, const wchar_t* str, long* ret);
	//运行可执行文件,可指定模式
	long RunApp(const wchar_t* cmdline, long mode, long* ret);
	//运行可执行文件，可指定显示模式
	long WinExec(const wchar_t* cmdline, long cmdshow, long* ret);

	//运行命令行并返回结果
	long GetCmdStr(const wchar_t* cmd,long millseconds, std::wstring& retstr);

	//--------------------Background -----------------------
	//bind window and beign capture screen
	long BindWindow(long hwnd, const wchar_t* display, const wchar_t* mouse, const wchar_t* keypad, long mode,long *ret);
	//
	long UnBindWindow(long* ret);
	//--------------------mouse & keyboard------------------
	//获取鼠标位置.
	long GetCursorPos(long* x, long* y, long* ret);
	//鼠标相对于上次的位置移动rx,ry.
	long MoveR(long x, long y, long* ret);
	//把鼠标移动到目的点(x,y)
	long MoveTo(long x, long y, long* ret);
	//把鼠标移动到目的范围内的任意一点
	long MoveToEx(long x, long y,long w,long h, long* ret);
	//按下鼠标左键
	long LeftClick(long* ret);
	//双击鼠标左键
	long LeftDoubleClick(long* ret);
	//按住鼠标左键
	long LeftDown(long* ret);
	//弹起鼠标左键
	long LeftUp(long* ret);
	//按下鼠标中键
	long MiddleClick(long* ret);
	//按住鼠标中键
	long MiddleDown(long* ret);
	//弹起鼠标中键
	long MiddleUp(long* ret);
	//按下鼠标右键
	long RightClick(long* ret);
	//按住鼠标右键
	long RightDown(long* ret);
	//弹起鼠标右键
	long RightUp(long* ret);
	//滚轮向下滚
	long WheelDown(long* ret);
	//滚轮向上滚
	long WheelUp(long* ret);
	//获取指定的按键状态.(前台信息,不是后台)
	long GetKeyState(long vk_code, long* ret);
	//按住指定的虚拟键码
	long KeyDown(long vk_code, long* ret);
	//按住指定的虚拟键码
	long KeyDownChar(const wchar_t* vk_code, long* ret);
	//弹起来虚拟键vk_code
	long KeyUp(long vk_code, long* ret);
	//弹起来虚拟键vk_code
	long KeyUpChar(const wchar_t* vk_code, long* ret);
	//等待指定的按键按下 (前台,不是后台)
	long WaitKey(long vk_code,long time_out, long* ret);
	//发送字符串
	//long SendString(long HWND)
	//弹起来虚拟键vk_code
	long KeyPress(long vk_code, long* ret);
	long KeyPressChar(const wchar_t* vk_code, long* ret);

	//--------------------image and color-----------------------
	//抓取指定区域(x1, y1, x2, y2)的图像, 保存为file
	long Capture(long x1, long y1, long x2, long y2, const wchar_t* file_name, long* ret);
	//比较指定坐标点(x,y)的颜色
	long CmpColor(long x, long y,const wchar_t* color,double sim, long* ret);
	//查找指定区域内的颜色
	long FindColor(long x1, long y1, long x2, long y2, const wchar_t* color,double sim,long dir, long* x, long* y, long* ret);
	//查找指定区域内的所有颜色
	long FindColorEx(long x1, long y1, long x2, long y2, const wchar_t* color, double sim,long dir, std::wstring& retstr);
	//根据指定的多点查找颜色坐标
	long FindMultiColor(long x1, long y1, long x2, long y2, const wchar_t* first_color, const wchar_t* offset_color, double sim, long dir, long* x, long* y, long* ret);
	//根据指定的多点查找所有颜色坐标
	long FindMultiColorEx(long x1, long y1, long x2, long y2, const wchar_t* first_color, const wchar_t* offset_color, double sim, long dir,std::wstring& retstr);
	//查找指定区域内的图片
	long FindPic(long x1,long y1,long x2,long y2,const wchar_t* files, const wchar_t* delta_color,double sim,long dir,long* x,long* y,long* ret);
	//查找多个图片
	long FindPicEx(long x1, long y1, long x2, long y2, const wchar_t* files, const wchar_t* delta_color, double sim, long dir,std::wstring& retstr);
	//获取(x,y)的颜色
	long GetColor(long x, long y, std::wstring& ret);
	//
	//设置图像输入方式，默认窗口截图
	long SetDisplayInput(const wchar_t* mode, long* ret);

	long LoadPic(const wchar_t* file_name, long* ret);

	long FreePic(const wchar_t* file_name, long* ret);
	//
	long GetScreenData(long x1, long y1, long x2, long y2, void** data,long* ret);
	//
	long GetScreenDataBmp(long x1, long y1, long x2, long y2, void** data,long* size, long* ret);
	//
	long GetScreenFrameInfo(long* frame_id, long* time);
	//----------------------ocr-------------------------
	//设置字库文件
	long SetDict(long idx, const wchar_t* file_name, long* ret);
	//使用哪个字库文件进行识别
	long UseDict(long idx,  long* ret);
	//识别屏幕范围(x1,y1,x2,y2)内符合color_format的字符串,并且相似度为sim,sim取值范围(0.1-1.0),
	long Ocr(long x1, long y1, long x2, long y2, const wchar_t* color, double sim,std::wstring& ret_str);
	//回识别到的字符串，以及每个字符的坐标.
	long OcrEx(long x1, long y1, long x2, long y2, const wchar_t* color, double sim, std::wstring& ret_str);
	//在屏幕范围(x1,y1,x2,y2)内,查找string(可以是任意个字符串的组合),并返回符合color_format的坐标位置
	long FindStr(long x1, long y1, long x2, long y2,const wchar_t* strs, const wchar_t* color, double sim, long* retx,long* rety,long* ret);
	//返回符合color_format的所有坐标位置
	long FindStrEx(long x1, long y1, long x2, long y2, const wchar_t* strs, const wchar_t* color, double sim,std::wstring& retstr);
	//识别屏幕范围(x1,y1,x2,y2)内的字符串,自动二值化，而无需指定颜色
	long OcrAuto(long x1, long y1, long x2, long y2, double sim, std::wstring& ret_str);
	//从文件中识别图片
	long OcrFromFile(const wchar_t* file_name,const wchar_t* color_format, double sim, std::wstring& retstr);
	//从文件中识别图片,无需指定颜色
	long OcrAutoFromFile(const wchar_t* file_name, double sim, std::wstring& retstr);
	//
	
	//向某进程写入数据
	long WriteData(long hwnd, const wchar_t* address, const wchar_t* data, long size, long* ret);
	//读取数据
	long ReadData(long hwnd, const wchar_t* address, long size, std::wstring& retstr);

};



