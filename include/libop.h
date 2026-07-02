// op::Client的声明
/*
所有op的开放接口都从此cpp类衍生而出
*/
#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <BaseTsd.h>
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
// op::Client
#undef FindWindow
#undef FindWindowEx
#undef SetWindowText

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

namespace op {

namespace internal {
struct ClientContext;
}

class OP_API Client {

  public:
    Client();
    ~Client();
    // 复制构造
    Client(Client const &) = delete;
    Client &operator=(Client const rhs) = delete;

    using RuntimeService = Client;
    using WindowService = Client;
    using BindingService = Client;
    using InputService = Client;
    using ImageService = Client;
    using OcrService = Client;
    using OpenCvService = Client;
    using YoloService = Client;
    using MemoryService = Client;

    RuntimeService &runtime() noexcept {
        return *this;
    }
    WindowService &window() noexcept {
        return *this;
    }
    BindingService &binding() noexcept {
        return *this;
    }
    InputService &input() noexcept {
        return *this;
    }
    ImageService &image() noexcept {
        return *this;
    }
    OcrService &ocr() noexcept {
        return *this;
    }
    OpenCvService &opencv() noexcept {
        return *this;
    }
    YoloService &yolo() noexcept {
        return *this;
    }
    MemoryService &memory() noexcept {
        return *this;
    }

  private:
    std::unique_ptr<internal::ClientContext> m_context;
    static std::atomic<int> s_id;

  public:
    // 以下 public 接口的参数需标注 SAL 2.0（_In_ / _Out_ / _Inout_）
    //---------------基本设置/属性-------------------

    // 1.版本号Version
    std::wstring Ver();
    // 设置目录
    void SetPath(_In_ const wchar_t *path, _Out_ long *ret);
    // 获取目录
    void GetPath(_Out_ std::wstring &ret);
    // 获取插件目录
    void GetBasePath(_Out_ std::wstring &ret);
    // 返回当前对象的ID值，这个值对于每个对象是唯一存在的。可以用来判定两个对象是否一致
    void GetID(_Out_ long *ret);
    //
    void GetLastError(_Out_ long *ret);
    // 设置是否弹出错误信息,默认是打开 0:关闭，1:显示为信息框，2:保存到文件,3:输出到标准输出
    void SetShowErrorMsg(_In_ long show_type, _Out_ long *ret);

