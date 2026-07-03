// #include "stdafx.h"
#include "ImageSearchService.h"
#include "../runtime/RuntimeUtils.h"
#include "../ocr/OcrService.h"
#include <algorithm>
#include <bitset>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <sstream>

namespace op::image {

using op::ocr::HttpOcrService;

namespace {
// 普通找图模板缓存。放在进程级别，多个 Op 实例共享同一份已解码图片。
// 匹配时会复制 shared_ptr 快照；FreePic 只移除缓存入口，不会提前释放正在匹配的图片。
std::map<wstring, std::shared_ptr<Image>> g_pic_cache;
std::shared_mutex g_pic_cache_mutex;

template <typename Target, typename Value> bool set_out(Target *target, Value value) {
    if (!target)
        return false;
    *target = static_cast<Target>(value);
    return true;
}

color_t sim_to_point_color_diff(double sim) {
    if (sim < 0.0 || sim > 1.0)
        sim = 1.0;

    const auto diff = static_cast<uchar>(std::ceil((1.0 - sim) * 255.0));
    color_t color_diff;
    color_diff.b = diff;
    color_diff.g = diff;
    color_diff.r = diff;
    return color_diff;
}

template <typename Fn> void for_each_dict_line(const wstring &dict_info, Fn fn) {
    long item_index = 0;
    size_t begin = 0;
    while (begin <= dict_info.size()) {
        size_t end = dict_info.find(L'\n', begin);
        if (end == std::wstring::npos)
            end = dict_info.size();

        std::wstring item = dict_info.substr(begin, end - begin);
        if (!item.empty() && item.back() == L'\r')
            item.pop_back();

        if (!item.empty())
            fn(item_index++, item);

        if (end == dict_info.size())
            break;
        begin = end + 1;
    }
}

long clamp_preprocess_mode(long mode) {
    if (mode < 0)
        return 0;
    if (mode > 3)
        return 3;
    return mode;
}

long clamp_long(long value, long low, long high) {
    if (value < low)
        return low;
    if (value > high)
        return high;
    return value;
}

std::shared_ptr<Image> find_cached_pic(const wstring &key) {
    std::shared_lock<std::shared_mutex> lock(g_pic_cache_mutex);
    auto it = g_pic_cache.find(key);
    return it == g_pic_cache.end() ? nullptr : it->second;
}

bool store_cached_pic(const wstring &key, std::shared_ptr<Image> image) {
    if (key.empty() || !image || image->empty())
        return false;

    std::unique_lock<std::shared_mutex> lock(g_pic_cache_mutex);
    g_pic_cache[key] = std::move(image);
    return true;
}

bool erase_cached_pic(const wstring &key) {
    std::unique_lock<std::shared_mutex> lock(g_pic_cache_mutex);
    return g_pic_cache.erase(key) > 0;
}

std::shared_ptr<Image> read_pic_file(const wstring &path) {
    auto image = std::make_shared<Image>();
    if (!image->read(path.data()) || image->empty())
        return nullptr;
    return image;
}

std::shared_ptr<Image> read_mem_pic(void *data, long size) {
    if (!data || size <= 0)
        return nullptr;

    auto image = std::make_shared<Image>();
    if (!image->read(data, size) || image->empty())
        return nullptr;
    return image;
}
} // namespace

ImageSearchService::ImageSearchService() {
    _curr_idx = 0;
    _enable_cache = 1;
    _binary_preprocess_mode = 0;
    _binary_isolated_threshold = 0;
    _binary_min_component_area = 2;
    _binary_bridge_gap = 1;
}

ImageSearchService::~ImageSearchService() {
}

long ImageSearchService::Capture(const std::wstring &file) {
    std::filesystem::path fpath(file);
    if (!fpath.is_absolute())
        fpath = std::filesystem::path(_curr_path) / fpath;

    return _src.write(fpath.c_str());
}

long ImageSearchService::CmpColor(long x, long y, const std::wstring &scolor, double sim) {
    std::vector<color_df_t> vcolor;
    str2colordfs(scolor, vcolor);
    color_t color;
    if (!ImageSearchAlgorithms::GetPixel(x, y, color))
        return 0;
    return ImageSearchAlgorithms::CmpColor(color, vcolor, sim);
}

long ImageSearchService::FindColor(const wstring &color, double sim, long dir, long &x, long &y) {
    std::vector<color_df_t> colors;
    str2colordfs(color, colors);
    // setlog("%s cr size=%d",colors[0].color.tostr().data(), colors.size());
    // setlog("sim:,dir:%d", dir);
    return ImageSearchAlgorithms::FindColor(colors, sim, dir, x, y);
}

long ImageSearchService::FindColorEx(const wstring &color, double sim, long dir, wstring &retstr) {
    std::vector<color_df_t> colors;
    str2colordfs(color, colors);
    return ImageSearchAlgorithms::FindColorEx(colors, sim, dir, retstr);
}

long ImageSearchService::FindMultiColor(const wstring &first_color, const wstring &offset_color, double sim, long dir, long &x,
                               long &y) {
    std::vector<color_df_t> vfirst_color;
    str2colordfs(first_color, vfirst_color);
    std::vector<wstring> vseconds;
    split(offset_color, vseconds, L",");
    std::vector<pt_cr_df_t> voffset_cr;
    for (auto &it : vseconds) {
        size_t id1, id2;
        id1 = it.find(L'|');
        id2 = (id1 == wstring::npos ? wstring::npos : it.find(L'|', id1 + 1));
        if (id2 != wstring::npos) {
            pt_cr_df_t tp;
            swscanf(it.c_str(), L"%d|%d", &tp.x, &tp.y);
            if (id2 + 1 != it.length())
                str2colordfs(it.substr(id2 + 1), tp.crdfs);
            else
                break;
            voffset_cr.push_back(tp);
        }
    }
    return ImageSearchAlgorithms::FindMultiColor(vfirst_color, voffset_cr, sim, dir, x, y);
}

long ImageSearchService::FindMultiColorEx(const wstring &first_color, const wstring &offset_color, double sim, long dir,
                                 wstring &retstr) {
    std::vector<color_df_t> vfirst_color;
    str2colordfs(first_color, vfirst_color);
    std::vector<wstring> vseconds;
    split(offset_color, vseconds, L",");
    std::vector<pt_cr_df_t> voffset_cr;
    for (auto &it : vseconds) {
        size_t id1, id2;
        id1 = it.find(L'|');
        id2 = (id1 == wstring::npos ? wstring::npos : it.find(L'|', id1 + 1));
        if (id2 != wstring::npos) {
            pt_cr_df_t tp;
            swscanf(it.c_str(), L"%d|%d", &tp.x, &tp.y);
            if (id2 + 1 != it.length())
                str2colordfs(it.substr(id2 + 1), tp.crdfs);
            else
                break;
            voffset_cr.push_back(tp);
        }
    }
    return ImageSearchAlgorithms::FindMultiColorEx(vfirst_color, voffset_cr, sim, dir, retstr);
}
// 图形定位
long ImageSearchService::FindPic(const std::wstring &files, const wstring &delta_colors, double sim, long dir, long &x,
                        long &y) {
    vector<Image *> vpic;
    // 算法层沿用裸指针入参，这里用 holders 保证匹配期间图片对象仍然存活。
    vector<std::shared_ptr<Image>> holders;
    color_t dfcolor;
    vector<std::wstring> vpic_name;
    files2mats(files, vpic, vpic_name, holders);
    dfcolor.str2color(delta_colors);
    sim = 0.5 + sim / 2;
    long ret = ImageSearchAlgorithms::FindPicTh(vpic, dfcolor, sim, dir, x, y);
    return ret;
}
//
long ImageSearchService::FindPicEx(const std::wstring &files, const wstring &delta_colors, double sim, long dir, wstring &retstr,
                          bool returnID) {
    vector<Image *> vpic;
    // 算法层沿用裸指针入参，这里用 holders 保证匹配期间图片对象仍然存活。
    vector<std::shared_ptr<Image>> holders;
    vpoint_desc_t vpd;
    color_t dfcolor;
    vector<std::wstring> vpic_name;
    files2mats(files, vpic, vpic_name, holders);
    dfcolor.str2color(delta_colors);
    sim = 0.5 + sim / 2;
    long ret = ImageSearchAlgorithms::FindPicExTh(vpic, dfcolor, sim, dir, vpd);
    std::wstringstream ss(std::wstringstream::in | std::wstringstream::out);
    if (returnID) {
        for (auto &it : vpd) {
            ss << it.id << L"," << it.pos << L"|";
        }
    } else {
        for (auto &it : vpd) {
            ss << vpic_name[it.id] << L"," << it.pos << L"|";
        }
    }
    retstr = ss.str();
    if (vpd.size())
        retstr.pop_back();
    return ret;
}

long ImageSearchService::FindColorBlock(const wstring &color, double sim, long count, long height, long width, long &x,
                               long &y) {
    str2binaryfbk(color, sim);
    return ImageSearchAlgorithms::FindColorBlock(count, height, width, x, y);
}

long ImageSearchService::FindColorBlockEx(const wstring &color, double sim, long count, long height, long width,
                                 wstring &retstr) {
    str2binaryfbk(color, sim);
    return ImageSearchAlgorithms::FindColorBlockEx(count, height, width, retstr);
}

long ImageSearchService::GetColorNum(const wstring &color, double sim) {
    std::vector<color_df_t> colors;
    str2colordfs(color, colors);
    return ImageSearchAlgorithms::FindColorNum(colors, sim);
}

long ImageSearchService::SetDict(int idx, const wstring &file_name) {
    if (idx < 0 || idx >= _max_dict)
        return 0;
    _dicts[idx].clear();
    wstring fullpath;
    if (Path2GlobalPath(file_name, _curr_path, fullpath)) {
        // SetDict 按文件内容识别：OP 二进制 .dict 优先，失败后兼容大漠 txt。
        _dicts[idx].read_dict(fullpath);
    } else {
        setlog(L"file '%s' does not exist", file_name.c_str());
    }

    return _dicts[idx].empty() ? 0 : 1;
}

std::wstring ImageSearchService::GetDict(long idx, long font_index) {
    wstring tp;
    if (idx < 0 || idx >= _max_dict)
        return tp;
    if (font_index < 0 || static_cast<size_t>(font_index) >= _dicts[idx].words.size())
        return tp;
    return _dicts[idx].words[font_index].to_string();
}

long ImageSearchService::SetMemDict(int idx, void *data, long size) {
    if (idx < 0 || idx >= _max_dict)
        return 0;
    _dicts[idx].clear();
    _dicts[idx].read_memory_dict_dm((const char *)data, size);
    return _dicts[idx].empty() ? 0 : 1;
}

long ImageSearchService::UseDict(int idx) {
    if (idx < 0 || idx >= _max_dict)
        return 0;
    _curr_idx = idx;
    return 1;
}

long ImageSearchService::AddDict(long idx, const wstring &dict_info) {
    if (idx < 0 || idx >= _max_dict)
        return 0;

    word1_t word;
    if (!dict_entry_importer::parse_text_dict_entry(dict_info, word))
        return 0;
    _dicts[idx].add_word(word);
    return 1;
}

long ImageSearchService::SaveDict(long idx, const wstring &file_name) {
    if (idx < 0 || idx >= _max_dict)
        return 0;
    return _dicts[idx].write_dict(file_name) ? 1 : 0;
}

long ImageSearchService::ClearDict(long idx) {
    if (idx < 0 || idx >= _max_dict)
        return 0;

    _dicts[idx].clear();
    return 1;
}

long ImageSearchService::GetDictCount(long idx) {
    if (idx < 0 || idx >= _max_dict)
        return 0;

    return _dicts[idx].info._word_count;
}

long ImageSearchService::GetNowDict() {
    return _curr_idx;
}

long ImageSearchService::SetBinaryPreprocess(long mode, long isolated_threshold, long min_component_area,
                                             long bridge_gap) {
    _binary_preprocess_mode = clamp_preprocess_mode(mode);
    _binary_isolated_threshold = clamp_long(isolated_threshold, 0, 8);
    _binary_min_component_area = min_component_area <= 0 ? 2 : clamp_long(min_component_area, 1, 4096);
    _binary_bridge_gap = bridge_gap > 0 ? 1 : 0;
    return 1;
}

long ImageSearchService::GetBinaryPreprocess(long &mode, long &isolated_threshold, long &min_component_area,
                                             long &bridge_gap) const {
    mode = _binary_preprocess_mode;
    isolated_threshold = _binary_isolated_threshold;
    min_component_area = _binary_min_component_area;
    bridge_gap = _binary_bridge_gap;
    return 1;
}

wstring ImageSearchService::FetchWord(rect_t rc, const wstring &color, const wstring &word) {
    return FetchWord(rc, color, 1.0, word);
}

wstring ImageSearchService::FetchWord(rect_t rc, const wstring &color, double sim, const wstring &word) {
    str2pointbinaryfbk(color, sim);
    return FetchWordFromBinary(rc, word);
}

wstring ImageSearchService::FetchWordFromBinary(rect_t rc, const wstring &word) {
    auto orc = rc;
    if (!bin_image_cut(2, rc, orc))
        return L"";
    // check is too large
    if (orc.width() > 255) {
        orc.x2 = orc.x1 + 255;
        rc = orc;
        if (!bin_image_cut(2, rc, orc))
            return L"";
    }
    if (orc.height() > 255) {
        orc.y2 = orc.y1 + 255;
        rc = orc;
        if (!bin_image_cut(2, rc, orc))
            return L"";
    }
    Dictionary dict_new;
    dict_new.add_word(_binary, orc);
    auto &wt = dict_new.words[0];
    wt.set_chars(word);
    return wt.to_string();
}

long ImageSearchService::FetchWordsFromBinary(const wstring &words, const std::vector<rect_t> &rects,
                                              std::wstring &out_str) {
    out_str.clear();
    if (rects.empty() || rects.size() != words.size())
        return 0;

    for (size_t i = 0; i < rects.size(); ++i) {
        if (!rects[i].valid() || rects[i].x2 > _src.width || rects[i].y2 > _src.height) {
            out_str.clear();
            return 0;
        }

        const std::wstring word(1, words[i]);
        const std::wstring entry = FetchWordFromBinary(rects[i], word);
        if (entry.empty()) {
            out_str.clear();
            return 0;
        }
        out_str += entry;
        out_str += L"\n";
    }
    if (!out_str.empty())
        out_str.pop_back();
    return static_cast<long>(rects.size());
}

long ImageSearchService::ExtractWordRects(const wstring &color, double sim, long min_word_h, std::vector<rect_t> &rects) {
    rects.clear();
    if (min_word_h <= 0)
        min_word_h = 2;
    str2pointbinaryfbk(color, sim);
    get_rois(static_cast<int>(min_word_h), rects);
    return static_cast<long>(rects.size());
}

long ImageSearchService::ExtractWordRectsEx(const wstring &color, double sim, long min_word_w, long min_word_h,
                                            long padding, std::vector<rect_t> &rects) {
    rects.clear();
    if (min_word_w <= 0)
        min_word_w = 1;
    if (min_word_h <= 0)
        min_word_h = 2;
    if (padding < 0)
        padding = 0;

    str2pointbinaryfbk(color, sim);
    get_rois(static_cast<int>(min_word_w), static_cast<int>(min_word_h), static_cast<int>(padding), rects);
    return static_cast<long>(rects.size());
}

long ImageSearchService::FetchWords(const wstring &color, double sim, const wstring &words, long min_word_h,
                                    std::wstring &out_str) {
    out_str.clear();
    if (min_word_h <= 0)
        min_word_h = 2;
    str2pointbinaryfbk(color, sim);
    std::vector<rect_t> rects;
    get_rois(static_cast<int>(min_word_h), rects);
    const long rect_count = static_cast<long>(rects.size());
    if (rect_count == 0 || rects.size() != words.size())
        return 0;
    return FetchWordsFromBinary(words, rects, out_str);
}

long ImageSearchService::FetchWordsEx(const wstring &color, double sim, const wstring &words, long min_word_w,
                                      long min_word_h, long padding, std::wstring &out_str) {
    out_str.clear();
    if (min_word_w <= 0)
        min_word_w = 1;
    if (min_word_h <= 0)
        min_word_h = 2;
    if (padding < 0)
        padding = 0;

    std::vector<rect_t> rects;
    str2pointbinaryfbk(color, sim);
    get_rois(static_cast<int>(min_word_w), static_cast<int>(min_word_h), static_cast<int>(padding), rects);
    const long rect_count = static_cast<long>(rects.size());
    if (rect_count == 0)
        return 0;
    return FetchWordsFromBinary(words, rects, out_str);
}

long ImageSearchService::FetchWordsByRects(const wstring &color, double sim, const wstring &words,
                                           const std::vector<rect_t> &rects, std::wstring &out_str) {
    out_str.clear();
    if (rects.empty() || rects.size() != words.size())
        return 0;
    str2pointbinaryfbk(color, sim);
    return FetchWordsFromBinary(words, rects, out_str);
}

long ImageSearchService::GetBinaryPreview(const wstring &color, double sim, std::wstring &out_str) {
    out_str.clear();
    str2pointbinaryfbk(color, sim);
    if (_binary.empty())
        return 0;

    long point_count = 0;
    out_str += std::to_wstring(_binary.width);
    out_str += L",";
    out_str += std::to_wstring(_binary.height);
    out_str += L"\n";
    for (int y = 0; y < _binary.height; ++y) {
        for (int x = 0; x < _binary.width; ++x) {
            if (_binary.at(y, x) == WORD_COLOR) {
                out_str += L"#";
                ++point_count;
            } else {
                out_str += L".";
            }
        }
        if (y + 1 < _binary.height)
            out_str += L"\n";
    }
    return point_count;
}

long ImageSearchService::GetWordPreview(const wstring &dict_info, std::wstring &out_str) {
    out_str.clear();
    word1_t word;
    if (!dict_entry_importer::parse_text_dict_entry(dict_info, word))
        return 0;

    out_str += word.info.name;
    out_str += L",";
    out_str += std::to_wstring(word.info.w);
    out_str += L",";
    out_str += std::to_wstring(word.info.h);
    out_str += L",";
    out_str += std::to_wstring(word.info.bit_cnt);
    out_str += L"\n";

    for (int y = 0; y < word.info.h; ++y) {
        for (int x = 0; x < word.info.w; ++x) {
            const int idx = x * word.info.h + y;
            out_str += GET_BIT(word.data[idx / 8], idx & 7) ? L"#" : L".";
        }
        if (y + 1 < word.info.h)
            out_str += L"\n";
    }
    return 1;
}

long ImageSearchService::CheckWordDict(const wstring &dict_info, std::wstring &out_str) {
    out_str.clear();

    long valid_count = 0;
    for_each_dict_line(dict_info, [&](long item_index, const std::wstring &item) {
        word1_t word;
        if (dict_entry_importer::parse_text_dict_entry(item, word)) {
            const int area = word.info.w * word.info.h;
            const int density = area > 0 ? static_cast<int>((word.info.bit_cnt * 100 + area / 2) / area) : 0;
            out_str += std::to_wstring(item_index);
            out_str += L",1,";
            out_str += word.info.name;
            out_str += L",";
            out_str += std::to_wstring(word.info.w);
            out_str += L",";
            out_str += std::to_wstring(word.info.h);
            out_str += L",";
            out_str += std::to_wstring(word.info.bit_cnt);
            out_str += L",";
            out_str += std::to_wstring(density);
            ++valid_count;
        } else {
            out_str += std::to_wstring(item_index);
            out_str += L",0,invalid";
        }
        out_str += L"|";
    });

    if (!out_str.empty())
        out_str.pop_back();
    return valid_count;
}

long ImageSearchService::NormalizeWordDict(const wstring &dict_info, std::wstring &out_str) {
    out_str.clear();

    long valid_count = 0;
    for_each_dict_line(dict_info, [&](long, const std::wstring &item) {
        word1_t word;
        if (dict_entry_importer::parse_text_dict_entry(item, word)) {
            if (!out_str.empty())
                out_str += L"\n";
            out_str += word.to_string();
            ++valid_count;
        }
    });

    return valid_count;
}

long ImageSearchService::RenameWordDict(const wstring &dict_info, const wstring &words, std::wstring &out_str) {
    out_str.clear();

    std::vector<word1_t> entries;
    for_each_dict_line(dict_info, [&](long, const std::wstring &item) {
        word1_t word;
        if (dict_entry_importer::parse_text_dict_entry(item, word))
            entries.push_back(word);
    });

    if (entries.empty() || entries.size() != words.size())
        return 0;

    for (size_t i = 0; i < entries.size(); ++i) {
        entries[i].set_chars(std::wstring(1, words[i]));
        if (!out_str.empty())
            out_str += L"\n";
        out_str += entries[i].to_string();
    }

    return static_cast<long>(entries.size());
}

long ImageSearchService::OCR(const wstring &color, double sim, std::wstring &out_str) {
    out_str.clear();
    if (sim < 0. || sim > 1.)
        sim = 1.;
    long s = 0;
    if (_dicts[_curr_idx].size() == 0) {
        vocr_rec_t res;
        HttpOcrService::getInstance()->ocr(_src.pdata, _src.width, _src.height, 4, res);
        for (auto &it : res) {
            if (it.confidence >= sim - 1e-9) {
                out_str += it.text;
            }
        }
    } else {
        str2pointbinaryfbk(color, sim);
        s = ImageSearchAlgorithms::Ocr(_dicts[_curr_idx], sim, out_str);
    }

    return s;
}

wstring ImageSearchService::GetColor(long x, long y) {
    color_t cr;
    if (ImageSearchAlgorithms::GetPixel(x, y, cr)) {
        return _s2wstring(cr.tostr());
    } else {
        return L"";
    }
}

int ImageSearchService::str2colordfs(const wstring &color_str, std::vector<color_df_t> &colors) {
    return str2colordfs(color_str, colors, nullptr);
}

int ImageSearchService::str2colordfs(const wstring &color_str, std::vector<color_df_t> &colors, std::vector<bool> *explicit_dfs) {
    std::vector<wstring> vstr, vstr2;
    color_df_t cr;
    colors.clear();
    if (explicit_dfs)
        explicit_dfs->clear();
    int ret = 0;
    if (color_str.empty()) { // default
        return 1;
    }
    if (color_str[0] == L'@') { // bk color info
        ret = 1;
    }
    split(ret ? color_str.substr(1) : color_str, vstr, L"|");
    for (auto &it : vstr) {
        split(it, vstr2, L"-");
        cr.color.str2color(vstr2[0]);
        const bool has_explicit_df = vstr2.size() == 2;
        cr.df.str2color(has_explicit_df ? vstr2[1] : L"000000");
        colors.push_back(cr);
        if (explicit_dfs)
            explicit_dfs->push_back(has_explicit_df);
    }
    return ret;
}

void ImageSearchService::str2colors(const wstring &color, std::vector<color_t> &vcolor) {
    std::vector<wstring> vstr, vstr2;
    color_t cr;
    vcolor.clear();
    split(color, vstr, L"|");
    for (auto &it : vstr) {
        cr.str2color(it);
        vcolor.push_back(cr);
    }
}

long ImageSearchService::LoadPic(const wstring &files) {
    std::vector<wstring> vstr;
    int loaded = 0;
    split(files, vstr, L"|");
    wstring tp;
    for (auto &it : vstr) {
        // 显式加载会按当前文件内容刷新全局缓存，EnablePicCache 只影响 FindPic 的自动缓存。
        if (!Path2GlobalPath(it, _curr_path, tp))
            continue;

        auto image = read_pic_file(tp);
        if (!image)
            continue;

        store_cached_pic(tp, std::move(image));
        loaded++;
    }
    return loaded;
}

long ImageSearchService::FreePic(const wstring &files) {
    std::vector<wstring> vstr;
    int loaded = 0;
    split(files, vstr, L"|");
    wstring tp;
    for (auto &it : vstr) {
        // 先按内存图片名或完整 key 删除。
        if (erase_cached_pic(it)) {
            loaded++;
            continue;
        }

        // 没查到再按资源目录解析后的文件路径删除。
        if (Path2GlobalPath(it, _curr_path, tp) && tp != it && erase_cached_pic(tp)) {
            loaded++;
        }
    }
    return loaded;
}

long ImageSearchService::LoadMemPic(const wstring &file_name, void *data, long size) {
    try {
        if (file_name.empty())
            return 0;

        auto image = read_mem_pic(data, size);
        if (!image)
            return 0;

        // 同名内存图按新内容覆盖，避免全局缓存复用到旧模板。
        return store_cached_pic(file_name, std::move(image)) ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

long ImageSearchService::GetPicSize(const wstring &file_name, long *width, long *height) {
    auto image = find_cached_pic(file_name);
    if (!image) {
        wstring tp;
        if (Path2GlobalPath(file_name, _curr_path, tp))
            image = find_cached_pic(tp);
    }

    if (image) {
        if (!set_out(width, image->width) || !set_out(height, image->height))
            return 0;
        return 1;
    }
    return 0;
}

void ImageSearchService::str2binaryfbk(const wstring &color) {
    vector<color_df_t> colors;
    if (str2colordfs(color, colors) == 0) {
        bgr2binary(colors);
    } else {
        bgr2binarybk(colors);
    }
}

void ImageSearchService::str2binaryfbk(const wstring &color, double sim) {
    vector<color_df_t> colors;
    vector<bool> explicit_dfs;
    if (str2colordfs(color, colors, &explicit_dfs) == 0) {
        const auto implicit_df = sim_to_point_color_diff(sim);
        for (size_t i = 0; i < colors.size() && i < explicit_dfs.size(); ++i) {
            if (!explicit_dfs[i])
                colors[i].df = implicit_df;
        }
        bgr2binary(colors);
    } else {
        bgr2binarybk(colors);
    }
}

void ImageSearchService::str2pointbinaryfbk(const wstring &color) {
    str2binaryfbk(color);
    ApplyBinaryPreprocess();
}

void ImageSearchService::str2pointbinaryfbk(const wstring &color, double sim) {
    str2binaryfbk(color, sim);
    ApplyBinaryPreprocess();
}

void ImageSearchService::ApplyBinaryPreprocess() {
    if (_binary_preprocess_mode <= 0 || _binary.empty())
        return;

    auto count_neighbors = [](const ImageBin &image, int x, int y) {
        int count = 0;
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0)
                    continue;
                const int nx = x + dx;
                const int ny = y + dy;
                if (nx < 0 || ny < 0 || nx >= image.width || ny >= image.height)
                    continue;
                if (image.at(ny, nx) == WORD_COLOR)
                    ++count;
            }
        }
        return count;
    };

