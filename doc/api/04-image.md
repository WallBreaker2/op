# 图像与颜色

`op::Op` 的这一组接口共 **22** 个。返回索引：[doc/api.md](../api.md)

> 通用约定（`long *ret` 的含义、坐标语义、线程安全、C API 命名映射）见 [索引页](../api.md#通用约定)。

## image and color（22）

- **`void Capture(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *file_name, _Out_ long *ret)`** → `void`
  抓取指定区域(x1, y1, x2, y2)的图像, 保存为file。 x2/y2 是右下角开区间: 实际宽 = x2-x1、高 = y2-y1, 所以 Capture(0, 0, 319, 239) 存下来的是 319x239 的图。

- **`void CmpColor(_In_ long x, _In_ long y, _In_ const wchar_t *color, _In_ double sim, _Out_ long *ret)`** → `void`
  比较指定坐标点(x,y)的颜色

- **`void FindColor(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim, _In_ long dir, _Out_ long *x, _Out_ long *y, _Out_ long *ret)`** → `void`
  查找指定区域内的颜色

- **`void FindColorEx(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim, _In_ long dir, _Out_ std::wstring &retstr)`** → `void`
  查找指定区域内的所有颜色

- **`void GetColorNum(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim, _Out_ long *ret)`** → `void`
  查找指定区域内的所有颜色数量

- **`void FindMultiColor(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *first_color, _In_ const wchar_t *offset_color, _In_ double sim, _In_ long dir, _Out_ long *x, _Out_ long *y, _Out_ long *ret)`** → `void`
  根据指定的多点查找颜色坐标

- **`void FindMultiColorEx(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *first_color, _In_ const wchar_t *offset_color, _In_ double sim, _In_ long dir, _Out_ std::wstring &retstr)`** → `void`
  根据指定的多点查找所有颜色坐标

- **`void FindPic(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *files, _In_ const wchar_t *delta_color, _In_ double sim, _In_ long dir, _Out_ long *x, _Out_ long *y, _Out_ long *ret)`** → `void`
  查找指定区域内的图片

- **`void FindPicEx(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *files, _In_ const wchar_t *delta_color, _In_ double sim, _In_ long dir, _Out_ std::wstring &retstr)`** → `void`
  查找多个图片

- **`void FindPicExS(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *files, _In_ const wchar_t *delta_color, _In_ double sim, _In_ long dir, _Out_ std::wstring &retstr)`** → `void`
  这个函数可以查找多个图片, 并且返回所有找到的图像的坐标.此函数同FindPicEx.只是返回值不同.(file1,x,y|file2,x,y|...)

- **`void FindColorBlock(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim, _In_ long count, _In_ long height, _In_ long width, _Out_ long *x, _Out_ long *y, _Out_ long *ret)`** → `void`
  查找指定区域内的颜色块,颜色格式"RRGGBB-DRDGDB",注意,和按键的颜色格式相反

- **`void FindColorBlockEx(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim, _In_ long count, _In_ long height, _In_ long width, _Out_ std::wstring &retstr)`** → `void`
  查找指定区域内的所有颜色块, 颜色格式"RRGGBB-DRDGDB", 注意, 和按键的颜色格式相反

- **`void GetColor(_In_ long x, _In_ long y, _Out_ std::wstring &ret)`** → `void`
  获取(x,y)的颜色

- **`void SetDisplayInput(_In_ const wchar_t *mode, _Out_ long *ret)`** → `void`
  设置图像输入方式，默认窗口截图

- **`void LoadPic(_In_ const wchar_t *file_name, _Out_ long *ret)`** → `void`
  把图片文件预加载进插件内部图片缓存，后续 `FindPic` 按名字命中缓存、不再读盘。

- **`void FreePic(_In_ const wchar_t *file_name, _Out_ long *ret)`** → `void`
  释放 `LoadPic` 预加载的缓存项。不调用也不影响正确性，只是占内存。

- **`void LoadMemPic(_In_ const wchar_t *file_name, _In_ void *data, _In_ long size, _Out_ long *ret)`** → `void`
  从内存加载要查找的图片

- **`void GetPicSize(_In_ const wchar_t *pic_name, _Out_ long *width, _Out_ long *height, _Out_ long *ret)`** → `void`
  获取指定图片的尺寸，如果指定的图片已经被加入缓存，则从缓存中获取信息.此接口也会把此图片加入缓存

- **`void GetScreenData(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _Out_ size_t *data, _Out_ long *ret)`** → `void`
  取指定区域的**原始像素**（BGRA 紧凑排列），不是 BMP。出参 `data` 指向内部复用缓冲，**下一次调用同一函数就会被覆盖**。

- **`void GetScreenDataBmp(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _Out_ size_t *data, _Out_ long *size, _Out_ long *ret)`** → `void`
  取指定区域的画面并编码成 24 位 BMP 字节流。出参 `data` 同样指向内部复用缓冲。

- **`void GetScreenFrameInfo(_Out_ long *frame_id, _Out_ long *time)`** → `void`
  取当前帧的序号与时间戳（毫秒）。用于判断“画面有没有变”，比逐像素比较便宜。

- **`void MatchPicName(_In_ const wchar_t *pic_name, _Out_ std::wstring &retstr)`** → `void`
  按 BMP 命名规则补全图片名（相对 `SetPath` 设置的目录），返回实际要打开的文件名。

