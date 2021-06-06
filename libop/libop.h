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
class opBackground;
class ImageProc;

using bytearray = std::vector<unsigned char>;
#ifdef U_STATIC_IMPLEMENTATION
#define OP_API
#else
#ifndef OP_API 
#if defined(OP_EXPORTS)
#define OP_API __declspec(dllexport)
#else
#define OP_API __declspec(dllimport)
#endif
#endif
#endif
// libop
#undef FindWindow
#undef FindWindowEx
#undef SetWindowText



class OP_API libop{
	
public:
	
	libop();
	~libop();
	//复制构造
	libop(libop const&) = delete;
	libop& operator=(libop const rhs) = delete;
private:
	
	//一些共用变量

	//1. Windows API
	WinApi* _winapi;
	// background module
	opBackground* _bkproc;
	//image process
	ImageProc* _image_proc;
	// work path
	std::wstring _curr_path;
	
	std::map<std::wstring, long> _vkmap;
	bytearray _screenData;
	bytearray _screenDataBmp;
	std::wstring m_opPath;
public:
	//---------------基本设置/属性-------------------

	//1.版本号Version
	std::wstring Ver();
	//设置目录
	void SetPath(const wchar_t* path, long* ret);
	//获取目录
	void GetPath(std::wstring& ret);
	//获取插件目录
	void GetBasePath(std::wstring& ret);
	//返回当前对象的ID值，这个值对于每个对象是唯一存在的。可以用来判定两个对象是否一致
	void GetID(long* ret);
	//
	void GetLastError(long* ret);
	//设置是否弹出错误信息,默认是打开 0:关闭，1:显示为信息框，2:保存到文件,3:输出到标准输出
	void SetShowErrorMsg(long show_type, long* ret);
	
	//sleep
	void Sleep(long millseconds, long* ret);
	//Process
	//inject dll
	void InjectDll(const wchar_t* process_name,const wchar_t* dll_name, long* ret);
	//设置是否开启或者关闭插件内部的图片缓存机制
	void EnablePicCache(long enable, long* ret);
	//取上次操作的图色区域，保存为file(24位位图)
	void CapturePre(const wchar_t* file_name, long* ret);
	//---------------------algorithm-------------------------------
	//A星算法
	void AStarFindPath(long mapWidth,long mapHeight,const wchar_t* disable_points,long beginX,long beginY, long endX,long endY,std::wstring& ret);
	//
	void FindNearestPos(const wchar_t* all_pos, long type, long x, long y, std::wstring& ret);
	//--------------------windows api------------------------------
	//根据指定条件,枚举系统中符合条件的窗口
	void EnumWindow(long parent, const wchar_t* title, const wchar_t* class_name, long filter, std::wstring& ret);
	//根据指定进程以及其它条件,枚举系统中符合条件的窗口
	void EnumWindowByProcess(const wchar_t* process_name, const wchar_t* title, const wchar_t* class_name, long filter, std::wstring& ret);
	//根据指定进程名,枚举系统中符合条件的进程PID
	void EnumProcess(const wchar_t* name, std::wstring& ret);
	//把窗口坐标转换为屏幕坐标
	void ClientToScreen(long ClientToScreen, long* x, long* y, long* bret);
	//查找符合类名或者标题名的顶层可见窗口
	void FindWindow(const wchar_t* class_name, const wchar_t* title, long* ret);
	//根据指定的进程名字，来查找可见窗口
	void FindWindowByProcess(const wchar_t* process_name, const wchar_t* class_name, const wchar_t* title, long* ret);
	//根据指定的进程Id，来查找可见窗口
	void FindWindowByProcessId(long process_id, const wchar_t* class_name, const wchar_t* title, long* ret);
	//查找符合类名或者标题名的顶层可见窗口,如果指定了parent,则在parent的第一层子窗口中查找
	void FindWindowEx(long parent, const wchar_t* class_name, const wchar_t* title, long* ret);
	//获取窗口客户区域在屏幕上的位置
	void GetClientRect(long hwnd, long* x1, long* y1, long* x2, long* y2, long* ret);
	//获取窗口客户区域的宽度和高度
	void GetClientSize(long hwnd, long* width, long* height, long* ret);
	//获取顶层活动窗口中具有输入焦点的窗口句柄
	void GetForegroundFocus(long* ret);
	//获取顶层活动窗口,可以获取到按键自带插件无法获取到的句柄
	void GetForegroundWindow(long* ret);
	//获取鼠标指向的可见窗口句柄
	void GetMousePointWindow(long* ret);
	//获取给定坐标的可见窗口句柄
	void GetPointWindow(long x, long y, long* ret);
	//根据指定的pid获取进程详细信息
	void GetProcessInfo(long pid, std::wstring& ret);
	//获取特殊窗口
	void GetSpecialWindow(long flag, long* ret);
	//获取给定窗口相关的窗口句柄
	void GetWindow(long hwnd, long flag, long* ret);
	//获取窗口的类名
	void GetWindowClass(long hwnd, std::wstring& ret);
	//获取指定窗口所在的进程ID
	void GetWindowProcessId(long hwnd, long* ret);
	//获取指定窗口所在的进程的exe文件全路径
	void GetWindowProcessPath(long hwnd, std::wstring& ret);
	//获取窗口在屏幕上的位置
	void GetWindowRect(long hwnd, long* x1, long* y1, long* x2, long* y2, long* ret);
	//获取指定窗口的一些属性
	void GetWindowState(long hwnd, long flag, long* ret);
	//获取窗口的标题
	void GetWindowTitle(long hwnd, std::wstring& rettitle);
	//移动指定窗口到指定位置
	void MoveWindow(long hwnd, long x, long y, long* ret);
	//把屏幕坐标转换为窗口坐标
	void ScreenToClient(long hwnd, long* x, long* y, long* ret);
	//向指定窗口发送粘贴命令
	void SendPaste(long hwnd, long* ret);
	//设置窗口客户区域的宽度和高度
	void SetClientSize(long hwnd, long width, long hight, long* ret);
	//设置窗口的状态
	void SetWindowState(long hwnd, long flag, long* ret);
	//设置窗口的大小
	void SetWindowSize(long hwnd, long width, long height, long* ret);
	//设置窗口的标题
	void SetWindowText(long hwnd, const wchar_t* title, long* ret);
	//设置窗口的透明度
	void SetWindowTransparent(long hwnd, long trans, long* ret);
	//向指定窗口发送文本数据
	void SendString(long hwnd, const wchar_t* str, long* ret);
	//向指定窗口发送文本数据-输入法
	void SendStringIme(long hwnd, const wchar_t* str, long* ret);
	//运行可执行文件,可指定模式
	void RunApp(const wchar_t* cmdline, long mode, long* ret);
	//运行可执行文件，可指定显示模式
	void WinExec(const wchar_t* cmdline, long cmdshow, long* ret);

