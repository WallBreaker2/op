# 鼠标与键盘

`op::Op` 的这一组接口共 **45** 个。返回索引：[doc/api.md](../api.md)

> 通用约定（`long *ret` 的含义、坐标语义、线程安全、C API 命名映射）见 [索引页](../api.md#通用约定)。

## mouse & keyboard（45）

- **`void GetCursorPos(_Out_ long *x, _Out_ long *y, _Out_ long *ret)`** → `void`
  获取鼠标位置.

- **`void GetCursorShape(_Out_ std::wstring &ret)`** → `void`
  获取当前鼠标形状: visible,hash,width,height,hotX,hotY.

- **`void MoveR(_In_ long x, _In_ long y, _Out_ long *ret)`** → `void`
  鼠标相对于上次的位置移动rx,ry.

- **`void MoveTo(_In_ long x, _In_ long y, _Out_ long *ret)`** → `void`
  把鼠标移动到目的点(x,y)

- **`void MoveToEx(_In_ long x, _In_ long y, _In_ long w, _In_ long h, _Out_ std::wstring &ret)`** → `void`
  把鼠标移动到目的范围内的任意一点

- **`void MoveToSmooth(_In_ long x, _In_ long y, _In_ long duration, _Out_ long *ret)`** → `void`
  按模拟轨迹移动到指定点

- **`void MoveToExSmooth(_In_ long x, _In_ long y, _In_ long w, _In_ long h, _In_ long duration, _Out_ std::wstring &ret)`** → `void`
  在目标范围内随机取点，并按模拟轨迹移动过去

- **`void MovePath(_In_ const wchar_t *path, _In_ long duration, _Out_ long *ret)`** → `void`
  按 x,y|x,y 格式提供的路径移动

- **`void DragPath(_In_ const wchar_t *path, _In_ long duration, _Out_ long *ret)`** → `void`
  按 x,y|x,y 格式提供的路径拖拽

- **`void SetMouseTrajectory(_In_ long mode, _In_ long min_duration, _In_ long max_duration, _In_ long jitter, _In_ long start_delay, _In_ long end_delay, _Out_ long *ret)`** → `void`
  设置模拟轨迹参数，只影响 Smooth 和 Path 系列鼠标移动

- **`void LeftClick(_Out_ long *ret)`** → `void`
  按下鼠标左键

- **`void LeftDoubleClick(_Out_ long *ret)`** → `void`
  双击鼠标左键

- **`void LeftDown(_Out_ long *ret)`** → `void`
  按住鼠标左键

- **`void LeftUp(_Out_ long *ret)`** → `void`
  弹起鼠标左键

- **`void MiddleClick(long *ret)`** → `void`
  按下鼠标中键

- **`void MiddleDoubleClick(long *ret)`** → `void`
  双击鼠标中键

- **`void MiddleDown(_Out_ long *ret)`** → `void`
  按住鼠标中键

- **`void MiddleUp(_Out_ long *ret)`** → `void`
  弹起鼠标中键

- **`void RightClick(long *ret)`** → `void`
  按下鼠标右键

- **`void RightDoubleClick(long *ret)`** → `void`
  双击鼠标右键

- **`void RightDown(_Out_ long *ret)`** → `void`
  按住鼠标右键

- **`void RightUp(long *ret)`** → `void`
  弹起鼠标右键

- **`void XButton1Click(long *ret)`** → `void`
  按下鼠标侧键1

- **`void XButton1DoubleClick(long *ret)`** → `void`
  双击鼠标侧键1

- **`void XButton1Down(long *ret)`** → `void`
  按住鼠标侧键1

- **`void XButton1Up(long *ret)`** → `void`
  弹起鼠标侧键1

- **`void XButton2Click(long *ret)`** → `void`
  按下鼠标侧键2

- **`void XButton2DoubleClick(long *ret)`** → `void`
  双击鼠标侧键2

- **`void XButton2Down(long *ret)`** → `void`
  按住鼠标侧键2

- **`void XButton2Up(long *ret)`** → `void`
  弹起鼠标侧键2

- **`void Wheel(long delta, long *ret)`** → `void`
  垂直滚轮滚动指定 delta

- **`void HWheel(long delta, long *ret)`** → `void`
  水平滚轮滚动指定 delta

- **`void WheelDown(_Out_ long *ret)`** → `void`
  滚轮向下滚

- **`void WheelUp(_Out_ long *ret)`** → `void`
  滚轮向上滚

- **`void SetMouseDelay(_In_ const wchar_t *type, _In_ long delay, _Out_ long *ret)`** → `void`
  设置鼠标单击或者双击时,鼠标按下和弹起的时间间隔

- **`void GetKeyState(_In_ long vk_code, _Out_ long *ret)`** → `void`
  获取指定的按键状态.(前台信息,不是后台)

- **`void KeyDown(_In_ long vk_code, _Out_ long *ret)`** → `void`
  按住指定的虚拟键码

- **`void KeyDownChar(_In_ const wchar_t *vk_code, _Out_ long *ret)`** → `void`
  按住指定的虚拟键码

- **`void KeyUp(_In_ long vk_code, _Out_ long *ret)`** → `void`
  弹起来虚拟键vk_code

- **`void KeyUpChar(_In_ const wchar_t *vk_code, _Out_ long *ret)`** → `void`
  弹起来虚拟键vk_code

- **`void WaitKey(_In_ long vk_code, _In_ long time_out, _Out_ long *ret)`** → `void`
  等待指定的按键按下 (前台,不是后台)

- **`void KeyPress(_In_ long vk_code, _Out_ long *ret)`** → `void`
  发送字符串 long SendString(long HWND) 弹起来虚拟键vk_code

- **`void KeyPressChar(_In_ const wchar_t *vk_code, _Out_ long *ret)`** → `void`
  按字符码按下并释放一个键。`vk_code` 传虚拟键码的十进制字符串（如 `L"65"` 即 `A`）；要输入整串文本用 `SendString`。

- **`void SetKeypadDelay(_In_ const wchar_t *type, _In_ long delay, _Out_ long *ret)`** → `void`
  设置按键时,键盘按下和弹起的时间间隔

- **`void KeyPressStr(_In_ const wchar_t *key_str, _In_ long delay, _Out_ long *ret)`** → `void`
  根据指定的字符串序列，依次按顺序按下其中的字符

