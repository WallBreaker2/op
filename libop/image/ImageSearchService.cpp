// #include "stdafx.h"
#include "ImageSearchService.h"
#include "../runtime/RuntimeUtils.h"
#include "../ocr/OcrService.h"
#include <algorithm>
#include <bitset>
#include <cmath>
#include <fstream>
#include <sstream>

namespace op::image {

using op::ocr::HttpOcrService;

namespace {
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
} // namespace

ImageSearchService::ImageSearchService() {
    _curr_idx = 0;
    _enable_cache = 1;
}

ImageSearchService::~ImageSearchService() {
}

long ImageSearchService::Capture(const std::wstring &file) {
    wstring fpath = file;
    if (fpath.find(L'\\') == -1)
        fpath = _curr_path + L"\\" + fpath;

    return _src.write(fpath.data());
}

long ImageSearchService::CmpColor(long x, long y, const std::wstring &scolor, double sim) {
    std::vector<color_df_t> vcolor;
    str2colordfs(scolor, vcolor);
    return ImageSearchAlgorithms::CmpColor(_src.at<color_t>(0, 0), vcolor, sim);
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
    pt_cr_df_t tp;
    for (auto &it : vseconds) {
        size_t id1, id2;
        id1 = it.find(L'|');
        id2 = (id1 == wstring::npos ? wstring::npos : it.find(L'|', id1));
        if (id2 != wstring::npos) {
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
    pt_cr_df_t tp;
    for (auto &it : vseconds) {
        size_t id1, id2;
        id1 = it.find(L'|');
        id2 = (id1 == wstring::npos ? wstring::npos : it.find(L'|', id1));
        if (id2 != wstring::npos) {
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
    // vector<color_t> vcolor;
    color_t dfcolor;
    vector<std::wstring> vpic_name;
    files2mats(files, vpic, vpic_name);
    dfcolor.str2color(delta_colors);
    // str2colors(delta_colors, vcolor);
    sim = 0.5 + sim / 2;
    // long ret = ImageSearchAlgorithms::FindPic(vpic, dfcolor, sim, x, y);
    long ret = ImageSearchAlgorithms::FindPicTh(vpic, dfcolor, sim, dir, x, y);
    // 清理缓存
    if (!_enable_cache)
        _pic_cache.clear();
    return ret;
}
//
long ImageSearchService::FindPicEx(const std::wstring &files, const wstring &delta_colors, double sim, long dir, wstring &retstr,
                          bool returnID) {
    vector<Image *> vpic;
    vpoint_desc_t vpd;
    // vector<color_t> vcolor;
    color_t dfcolor;
    vector<std::wstring> vpic_name;
    files2mats(files, vpic, vpic_name);
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
    // 清理缓存
    if (!_enable_cache)
        _pic_cache.clear();
    retstr = ss.str();
    if (vpd.size())
        retstr.pop_back();
    return ret;
}

long ImageSearchService::FindColorBlock(const wstring &color, double sim, long count, long height, long width, long &x,
                               long &y) {
    str2binaryfbk(color);
    return ImageSearchAlgorithms::FindColorBlock(sim, count, height, width, x, y);
}

long ImageSearchService::FindColorBlockEx(const wstring &color, double sim, long count, long height, long width,
                                 wstring &retstr) {
    str2binaryfbk(color);
    return ImageSearchAlgorithms::FindColorBlockEx(sim, count, height, width, retstr);
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

wstring ImageSearchService::FetchWord(rect_t rc, const wstring &color, const wstring &word) {
    str2binaryfbk(color);
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
        str2binaryfbk(color, sim);
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
    // std::vector<wstring>vstr, vstr2;
    std::vector<wstring> vstr;
    int loaded = 0;
    split(files, vstr, L"|");
    wstring tp;
    for (auto &it : vstr) {
        // 路径转化
        if (!Path2GlobalPath(it, _curr_path, tp))
            continue;
        // 先在缓存中查找
        if (!_pic_cache.count(tp)) {
            _pic_cache[tp].read(tp.data());
        }
        // 已存在于缓存中的文件也算加载成功
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
        // 看当前目录
        auto cache_it = _pic_cache.find(it);
        // 没查到再看一下资源目录
        if (cache_it == _pic_cache.end()) {
            cache_it = _pic_cache.find(_curr_path + L"\\" + it);
        }
        // 查到了就释放
        if (cache_it != _pic_cache.end()) {
            cache_it->second.release();
            _pic_cache.erase(cache_it);
            loaded++;
        }
    }
    return loaded;
}

long ImageSearchService::LoadMemPic(const wstring &file_name, void *data, long size) {
    try {
        if (!_pic_cache.count(file_name)) {
            _pic_cache[file_name].read(data, size);
        }
    } catch (...) {
        return 0;
    }
    return 1;
}

long ImageSearchService::GetPicSize(const wstring &file_name, long *width, long *height) {
    // 看当前目录
    auto cache_it = _pic_cache.find(file_name);
    // 没查到再看一下资源目录
    if (cache_it == _pic_cache.end()) {
        cache_it = _pic_cache.find(_curr_path + L"\\" + file_name);
    }
    // 查到了就释放
    if (cache_it != _pic_cache.end()) {
        if (!set_out(width, cache_it->second.width) || !set_out(height, cache_it->second.height))
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

void ImageSearchService::files2mats(const wstring &files, std::vector<Image *> &vpic, std::vector<wstring> &vstr) {
    // std::vector<wstring>vstr, vstr2;
    Image *pm;
    vpic.clear();
    split(files, vstr, L"|");
    wstring tp;
    for (auto &it : vstr) {
        // 先在缓存中查找是否已加载，包括从内存中加载的文件
        if (_pic_cache.count(it)) {
            pm = &_pic_cache[it];
        } else {
            // 路径转化
            if (!Path2GlobalPath(it, _curr_path, tp))
                continue;
            // 再检测一次，包括绝对路径的文件
            if (_pic_cache.count(tp)) {
                pm = &_pic_cache[tp];
            } else {
                _pic_cache[tp].read(tp.data());
                pm = &_pic_cache[tp];
            }
        }
        vpic.push_back(pm);
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
        str2binaryfbk(color, sim);
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
        str2binaryfbk(color, sim);
        ImageSearchAlgorithms::bin_ocr(_dicts[_curr_idx], sim, ocr_res);
    }
    return ImageSearchAlgorithms::FindStr(ocr_res, vstr, sim, retx, rety);
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
        str2binaryfbk(color, sim);
        ImageSearchAlgorithms::bin_ocr(_dicts[_curr_idx], sim, ocr_res);
    }
    return ImageSearchAlgorithms::FindStrEx(ocr_res, vstr, sim, out_str);
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
    str2binaryfbk(color);
    if (sim < 0. || sim > 1.)
        sim = 1.;
    return ImageSearchAlgorithms::FindLine(sim, retStr);
}

} // namespace op::image