	//运行命令行并返回结果
	void GetCmdStr(const wchar_t* cmd,long millseconds, std::wstring& retstr);

	//--------------------Background -----------------------
	//bind window and beign capture screen
	void BindWindow(long hwnd, const wchar_t* display, const wchar_t* mouse, const wchar_t* keypad, long mode,long *ret);
	//
	void UnBindWindow(long* ret);
	//--------------------mouse & keyboard------------------
	//获取鼠标位置.
	void GetCursorPos(long* x, long* y, long* ret);
	//鼠标相对于上次的位置移动rx,ry.
	void MoveR(long x, long y, long* ret);
	//把鼠标移动到目的点(x,y)
	void MoveTo(long x, long y, long* ret);
	//把鼠标移动到目的范围内的任意一点
	void MoveToEx(long x, long y,long w,long h, long* ret);
	//按下鼠标左键
	void LeftClick(long* ret);
	//双击鼠标左键
	void LeftDoubleClick(long* ret);
	//按住鼠标左键
	void LeftDown(long* ret);
	//弹起鼠标左键
	void LeftUp(long* ret);
	//按下鼠标中键
	void MiddleClick(long* ret);
	//按住鼠标中键
	void MiddleDown(long* ret);
	//弹起鼠标中键
	void MiddleUp(long* ret);
	//按下鼠标右键
	void RightClick(long* ret);
	//按住鼠标右键
	void RightDown(long* ret);
	//弹起鼠标右键
	void RightUp(long* ret);
	//滚轮向下滚
	void WheelDown(long* ret);
	//滚轮向上滚
	void WheelUp(long* ret);
	//获取指定的按键状态.(前台信息,不是后台)
	void GetKeyState(long vk_code, long* ret);
	//按住指定的虚拟键码
	void KeyDown(long vk_code, long* ret);
	//按住指定的虚拟键码
	void KeyDownChar(const wchar_t* vk_code, long* ret);
	//弹起来虚拟键vk_code
	void KeyUp(long vk_code, long* ret);
	//弹起来虚拟键vk_code
	void KeyUpChar(const wchar_t* vk_code, long* ret);
	//等待指定的按键按下 (前台,不是后台)
	void WaitKey(long vk_code,long time_out, long* ret);
	//发送字符串
	//long SendString(long HWND)
	//弹起来虚拟键vk_code
	void KeyPress(long vk_code, long* ret);
	void KeyPressChar(const wchar_t* vk_code, long* ret);

