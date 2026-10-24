# OpenCV

`op::Op` 的这一组接口共 **35** 个。返回索引：[doc/api.md](../api.md)

> 通用约定（`long *ret` 的含义、坐标语义、线程安全、C API 命名映射）见 [索引页](../api.md#通用约定)。

## opcv（35）

- **`void CvLoadTemplate(_In_ const wchar_t *name, _In_ const wchar_t *file_path, _Out_ long *ret)`** → `void`
  按名字加载一张模板图，之后 `CvMatch*` 用名字引用它。

- **`void CvLoadMaskedTemplate(_In_ const wchar_t *name, _In_ const wchar_t *template_path, _In_ const wchar_t *mask_path, _Out_ long *ret)`** → `void`
  加载带掩模的模板图：只有掩模非零的像素参与匹配，用于目标带透明背景或杂边的场景。

- **`void CvRemoveTemplate(_In_ const wchar_t *name, _Out_ long *ret)`** → `void`
  移除指定名字的模板。

- **`void CvRemoveAllTemplates(_Out_ long *ret)`** → `void`
  清空所有已加载的模板。

- **`void CvHasTemplate(_In_ const wchar_t *name, _Out_ long *ret)`** → `void`
  查询某个名字的模板是否已加载（`ret` 为 1 表示存在）。

- **`void CvGetTemplateCount(_Out_ long *ret)`** → `void`
  取当前已加载的模板数量。

- **`void CvGetAllTemplateNames(_Out_ std::wstring &retstr)`** → `void`
  取全部模板名，用 `|` 连接。

- **`void CvGetOpenCvVersion(_Out_ std::wstring &retstr)`** → `void`
  取底层 OpenCV 的版本字符串。

- **`void CvLoadTemplateList(_In_ const wchar_t *template_list, _Out_ long *ret)`** → `void`
  批量加载模板。清单格式是 `名字,路径|名字,路径`（逗号分隔名与路径，竖线分隔条目）。

- **`void CvToGray(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _Out_ long *ret)`** → `void`
  转灰度图。

- **`void CvToBinary(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _Out_ long *ret)`** → `void`
  按内部默认参数二值化（需要自定义阈值时用 `CvThreshold`）。

- **`void CvToEdge(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _Out_ long *ret)`** → `void`
  提取边缘（Canny）。

- **`void CvToOutline(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _Out_ long *ret)`** → `void`
  提取轮廓线。

- **`void CvDenoise(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _Out_ long *ret)`** → `void`
  去噪（中值/高斯滤波的组合）。

- **`void CvEqualize(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _Out_ long *ret)`** → `void`
  直方图均衡化，提升低对比度图的可辨识度。

- **`void CvCLAHE(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _In_ double clip_limit, _In_ long tile_grid_size, _Out_ long *ret)`** → `void`
  自适应直方图均衡（CLAHE）。`clip_limit` 控对比度上限，`tile_grid_size` 是分块大小。

- **`void CvBlur(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _In_ const wchar_t *mode, _In_ long kernel_size, _Out_ long *ret)`** → `void`
  模糊。`mode` 选模糊类型，`kernel_size` 是核尺寸（必须为正奇数）。

- **`void CvSharpen(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _In_ double strength, _Out_ long *ret)`** → `void`
  锐化，`strength` 控制强度。

- **`void CvCropValid(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _Out_ long *ret)`** → `void`
  自动裁掉四周的纯色边框，只留有内容的部分。

- **`void CvConnectedComponents(_In_ const wchar_t *src_file, _In_ double min_area, _Out_ std::wstring &retjson, _Out_ long *ret)`** → `void`
  连通域统计，出参是 JSON：每个连通域的位置、面积；`min_area` 以下的会被丢掉。

- **`void CvFindContours(_In_ const wchar_t *src_file, _In_ double min_area, _Out_ std::wstring &retjson, _Out_ long *ret)`** → `void`
  查找轮廓，出参 JSON 里每个轮廓带外接矩形与面积；`min_area` 以下丢弃。

- **`void CvPreprocessPipeline(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _In_ const wchar_t *pipeline, _Out_ long *ret)`** → `void`
  按给定步骤串联做图像预处理。`pipeline` 形如 `gray|threshold,120|morphology,open`，每步只把当前图像交给下一步。

- **`void CvCrop(_In_ const wchar_t *src_file, _In_ long x, _In_ long y, _In_ long width, _In_ long height, _In_ const wchar_t *dst_file, _Out_ long *ret)`** → `void`
  按矩形裁剪。

- **`void CvResize(_In_ const wchar_t *src_file, _In_ long width, _In_ long height, _In_ const wchar_t *dst_file, _Out_ long *ret)`** → `void`
  缩放到指定宽高。

- **`void CvThreshold(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _In_ double threshold, _In_ double max_value, _In_ const wchar_t *mode, _Out_ long *ret)`** → `void`
  二值化。`threshold` 是阈值、`max_value` 是超过阈值时的赋值，`type` 对应 OpenCV 的 `THRESH_*`。

- **`void CvInRange(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _In_ const wchar_t *color_space, _In_ const wchar_t *lower, _In_ const wchar_t *upper, _Out_ long *ret)`** → `void`
  按颜色区间做掩模。`color_space` 选色彩空间（如 `hsv`/`bgr`），上下界是 `min|max` 形式的字符串。

- **`void CvMorphology(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _In_ const wchar_t *mode, _In_ long kernel_size, _In_ long iterations, _Out_ long *ret)`** → `void`
  形态学运算。`mode` 取 `erode`/`dilate`/`open`/`close`，`kernel_size` 是结构元尺寸。

- **`void CvThin(_In_ const wchar_t *src_file, _In_ const wchar_t *dst_file, _In_ const wchar_t *mode, _Out_ long *ret)`** → `void`
  细化（骨架化），`mode` 选择细化算法。

- **`void CvMatchTemplate(_In_ long x, _In_ long y, _In_ long width, _In_ long height, _In_ const wchar_t *template_name, _In_ double threshold, _In_ long dir, _In_ long strip_mode, _In_ long method, _In_ long color_mode, _Out_ std::wstring &retjson, _Out_ long *ret)`** → `void`
  在**当前捕获图**的 `x,y,width,height` 区域内匹配单模板。出参 JSON：`{"ok":1,"x":..,"y":..,"width":..,"height":..,"score":..}`，失败是 `{"ok":0}`。

- **`void CvMatchTemplateScale(_In_ long x, _In_ long y, _In_ long width, _In_ long height, _In_ const wchar_t *template_name, _In_ const wchar_t *scales, _In_ double threshold, _In_ long method, _In_ long color_mode, _Out_ std::wstring &retjson, _Out_ long *ret)`** → `void`
  同上，但会按 `scales`（如 `1.0|0.9|1.1`）逐档缩放模板后再匹配，用于目标大小不确定的场景。

- **`void CvMatchAnyTemplate(_In_ long x, _In_ long y, _In_ long width, _In_ long height, _In_ const wchar_t *template_names, _In_ double threshold, _In_ long dir, _In_ long strip_mode, _In_ long method, _In_ long color_mode, _Out_ std::wstring &retjson, _Out_ long *ret)`** → `void`
  在多个模板里取最先命中的那个。`template_names` 用 `|` 分隔。

- **`void CvMatchAllTemplates(_In_ long x, _In_ long y, _In_ long width, _In_ long height, _In_ const wchar_t *template_names, _In_ double threshold, _In_ long dir, _In_ long strip_mode, _In_ long method, _In_ long color_mode, _Out_ std::wstring &retjson, _Out_ long *ret)`** → `void`
  把区域内所有模板的所有命中位置都返回，出参是多条 JSON。

- **`void CvFeatureMatchTemplate(_In_ long x, _In_ long y, _In_ long width, _In_ long height, _In_ const wchar_t *template_name, _In_ double threshold, _Out_ std::wstring &retjson, _Out_ long *ret)`** → `void`
  特征点匹配（对缩放、旋转、光照变化更稳，但比模板匹配慢）。出参同样是 JSON。

- **`void CvEdgeMatchTemplate(_In_ long x, _In_ long y, _In_ long width, _In_ long height, _In_ const wchar_t *template_name, _In_ double threshold, _Out_ std::wstring &retjson, _Out_ long *ret)`** → `void`
  基于边缘的模板匹配，适合背景杂乱但目标轮廓清晰的场景。

- **`void CvShapeMatchTemplate(_In_ long x, _In_ long y, _In_ long width, _In_ long height, _In_ const wchar_t *template_name, _In_ double threshold, _Out_ std::wstring &retjson, _Out_ long *ret)`** → `void`
  基于形状的匹配，适合目标内部纹理变化大、只有外形稳定的场景。

