#pragma once
#include "ImageSearchAlgorithms.h"
#include <memory>
#include <string>
// #include "TesseractOcr.h"

namespace op::image {

/*
此类为图像处理，包含以下工作
1.像素比较，查找
2.颜色转化
3.图像定位
4.简单OCR
5....

同一个 ImageSearchService 实例会复用截图和算法工作区，不支持并发调用；
多线程场景应保持每个线程使用独立 Op 对象。
*/
class ImageSearchService : public ImageSearchAlgorithms {
  public:
    const static int _max_dict = 10;

    ImageSearchService();
    ~ImageSearchService();
    //
    long Capture(const std::wstring &file);

    long CmpColor(long x, long y, const std::wstring &scolor, double sim);

    long FindColor(const wstring &color, double sim, long dir, long &x, long &y);

    long FindColorEx(const wstring &color, double sim, long dir, wstring &retstr);

    long FindMultiColor(const wstring &first_color, const wstring &offset_color, double sim, long dir, long &x,
                        long &y);

    long FindMultiColorEx(const wstring &first_color, const wstring &offset_color, double sim, long dir,
                          wstring &retstr);
    // 图形定位
    long FindPic(const std::wstring &files, const wstring &delta_colors, double sim, long dir, long &x, long &y);
    //
    long FindPicEx(const std::wstring &files, const wstring &delta_colors, double sim, long dir, wstring &retstr,
                   bool returnID = true);

    long FindColorBlock(const wstring &color, double sim, long count, long height, long width, long &x, long &y);

    long FindColorBlockEx(const wstring &color, double sim, long count, long height, long width, wstring &retstr);

    std::wstring GetColor(long x, long y);

    long GetColorNum(const wstring &color, double sim);

    long SetMemDict(int idx, void *data, long size);

    long SetDict(int idx, const wstring &file);

    std::wstring GetDict(long idx, long font_index);

    long UseDict(int idx);

    long AddDict(long idx, const wstring &dict_info);

    long SaveDict(long idx, const wstring &file_name);

    long ClearDict(long idx);

    long GetDictCount(long idx);

    long GetNowDict();

    long SetBinaryPreprocess(long mode, long isolated_threshold, long min_component_area, long bridge_gap);

    long GetBinaryPreprocess(long &mode, long &isolated_threshold, long &min_component_area, long &bridge_gap) const;

    std::wstring FetchWord(rect_t rc, const wstring &color, const wstring &word);

    std::wstring FetchWord(rect_t rc, const wstring &color, double sim, const wstring &word);

    long ExtractWordRects(const wstring &color, double sim, long min_word_h, std::vector<rect_t> &rects);

    long ExtractWordRectsEx(const wstring &color, double sim, long min_word_w, long min_word_h, long padding,
                            std::vector<rect_t> &rects);

    long FetchWords(const wstring &color, double sim, const wstring &words, long min_word_h, std::wstring &out_str);

    long FetchWordsEx(const wstring &color, double sim, const wstring &words, long min_word_w, long min_word_h,
                      long padding, std::wstring &out_str);

    long FetchWordsByRects(const wstring &color, double sim, const wstring &words, const std::vector<rect_t> &rects,
                           std::wstring &out_str);

    long GetBinaryPreview(const wstring &color, double sim, std::wstring &out_str);

    long GetWordPreview(const wstring &dict_info, std::wstring &out_str);

    long CheckWordDict(const wstring &dict_info, std::wstring &out_str);

    long NormalizeWordDict(const wstring &dict_info, std::wstring &out_str);

    long RenameWordDict(const wstring &dict_info, const wstring &words, std::wstring &out_str);

    long OCR(const wstring &color, double sim, std::wstring &out_str);

    long OcrEx(const wstring &color, double sim, std::wstring &out_str);

    long FindStr(const wstring &str, const wstring &color, double sim, long &retx, long &rety);

    long FindStrEx(const wstring &str, const wstring &color, double sim, std::wstring &out_str);

    long OcrAuto(double sim, std::wstring &retstr);

    long OcrFromFile(const wstring &files, const wstring &color, double sim, std::wstring &retstr);

    long OcrAutoFromFile(const wstring &files, double sim, std::wstring &retstr);

    long FindLine(const wstring &color, double sim, wstring &retStr);

    long LoadPic(const wstring &files);

    long FreePic(const wstring &files);

    long LoadMemPic(const wstring &file_name, void *data, long size);

    long GetPicSize(const wstring &file_name, long *x, long *y);

    void str2binaryfbk(const wstring &color);

  private:
    // 字库
    Dictionary _dicts[_max_dict];
    // 当前字库索引
    int _curr_idx;
    long _binary_preprocess_mode;
    long _binary_isolated_threshold;
    long _binary_min_component_area;
    long _binary_bridge_gap;

  public:
    // 当前目录
    wstring _curr_path;
    // 是否允许 FindPic 自动缓存本地文件；LoadPic/LoadMemPic 属于显式加载，不受此开关影响。
    int _enable_cache;

  private:
    // RETURN TYPE 0:word colors info; 1:bk color info
    int str2colordfs(const wstring &color_str, std::vector<color_df_t> &colors);
    int str2colordfs(const wstring &color_str, std::vector<color_df_t> &colors, std::vector<bool> *explicit_dfs);
    void str2binaryfbk(const wstring &color, double sim);
    void str2pointbinaryfbk(const wstring &color);
    void str2pointbinaryfbk(const wstring &color, double sim);
    void ApplyBinaryPreprocess();
    std::wstring FetchWordFromBinary(rect_t rc, const wstring &word);
    long FetchWordsFromBinary(const wstring &words, const std::vector<rect_t> &rects, std::wstring &out_str);
    void str2colors(const wstring &color, std::vector<color_t> &vcolor);
    // vmatches 是算法层使用的裸指针视图，holders 负责延长共享缓存对象的生命周期。
    void files2mats(const wstring &files, std::vector<PicMatchTemplate *> &vmatches, std::vector<wstring> &vstr,
                    std::vector<std::shared_ptr<Image>> &holders,
                    std::vector<std::shared_ptr<PicMatchTemplate>> &match_holders);
};

} // namespace op::image