    // sleep
    void Sleep(_In_ long millseconds, _Out_ long *ret);
    // Process
    // inject dll
    void InjectDll(_In_ const wchar_t *process_name, _In_ const wchar_t *dll_name, _Out_ long *ret);
    // 设置是否开启或者关闭插件内部的图片缓存机制
    void EnablePicCache(_In_ long enable, _Out_ long *ret);
    // 取上次操作的图色区域，保存为file(24位位图)
    void CapturePre(_In_ const wchar_t *file_name, _Out_ long *ret);
    // 设置屏幕数据模式，0:从上到下(默认),1:从下到上
    void SetScreenDataMode(_In_ long mode, _Out_ long *ret);
    //---------------------algorithm-------------------------------
    // A星算法
    void AStarFindPath(_In_ long mapWidth, _In_ long mapHeight, _In_ const wchar_t *disable_points, _In_ long beginX,
                       _In_ long beginY, _In_ long endX, _In_ long endY, _Out_ std::wstring &ret);
    // 从坐标列表中查找距离指定坐标最近的点
    void FindNearestPos(_In_ const wchar_t *all_pos, _In_ long type, _In_ long x, _In_ long y, _Out_ std::wstring &ret);
    //--------------------windows api------------------------------
    // 根据指定条件,枚举系统中符合条件的窗口
    void EnumWindow(_In_ LONG_PTR parent, _In_ const wchar_t *title, _In_ const wchar_t *class_name, _In_ long filter,
                    _Out_ std::wstring &ret);
    // 根据指定进程以及其它条件,枚举系统中符合条件的窗口
    void EnumWindowByProcess(_In_ const wchar_t *process_name, _In_ const wchar_t *title, _In_ const wchar_t *class_name,
                             _In_ long filter, _Out_ std::wstring &ret);
    // 根据指定进程名,枚举系统中符合条件的进程PID
    void EnumProcess(_In_ const wchar_t *name, _Out_ std::wstring &ret);
    // 把窗口坐标转换为屏幕坐标
    void ClientToScreen(_In_ LONG_PTR hwnd, _Inout_ long *x, _Inout_ long *y, _Out_ long *bret);
    // 查找符合类名或者标题名的顶层可见窗口
    void FindWindow(_In_ const wchar_t *class_name, _In_ const wchar_t *title, _Out_ LONG_PTR *ret);
    // 根据指定的进程名字，来查找可见窗口
    void FindWindowByProcess(_In_ const wchar_t *process_name, _In_ const wchar_t *class_name, _In_ const wchar_t *title,
                             _Out_ LONG_PTR *ret);
    // 根据指定的进程Id，来查找可见窗口
    void FindWindowByProcessId(_In_ long process_id, _In_ const wchar_t *class_name, _In_ const wchar_t *title,
                               _Out_ LONG_PTR *ret);
    // 查找符合类名或者标题名的顶层可见窗口,如果指定了parent,则在parent的第一层子窗口中查找
    void FindWindowEx(_In_ LONG_PTR parent, _In_ const wchar_t *class_name, _In_ const wchar_t *title,
                      _Out_ LONG_PTR *ret);
    // 获取窗口客户区域在屏幕上的位置
    void GetClientRect(_In_ LONG_PTR hwnd, _Out_ long *x1, _Out_ long *y1, _Out_ long *x2, _Out_ long *y2,
                       _Out_ long *ret);
    // 获取窗口客户区域的宽度和高度
    void GetClientSize(_In_ LONG_PTR hwnd, _Out_ long *width, _Out_ long *height, _Out_ long *ret);
    // 获取顶层活动窗口中具有输入焦点的窗口句柄
    void GetForegroundFocus(_Out_ LONG_PTR *ret);
    // 获取顶层活动窗口,可以获取到按键自带插件无法获取到的句柄
    void GetForegroundWindow(_Out_ LONG_PTR *ret);
    // 获取鼠标指向的可见窗口句柄
    void GetMousePointWindow(_Out_ LONG_PTR *ret);
    // 获取给定坐标的可见窗口句柄
    void GetPointWindow(_In_ long x, _In_ long y, _Out_ LONG_PTR *ret);
    // 根据指定的pid获取进程详细信息
    void GetProcessInfo(_In_ long pid, _Out_ std::wstring &ret);
    // 获取特殊窗口
    void GetSpecialWindow(_In_ long flag, _Out_ LONG_PTR *ret);
    // 获取给定窗口相关的窗口句柄
    void GetWindow(_In_ LONG_PTR hwnd, _In_ long flag, _Out_ LONG_PTR *ret);
    // 获取窗口的类名
    void GetWindowClass(_In_ LONG_PTR hwnd, _Out_ std::wstring &ret);
    // 获取指定窗口所在的进程ID
    void GetWindowProcessId(_In_ LONG_PTR hwnd, _Out_ long *ret);
    // 获取指定窗口所在的进程的exe文件全路径
    void GetWindowProcessPath(_In_ LONG_PTR hwnd, _Out_ std::wstring &ret);
    // 获取窗口在屏幕上的位置
    void GetWindowRect(_In_ LONG_PTR hwnd, _Out_ long *x1, _Out_ long *y1, _Out_ long *x2, _Out_ long *y2,
                       _Out_ long *ret);
    // 获取指定窗口的一些属性
    void GetWindowState(_In_ LONG_PTR hwnd, _In_ long flag, _Out_ long *ret);
    // 获取窗口的标题
    void GetWindowTitle(_In_ LONG_PTR hwnd, _Out_ std::wstring &rettitle);
    // 移动指定窗口到指定位置
    void MoveWindow(_In_ LONG_PTR hwnd, _In_ long x, _In_ long y, _Out_ long *ret);
    // 把屏幕坐标转换为窗口坐标
    void ScreenToClient(_In_ LONG_PTR hwnd, _Inout_ long *x, _Inout_ long *y, _Out_ long *ret);
    // 向指定窗口发送粘贴命令
    void SendPaste(_In_ LONG_PTR hwnd, _Out_ long *ret);
    // 设置窗口客户区域的宽度和高度
    void SetClientSize(_In_ LONG_PTR hwnd, _In_ long width, _In_ long hight, _Out_ long *ret);
    // 设置窗口的状态
    void SetWindowState(_In_ LONG_PTR hwnd, _In_ long flag, _Out_ long *ret);
    // 设置窗口的大小
    void SetWindowSize(_In_ LONG_PTR hwnd, _In_ long width, _In_ long height, _Out_ long *ret);
    // 按指定布局批量排列多个窗口。hwnds 格式例如 "123|456|789"。
    void LayoutWindows(_In_ const wchar_t *hwnds, _In_ long layout_type, _In_ long columns, _In_ long start_x,
                       _In_ long start_y, _In_ long gap_x, _In_ long gap_y, _In_ long size_mode, _In_ long window_width,
                       _In_ long window_height, _In_ long anchor_mode, _Out_ long *ret);
    // 设置窗口的标题
    void SetWindowText(_In_ LONG_PTR hwnd, _In_ const wchar_t *title, _Out_ long *ret);
    // 设置窗口的透明度
    void SetWindowTransparent(_In_ LONG_PTR hwnd, _In_ long trans, _Out_ long *ret);
    // 向指定窗口发送文本数据
    void SendString(_In_ LONG_PTR hwnd, _In_ const wchar_t *str, _Out_ long *ret);
    // 向指定窗口发送文本数据-输入法
    void SendStringIme(_In_ LONG_PTR hwnd, _In_ const wchar_t *str, _Out_ long *ret);
    // 运行可执行文件,可指定模式
    void RunApp(_In_ const wchar_t *cmdline, _In_ long mode, _Out_ unsigned long *pid, _Out_ long *ret);
    // 运行可执行文件，可指定显示模式
    void WinExec(_In_ const wchar_t *cmdline, _In_ long cmdshow, _Out_ long *ret);

