# 运行时与算法

`op::Op` 的这一组接口共 **14** 个。返回索引：[doc/api.md](../api.md)

> 通用约定（`long *ret` 的含义、坐标语义、线程安全、C API 命名映射）见 [索引页](../api.md#通用约定)。

## 基本设置/属性（12）

- **`std::wstring Ver()`** → `std::wstring`
  1.版本号Version

- **`void SetPath(_In_ const wchar_t *path, _Out_ long *ret)`** → `void`
  设置目录

- **`void GetPath(_Out_ std::wstring &ret)`** → `void`
  获取目录

- **`void GetBasePath(_Out_ std::wstring &ret)`** → `void`
  获取插件目录

- **`void GetID(_Out_ long *ret)`** → `void`
  返回当前对象的ID值，这个值对于每个对象是唯一存在的。可以用来判定两个对象是否一致

- **`void GetLastError(_Out_ long *ret)`** → `void`
  取最近一次内部错误的描述文本。与 Win32 的 `::GetLastError` 无关：两者同名，头文件里已 `#undef` 相关宏；本方法返回的是插件自己的错误原因。

- **`void SetShowErrorMsg(_In_ long show_type, _Out_ long *ret)`** → `void`
  设置是否弹出错误信息,默认是打开 0:关闭，1:显示为信息框，2:保存到文件,3:输出到标准输出

- **`void Sleep(_In_ long millseconds, _Out_ long *ret)`** → `void`
  sleep

- **`void InjectDll(_In_ const wchar_t *process_name, _In_ const wchar_t *dll_name, _Out_ long *ret)`** → `void`
  Process inject dll

- **`void EnablePicCache(_In_ long enable, _Out_ long *ret)`** → `void`
  设置是否开启或者关闭插件内部的图片缓存机制

- **`void CapturePre(_In_ const wchar_t *file_name, _Out_ long *ret)`** → `void`
  取上次操作的图色区域，保存为file(24位位图)

- **`void SetScreenDataMode(_In_ long mode, _Out_ long *ret)`** → `void`
  设置屏幕数据模式，0:从上到下(默认),1:从下到上

## algorithm（2）

- **`void AStarFindPath(_In_ long mapWidth, _In_ long mapHeight, _In_ const wchar_t *disable_points, _In_ long beginX, _In_ long beginY, _In_ long endX, _In_ long endY, _Out_ std::wstring &ret)`** → `void`
  A星算法

- **`void FindNearestPos(_In_ const wchar_t *all_pos, _In_ long type, _In_ long x, _In_ long y, _Out_ std::wstring &ret)`** → `void`
  从坐标列表中查找距离指定坐标最近的点

