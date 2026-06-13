#pragma once

#include <string>
#include <vector>

namespace opcv {

// 原始图像内存支持的像素格式。
enum class PixelFormat {
    Gray,
    Bgr,
    Bgra,
};

// 外部传入的原始图像内存视图。
struct RawImageView {
    const unsigned char *data = nullptr;
    int width = 0;
    int height = 0;
    int stride = 0;
    PixelFormat format = PixelFormat::Bgra;
};

// OpenCV 图像对象的轻量包装。
struct ImageHandle {
    std::vector<unsigned char> bytes;
    int width = 0;
    int height = 0;
    int channels = 0;
};

// 模板匹配结果。
struct MatchResult {
    int x = -1;
    int y = -1;
    int width = 0;
    int height = 0;
    double score = 0.0;
};

// 带模板名的匹配结果。
struct NamedMatchResult {
    std::wstring name;
    MatchResult match;
};

// 模板匹配时使用的颜色模式。
enum class MatchColorMode {
    Color,
    Gray,
};

// OpenCV 匹配接口使用的搜索方向。
// 说明：
// - 用于指导单次模板搜索的扫描方向。
// - 条带模式下，也会影响条带分发顺序，用于优先搜索某个方向上的区域。
enum class SearchDirection {
    LeftToRight = 0,
    RightToLeft = 1,
    TopToBottom = 2,
    BottomToTop = 3,
};

// 条带搜索模式。
// 说明：
// - 用于把 ROI 拆成多个条带并行搜索。
// - 命中即停时更偏向低延迟，不保证返回全局最佳命中。
enum class StripMode {
    None = 0,
    Horizontal = 1,
    Vertical = 2,
};

// 阈值化模式。
enum class ThresholdMode {
    Binary = 0,
    BinaryInv = 1,
    Otsu = 2,
    OtsuInv = 3,
    Adaptive = 4,
    AdaptiveInv = 5,
};

// 颜色范围过滤使用的颜色空间。
enum class InRangeColorSpace {
    Bgr = 0,
    Hsv = 1,
    Gray = 2,
};

// 形态学操作模式。
enum class MorphologyMode {
    Erode = 0,
    Dilate = 1,
    Open = 2,
    Close = 3,
};

// 细化/骨架化算法模式。
enum class ThinMode {
    ZhangSuen = 0,
    GuoHall = 1,
    Morph = 2,
};

// 模糊/平滑模式。
enum class BlurMode {
    Gaussian = 0,
    Median = 1,
    Bilateral = 2,
    Box = 3,
};

// 匹配候选项，score 为归一化相似度，越大越匹配。
struct MatchCandidate {
    int x = -1;
    int y = -1;
    double score = 0.0;
};

// 区域描述。
struct Region {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

// 区域分析结果，用于连通域和轮廓外接框。
struct RegionAnalysisResult {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    double area = 0.0;
};

// 轮廓分析结果。
struct ContourAnalysisResult : RegionAnalysisResult {
    double perimeter = 0.0;
    int points = 0;
};

// 获取当前链接到工程中的 OpenCV 版本号。
std::wstring GetOpenCvVersion();

// 从本地文件加载图像到内存对象。
bool LoadImageFromFile(const std::wstring &file_path, ImageHandle &image);

// 把图像对象保存到本地文件。
bool SaveImageToFile(const ImageHandle &image, const std::wstring &file_path);

// 从原始像素内存加载图像到内存对象。
bool LoadImageFromMemory(const RawImageView &source, ImageHandle &image);

// 把图像转换为灰度图。
bool ToGray(const ImageHandle &source, ImageHandle &gray);

// 把图像转换为二值图。
bool ToBinary(const ImageHandle &source, ImageHandle &output);

// 按指定模式执行阈值化，输出单通道二值图。
bool Threshold(const ImageHandle &source, ImageHandle &output, double threshold, double max_value, ThresholdMode mode);

// 按颜色范围生成 mask，输出单通道二值图。
bool InRange(const ImageHandle &source, ImageHandle &output, InRangeColorSpace color_space,
             const std::vector<double> &lower, const std::vector<double> &upper);

// 对二值图/mask 执行形态学处理。
bool Morphology(const ImageHandle &source, ImageHandle &output, MorphologyMode mode, int kernel_size, int iterations);

// 对图像做边缘提取。
bool ToEdge(const ImageHandle &source, ImageHandle &output);

// 对图像做轮廓线提取，输出单通道轮廓图。
bool ToOutline(const ImageHandle &source, ImageHandle &output);

// 对图像进行去噪处理。
bool Denoise(const ImageHandle &source, ImageHandle &output);

// 对灰度图执行全局直方图均衡化。
bool Equalize(const ImageHandle &source, ImageHandle &output);

// 对灰度图执行局部自适应直方图均衡化。
bool CLAHE(const ImageHandle &source, ImageHandle &output, double clip_limit, int tile_grid_size);

// 对图像执行模糊/平滑处理。
bool Blur(const ImageHandle &source, ImageHandle &output, BlurMode mode, int kernel_size);

// 对图像执行锐化处理。
bool Sharpen(const ImageHandle &source, ImageHandle &output, double strength);

// 对图像进行细化/抽骨处理。
bool Thin(const ImageHandle &source, ImageHandle &output, ThinMode mode = ThinMode::ZhangSuen);

// 裁剪指定区域图像。
bool Crop(const ImageHandle &source, const Region &region, ImageHandle &output);

// 把图像缩放到指定大小。
bool Resize(const ImageHandle &source, int width, int height, ImageHandle &output);

// 裁剪图像中的有效区域。
bool CropValid(const ImageHandle &source, ImageHandle &output);

// 获取二值图/mask 中的连通区域。
bool ConnectedComponents(const ImageHandle &source, double min_area, std::vector<RegionAnalysisResult> &results);

// 获取二值图/mask 中的轮廓区域。
bool FindContours(const ImageHandle &source, double min_area, std::vector<ContourAnalysisResult> &results);

// 使用图像对象在指定区域内执行模板匹配。
bool MatchTemplate(const ImageHandle &source, const ImageHandle &templ, const Region &region,
                   std::vector<MatchCandidate> &matches, int method = 1,
                   MatchColorMode color_mode = MatchColorMode::Gray);

// 从候选匹配结果中选出最佳结果。
bool GetBestMatch(const std::vector<MatchCandidate> &matches, int templ_width, int templ_height, MatchResult &result);

// 判断匹配是否达到给定阈值。
bool IsMatchAboveThreshold(const MatchResult &result, double threshold);

// 加载单个模板到内部缓存。
bool LoadTemplate(const std::wstring &name, const std::wstring &file_path);

// 加载带 mask 的模板到内部缓存。
bool LoadMaskedTemplate(const std::wstring &name, const std::wstring &template_path, const std::wstring &mask_path);

// 批量加载多个模板到内部缓存；会尽量加载所有项，任意一项失败则返回 false。
bool LoadTemplateList(const std::vector<std::pair<std::wstring, std::wstring>> &templates);

// 判断指定名字的模板是否已加载。
bool HasTemplate(const std::wstring &name);

// 删除指定名字的模板。
bool RemoveTemplate(const std::wstring &name);

// 删除所有已加载模板。
void RemoveAllTemplates();

// 获取所有已加载模板的名字。
std::vector<std::wstring> GetAllTemplateNames();

// 获取当前已加载模板数量。
int GetTemplateCount();

// 使用指定模板在给定区域内执行匹配，并返回一个达到阈值的命中结果。
// 说明：
// - SearchDirection 决定首命中扫描方向。
// - 非条带模式按方向分块搜索，首个达到阈值的命中会立即返回。
// - 条带模式并行搜索，谁先达到阈值谁返回。
bool MatchTemplate(const ImageHandle &source, const std::wstring &template_name, const Region &region, double threshold,
                   MatchResult &result, SearchDirection dir = SearchDirection::LeftToRight,
                   StripMode strip_mode = StripMode::None, int method = 1,
                   MatchColorMode color_mode = MatchColorMode::Gray);

// 使用指定模板在多个缩放比例下匹配；scales 为空时自动尝试内部候选，并返回最佳命中。
bool MatchTemplateScale(const ImageHandle &source, const std::wstring &template_name, const Region &region,
                        const std::vector<double> &scales, double threshold, MatchResult &result, int method = 1,
                        MatchColorMode color_mode = MatchColorMode::Gray);

// 在指定区域内并行寻找任意一个命中的模板。
// 说明：
// - 模板之间并行搜索，谁先命中谁返回。
// - SearchDirection 用于指导单模板内部的扫描顺序，以及条带模式下的条带搜索顺序。
bool MatchAnyTemplate(const ImageHandle &source, const std::vector<std::wstring> &template_names, const Region &region,
                      double threshold, NamedMatchResult &result, SearchDirection dir = SearchDirection::LeftToRight,
                      StripMode strip_mode = StripMode::None, int method = 1,
                      MatchColorMode color_mode = MatchColorMode::Gray);

// 在指定区域内从多个模板中返回所有达到阈值的结果，结果中可能包含同一模板的多个命中点。
bool MatchAllTemplates(const ImageHandle &source, const std::vector<std::wstring> &template_names, const Region &region,
                       double threshold, std::vector<NamedMatchResult> &results,
                       SearchDirection dir = SearchDirection::LeftToRight, StripMode strip_mode = StripMode::None,
                       int method = 1, MatchColorMode color_mode = MatchColorMode::Gray);

// 使用特征匹配在给定区域内查找指定模板。
bool FeatureMatchTemplate(const ImageHandle &source, const std::wstring &template_name, const Region &region,
                          double threshold, MatchResult &result);

// 使用边缘匹配在给定区域内查找指定模板。
bool EdgeMatchTemplate(const ImageHandle &source, const std::wstring &template_name, const Region &region,
                       double threshold, MatchResult &result);

// 使用轮廓/形状匹配在给定区域内查找指定模板。
bool ShapeMatchTemplate(const ImageHandle &source, const std::wstring &template_name, const Region &region,
                        double threshold, MatchResult &result);

} // namespace opcv