    // 运行命令行并返回结果
    void GetCmdStr(_In_ const wchar_t *cmd, _In_ long millseconds, _Out_ std::wstring &retstr);
    // 设置剪贴板数据
    void SetClipboard(_In_ const wchar_t *str, _Out_ long *ret);
    // 获取剪贴板数据
    void GetClipboard(_Out_ std::wstring &ret);
    // 延时指定的毫秒,过程中不阻塞UI操作
    void Delay(_In_ long mis, _Out_ long *ret);
    // 延时指定范围内随机毫秒,过程中不阻塞UI操作
    void Delays(_In_ long mis_min, _In_ long mis_max, _Out_ long *ret);
    //--------------------Background -----------------------
    // 兼容旧接口的单句柄绑定。显示和输入都使用同一个 hwnd。
    void BindWindow(_In_ LONG_PTR hwnd, _In_ const wchar_t *display, _In_ const wchar_t *mouse, _In_ const wchar_t *keypad,
                    _In_ long mode, _Out_ long *ret);
    // 扩展绑定接口。显示截图使用 display_hwnd，鼠标和键盘输入使用 input_hwnd。
    void BindWindowEx(_In_ LONG_PTR display_hwnd, _In_ LONG_PTR input_hwnd, _In_ const wchar_t *display,
                      _In_ const wchar_t *mouse, _In_ const wchar_t *keypad, _In_ long mode, _Out_ long *ret);
    // 解绑窗口
    void UnBindWindow(_Out_ long *ret);
    // 获取当前对象已经绑定的显示窗口句柄. 无绑定返回0
    void GetBindWindow(_Out_ LONG_PTR *ret);
    // 判定当前对象是否已绑定窗口.
    void IsBind(_Out_ long *ret);
    //--------------------mouse & keyboard------------------
    // 获取鼠标位置.
    void GetCursorPos(_Out_ long *x, _Out_ long *y, _Out_ long *ret);
    // 获取当前鼠标形状: visible,hash,width,height,hotX,hotY.
    void GetCursorShape(_Out_ std::wstring &ret);
    // 鼠标相对于上次的位置移动rx,ry.
    void MoveR(_In_ long x, _In_ long y, _Out_ long *ret);
    // 把鼠标移动到目的点(x,y)
    void MoveTo(_In_ long x, _In_ long y, _Out_ long *ret);
    // 把鼠标移动到目的范围内的任意一点
    void MoveToEx(_In_ long x, _In_ long y, _In_ long w, _In_ long h, _Out_ std::wstring &ret);
    // 按下鼠标左键
    void LeftClick(_Out_ long *ret);
    // 双击鼠标左键
    void LeftDoubleClick(_Out_ long *ret);
    // 按住鼠标左键
    void LeftDown(_Out_ long *ret);
    // 弹起鼠标左键
    void LeftUp(_Out_ long *ret);
    // 按下鼠标中键
    void MiddleClick(long *ret);
    // 双击鼠标中键
    void MiddleDoubleClick(long *ret);
    // 按住鼠标中键
    void MiddleDown(_Out_ long *ret);
    // 弹起鼠标中键
    void MiddleUp(_Out_ long *ret);
    // 按下鼠标右键
    void RightClick(long *ret);
    // 双击鼠标右键
    void RightDoubleClick(long *ret);
    // 按住鼠标右键
    void RightDown(_Out_ long *ret);
    // 弹起鼠标右键
    void RightUp(long *ret);
    // 按下鼠标侧键1
    void XButton1Click(long *ret);
    // 双击鼠标侧键1
    void XButton1DoubleClick(long *ret);
    // 按住鼠标侧键1
    void XButton1Down(long *ret);
    // 弹起鼠标侧键1
    void XButton1Up(long *ret);
    // 按下鼠标侧键2
    void XButton2Click(long *ret);
    // 双击鼠标侧键2
    void XButton2DoubleClick(long *ret);
    // 按住鼠标侧键2
    void XButton2Down(long *ret);
    // 弹起鼠标侧键2
    void XButton2Up(long *ret);
    // 垂直滚轮滚动指定 delta
    void Wheel(long delta, long *ret);
    // 水平滚轮滚动指定 delta
    void HWheel(long delta, long *ret);
    // 滚轮向下滚
    void WheelDown(_Out_ long *ret);
    // 滚轮向上滚
    void WheelUp(_Out_ long *ret);
    // 设置鼠标单击或者双击时,鼠标按下和弹起的时间间隔
    void SetMouseDelay(_In_ const wchar_t *type, _In_ long delay, _Out_ long *ret);
    // 获取指定的按键状态.(前台信息,不是后台)
    void GetKeyState(_In_ long vk_code, _Out_ long *ret);
    // 按住指定的虚拟键码
    void KeyDown(_In_ long vk_code, _Out_ long *ret);
    // 按住指定的虚拟键码
    void KeyDownChar(_In_ const wchar_t *vk_code, _Out_ long *ret);
    // 弹起来虚拟键vk_code
    void KeyUp(_In_ long vk_code, _Out_ long *ret);
    // 弹起来虚拟键vk_code
    void KeyUpChar(_In_ const wchar_t *vk_code, _Out_ long *ret);
    // 等待指定的按键按下 (前台,不是后台)
    void WaitKey(_In_ long vk_code, _In_ long time_out, _Out_ long *ret);
    // 发送字符串
    // long SendString(long HWND)
    // 弹起来虚拟键vk_code
    void KeyPress(_In_ long vk_code, _Out_ long *ret);
    void KeyPressChar(_In_ const wchar_t *vk_code, _Out_ long *ret);
    // 设置按键时,键盘按下和弹起的时间间隔
    void SetKeypadDelay(_In_ const wchar_t *type, _In_ long delay, _Out_ long *ret);
    // 根据指定的字符串序列，依次按顺序按下其中的字符
    void KeyPressStr(_In_ const wchar_t *key_str, _In_ long delay, _Out_ long *ret);
    //--------------------image and color-----------------------
    // 抓取指定区域(x1, y1, x2, y2)的图像, 保存为file
    void Capture(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *file_name, _Out_ long *ret);
    // 比较指定坐标点(x,y)的颜色
    void CmpColor(_In_ long x, _In_ long y, _In_ const wchar_t *color, _In_ double sim, _Out_ long *ret);
    // 查找指定区域内的颜色
    void FindColor(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim,
                   _In_ long dir, _Out_ long *x, _Out_ long *y, _Out_ long *ret);
    // 查找指定区域内的所有颜色
    void FindColorEx(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim,
                     _In_ long dir, _Out_ std::wstring &retstr);
    // 查找指定区域内的所有颜色数量
    void GetColorNum(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim,
                     _Out_ long *ret);
    // 根据指定的多点查找颜色坐标
    void FindMultiColor(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *first_color,
                        _In_ const wchar_t *offset_color, _In_ double sim, _In_ long dir, _Out_ long *x, _Out_ long *y,
                        _Out_ long *ret);
    // 根据指定的多点查找所有颜色坐标
    void FindMultiColorEx(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *first_color,
                          _In_ const wchar_t *offset_color, _In_ double sim, _In_ long dir, _Out_ std::wstring &retstr);
    // 查找指定区域内的图片
    void FindPic(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *files,
                 _In_ const wchar_t *delta_color, _In_ double sim, _In_ long dir, _Out_ long *x, _Out_ long *y,
                 _Out_ long *ret);
    // 查找多个图片
    void FindPicEx(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *files,
                   _In_ const wchar_t *delta_color, _In_ double sim, _In_ long dir, _Out_ std::wstring &retstr);
    //
    // 这个函数可以查找多个图片, 并且返回所有找到的图像的坐标.此函数同FindPicEx.只是返回值不同.(file1,x,y|file2,x,y|...)
    void FindPicExS(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *files,
                    _In_ const wchar_t *delta_color, _In_ double sim, _In_ long dir, _Out_ std::wstring &retstr);
    // 查找指定区域内的颜色块,颜色格式"RRGGBB-DRDGDB",注意,和按键的颜色格式相反
    void FindColorBlock(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color,
                        _In_ double sim, _In_ long count, _In_ long height, _In_ long width, _Out_ long *x, _Out_ long *y,
                        _Out_ long *ret);
    // 查找指定区域内的所有颜色块, 颜色格式"RRGGBB-DRDGDB", 注意, 和按键的颜色格式相反
    void FindColorBlockEx(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color,
                          _In_ double sim, _In_ long count, _In_ long height, _In_ long width, _Out_ std::wstring &retstr);
    // 获取(x,y)的颜色
    void GetColor(_In_ long x, _In_ long y, _Out_ std::wstring &ret);
    //
    // 设置图像输入方式，默认窗口截图
    void SetDisplayInput(_In_ const wchar_t *mode, _Out_ long *ret);

