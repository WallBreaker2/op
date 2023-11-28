// libop的声明
/*
所有op的开放接口都从此cpp类衍生而出
*/
#pragma once

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

// __stdcall 在32位程序编译出来函数名会变成类似_DeleteOp@4这种，64位程序正常。 __cdecl都正常，不知道为什么。。。。
#define OP_API_CALL __cdecl

#ifdef __cplusplus
extern "C" {
#endif
typedef void *opHwnd;

// 额外的C API
OP_API opHwnd OP_API_CALL CreateOp();

OP_API void OP_API_CALL DeleteOp(opHwnd pOp);

///////////////////////////////////////
//// C API
/////////////////////////////////////

//1.版本号Version
OP_API void OP_API_CALL Ver(opHwnd pOp, wchar_t *ret);
//设置目录
OP_API long OP_API_CALL SetPath(opHwnd pOp, const wchar_t *path);
//获取目录
OP_API void OP_API_CALL GetPath(opHwnd pOp, wchar_t *ret);
//获取插件目录
OP_API void OP_API_CALL GetBasePath(opHwnd pOp, wchar_t *ret);
//返回当前对象的ID值，这个值对于每个对象是唯一存在的。可以用来判定两个对象是否一致
OP_API long OP_API_CALL GetID(opHwnd pOp);
//
OP_API long OP_API_CALL OPGetLastError(opHwnd pOp);
//设置是否弹出错误信息,默认是打开 0:关闭，1:显示为信息框，2:保存到文件,3:输出到标准输出
OP_API long OP_API_CALL SetShowErrorMsg(opHwnd pOp, long show_type);

//sleep
OP_API long OP_API_CALL OPSleep(opHwnd pOp, long millseconds);
//Process
//inject dll
OP_API long OP_API_CALL InjectDll(opHwnd pOp, const wchar_t *process_name, const wchar_t *dll_name);
//设置是否开启或者关闭插件内部的图片缓存机制
OP_API long OP_API_CALL EnablePicCache(opHwnd pOp, long enable);
//取上次操作的图色区域，保存为file(24位位图)
OP_API long OP_API_CALL CapturePre(opHwnd pOp, const wchar_t *file_name);
//设置屏幕数据模式，0:从上到下(默认),1:从下到上
OP_API long OP_API_CALL SetScreenDataMode(opHwnd pOp, long mode);
//---------------------algorithm-------------------------------
//A星算法
OP_API void OP_API_CALL AStarFindPath(opHwnd pOp, long mapWidth, long mapHeight, const wchar_t *disable_points, long beginX, long beginY, long endX, long endY, wchar_t *ret);
//
OP_API void OP_API_CALL FindNearestPos(opHwnd pOp, const wchar_t *all_pos, long type, long x, long y, wchar_t *ret);
//--------------------windows api------------------------------
//根据指定条件,枚举系统中符合条件的窗口
OP_API void OP_API_CALL EnumWindow(opHwnd pOp, long parent, const wchar_t *title, const wchar_t *class_name, long filter, wchar_t *ret);
//根据指定进程以及其它条件,枚举系统中符合条件的窗口
OP_API void OP_API_CALL EnumWindowByProcess(opHwnd pOp, const wchar_t *process_name, const wchar_t *title, const wchar_t *class_name, long filter, wchar_t *ret);
//根据指定进程名,枚举系统中符合条件的进程PID
OP_API void OP_API_CALL EnumProcess(opHwnd pOp, const wchar_t *name, wchar_t *ret);
//把窗口坐标转换为屏幕坐标
OP_API long OP_API_CALL OPClientToScreen(opHwnd pOp, long ClientToScreen, long *x, long *y);
//查找符合类名或者标题名的顶层可见窗口
OP_API long OP_API_CALL FindWindow(opHwnd pOp, const wchar_t *class_name, const wchar_t *title);
//根据指定的进程名字，来查找可见窗口
OP_API long OP_API_CALL FindWindowByProcess(opHwnd pOp, const wchar_t *process_name, const wchar_t *class_name, const wchar_t *title);
//根据指定的进程Id，来查找可见窗口
OP_API long OP_API_CALL FindWindowByProcessId(opHwnd pOp, long process_id, const wchar_t *class_name, const wchar_t *title);
//查找符合类名或者标题名的顶层可见窗口,如果指定了parent,则在parent的第一层子窗口中查找
OP_API long OP_API_CALL FindWindowEx(opHwnd pOp, long parent, const wchar_t *class_name, const wchar_t *title);
//获取窗口客户区域在屏幕上的位置
OP_API long OP_API_CALL OPGetClientRect(opHwnd pOp, long hwnd, long *x1, long *y1, long *x2, long *y2);
//获取窗口客户区域的宽度和高度
OP_API long OP_API_CALL GetClientSize(opHwnd pOp, long hwnd, long *width, long *height);
//获取顶层活动窗口中具有输入焦点的窗口句柄
OP_API long OP_API_CALL GetForegroundFocus(opHwnd pOp);
//获取顶层活动窗口,可以获取到按键自带插件无法获取到的句柄
OP_API long OP_API_CALL OPGetForegroundWindow(opHwnd pOp);
//获取鼠标指向的可见窗口句柄
OP_API long OP_API_CALL GetMousePointWindow(opHwnd pOp);
//获取给定坐标的可见窗口句柄
OP_API long OP_API_CALL GetPointWindow(opHwnd pOp, long x, long y);
//根据指定的pid获取进程详细信息
OP_API void OP_API_CALL GetProcessInfo(opHwnd pOp, long pid, wchar_t *ret);
//获取特殊窗口
OP_API long OP_API_CALL GetSpecialWindow(opHwnd pOp, long flag);
//获取给定窗口相关的窗口句柄
OP_API long OP_API_CALL OPGetWindow(opHwnd pOp, long hwnd, long flag);
//获取窗口的类名
OP_API void OP_API_CALL GetWindowClass(opHwnd pOp, long hwnd, wchar_t *ret);
//获取指定窗口所在的进程ID
OP_API long OP_API_CALL GetWindowProcessId(opHwnd pOp, long hwnd);
//获取指定窗口所在的进程的exe文件全路径
OP_API void OP_API_CALL GetWindowProcessPath(opHwnd pOp, long hwnd, wchar_t *ret);
//获取窗口在屏幕上的位置
OP_API long OP_API_CALL OPGetWindowRect(opHwnd pOp, long hwnd, long *x1, long *y1, long *x2, long *y2);
//获取指定窗口的一些属性
OP_API long OP_API_CALL GetWindowState(opHwnd pOp, long hwnd, long flag);
//获取窗口的标题
OP_API void OP_API_CALL GetWindowTitle(opHwnd pOp, long hwnd, wchar_t *rettitle);
//移动指定窗口到指定位置
OP_API long OP_API_CALL OPMoveWindow(opHwnd pOp, long hwnd, long x, long y);
//把屏幕坐标转换为窗口坐标
OP_API long OP_API_CALL OPScreenToClient(opHwnd pOp, long hwnd, long *x, long *y);
//向指定窗口发送粘贴命令
OP_API long OP_API_CALL SendPaste(opHwnd pOp, long hwnd);
//设置窗口客户区域的宽度和高度
OP_API long OP_API_CALL SetClientSize(opHwnd pOp, long hwnd, long width, long hight);
//设置窗口的状态
OP_API long OP_API_CALL SetWindowState(opHwnd pOp, long hwnd, long flag);
//设置窗口的大小
OP_API long OP_API_CALL SetWindowSize(opHwnd pOp, long hwnd, long width, long height);
//设置窗口的标题
OP_API long OP_API_CALL SetWindowText(opHwnd pOp, long hwnd, const wchar_t *title);
//设置窗口的透明度
OP_API long OP_API_CALL SetWindowTransparent(opHwnd pOp, long hwnd, long trans);
//向指定窗口发送文本数据
OP_API long OP_API_CALL SendString(opHwnd pOp, long hwnd, const wchar_t *str);
//向指定窗口发送文本数据-输入法
OP_API long OP_API_CALL SendStringIme(opHwnd pOp, long hwnd, const wchar_t *str);
//运行可执行文件,可指定模式
OP_API long OP_API_CALL RunApp(opHwnd pOp, const wchar_t *cmdline, long mode);
//运行可执行文件，可指定显示模式
OP_API long OP_API_CALL OPWinExec(opHwnd pOp, const wchar_t *cmdline, long cmdshow);

//运行命令行并返回结果
OP_API void OP_API_CALL GetCmdStr(opHwnd pOp, const wchar_t *cmd, long millseconds, wchar_t *ret);

//--------------------Background -----------------------
//bind window and beign capture screen
OP_API long OP_API_CALL BindWindow(opHwnd pOp, long hwnd, const wchar_t *display, const wchar_t *mouse, const wchar_t *keypad, long mode);
//
OP_API long OP_API_CALL UnBindWindow(opHwnd pOp);
//--------------------mouse & keyboard------------------
//获取鼠标位置.
OP_API long OP_API_CALL OPGetCursorPos(opHwnd pOp, long *x, long *y);
//鼠标相对于上次的位置移动rx,ry.
OP_API long OP_API_CALL MoveR(opHwnd pOp, long x, long y);
//把鼠标移动到目的点(x,y)
OP_API long OP_API_CALL MoveTo(opHwnd pOp, long x, long y);
//把鼠标移动到目的范围内的任意一点
OP_API long OP_API_CALL MoveToEx(opHwnd pOp, long x, long y, long w, long h);
//按下鼠标左键
OP_API long OP_API_CALL LeftClick(opHwnd pOp);
//双击鼠标左键
OP_API long OP_API_CALL LeftDoubleClick(opHwnd pOp);
//按住鼠标左键
OP_API long OP_API_CALL LeftDown(opHwnd pOp);
//弹起鼠标左键
OP_API long OP_API_CALL LeftUp(opHwnd pOp);
//按下鼠标中键
OP_API long OP_API_CALL MiddleClick(opHwnd pOp);
//按住鼠标中键
OP_API long OP_API_CALL MiddleDown(opHwnd pOp);
//弹起鼠标中键
OP_API long OP_API_CALL MiddleUp(opHwnd pOp);
//按下鼠标右键
OP_API long OP_API_CALL RightClick(opHwnd pOp);
//按住鼠标右键
OP_API long OP_API_CALL RightDown(opHwnd pOp);
//弹起鼠标右键
OP_API long OP_API_CALL RightUp(opHwnd pOp);
//滚轮向下滚
OP_API long OP_API_CALL WheelDown(opHwnd pOp);
//滚轮向上滚
OP_API long OP_API_CALL WheelUp(opHwnd pOp);
//获取指定的按键状态.(前台信息,不是后台)
OP_API long OP_API_CALL GetKeyState(opHwnd pOp, long vk_code);
//按住指定的虚拟键码
OP_API long OP_API_CALL KeyDown(opHwnd pOp, long vk_code);
//按住指定的虚拟键码
OP_API long OP_API_CALL KeyDownChar(opHwnd pOp, const wchar_t *vk_code);
//弹起来虚拟键vk_code
OP_API long OP_API_CALL KeyUp(opHwnd pOp, long vk_code);
//弹起来虚拟键vk_code
OP_API long OP_API_CALL KeyUpChar(opHwnd pOp, const wchar_t *vk_code);
//等待指定的按键按下 (前台,不是后台)
OP_API long OP_API_CALL WaitKey(opHwnd pOp, long vk_code, long time_out);
//发送字符串
//long SendString(long HWND)
//弹起来虚拟键vk_code
OP_API long OP_API_CALL KeyPress(opHwnd pOp, long vk_code);
OP_API long OP_API_CALL KeyPressChar(opHwnd pOp, const wchar_t *vk_code);

//--------------------image and color-----------------------
//抓取指定区域(x1, y1, x2, y2)的图像, 保存为file
OP_API long OP_API_CALL Capture(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *file_name);
//比较指定坐标点(x,y)的颜色
OP_API long OP_API_CALL CmpColor(opHwnd pOp, long x, long y, const wchar_t *color, double sim);
//查找指定区域内的颜色
OP_API long OP_API_CALL FindColor(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *color, double sim, long dir, long *x, long *y);
//查找指定区域内的所有颜色
OP_API void OP_API_CALL FindColorEx(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *color, double sim, long dir, wchar_t *ret);
//根据指定的多点查找颜色坐标
OP_API long OP_API_CALL FindMultiColor(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *first_color, const wchar_t *offset_color, double sim, long dir, long *x, long *y);
//根据指定的多点查找所有颜色坐标
OP_API void OP_API_CALL FindMultiColorEx(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *first_color, const wchar_t *offset_color, double sim, long dir, wchar_t *ret);
//查找指定区域内的图片
OP_API long OP_API_CALL FindPic(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *files, const wchar_t *delta_color, double sim, long dir, long *x, long *y);
//查找多个图片
OP_API void OP_API_CALL FindPicEx(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *files, const wchar_t *delta_color, double sim, long dir, wchar_t *ret);
//
//这个函数可以查找多个图片, 并且返回所有找到的图像的坐标.此函数同FindPicEx.只是返回值不同.(file1,x,y|file2,x,y|...)
OP_API void OP_API_CALL FindPicExS(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *files, const wchar_t *delta_color, double sim, long dir, wchar_t *ret);
//查找指定区域内的颜色块,颜色格式"RRGGBB-DRDGDB",注意,和按键的颜色格式相反
OP_API long OP_API_CALL FindColorBlock(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *color, double sim, long count, long height, long width, long *x, long *y);
//查找指定区域内的所有颜色块, 颜色格式"RRGGBB-DRDGDB", 注意, 和按键的颜色格式相反
OP_API void OP_API_CALL FindColorBlockEx(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *color, double sim, long count, long height, long width, wchar_t *ret);
//获取(x,y)的颜色
OP_API void OP_API_CALL GetColor(opHwnd pOp, long x, long y, wchar_t *ret);
//
//设置图像输入方式，默认窗口截图
OP_API long OP_API_CALL SetDisplayInput(opHwnd pOp, const wchar_t *mode);

OP_API long OP_API_CALL LoadPic(opHwnd pOp, const wchar_t *file_name);

OP_API long OP_API_CALL FreePic(opHwnd pOp, const wchar_t *file_name);
//从内存加载要查找的图片
OP_API long OP_API_CALL LoadMemPic(opHwnd pOp, const wchar_t *file_name, void *data, long size);
//
OP_API long OP_API_CALL GetScreenData(opHwnd pOp, long x1, long y1, long x2, long y2, size_t *data);
//
OP_API long OP_API_CALL GetScreenDataBmp(opHwnd pOp, long x1, long y1, long x2, long y2, size_t *data, long *size);
//
OP_API void OP_API_CALL GetScreenFrameInfo(opHwnd pOp, long *frame_id, long *time);
//
OP_API void OP_API_CALL MatchPicName(opHwnd pOp, const wchar_t *pic_name, wchar_t *ret);
//----------------------ocr-------------------------
//设置字库文件
OP_API long OP_API_CALL SetDict(opHwnd pOp, long idx, const wchar_t *file_name);
//设置内存字库文件
OP_API long OP_API_CALL SetMemDict(opHwnd pOp, long idx, const wchar_t *data, long size);
//使用哪个字库文件进行识别
OP_API long OP_API_CALL UseDict(opHwnd pOp, long idx);
//识别屏幕范围(x1,y1,x2,y2)内符合color_format的字符串,并且相似度为sim,sim取值范围(0.1-1.0),
OP_API void OP_API_CALL Ocr(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *color, double sim, wchar_t *ret_str);
//回识别到的字符串，以及每个字符的坐标.
OP_API void OP_API_CALL OcrEx(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *color, double sim, wchar_t *ret_str);
//在屏幕范围(x1,y1,x2,y2)内,查找string(可以是任意个字符串的组合),并返回符合color_format的坐标位置
OP_API long OP_API_CALL FindStr(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *strs, const wchar_t *color, double sim, long *retx, long *rety);
//返回符合color_format的所有坐标位置
OP_API void OP_API_CALL FindStrEx(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *strs, const wchar_t *color, double sim, wchar_t *ret);
//识别屏幕范围(x1,y1,x2,y2)内的字符串,自动二值化，而无需指定颜色
OP_API void OP_API_CALL OcrAuto(opHwnd pOp, long x1, long y1, long x2, long y2, double sim, wchar_t *ret_str);
//从文件中识别图片
OP_API void OP_API_CALL OcrFromFile(opHwnd pOp, const wchar_t *file_name, const wchar_t *color_format, double sim, wchar_t *ret);
//从文件中识别图片,无需指定颜色
OP_API void OP_API_CALL OcrAutoFromFile(opHwnd pOp, const wchar_t *file_name, double sim, wchar_t *ret);
//查找频幕中的直线
OP_API void OP_API_CALL FindLine(opHwnd pOp, long x1, long y1, long x2, long y2, const wchar_t *color, double sim, wchar_t *ret);

//向某进程写入数据
OP_API long OP_API_CALL WriteData(opHwnd pOp, long hwnd, const wchar_t *address, const wchar_t *data, long size);
//读取数据
OP_API void OP_API_CALL ReadData(opHwnd pOp, long hwnd, const wchar_t *address, long size, wchar_t *ret);



///////////////////
#ifdef __cplusplus
}
#endif