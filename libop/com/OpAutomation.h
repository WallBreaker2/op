// OpAutomation.h: OpAutomation 的声明

#pragma once
#include "resource.h" // 主符号

#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>

#undef FindWindow
#undef FindWindowEx
#undef SetWindowText
#include "op_i.h"
// #include "Types.h"
// #include "WindowService.h"
// #include "BKbase.h"
// #include "ImageSearchService.h"
#include "../../include/libop.h"
#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error                                                                                                                 \
    "Windows CE 平台(如不提供完全 DCOM 支持的 Windows Mobile 平台)上无法正确支持单线程 COM 对象。定义 _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA 可强制 ATL 支持创建单线程 COM 对象实现并允许使用其单线程 COM 对象实现。rgs 文件中的线程模型已被设置为“Free”，原因是该模型是非 DCOM Windows CE 平台支持的唯一线程模型。"
#endif

using namespace ATL;

// OpAutomation

class ATL_NO_VTABLE OpAutomation
    : public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<OpAutomation, &CLSID_OpAutomation>,
      public IDispatchImpl<IOpAutomation, &IID_IOpAutomation, &LIBID_opLib, /*wMajor =*/1, /*wMinor =*/0> {
  public:
    OpAutomation();

    DECLARE_REGISTRY_RESOURCEID(IDR_OPAUTOMATION)

    BEGIN_COM_MAP(OpAutomation)
    COM_INTERFACE_ENTRY(IOpAutomation)
    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    HRESULT FinalConstruct() {
        return S_OK;
    }

    void FinalRelease() {
    }

  private:
    op::Client obj;

  public:
    //---------------基本设置/属性-------------------

    // 1.版本号Version
    STDMETHOD(RuntimeVer)(BSTR *ret);
    // 设置目录
    STDMETHOD(RuntimeSetPath)(BSTR path, LONG *ret);
    // 获取目录
    STDMETHOD(RuntimeGetPath)(BSTR *path);
    // 获取插件目录
    STDMETHOD(RuntimeGetBasePath)(BSTR *path);
    //
    STDMETHOD(RuntimeGetID)(LONG *ret);
    //
    STDMETHOD(RuntimeGetLastError)(LONG *ret);
    // 设置是否弹出错误信息,默认是打开 0为关闭，1为显示为信息框，2为保存到文件
    STDMETHOD(RuntimeSetShowErrorMsg)(LONG show_type, LONG *ret);

    // sleep
    STDMETHOD(RuntimeSleep)(LONG millseconds, LONG *ret);
    // Process
    // inject dll
    STDMETHOD(WindowInjectDll)(BSTR process_name, BSTR dll_name, LONG *ret);
    // 设置是否开启或者关闭插件内部的图片缓存机制
    STDMETHOD(ImageEnablePicCache)(LONG enable, LONG *ret);
    // 取上次操作的图色区域，保存为file(24位位图)
    STDMETHOD(ImageCapturePre)(BSTR file_name, LONG *ret);
    //---------------------algorithm-------------------------------
    // A星算法
    STDMETHOD(AlgorithmAStarFindPath)
    (LONG mapWidth, LONG mapHeight, BSTR disable_points, LONG beginX, LONG beginY, LONG endX, LONG endY, BSTR *path);
    // 根据部分Ex接口的返回值，然后在所有坐标里找出距离指定坐标最近的那个坐标.
    STDMETHOD(AlgorithmFindNearestPos)(BSTR all_pos, LONG type, LONG x, LONG y, BSTR *retstr);
    //--------------------windows api------------------------------
    // 根据指定条件,枚举系统中符合条件的窗口
    STDMETHOD(WindowEnumWindow)(LONGLONG parent, BSTR title, BSTR class_name, LONG filter, BSTR *retstr);
    // 根据指定进程以及其它条件,枚举系统中符合条件的窗口
    STDMETHOD(WindowEnumWindowByProcess)(BSTR process_name, BSTR title, BSTR class_name, LONG filter, BSTR *retstring);
    // 根据指定进程名,枚举系统中符合条件的进程PID
    STDMETHOD(WindowEnumProcess)(BSTR name, BSTR *retstring);
    // 把窗口坐标转换为屏幕坐标
    STDMETHOD(WindowClientToScreen)(LONGLONG hwnd, VARIANT *x, VARIANT *y, LONG *bret);
    // 查找符合类名或者标题名的顶层可见窗口
    STDMETHOD(WindowFindWindow)(BSTR class_name, BSTR title, LONGLONG *rethwnd);
    // 根据指定的进程名字，来查找可见窗口
    STDMETHOD(WindowFindWindowByProcess)(BSTR process_name, BSTR class_name, BSTR title, LONGLONG *rethwnd);
    // 根据指定的进程Id，来查找可见窗口
    STDMETHOD(WindowFindWindowByProcessId)(LONG process_id, BSTR class_name, BSTR title, LONGLONG *rethwnd);
    // 查找符合类名或者标题名的顶层可见窗口,如果指定了parent,则在parent的第一层子窗口中查找
    STDMETHOD(WindowFindWindowEx)(LONGLONG parent, BSTR class_name, BSTR title, LONGLONG *rethwnd);
    // 获取窗口客户区域在屏幕上的位置
    STDMETHOD(WindowGetClientRect)(LONGLONG hwnd, VARIANT *x1, VARIANT *y1, VARIANT *x2, VARIANT *y2, LONG *nret);
    // 获取窗口客户区域的宽度和高度
    STDMETHOD(WindowGetClientSize)(LONGLONG hwnd, VARIANT *width, VARIANT *height, LONG *nret);
    // 获取顶层活动窗口中具有输入焦点的窗口句柄
    STDMETHOD(WindowGetForegroundFocus)(LONGLONG *rethwnd);
    // 获取顶层活动窗口,可以获取到按键自带插件无法获取到的句柄
    STDMETHOD(WindowGetForegroundWindow)(LONGLONG *rethwnd);
    // 获取鼠标指向的可见窗口句柄
    STDMETHOD(WindowGetMousePointWindow)(LONGLONG *rethwnd);
    // 获取给定坐标的可见窗口句柄
    STDMETHOD(WindowGetPointWindow)(LONG x, LONG y, LONGLONG *rethwnd);
    // 根据指定的pid获取进程详细信息
    STDMETHOD(WindowGetProcessInfo)(LONG pid, BSTR *retstring);
    // 获取特殊窗口
    STDMETHOD(WindowGetSpecialWindow)(LONG flag, LONGLONG *rethwnd);
    // 获取给定窗口相关的窗口句柄
    STDMETHOD(WindowGetWindow)(LONGLONG hwnd, LONG flag, LONGLONG *nret);
    // 获取窗口的类名
    STDMETHOD(WindowGetWindowClass)(LONGLONG hwnd, BSTR *retstring);
    // 获取指定窗口所在的进程ID
    STDMETHOD(WindowGetWindowProcessId)(LONGLONG hwnd, LONG *nretpid);
    // 获取指定窗口所在的进程的exe文件全路径
    STDMETHOD(WindowGetWindowProcessPath)(LONGLONG hwnd, BSTR *retstring);
    // 获取窗口在屏幕上的位置
    STDMETHOD(WindowGetWindowRect)(LONGLONG hwnd, VARIANT *x1, VARIANT *y1, VARIANT *x2, VARIANT *y2, LONG *nret);
    // 获取指定窗口的一些属性
    STDMETHOD(WindowGetWindowState)(LONGLONG hwnd, LONG flag, LONG *rethwnd);
    // 获取窗口的标题
    STDMETHOD(WindowGetWindowTitle)(LONGLONG hwnd, BSTR *rettitle);
    // 移动指定窗口到指定位置
    STDMETHOD(WindowMoveWindow)(LONGLONG hwnd, LONG x, LONG y, LONG *nret);
    // 把屏幕坐标转换为窗口坐标
    STDMETHOD(WindowScreenToClient)(LONGLONG hwnd, VARIANT *x, VARIANT *y, LONG *nret);
    // 向指定窗口发送粘贴命令
    STDMETHOD(WindowSendPaste)(LONGLONG hwnd, LONG *nret);
    // 设置窗口客户区域的宽度和高度
    STDMETHOD(WindowSetClientSize)(LONGLONG hwnd, LONG width, LONG hight, LONG *nret);
    // 设置窗口的状态
    STDMETHOD(WindowSetWindowState)(LONGLONG hwnd, LONG flag, LONG *nret);
    // 设置窗口的大小
    STDMETHOD(WindowSetWindowSize)(LONGLONG hwnd, LONG width, LONG height, LONG *nret);
    // 设置窗口的标题
    STDMETHOD(WindowSetWindowText)(LONGLONG hwnd, BSTR title, LONG *nret);
    // 设置窗口的透明度
    STDMETHOD(WindowSetWindowTransparent)(LONGLONG hwnd, LONG trans, LONG *nret);
    // 向指定窗口发送文本数据
    STDMETHOD(WindowSendString)(LONGLONG hwnd, BSTR str, LONG *ret);
    // 向指定窗口发送文本数据-输入法
    STDMETHOD(WindowSendStringIme)(LONGLONG hwnd, BSTR str, LONG *ret);
    // 运行可执行文件,可指定模式
    STDMETHOD(WindowRunApp)(BSTR cmdline, LONG mode, ULONG *pid, LONG *ret);
    // 按指定布局批量排列多个窗口。hwnds 格式例如 "123|456|789"。
    STDMETHOD(WindowLayoutWindows)
    (BSTR hwnds, LONG layout_type, LONG columns, LONG start_x, LONG start_y, LONG gap_x, LONG gap_y, LONG size_mode,
     LONG window_width, LONG window_height, LONG anchor_mode, LONG *ret);
    // 运行可执行文件，可指定显示模式
    STDMETHOD(WindowWinExec)(BSTR cmdline, LONG cmdshow, LONG *ret);

    // 运行命令行并返回结果
    STDMETHOD(WindowGetCmdStr)(BSTR cmd, LONG millseconds, BSTR *retstr);
    // 设置剪贴板数据
    STDMETHOD(WindowSetClipboard)(BSTR str, LONG *ret);
    // 获取剪贴板数据
    STDMETHOD(WindowGetClipboard)(BSTR *ret);
    // 延时指定的毫秒,过程中不阻塞UI操作
    STDMETHOD(RuntimeDelay)(LONG mis, LONG *ret);
    // 延时指定范围内随机毫秒,过程中不阻塞UI操作
    STDMETHOD(RuntimeDelays)(LONG mis_min, LONG mis_max, LONG *ret);
    //--------------------Background -----------------------
    // 兼容旧接口的单句柄绑定。显示和输入都使用同一个 hwnd。
    STDMETHOD(BindingBindWindow)(LONGLONG hwnd, BSTR display, BSTR mouse, BSTR keypad, LONG mode, LONG *ret);
    // 扩展绑定接口。显示截图使用 display_hwnd，鼠标和键盘输入使用 input_hwnd。
    STDMETHOD(BindingBindWindowEx)
    (LONGLONG display_hwnd, LONGLONG input_hwnd, BSTR display, BSTR mouse, BSTR keypad, LONG mode, LONG *ret);
    //
    STDMETHOD(BindingUnBindWindow)(LONG *ret);
    // 获取当前对象已经绑定的窗口句柄. 无绑定返回0
    STDMETHOD(BindingGetBindWindow)(LONGLONG *ret);
    // 判定当前对象是否已绑定窗口.
    STDMETHOD(BindingIsBind)(LONG *ret);
    //--------------------mouse & keyboard------------------
    // 获取鼠标位置.
    STDMETHOD(MouseGetCursorPos)(VARIANT *x, VARIANT *y, LONG *ret);
    // 获取当前鼠标形状: visible,hash,width,height,hotX,hotY.
    STDMETHOD(MouseGetCursorShape)(BSTR *ret);
    // 鼠标相对于上次的位置移动rx,ry.
    STDMETHOD(MouseMoveR)(LONG x, LONG y, LONG *ret);
    // 把鼠标移动到目的点(x,y)
    STDMETHOD(MouseMoveTo)(LONG x, LONG y, LONG *ret);
    // 把鼠标移动到目的范围内的任意一点
    STDMETHOD(MouseMoveToEx)(LONG x, LONG y, LONG w, LONG h, BSTR *ret);
    // 按下鼠标左键
    STDMETHOD(MouseLeftClick)(LONG *ret);
    // 双击鼠标左键
    STDMETHOD(MouseLeftDoubleClick)(LONG *ret);
    // 按住鼠标左键
    STDMETHOD(MouseLeftDown)(LONG *ret);
    // 弹起鼠标左键
    STDMETHOD(MouseLeftUp)(LONG *ret);
    // 按下鼠标中键
    STDMETHOD(MouseMiddleClick)(LONG *ret);
    // 按住鼠标中键
    STDMETHOD(MouseMiddleDown)(LONG *ret);
    // 弹起鼠标中键
    STDMETHOD(MouseMiddleUp)(LONG *ret);
    // 按下鼠标右键
    STDMETHOD(MouseRightClick)(LONG *ret);
    // 按住鼠标右键
    STDMETHOD(MouseRightDown)(LONG *ret);
    // 弹起鼠标右键
    STDMETHOD(MouseRightUp)(LONG *ret);
    // 滚轮向下滚
    STDMETHOD(MouseWheelDown)(LONG *ret);
    // 滚轮向上滚
    STDMETHOD(MouseWheelUp)(LONG *ret);
    // 设置鼠标单击或者双击时,鼠标按下和弹起的时间间隔
    STDMETHOD(MouseSetMouseDelay)(BSTR type, LONG delay, LONG *ret);
    // 获取指定的按键状态.(前台信息,不是后台)
    STDMETHOD(KeyboardGetKeyState)(LONG vk_code, LONG *ret);
    // 按住指定的虚拟键码
    STDMETHOD(KeyboardKeyDown)(LONG vk_code, LONG *ret);
    // 按住指定的虚拟键码
    STDMETHOD(KeyboardKeyDownChar)(BSTR vk_code, LONG *ret);
    // 弹起来虚拟键vk_code
    STDMETHOD(KeyboardKeyUp)(LONG vk_code, LONG *ret);
    // 弹起来虚拟键vk_code
    STDMETHOD(KeyboardKeyUpChar)(BSTR vk_code, LONG *ret);
    // 等待指定的按键按下 (前台,不是后台)
    STDMETHOD(KeyboardWaitKey)(LONG vk_code, LONG time_out, LONG *ret);
    // 发送字符串
    // STDMETHOD(WindowSendString)(LONG HWND)
    // 弹起来虚拟键vk_code
    STDMETHOD(KeyboardKeyPress)(LONG vk_code, LONG *ret);
    STDMETHOD(KeyboardKeyPressChar)(BSTR vk_code, LONG *ret);
    // 设置按键时,键盘按下和弹起的时间间隔
    STDMETHOD(KeyboardSetKeypadDelay)(BSTR type, LONG delay, LONG *ret);
    // 根据指定的字符串序列，依次按顺序按下其中的字符
    STDMETHOD(KeyboardKeyPressStr)(BSTR key_str, LONG delay, LONG *ret);
    //--------------------image and color-----------------------
    // 抓取指定区域(x1, y1, x2, y2)的图像, 保存为file
    STDMETHOD(ImageCapture)(LONG x1, LONG y1, LONG x2, LONG y2, BSTR file_name, LONG *ret);
    // 比较指定坐标点(x,y)的颜色
    STDMETHOD(ImageCmpColor)(LONG x, LONG y, BSTR color, DOUBLE sim, LONG *ret);
    // 查找指定区域内的颜色
    STDMETHOD(ImageFindColor)
    (LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG dir, VARIANT *x, VARIANT *y, LONG *ret);
    // 查找指定区域内的所有颜色
    STDMETHOD(ImageFindColorEx)(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG dir, BSTR *retstr);
    // 查找指定区域内的颜色数量
    STDMETHOD(ImageGetColorNum)(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG *ret);
    // 根据指定的多点查找颜色坐标
    STDMETHOD(ImageFindMultiColor)
    (LONG x1, LONG y1, LONG x2, LONG y2, BSTR first_color, BSTR offset_color, DOUBLE sim, LONG dir, VARIANT *x,
     VARIANT *y, LONG *ret);
    // 根据指定的多点查找所有颜色坐标
    STDMETHOD(ImageFindMultiColorEx)
    (LONG x1, LONG y1, LONG x2, LONG y2, BSTR first_color, BSTR offset_color, DOUBLE sim, LONG dir, BSTR *retstr);
    // 查找指定区域内的图片
    STDMETHOD(ImageFindPic)
    (LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim, LONG dir, VARIANT *x, VARIANT *y,
     LONG *ret);
    // 查找多个图片
    STDMETHOD(ImageFindPicEx)
    (LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim, LONG dir, BSTR *retstr);
    // 这个函数可以查找多个图片, 并且返回所有找到的图像的坐标.此函数同FindPicEx.只是返回值不同.(file1,x,y|file2,x,y|...)
    STDMETHOD(ImageFindPicExS)
    (LONG x1, LONG y1, LONG x2, LONG y2, BSTR files, BSTR delta_color, DOUBLE sim, LONG dir, BSTR *retstr);
    // 查找指定区域内的颜色块,颜色格式"RRGGBB-DRDGDB",注意,和按键的颜色格式相反
    STDMETHOD(ImageFindColorBlock)
    (LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG count, LONG height, LONG width, VARIANT *x,
     VARIANT *y, LONG *ret);
    // 查找指定区域内的所有颜色块, 颜色格式"RRGGBB-DRDGDB", 注意, 和按键的颜色格式相反
    STDMETHOD(ImageFindColorBlockEx)
    (LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, LONG count, LONG height, LONG width, BSTR *ret);
    // 获取(x,y)的颜色
    STDMETHOD(ImageGetColor)(LONG x, LONG y, BSTR *ret);
    // 设置图像输入方式，默认窗口截图
    STDMETHOD(ImageSetDisplayInput)(BSTR mode, LONG *ret);
    STDMETHOD(ImageLoadPic)(BSTR pic_name, LONG *ret);
    STDMETHOD(ImageFreePic)(BSTR pic_name, LONG *ret);
    STDMETHOD(ImageLoadMemPic)(BSTR pic_name, long long data, LONG size, LONG *ret);
    STDMETHOD(ImageGetPicSize)(BSTR pic_name, VARIANT *width, VARIANT *height, LONG *ret);
    // 获取指定区域的图像,用二进制数据的方式返回
    STDMETHOD(ImageGetScreenData)(LONG x1, LONG y1, LONG x2, LONG y2, LONG *ret);
    // 获取指定区域的图像,用24位位图的数据格式返回,方便二次开发.（或者可以配合SetDisplayInput的mem模式）
    STDMETHOD(ImageGetScreenDataBmp)(LONG x1, LONG y1, LONG x2, LONG y2, VARIANT *data, VARIANT *size, LONG *ret);
    // 根据通配符获取文件集合. 方便用于FindPic和FindPicEx
    STDMETHOD(ImageMatchPicName)(BSTR pic_name, BSTR *ret);

    //----------------------ocr-------------------------
    // 设置字库文件
    STDMETHOD(OcrSetDict)(LONG idx, BSTR file_name, LONG *ret);
    STDMETHOD(OcrGetDict)(LONG idx, LONG font_index, BSTR *ret_str);
    // 设置内存字库文件
    STDMETHOD(OcrSetMemDict)(LONG idx, BSTR data, LONG size, LONG *ret);
    // 使用哪个字库文件进行识别
    STDMETHOD(OcrUseDict)(LONG idx, LONG *ret);
    // 给指定的字库中添加一条字库信息
    STDMETHOD(OcrAddDict)(LONG idx, BSTR dict_info, LONG *ret);
    STDMETHOD(OcrSaveDict)(LONG idx, BSTR file_name, LONG *ret);
    // 清空指定的字库
    STDMETHOD(OcrClearDict)(LONG idx, LONG *ret);
    // 获取指定的字库中的字符数量
    STDMETHOD(OcrGetDictCount)(LONG idx, LONG *ret);
    // 获取当前使用的字库序号
    STDMETHOD(OcrGetNowDict)(LONG *ret);
    // 根据指定的范围,以及指定的颜色描述，提取点阵信息，类似于大漠工具里的单独提取
    STDMETHOD(OcrFetchWord)(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, BSTR word, BSTR *ret_str);
    // 识别这个范围内所有满足条件的词组，这个识别函数不会用到字库. 只是识别大概形状的位置
    STDMETHOD(OcrGetWordsNoDict)(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, BSTR *ret_str);
    // 在使用GetWords进行词组识别以后,可以用此接口进行识别词组数量的计算
    STDMETHOD(OcrGetWordResultCount)(BSTR result, LONG *ret);
    // 在使用GetWords进行词组识别以后,可以用此接口进行识别各个词组的坐标
    STDMETHOD(OcrGetWordResultPos)(BSTR result, LONG index, VARIANT *x, VARIANT *y, LONG *ret);
    // 在使用GetWords进行词组识别以后,可以用此接口进行识别各个词组的内容
    STDMETHOD(OcrGetWordResultStr)(BSTR result, LONG index, BSTR *ret_str);
    // 识别屏幕范围(x1,y1,x2,y2)内符合color_format的字符串,并且相似度为sim,sim取值范围(0.1-1.0),
    STDMETHOD(OcrRecognize)(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR *ret_str);
    // 回识别到的字符串，以及每个字符的坐标.
    STDMETHOD(OcrRecognizeEx)(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR *ret_str);
    // 在屏幕范围(x1,y1,x2,y2)内,查找string(可以是任意个字符串的组合),并返回符合color_format的坐标位置
    STDMETHOD(OcrFindStr)
    (LONG x1, LONG y1, LONG x2, LONG y2, BSTR strs, BSTR color, DOUBLE sim, VARIANT *retx, VARIANT *rety, LONG *ret);
    // 返回符合color_format的所有坐标位置
    STDMETHOD(OcrFindStrEx)(LONG x1, LONG y1, LONG x2, LONG y2, BSTR strs, BSTR color, DOUBLE sim, BSTR *retstr);
    // 识别屏幕范围(x1,y1,x2,y2)内的字符串,使用tesseract库识别
    STDMETHOD(OcrRecognizeAuto)(LONG x1, LONG y1, LONG x2, LONG y2, DOUBLE sim, BSTR *ret_str);
    // 从文件中识别图片
    STDMETHOD(OcrRecognizeFromFile)(BSTR file_name, BSTR color_format, DOUBLE sim, BSTR *retstr);
    // 从文件中识别图片,使用tesseract库识别
    STDMETHOD(OcrRecognizeAutoFromFile)(BSTR file_name, DOUBLE sim, BSTR *retstr);
    // 查找频幕中的直线
    STDMETHOD(OcrFindLine)(LONG x1, LONG y1, LONG x2, LONG y2, BSTR color, DOUBLE sim, BSTR *retstr);
    // 设置 HTTP OCR 引擎地址和参数.
    STDMETHOD(OcrSetEngine)(BSTR path_of_engine, BSTR dll_name, BSTR argv, LONG *ret);
    STDMETHOD(YoloSetEngine)(BSTR path_of_engine, BSTR dll_name, BSTR argv, LONG *ret);
    STDMETHOD(YoloDetect)(LONG x1, LONG y1, LONG x2, LONG y2, DOUBLE conf, DOUBLE iou, BSTR *retjson, LONG *ret);
    STDMETHOD(YoloDetectFromFile)(BSTR file_name, DOUBLE conf, DOUBLE iou, BSTR *retjson, LONG *ret);

    //-----------------------memory---------------------------------
    // 向某进程写入数据
    STDMETHOD(MemoryWriteData)(LONGLONG hwnd, BSTR address, BSTR data, LONG size, LONG *ret);
    // 读取数据
    STDMETHOD(MemoryReadData)(LONGLONG hwnd, BSTR address, LONG size, BSTR *retstr);
    // 类型化读写，和内存地址表达式规则保持一致。
    STDMETHOD(MemoryReadInt)(LONGLONG hwnd, BSTR address, LONG type, LONGLONG *ret);
    STDMETHOD(MemoryWriteInt)(LONGLONG hwnd, BSTR address, LONG type, LONGLONG value, LONG *ret);
    STDMETHOD(MemoryReadFloat)(LONGLONG hwnd, BSTR address, DOUBLE *ret);
    STDMETHOD(MemoryWriteFloat)(LONGLONG hwnd, BSTR address, DOUBLE value, LONG *ret);
    STDMETHOD(MemoryReadDouble)(LONGLONG hwnd, BSTR address, DOUBLE *ret);
    STDMETHOD(MemoryWriteDouble)(LONGLONG hwnd, BSTR address, DOUBLE value, LONG *ret);
    STDMETHOD(MemoryReadString)(LONGLONG hwnd, BSTR address, LONG type, LONG len, BSTR *retstr);
    STDMETHOD(MemoryWriteString)(LONGLONG hwnd, BSTR address, LONG type, BSTR value, LONG *ret);
    //-----------------------opcv---------------------------------
    STDMETHOD(OpenCvLoadTemplate)(BSTR name, BSTR file_path, LONG *ret);
    STDMETHOD(OpenCvLoadMaskedTemplate)(BSTR name, BSTR template_path, BSTR mask_path, LONG *ret);
    STDMETHOD(OpenCvRemoveTemplate)(BSTR name, LONG *ret);
    STDMETHOD(OpenCvRemoveAllTemplates)(LONG *ret);
    STDMETHOD(OpenCvHasTemplate)(BSTR name, LONG *ret);
    STDMETHOD(OpenCvGetTemplateCount)(LONG *ret);
    STDMETHOD(OpenCvGetAllTemplateNames)(BSTR *retstr);
    STDMETHOD(OpenCvGetOpenCvVersion)(BSTR *retstr);
    STDMETHOD(OpenCvLoadTemplateList)(BSTR template_list, LONG *ret);
    STDMETHOD(OpenCvToGray)(BSTR src_file, BSTR dst_file, LONG *ret);
    STDMETHOD(OpenCvToBinary)(BSTR src_file, BSTR dst_file, LONG *ret);
    STDMETHOD(OpenCvToEdge)(BSTR src_file, BSTR dst_file, LONG *ret);
    STDMETHOD(OpenCvToOutline)(BSTR src_file, BSTR dst_file, LONG *ret);
    STDMETHOD(OpenCvDenoise)(BSTR src_file, BSTR dst_file, LONG *ret);
    STDMETHOD(OpenCvEqualize)(BSTR src_file, BSTR dst_file, LONG *ret);
    STDMETHOD(OpenCvCLAHE)(BSTR src_file, BSTR dst_file, DOUBLE clip_limit, LONG tile_grid_size, LONG *ret);
    STDMETHOD(OpenCvBlur)(BSTR src_file, BSTR dst_file, BSTR mode, LONG kernel_size, LONG *ret);
    STDMETHOD(OpenCvSharpen)(BSTR src_file, BSTR dst_file, DOUBLE strength, LONG *ret);
    STDMETHOD(OpenCvCropValid)(BSTR src_file, BSTR dst_file, LONG *ret);
    STDMETHOD(OpenCvConnectedComponents)(BSTR src_file, DOUBLE min_area, BSTR *retjson, LONG *ret);
    STDMETHOD(OpenCvFindContours)(BSTR src_file, DOUBLE min_area, BSTR *retjson, LONG *ret);
    STDMETHOD(OpenCvPreprocessPipeline)(BSTR src_file, BSTR dst_file, BSTR pipeline, LONG *ret);
    STDMETHOD(OpenCvCrop)(BSTR src_file, LONG x, LONG y, LONG width, LONG height, BSTR dst_file, LONG *ret);
    STDMETHOD(OpenCvResize)(BSTR src_file, LONG width, LONG height, BSTR dst_file, LONG *ret);
    STDMETHOD(OpenCvThreshold)(BSTR src_file, BSTR dst_file, DOUBLE threshold, DOUBLE max_value, BSTR mode, LONG *ret);
    STDMETHOD(OpenCvInRange)(BSTR src_file, BSTR dst_file, BSTR color_space, BSTR lower, BSTR upper, LONG *ret);
    STDMETHOD(OpenCvMorphology)(BSTR src_file, BSTR dst_file, BSTR mode, LONG kernel_size, LONG iterations, LONG *ret);
    STDMETHOD(OpenCvThin)(BSTR src_file, BSTR dst_file, BSTR mode, LONG *ret);
    STDMETHOD(OpenCvMatchTemplate)
    (LONG x, LONG y, LONG width, LONG height, BSTR template_name, DOUBLE threshold, LONG dir, LONG strip_mode,
     LONG method, LONG color_mode,
     BSTR *retjson, LONG *ret);
    STDMETHOD(OpenCvMatchTemplateScale)
    (LONG x, LONG y, LONG width, LONG height, BSTR template_name, BSTR scales, DOUBLE threshold, LONG method,
     LONG color_mode, BSTR *retjson, LONG *ret);
    STDMETHOD(OpenCvMatchAnyTemplate)
    (LONG x, LONG y, LONG width, LONG height, BSTR template_names, DOUBLE threshold, LONG dir, LONG strip_mode,
     LONG method, LONG color_mode,
     BSTR *retjson, LONG *ret);
    STDMETHOD(OpenCvMatchAllTemplates)
    (LONG x, LONG y, LONG width, LONG height, BSTR template_names, DOUBLE threshold, LONG dir, LONG strip_mode,
     LONG method, LONG color_mode,
     BSTR *retjson, LONG *ret);
    STDMETHOD(OpenCvFeatureMatchTemplate)
    (LONG x, LONG y, LONG width, LONG height, BSTR template_name, DOUBLE threshold, BSTR *retjson, LONG *ret);
    STDMETHOD(OpenCvEdgeMatchTemplate)
    (LONG x, LONG y, LONG width, LONG height, BSTR template_name, DOUBLE threshold, BSTR *retjson, LONG *ret);
    STDMETHOD(OpenCvShapeMatchTemplate)
    (LONG x, LONG y, LONG width, LONG height, BSTR template_name, DOUBLE threshold, BSTR *retjson, LONG *ret);
};

OBJECT_ENTRY_AUTO(__uuidof(OpAutomation), OpAutomation)