    void LoadPic(_In_ const wchar_t *file_name, _Out_ long *ret);

    void FreePic(_In_ const wchar_t *file_name, _Out_ long *ret);
    // 从内存加载要查找的图片
    void LoadMemPic(_In_ const wchar_t *file_name, _In_ void *data, _In_ long size, _Out_ long *ret);
    // 获取指定图片的尺寸，如果指定的图片已经被加入缓存，则从缓存中获取信息.此接口也会把此图片加入缓存
    void GetPicSize(_In_ const wchar_t *pic_name, _Out_ long *width, _Out_ long *height, _Out_ long *ret);
    //
    void GetScreenData(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _Out_ size_t *data, _Out_ long *ret);
    //
    void GetScreenDataBmp(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _Out_ size_t *data, _Out_ long *size,
                          _Out_ long *ret);
    //
    void GetScreenFrameInfo(_Out_ long *frame_id, _Out_ long *time);
    //
    void MatchPicName(_In_ const wchar_t *pic_name, _Out_ std::wstring &retstr);
    //----------------------opcv-------------------------
    void CvLoadTemplate(_In_ const wchar_t *name, _In_ const wchar_t *file_path, _Out_ long *ret);
    void CvLoadMaskedTemplate(_In_ const wchar_t *name, _In_ const wchar_t *template_path, _In_ const wchar_t *mask_path,
                              _Out_ long *ret);
    void CvRemoveTemplate(_In_ const wchar_t *name, _Out_ long *ret);
    void CvRemoveAllTemplates(_Out_ long *ret);
    void CvHasTemplate(_In_ const wchar_t *name, _Out_ long *ret);
    void CvGetTemplateCount(_Out_ long *ret);
    void CvGetAllTemplateNames(_Out_ std::wstring &retstr);
    void CvGetOpenCvVersion(_Out_ std::wstring &retstr);
    void CvLoadTemplateList(_In_ const wchar_t *template_list, _Out_ long *ret);
    void CvToGray(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _Out_ long *ret);
    void CvToBinary(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _Out_ long *ret);
    void CvToEdge(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _Out_ long *ret);
    void CvToOutline(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _Out_ long *ret);
    void CvDenoise(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _Out_ long *ret);
    void CvEqualize(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _Out_ long *ret);
    void CvCLAHE(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _In_ double clip_limit, _In_ long tile_grid_size,
                 _Out_ long *ret);
    void CvBlur(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _In_ const wchar_t *mode, _In_ long kernel_size,
               _Out_ long *ret);
    void CvSharpen(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _In_ double strength, _Out_ long *ret);
    void CvCropValid(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _Out_ long *ret);
    void CvConnectedComponents(_In_ const wchar_t *src_file, _In_ double min_area, _Out_ std::wstring &retjson,
                               _Out_ long *ret);
    void CvFindContours(_In_ const wchar_t *src_file, _In_ double min_area, _Out_ std::wstring &retjson, _Out_ long *ret);
    void CvPreprocessPipeline(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _In_ const wchar_t *pipeline,
                              _Out_ long *ret);
    void CvCrop(_In_ const wchar_t *src_file, _In_ long x, _In_ long y, _In_ long width, _In_ long height,
                _In_ const wchar_t *dst_file, _Out_ long *ret);
    void CvResize(_In_ const wchar_t *src_file, _In_ long width, _In_ long height, _In_ const wchar_t *dst_file,
                  _Out_ long *ret);
    void CvThreshold(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _In_ double threshold, _In_ double max_value,
                     _In_ const wchar_t *mode, _Out_ long *ret);
    void CvInRange(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _In_ const wchar_t *color_space,
                   _In_ const wchar_t *lower, _In_ const wchar_t *upper, _Out_ long *ret);
    void CvMorphology(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _In_ const wchar_t *mode,
                      _In_ long kernel_size, _In_ long iterations, _Out_ long *ret);
    void CvThin(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _In_ const wchar_t *mode, _Out_ long *ret);
    void CvMatchTemplate(_In_ long x, _In_ long y, _In_ long width, _In_ long height, _In_ const wchar_t *template_name,
                         _In_ double threshold, _In_ long dir, _In_ long strip_mode, _In_ long method, _In_ long color_mode,
                         _Out_ std::wstring &retjson, _Out_ long *ret);
    void CvMatchTemplateScale(_In_ long x, _In_ long y, _In_ long width, _In_ long height, _In_ const wchar_t *template_name,
                              _In_ const wchar_t *scales, _In_ double threshold, _In_ long method, _In_ long color_mode,
                              _Out_ std::wstring &retjson, _Out_ long *ret);
    void CvMatchAnyTemplate(_In_ long x, _In_ long y, _In_ long width, _In_ long height, _In_ const wchar_t *template_names,
                            _In_ double threshold, _In_ long dir, _In_ long strip_mode, _In_ long method, _In_ long color_mode,
                            _Out_ std::wstring &retjson, _Out_ long *ret);
    void CvMatchAllTemplates(_In_ long x, _In_ long y, _In_ long width, _In_ long height, _In_ const wchar_t *template_names,
                             _In_ double threshold, _In_ long dir, _In_ long strip_mode, _In_ long method, _In_ long color_mode,
                             _Out_ std::wstring &retjson, _Out_ long *ret);
    void CvFeatureMatchTemplate(_In_ long x, _In_ long y, _In_ long width, _In_ long height, _In_ const wchar_t *template_name,
                                _In_ double threshold, _Out_ std::wstring &retjson, _Out_ long *ret);
    void CvEdgeMatchTemplate(_In_ long x, _In_ long y, _In_ long width, _In_ long height, _In_ const wchar_t *template_name,
                             _In_ double threshold, _Out_ std::wstring &retjson, _Out_ long *ret);
    void CvShapeMatchTemplate(_In_ long x, _In_ long y, _In_ long width, _In_ long height, _In_ const wchar_t *template_name,
                              _In_ double threshold, _Out_ std::wstring &retjson, _Out_ long *ret);
    //----------------------ocr-------------------------
    long SetOcrEngine(_In_ const wchar_t *path_of_engine, _In_ const wchar_t *dll_name, _In_ const wchar_t *argv);
    long SetYoloEngine(_In_ const wchar_t *path_of_engine, _In_ const wchar_t *dll_name, _In_ const wchar_t *argv);
    void YoloDetect(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ double conf, _In_ double iou,
                    _Out_ std::wstring &retjson, _Out_ long *ret);
    void YoloDetectFromFile(_In_ const wchar_t *file_name, _In_ double conf, _In_ double iou, _Out_ std::wstring &retjson,
                            _Out_ long *ret);
    // 设置字库文件
    void SetDict(_In_ long idx, _In_ const wchar_t *file_name, _Out_ long *ret);
    // 获取指定字库中指定条目的字库信息
    void GetDict(_In_ long idx, _In_ long font_index, _Out_ std::wstring &retstr);
    // 设置内存字库文件
    void SetMemDict(_In_ long idx, _In_ const wchar_t *data, _In_ long size, _Out_ long *ret);
    // 使用哪个字库文件进行识别
    void UseDict(_In_ long idx, _Out_ long *ret);
    // 给指定的字库中添加一条字库信息
    void AddDict(_In_ long idx, _In_ const wchar_t *dict_info, _Out_ long *ret);
    // 保存指定的字库到指定的文件中
    void SaveDict(_In_ long idx, _In_ const wchar_t *file_name, _Out_ long *ret);
    // 清空指定的字库
    void ClearDict(_In_ long idx, _Out_ long *ret);
    // 获取指定的字库中的字符数量
    void GetDictCount(_In_ long idx, _Out_ long *ret);
    // 获取当前使用的字库序号
    void GetNowDict(_Out_ long *ret);
    // 根据指定的范围,以及指定的颜色描述，提取点阵信息，类似于大漠工具里的单独提取
    void FetchWord(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ const wchar_t *word,
                   _Out_ std::wstring &retstr);
    // 识别这个范围内所有满足条件的词组，这个识别函数不会用到字库. 只是识别大概形状的位置
    void GetWordsNoDict(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color,
                        _Out_ std::wstring &retstr);
    // 在使用GetWords进行词组识别以后,可以用此接口进行识别词组数量的计算
    void GetWordResultCount(_In_ const wchar_t *result, _Out_ long *ret);
    // 在使用GetWords进行词组识别以后,可以用此接口进行识别各个词组的坐标
    void GetWordResultPos(_In_ const wchar_t *result, _In_ long index, _Out_ long *x, _Out_ long *y, _Out_ long *ret);
    // 在使用GetWords进行词组识别以后,可以用此接口进行识别各个词组的内容
    void GetWordResultStr(_In_ const wchar_t *result, _In_ long index, _Out_ std::wstring &ret_str);
    // 识别屏幕范围(x1,y1,x2,y2)内符合color_format的字符串,并且相似度为sim,sim取值范围(0.1-1.0),
    void Ocr(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim,
             _Out_ std::wstring &ret_str);
    // 回识别到的字符串，以及每个字符的坐标.
    void OcrEx(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim,
               _Out_ std::wstring &ret_str);
    // 在屏幕范围(x1,y1,x2,y2)内,查找string(可以是任意个字符串的组合),并返回符合color_format的坐标位置
    void FindStr(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *strs, _In_ const wchar_t *color,
                 _In_ double sim, _Out_ long *retx, _Out_ long *rety, _Out_ long *ret);
    // 返回符合color_format的所有坐标位置
    void FindStrEx(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *strs, _In_ const wchar_t *color,
                   _In_ double sim, _Out_ std::wstring &retstr);
    // 识别屏幕范围(x1,y1,x2,y2)内的字符串,自动二值化，而无需指定颜色
    void OcrAuto(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ double sim, _Out_ std::wstring &ret_str);
    // 从文件中识别图片
    void OcrFromFile(_In_ const wchar_t *file_name, _In_ const wchar_t *color_format, _In_ double sim, _Out_ std::wstring &retstr);
    // 从文件中识别图片,无需指定颜色
    void OcrAutoFromFile(_In_ const wchar_t *file_name, _In_ double sim, _Out_ std::wstring &retstr);
    // 查找频幕中的直线
    void FindLine(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim,
                  _Out_ std::wstring &retstr);

    // 向某进程写入数据
    void WriteData(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _In_ const wchar_t *data, _In_ long size, _Out_ long *ret);
    // 读取数据
    void ReadData(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _In_ long size, _Out_ std::wstring &retstr);
    // 大漠兼容内存接口
    void ReadInt(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _In_ long type, _Out_ int64_t *ret);
    void WriteInt(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _In_ long type, _In_ int64_t value, _Out_ long *ret);
    void ReadFloat(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _Out_ float *ret);
    void WriteFloat(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _In_ float value, _Out_ long *ret);
    void ReadDouble(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _Out_ double *ret);
    void WriteDouble(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _In_ double value, _Out_ long *ret);
    void ReadString(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _In_ long type, _In_ long len, _Out_ std::wstring &retstr);
    void WriteString(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _In_ long type, _In_ const wchar_t *value, _Out_ long *ret);
};

} // namespace op

#ifdef _MSC_VER
#pragma warning(pop)
#endif