	//--------------------image and color-----------------------
	//抓取指定区域(x1, y1, x2, y2)的图像, 保存为file
	void Capture(long x1, long y1, long x2, long y2, const wchar_t* file_name, long* ret);
	//比较指定坐标点(x,y)的颜色
	void CmpColor(long x, long y,const wchar_t* color,double sim, long* ret);
	//查找指定区域内的颜色
	void FindColor(long x1, long y1, long x2, long y2, const wchar_t* color,double sim,long dir, long* x, long* y, long* ret);
	//查找指定区域内的所有颜色
	void FindColorEx(long x1, long y1, long x2, long y2, const wchar_t* color, double sim,long dir, std::wstring& retstr);
	//根据指定的多点查找颜色坐标
	void FindMultiColor(long x1, long y1, long x2, long y2, const wchar_t* first_color, const wchar_t* offset_color, double sim, long dir, long* x, long* y, long* ret);
	//根据指定的多点查找所有颜色坐标
	void FindMultiColorEx(long x1, long y1, long x2, long y2, const wchar_t* first_color, const wchar_t* offset_color, double sim, long dir,std::wstring& retstr);
	//查找指定区域内的图片
	void FindPic(long x1,long y1,long x2,long y2,const wchar_t* files, const wchar_t* delta_color,double sim,long dir,long* x,long* y,long* ret);
	//查找多个图片
	void FindPicEx(long x1, long y1, long x2, long y2, const wchar_t* files, const wchar_t* delta_color, double sim, long dir,std::wstring& retstr);
	//
	//这个函数可以查找多个图片, 并且返回所有找到的图像的坐标.此函数同FindPicEx.只是返回值不同.(file1,x,y|file2,x,y|...)
	void FindPicExS(long x1, long y1, long x2, long y2, const wchar_t* files, const wchar_t* delta_color, double sim, long dir, std::wstring& retstr);
	//查找指定区域内的颜色块,颜色格式"RRGGBB-DRDGDB",注意,和按键的颜色格式相反
	void FindColorBlock(long x1, long y1, long x2, long y2, const wchar_t*  color, double sim, long count, long height, long width, long* x, long* y, long* ret);
	//查找指定区域内的所有颜色块, 颜色格式"RRGGBB-DRDGDB", 注意, 和按键的颜色格式相反
	void FindColorBlockEx(long x1, long y1, long x2, long y2, const wchar_t*  color, double sim, long count, long height, long width, std::wstring& retstr);
	//获取(x,y)的颜色
	void GetColor(long x, long y, std::wstring& ret);
	//
	//设置图像输入方式，默认窗口截图
	void SetDisplayInput(const wchar_t* mode, long* ret);

	void LoadPic(const wchar_t* file_name, long* ret);

	void FreePic(const wchar_t* file_name, long* ret);
	//从内存加载要查找的图片
	void LoadMemPic(const wchar_t* file_name,void* data,long size, long* ret);
	//
	void GetScreenData(long x1, long y1, long x2, long y2, void** data,long* ret);
	//
	void GetScreenDataBmp(long x1, long y1, long x2, long y2, void** data,long* size, long* ret);
	//
	void GetScreenFrameInfo(long* frame_id, long* time);
	//
	void MatchPicName(const wchar_t* pic_name, std::wstring& retstr);
	//----------------------ocr-------------------------
	//设置字库文件
	void SetDict(long idx, const wchar_t* file_name, long* ret);
	//设置内存字库文件
	void SetMemDict(long idx, const wchar_t* data, long size, long* ret);
	//使用哪个字库文件进行识别
	void UseDict(long idx,  long* ret);
	//识别屏幕范围(x1,y1,x2,y2)内符合color_format的字符串,并且相似度为sim,sim取值范围(0.1-1.0),
	void Ocr(long x1, long y1, long x2, long y2, const wchar_t* color, double sim,std::wstring& ret_str);
	//回识别到的字符串，以及每个字符的坐标.
	void OcrEx(long x1, long y1, long x2, long y2, const wchar_t* color, double sim, std::wstring& ret_str);
	//在屏幕范围(x1,y1,x2,y2)内,查找string(可以是任意个字符串的组合),并返回符合color_format的坐标位置
	void FindStr(long x1, long y1, long x2, long y2,const wchar_t* strs, const wchar_t* color, double sim, long* retx,long* rety,long* ret);
	//返回符合color_format的所有坐标位置
	void FindStrEx(long x1, long y1, long x2, long y2, const wchar_t* strs, const wchar_t* color, double sim,std::wstring& retstr);
	//识别屏幕范围(x1,y1,x2,y2)内的字符串,自动二值化，而无需指定颜色
	void OcrAuto(long x1, long y1, long x2, long y2, double sim, std::wstring& ret_str);
	//从文件中识别图片
	void OcrFromFile(const wchar_t* file_name,const wchar_t* color_format, double sim, std::wstring& retstr);
	//从文件中识别图片,无需指定颜色
	void OcrAutoFromFile(const wchar_t* file_name, double sim, std::wstring& retstr);
	//查找频幕中的直线
	void FindLine(long x1, long y1, long x2, long y2, const wchar_t* color, double sim, std::wstring& retstr);
	
	//向某进程写入数据
	void WriteData(long hwnd, const wchar_t* address, const wchar_t* data, long size, long* ret);
	//读取数据
	void ReadData(long hwnd, const wchar_t* address, long size, std::wstring& retstr);
		
};



