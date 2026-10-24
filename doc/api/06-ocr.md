# OCR 与 YOLO

`op::Op` 的这一组接口共 **51** 个。返回索引：[doc/api.md](../api.md)

> 通用约定（`long *ret` 的含义、坐标语义、线程安全、C API 命名映射）见 [索引页](../api.md#通用约定)。

## ocr（51）

- **`long SetOcrEngine(_In_ const wchar_t *path_of_engine, _In_ const wchar_t *dll_name, _In_ const wchar_t *argv)`** → `long`
  选择 OCR 引擎（本地字典 or HTTP 服务）。不设置时用默认策略：有字典走本地，否则走 HTTP。

- **`long SetYoloEngine(_In_ const wchar_t *path_of_engine, _In_ const wchar_t *dll_name, _In_ const wchar_t *argv)`** → `long`
  设置 YOLO 检测服务的端点，形如 `http://127.0.0.1:8090/api/v1/detect`。

- **`void YoloDetect(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ double conf, _In_ double iou, _Out_ std::wstring &retjson, _Out_ long *ret)`** → `void`
  在**当前捕获图**的 `x1,y1,x2,y2` 区域内做目标检测，`conf` 是置信度门限、`iou` 是 NMS 的 IoU 门限，出参是 JSON。

- **`void YoloDetectFromFile(_In_ const wchar_t *file_name, _In_ double conf, _In_ double iou, _Out_ std::wstring &retjson, _Out_ long *ret)`** → `void`
  对指定图片文件做目标检测，参数与出参同 `YoloDetect`。

- **`void SetDict(_In_ long idx, _In_ const wchar_t *file_name, _Out_ long *ret)`** → `void`
  设置字库文件

- **`void GetDict(_In_ long idx, _In_ long font_index, _Out_ std::wstring &retstr)`** → `void`
  获取指定字库中指定条目的字库信息

- **`void SetMemDict(_In_ long idx, _In_reads_bytes_(size) const void *data, _In_ long size, _Out_ long *ret)`** → `void`
  设置内存字库文件

- **`void UseDict(_In_ long idx, _Out_ long *ret)`** → `void`
  使用哪个字库文件进行识别

- **`void AddDict(_In_ long idx, _In_ const wchar_t *dict_info, _Out_ long *ret)`** → `void`
  给指定的字库中添加一条字库信息

- **`void SaveDict(_In_ long idx, _In_ const wchar_t *file_name, _Out_ long *ret)`** → `void`
  保存指定的字库到指定的文件中

- **`void ClearDict(_In_ long idx, _Out_ long *ret)`** → `void`
  清空指定的字库

- **`void GetDictCount(_In_ long idx, _Out_ long *ret)`** → `void`
  获取指定的字库中的字符数量

- **`void GetNowDict(_Out_ long *ret)`** → `void`
  获取当前使用的字库序号

- **`void SetBinaryPreprocess(_In_ long mode, _In_ long isolated_threshold, _In_ long min_component_area, _In_ long bridge_gap, _Out_ long *ret)`** → `void`
  设置点阵二值图预处理。mode: 0关闭, 1去孤立点, 2再过滤小连通域, 3允许按 bridge_gap 补1像素断笔

- **`void GetBinaryPreprocess(_Out_ long *mode, _Out_ long *isolated_threshold, _Out_ long *min_component_area, _Out_ long *bridge_gap, _Out_ long *ret)`** → `void`
  读取当前点阵二值图预处理参数

- **`void FetchWord(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ const wchar_t *word, _Out_ std::wstring &retstr)`** → `void`
  根据指定的范围,以及指定的颜色描述，提取点阵信息，类似于大漠工具里的单独提取

- **`void FetchWordEx(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim, _In_ const wchar_t *word, _Out_ std::wstring &retstr)`** → `void`
  根据相似度提取单个点阵字，适合有抗锯齿或轻微色差的文字

- **`void ExtractWordRects(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim, _In_ long min_word_h, _Out_ std::wstring &retstr)`** → `void`
  自动切出指定范围内的点阵字块，返回格式: x1,y1,x2,y2|...

- **`void ExtractWordRectsEx(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim, _In_ long min_word_w, _In_ long min_word_h, _In_ long padding, _Out_ std::wstring &retstr)`** → `void`
  自动切出点阵字块，并按最小宽高过滤噪点；padding 用来给每个字块保留一点边缘

- **`void FetchWords(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim, _In_ const wchar_t *words, _In_ long min_word_h, _Out_ std::wstring &retstr)`** → `void`
  自动切字并按 words 的字符顺序生成多条点阵字库，返回多行字库文本

- **`void FetchWordsEx(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim, _In_ const wchar_t *words, _In_ long min_word_w, _In_ long min_word_h, _In_ long padding, _Out_ std::wstring &retstr)`** → `void`
  使用更细的切字参数批量生成点阵字库，适合截图里夹杂小噪点的情况

- **`void FetchWordsByRects(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim, _In_ const wchar_t *words, _In_ const wchar_t *rects, _Out_ std::wstring &retstr)`** → `void`
  按指定字块生成多条点阵字库，rects 格式与 ExtractWordRects 的返回值一致

- **`void GetBinaryPreview(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim, _Out_ std::wstring &retstr, _Out_ long *ret)`** → `void`
  返回指定区域按颜色规则二值化后的文本预览，ret 为前景点数量

- **`void GetWordPreview(_In_ const wchar_t *dict_info, _Out_ std::wstring &retstr, _Out_ long *ret)`** → `void`
  返回点阵字库条目的文本预览，ret 为条目是否合法

- **`void CheckWordDict(_In_ const wchar_t *dict_info, _Out_ std::wstring &retstr, _Out_ long *ret)`** → `void`
  检查一段点阵字库文本，ret 为合法条目数量，retstr 为每条字库的摘要

- **`void NormalizeWordDict(_In_ const wchar_t *dict_info, _Out_ std::wstring &retstr, _Out_ long *ret)`** → `void`
  清理点阵字库文本，只保留合法条目，并统一输出为当前字库格式

- **`void RenameWordDict(_In_ const wchar_t *dict_info, _In_ const wchar_t *words, _Out_ std::wstring &retstr, _Out_ long *ret)`** → `void`
  按顺序替换点阵字库条目的字名，适合制作后统一校正标签

- **`void GetWordsNoDict(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _Out_ std::wstring &retstr)`** → `void`
  识别这个范围内所有满足条件的词组，这个识别函数不会用到字库. 只是识别大概形状的位置

- **`void GetWordResultCount(_In_ const wchar_t *result, _Out_ long *ret)`** → `void`
  在使用GetWords进行词组识别以后,可以用此接口进行识别词组数量的计算

- **`void GetWordResultPos(_In_ const wchar_t *result, _In_ long index, _Out_ long *x, _Out_ long *y, _Out_ long *ret)`** → `void`
  在使用GetWords进行词组识别以后,可以用此接口进行识别各个词组的坐标

- **`void GetWordResultStr(_In_ const wchar_t *result, _In_ long index, _Out_ std::wstring &ret_str)`** → `void`
  在使用GetWords进行词组识别以后,可以用此接口进行识别各个词组的内容

- **`void Ocr(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim, _Out_ std::wstring &ret_str)`** → `void`
  识别屏幕范围(x1,y1,x2,y2)内符合color_format的字符串,并且相似度为sim,sim取值范围(0.1-1.0),

- **`void OcrEx(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim, _Out_ std::wstring &ret_str)`** → `void`
  回识别到的字符串，以及每个字符的坐标.

- **`void FindStr(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *strs, _In_ const wchar_t *color, _In_ double sim, _Out_ long *retx, _Out_ long *rety, _Out_ long *ret)`** → `void`
  在屏幕范围(x1,y1,x2,y2)内,查找string(可以是任意个字符串的组合),并返回符合color_format的坐标位置

- **`void FindStrEx(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *strs, _In_ const wchar_t *color, _In_ double sim, _Out_ std::wstring &retstr)`** → `void`
  返回符合color_format的所有坐标位置

- **`void OcrAuto(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ double sim, _Out_ std::wstring &ret_str)`** → `void`
  识别屏幕范围(x1,y1,x2,y2)内的字符串,自动二值化，而无需指定颜色

- **`void OcrFromFile(_In_ const wchar_t *file_name, _In_ const wchar_t *color_format, _In_ double sim, _Out_ std::wstring &retstr)`** → `void`
  从文件中识别图片

- **`void OcrAutoFromFile(_In_ const wchar_t *file_name, _In_ double sim, _Out_ std::wstring &retstr)`** → `void`
  从文件中识别图片,无需指定颜色

- **`void FindLine(_In_ long x1, _In_ long y1, _In_ long x2, _In_ long y2, _In_ const wchar_t *color, _In_ double sim, _Out_ std::wstring &retstr)`** → `void`
  查找频幕中的直线

- **`void WriteData(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _In_ const wchar_t *data, _In_ long size, _Out_ long *ret)`** → `void`
  向某进程写入数据

- **`void ReadData(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _In_ long size, _Out_ std::wstring &retstr)`** → `void`
  读取数据

- **`void ReadInt(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _In_ long type, _Out_ int64_t *ret)`** → `void`
  大漠兼容内存接口

- **`void WriteInt(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _In_ long type, _In_ int64_t value, _Out_ long *ret)`** → `void`
  写整数。`type` 决定宽度与符号：`0`/`4` → 4 字节、`1`/`5` → 2 字节、`2`/`6` → 1 字节、`3` → 8 字节。

- **`void ReadIntBatch(_In_ LONG_PTR hwnd, _In_ const wchar_t *const *addresses, _In_ long count, _In_ long type, _Out_ int64_t *values, _Out_ long *ret, _Out_opt_ bool *ok = nullptr)`** → `void`
  批量形状：一次调用读一整组地址，省掉每条地址都要付的固定开销（进程上下文、字符串转换、异常帧）。 addresses 是 count 个以 0 结尾的地址表达式；values[i] 读失败写 0；ret 返回成功条数。 ok（可空）逐项标记成功与否：COM 层要靠它把失败项输出成空字段，否则 0 和"读失败"分不开。

- **`void WriteIntBatch(_In_ LONG_PTR hwnd, _In_ const wchar_t *const *addresses, _In_ long count, _In_ long type, _In_ const int64_t *values, _Out_ long *ret, _Out_opt_ bool *ok = nullptr)`** → `void`
  批量写整数。`addresses` 与 `values` 等长，`ok` 逐项标记成功与否——**单项失败不影响其它项**。

- **`void ReadFloat(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _Out_ float *ret)`** → `void`
  读一个 32 位浮点数。

- **`void WriteFloat(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _In_ float value, _Out_ long *ret)`** → `void`
  写一个 32 位浮点数。

- **`void ReadDouble(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _Out_ double *ret)`** → `void`
  读一个 64 位浮点数。

- **`void WriteDouble(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _In_ double value, _Out_ long *ret)`** → `void`
  写一个 64 位浮点数。

- **`void ReadString(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _In_ long type, _In_ long len, _Out_ std::wstring &retstr)`** → `void`
  读字符串。`type` 是历史 DM 约定：`0` = ACP/GBK、`1` = UTF-16、`2` = UTF-8；`len` 是**字节数**（UTF-16 时每字符 2 字节）。

- **`void WriteString(_In_ LONG_PTR hwnd, _In_ const wchar_t *address, _In_ long type, _In_ const wchar_t *value, _Out_ long *ret)`** → `void`
  写字符串，`type` 与 `len` 语义同 `ReadString`。