    if (_binary_preprocess_mode >= 1) {
        ImageBin source = _binary;
        for (int y = 0; y < source.height; ++y) {
            for (int x = 0; x < source.width; ++x) {
                if (source.at(y, x) == WORD_COLOR &&
                    count_neighbors(source, x, y) <= _binary_isolated_threshold) {
                    _binary.at(y, x) = WORD_BKCOLOR;
                }
            }
        }
    }

    if (_binary_preprocess_mode >= 2 && _binary_min_component_area > 1) {
        ImageBin visited;
        visited.create(_binary.width, _binary.height);
        std::fill(visited.begin(), visited.end(), 0);

        std::vector<point_t> stack;
        std::vector<point_t> component;
        for (int y = 0; y < _binary.height; ++y) {
            for (int x = 0; x < _binary.width; ++x) {
                if (_binary.at(y, x) != WORD_COLOR || visited.at(y, x))
                    continue;

                stack.clear();
                component.clear();
                stack.push_back(point_t(x, y));
                visited.at(y, x) = 1;

                while (!stack.empty()) {
                    const point_t p = stack.back();
                    stack.pop_back();
                    component.push_back(p);

                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            if (dx == 0 && dy == 0)
                                continue;
                            const int nx = p.x + dx;
                            const int ny = p.y + dy;
                            if (nx < 0 || ny < 0 || nx >= _binary.width || ny >= _binary.height)
                                continue;
                            if (visited.at(ny, nx) || _binary.at(ny, nx) != WORD_COLOR)
                                continue;

                            visited.at(ny, nx) = 1;
                            stack.push_back(point_t(nx, ny));
                        }
                    }
                }

                if (static_cast<long>(component.size()) < _binary_min_component_area) {
                    for (const auto &p : component)
                        _binary.at(p.y, p.x) = WORD_BKCOLOR;
                }
            }
        }
    }

    if (_binary_preprocess_mode >= 3 && _binary_bridge_gap > 0) {
        ImageBin source = _binary;
        for (int y = 0; y < source.height; ++y) {
            for (int x = 0; x < source.width; ++x) {
                if (source.at(y, x) == WORD_COLOR)
                    continue;

                const bool bridge_x = x > 0 && x + 1 < source.width && source.at(y, x - 1) == WORD_COLOR &&
                                      source.at(y, x + 1) == WORD_COLOR;
                const bool bridge_y = y > 0 && y + 1 < source.height && source.at(y - 1, x) == WORD_COLOR &&
                                      source.at(y + 1, x) == WORD_COLOR;
                if (bridge_x || bridge_y)
                    _binary.at(y, x) = WORD_COLOR;
            }
        }
    }
}

void ImageSearchService::files2mats(const wstring &files, std::vector<Image *> &vpic, std::vector<wstring> &vstr,
                                    std::vector<std::shared_ptr<Image>> &holders) {
    // std::vector<wstring>vstr, vstr2;
    vpic.clear();
    vstr.clear();
    holders.clear();
    std::vector<wstring> names;
    split(files, names, L"|");
    wstring tp;
    for (auto &it : names) {
        // 先按原始名字查找，覆盖 LoadMemPic 名称和完整路径两种情况。
        auto image = find_cached_pic(it);
        if (!image) {
            if (!Path2GlobalPath(it, _curr_path, tp))
                continue;
            image = find_cached_pic(tp);
            if (!image) {
                image = read_pic_file(tp);
                if (!image)
                    continue;
                // 自动读取本地文件时，只有开启缓存才写入全局缓存。
                if (_enable_cache)
                    store_cached_pic(tp, image);
            }
        }

        holders.push_back(image);
        vpic.push_back(image.get());
        vstr.push_back(it);
    }
}

long ImageSearchService::OcrEx(const wstring &color, double sim, std::wstring &retstr) {
    retstr.clear();
    if (sim < 0. || sim > 1.)
        sim = 1.;
    if (_dicts[_curr_idx].size() == 0) {
        vocr_rec_t res;
        int find_ct = 0;
        HttpOcrService::getInstance()->ocr(_src.pdata, _src.width, _src.height, 4, res);
        for (auto &it : res) {
            if (it.confidence >= sim - 1e-9) {
                retstr += std::to_wstring(it.left_top.x + _x1 + _dx);
                retstr += L",";
                retstr += std::to_wstring(it.left_top.y + _y1 + _dy);
                retstr += L",";
                retstr += it.text;
                retstr += L"|";
                ++find_ct;
                if (find_ct > _max_return_obj_ct)
                    break;
            }
        }
        if (!retstr.empty() && retstr.back() == L'|')
            retstr.pop_back();
        return find_ct;
    } else {
        str2pointbinaryfbk(color, sim);
        return ImageSearchAlgorithms::OcrEx(_dicts[_curr_idx], sim, retstr);
    }
}

long ImageSearchService::FindStr(const wstring &str, const wstring &color, double sim, long &retx, long &rety) {
    vector<wstring> vstr;
    split(str, vstr, L"|");
    if (sim < 0. || sim > 1.)
        sim = 1.;
    std::map<point_t, ocr_rec_t> ocr_res;
    if (_dicts[_curr_idx].size() == 0) {
        vocr_rec_t res;
        HttpOcrService::getInstance()->ocr(_src.pdata, _src.width, _src.height, 4, res);
        for (auto &it : res) {
            if (it.confidence >= sim - 1e-9) {
                ocr_res[it.left_top] = it;
            }
        }
    } else {
        str2pointbinaryfbk(color, sim);
        ImageSearchAlgorithms::bin_ocr(_dicts[_curr_idx], sim, ocr_res);
    }
    return ImageSearchAlgorithms::FindStr(ocr_res, vstr, retx, rety);
}

long ImageSearchService::FindStrEx(const wstring &str, const wstring &color, double sim, std::wstring &out_str) {
    out_str.clear();
    vector<wstring> vstr;
    split(str, vstr, L"|");
    if (sim < 0. || sim > 1.)
        sim = 1.;
    std::map<point_t, ocr_rec_t> ocr_res;
    if (_dicts[_curr_idx].size() == 0) {
        vocr_rec_t res;
        HttpOcrService::getInstance()->ocr(_src.pdata, _src.width, _src.height, 4, res);
        for (auto &it : res) {
            if (it.confidence >= sim - 1e-9) {
                ocr_res[it.left_top] = it;
            }
        }
    } else {
        str2pointbinaryfbk(color, sim);
        ImageSearchAlgorithms::bin_ocr(_dicts[_curr_idx], sim, ocr_res);
    }
    return ImageSearchAlgorithms::FindStrEx(ocr_res, vstr, out_str);
}

long ImageSearchService::OcrAuto(double sim, std::wstring &retstr) {
    return OCR(L"", sim, retstr);
}

long ImageSearchService::OcrFromFile(const wstring &files, const wstring &color, double sim, std::wstring &retstr) {
    retstr.clear();
    if (sim < 0. || sim > 1.)
        sim = 1.;
    wstring fullpath;
    if (Path2GlobalPath(files, _curr_path, fullpath)) {
        _src.read(fullpath.data());
        return OCR(color, sim, retstr);
    }
    return 0;
}

long ImageSearchService::OcrAutoFromFile(const wstring &files, double sim, std::wstring &retstr) {
    retstr.clear();
    if (sim < 0. || sim > 1.)
        sim = 1.;
    wstring fullpath;

    if (Path2GlobalPath(files, _curr_path, fullpath)) {
        _src.read(fullpath.data());
        return OCR(L"", sim, retstr);
    }
    return 0;
}

long ImageSearchService::FindLine(const wstring &color, double sim, wstring &retStr) {
    retStr.clear();
    if (sim < 0. || sim > 1.)
        sim = 1.;
    str2binaryfbk(color, sim);
    return ImageSearchAlgorithms::FindLine(retStr);
}

} // namespace op::image
