#include "OpContext.h"
#include "OpResult.h"

#include "ocr/OcrService.h"
#include "runtime/RuntimeUtils.h"

#include <libop.h>

#include <cwctype>
#include <cwchar>
#include <string>
#include <vector>

namespace {

bool parse_word_result_item(const wchar_t *begin, const wchar_t *end, long &x, long &y, const wchar_t **word_sep) {
    if (!begin || !end || begin >= end)
        return false;

    wchar_t *parse_end = nullptr;
    const long parsed_x = wcstol(begin, &parse_end, 10);
    if (parse_end == begin || parse_end >= end || *parse_end != L',')
        return false;

    const wchar_t *y_begin = parse_end + 1;
    const long parsed_y = wcstol(y_begin, &parse_end, 10);
    if (parse_end == y_begin || parse_end >= end || *parse_end != L'-')
        return false;

    x = parsed_x;
    y = parsed_y;
    if (word_sep)
        *word_sep = parse_end;
    return true;
}

void append_word_rects(const std::vector<op::rect_t> &rects, long offset_x, long offset_y, std::wstring &out) {
    out.clear();
    for (const auto &rc : rects) {
        out += std::to_wstring(rc.x1 + offset_x);
        out += L",";
        out += std::to_wstring(rc.y1 + offset_y);
        out += L",";
        out += std::to_wstring(rc.x2 + offset_x);
        out += L",";
        out += std::to_wstring(rc.y2 + offset_y);
        out += L"|";
    }
    if (!out.empty())
        out.pop_back();
}

void skip_spaces(const wchar_t *&p) {
    while (*p && iswspace(*p))
        ++p;
}

bool read_rect_value(const wchar_t *&p, long &value) {
    skip_spaces(p);
    wchar_t *end = nullptr;
    const long parsed = wcstol(p, &end, 10);
    if (end == p)
        return false;
    value = parsed;
    p = end;
    skip_spaces(p);
    return true;
}

bool parse_word_rects(const wchar_t *text, long offset_x, long offset_y, long width, long height,
                      std::vector<op::rect_t> &rects) {
    rects.clear();
    if (!text || !*text)
        return false;

    const wchar_t *p = text;
    while (*p) {
        long values[4] = {};
        for (int i = 0; i < 4; ++i) {
            if (!read_rect_value(p, values[i]))
                return false;
            if (i < 3) {
                if (*p != L',')
                    return false;
                ++p;
            }
        }

        const long local_x1 = values[0] - offset_x;
        const long local_y1 = values[1] - offset_y;
        const long local_x2 = values[2] - offset_x;
        const long local_y2 = values[3] - offset_y;
        if (local_x1 < 0 || local_y1 < 0 || local_x1 >= local_x2 || local_y1 >= local_y2 || local_x2 > width ||
            local_y2 > height) {
            rects.clear();
            return false;
        }

        rects.emplace_back(static_cast<int>(local_x1), static_cast<int>(local_y1), static_cast<int>(local_x2),
                           static_cast<int>(local_y2));

        skip_spaces(p);
        if (*p == L'|') {
            ++p;
            skip_spaces(p);
            if (!*p)
                return false;
        } else if (*p) {
            return false;
        }
    }

    return !rects.empty();
}

double normalize_similarity(double sim) {
    return sim < 0. || sim > 1. ? 1. : sim;
}

template <typename Fn>
void capture_converted_region(op::internal::OpContext *context, long x1, long y1, long x2, long y2, Fn fn) {
    if (!context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, context->image_proc._src)) {
        setlog("error requestCapture");
        return;
    }

    context->image_proc.set_offset(x1, y1);
    fn();
}

template <typename Fn>
void with_captured_region(op::internal::OpContext *context, long &x1, long &y1, long &x2, long &y2, Fn fn) {
    if (!context->bkproc.check_bind() || !context->bkproc.RectConvert(x1, y1, x2, y2))
        return;

    capture_converted_region(context, x1, y1, x2, y2, fn);
}

} // namespace

long op::Op::SetOcrEngine(const wchar_t *path_of_engine, const wchar_t *dll_name, const wchar_t *argv) {
    string argvs = argv ? _ws2string(argv) : "";
    vector<string> vstr;
    split(argvs, vstr, " ");
    const std::wstring engine = path_of_engine ? path_of_engine : L"";
    const std::wstring dll = dll_name ? dll_name : L"";
    return op::ocr::HttpOcrService::getInstance()->init(engine, dll, vstr) == 0 ? 1 : 0;
}
void op::Op::SetDict(long idx, const wchar_t *file_name, long *ret) {
    internal::set_result(ret, m_context->image_proc.SetDict(idx, file_name));
}

void op::Op::GetDict(long idx, long font_index, std::wstring &retstr) {
    retstr = m_context->image_proc.GetDict(idx, font_index);
}

// 设置内存字库文件
void op::Op::SetMemDict(long idx, const wchar_t *data, long size, long *ret) {
    internal::set_result(ret, m_context->image_proc.SetMemDict(idx, (void *)data, size));
}

// 使用哪个字库文件进行识别
void op::Op::UseDict(long idx, long *ret) {
    internal::set_result(ret, m_context->image_proc.UseDict(idx));
}

// 给指定的字库中添加一条字库信息
void op::Op::AddDict(long idx, const wchar_t *dict_info, long *ret) {
    internal::set_result(ret, m_context->image_proc.AddDict(idx, dict_info));
}
void op::Op::SaveDict(long idx, const wchar_t *file_name, long *ret) {
    internal::set_result(ret, m_context->image_proc.SaveDict(idx, file_name));
}
// 清空指定的字库
void op::Op::ClearDict(long idx, long *ret) {
    internal::set_result(ret, m_context->image_proc.ClearDict(idx));
}
// 获取指定的字库中的字符数量
void op::Op::GetDictCount(long idx, long *ret) {
    internal::set_result(ret, m_context->image_proc.GetDictCount(idx));
}
// 获取当前使用的字库序号
void op::Op::GetNowDict(long *ret) {
    internal::set_result(ret, m_context->image_proc.GetNowDict());
}

void op::Op::SetBinaryPreprocess(long mode, long isolated_threshold, long min_component_area, long bridge_gap,
                                 long *ret) {
    internal::set_result(ret,
                         m_context->image_proc.SetBinaryPreprocess(mode, isolated_threshold, min_component_area,
                                                                    bridge_gap));
}

void op::Op::GetBinaryPreprocess(long *mode, long *isolated_threshold, long *min_component_area, long *bridge_gap,
                                 long *ret) {
    long local_mode = 0;
    long local_isolated_threshold = 0;
    long local_min_component_area = 0;
    long local_bridge_gap = 0;
    const long result = m_context->image_proc.GetBinaryPreprocess(local_mode, local_isolated_threshold,
                                                                  local_min_component_area, local_bridge_gap);
    internal::set_result(mode, local_mode);
    internal::set_result(isolated_threshold, local_isolated_threshold);
    internal::set_result(min_component_area, local_min_component_area);
    internal::set_result(bridge_gap, local_bridge_gap);
    internal::set_result(ret, result);
}
// 根据指定的范围,以及指定的颜色描述，提取点阵信息，类似于大漠工具里的单独提取
void op::Op::FetchWord(long x1, long y1, long x2, long y2, const wchar_t *color, const wchar_t *word,
                      std::wstring &retstr) {
    FetchWordEx(x1, y1, x2, y2, color, 1.0, word, retstr);
}

void op::Op::FetchWordEx(long x1, long y1, long x2, long y2, const wchar_t *color, double sim, const wchar_t *word,
                         std::wstring &retstr) {
    retstr.clear();
    const std::wstring color_text = color ? color : L"";
    const std::wstring word_text = word ? word : L"";
    sim = normalize_similarity(sim);

    with_captured_region(m_context.get(), x1, y1, x2, y2, [&]() {
        rect_t rc;
        rc.x1 = rc.y1 = 0;
        rc.x2 = x2 - x1;
        rc.y2 = y2 - y1;
        retstr = m_context->image_proc.FetchWord(rc, color_text, sim, word_text);
    });
}

void op::Op::ExtractWordRects(long x1, long y1, long x2, long y2, const wchar_t *color, double sim, long min_word_h,
                              std::wstring &retstr) {
    retstr.clear();
    const std::wstring color_text = color ? color : L"";
    sim = normalize_similarity(sim);

    with_captured_region(m_context.get(), x1, y1, x2, y2, [&]() {
        std::vector<rect_t> rects;
        m_context->image_proc.ExtractWordRects(color_text, sim, min_word_h, rects);
        append_word_rects(rects, x1, y1, retstr);
    });
}

void op::Op::ExtractWordRectsEx(long x1, long y1, long x2, long y2, const wchar_t *color, double sim, long min_word_w,
                                long min_word_h, long padding, std::wstring &retstr) {
    retstr.clear();
    const std::wstring color_text = color ? color : L"";
    sim = normalize_similarity(sim);

    with_captured_region(m_context.get(), x1, y1, x2, y2, [&]() {
        std::vector<rect_t> rects;
        m_context->image_proc.ExtractWordRectsEx(color_text, sim, min_word_w, min_word_h, padding, rects);
        append_word_rects(rects, x1, y1, retstr);
    });
}

void op::Op::FetchWords(long x1, long y1, long x2, long y2, const wchar_t *color, double sim, const wchar_t *words,
                       long min_word_h, std::wstring &retstr) {
    retstr.clear();
    const std::wstring color_text = color ? color : L"";
    const std::wstring word_text = words ? words : L"";
    sim = normalize_similarity(sim);

    with_captured_region(m_context.get(), x1, y1, x2, y2, [&]() {
        m_context->image_proc.FetchWords(color_text, sim, word_text, min_word_h, retstr);
    });
}

void op::Op::FetchWordsEx(long x1, long y1, long x2, long y2, const wchar_t *color, double sim, const wchar_t *words,
                          long min_word_w, long min_word_h, long padding, std::wstring &retstr) {
    retstr.clear();
    const std::wstring color_text = color ? color : L"";
    const std::wstring word_text = words ? words : L"";
    sim = normalize_similarity(sim);

    with_captured_region(m_context.get(), x1, y1, x2, y2, [&]() {
        m_context->image_proc.FetchWordsEx(color_text, sim, word_text, min_word_w, min_word_h, padding, retstr);
    });
}

void op::Op::FetchWordsByRects(long x1, long y1, long x2, long y2, const wchar_t *color, double sim,
                               const wchar_t *words, const wchar_t *rects, std::wstring &retstr) {
    retstr.clear();
    const std::wstring color_text = color ? color : L"";
    const std::wstring word_text = words ? words : L"";
    sim = normalize_similarity(sim);

    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        std::vector<rect_t> local_rects;
        if (!parse_word_rects(rects, x1, y1, x2 - x1, y2 - y1, local_rects))
            return;
        if (local_rects.size() != word_text.size())
            return;

        capture_converted_region(m_context.get(), x1, y1, x2, y2, [&]() {
            m_context->image_proc.FetchWordsByRects(color_text, sim, word_text, local_rects, retstr);
        });
    }
}

void op::Op::GetBinaryPreview(long x1, long y1, long x2, long y2, const wchar_t *color, double sim,
                              std::wstring &retstr, long *ret) {
    retstr.clear();
    internal::set_result(ret, 0L);
    const std::wstring color_text = color ? color : L"";
    sim = normalize_similarity(sim);

    with_captured_region(m_context.get(), x1, y1, x2, y2, [&]() {
        internal::set_result(ret, m_context->image_proc.GetBinaryPreview(color_text, sim, retstr));
    });
}

void op::Op::GetWordPreview(const wchar_t *dict_info, std::wstring &retstr, long *ret) {
    retstr.clear();
    internal::set_result(ret, m_context->image_proc.GetWordPreview(dict_info ? dict_info : L"", retstr));
}

void op::Op::CheckWordDict(const wchar_t *dict_info, std::wstring &retstr, long *ret) {
    retstr.clear();
    internal::set_result(ret, m_context->image_proc.CheckWordDict(dict_info ? dict_info : L"", retstr));
}

void op::Op::NormalizeWordDict(const wchar_t *dict_info, std::wstring &retstr, long *ret) {
    retstr.clear();
    internal::set_result(ret, m_context->image_proc.NormalizeWordDict(dict_info ? dict_info : L"", retstr));
}

void op::Op::RenameWordDict(const wchar_t *dict_info, const wchar_t *words, std::wstring &retstr, long *ret) {
    retstr.clear();
    internal::set_result(ret,
                         m_context->image_proc.RenameWordDict(dict_info ? dict_info : L"", words ? words : L"", retstr));
}

// 识别这个范围内所有满足条件的词组，这个识别函数不会用到字库. 只是识别大概形状的位置
void op::Op::GetWordsNoDict(long x1, long y1, long x2, long y2, const wchar_t *color, std::wstring &retstr) {
    wstring str;
    const std::wstring color_text = color ? color : L"";
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            m_context->image_proc.str2binaryfbk(color_text);
            std::vector<rect_t> vroi;
            m_context->image_proc.get_rois(5, vroi);
            for (auto &it : vroi) {
                const wstring tempWord = m_context->image_proc.FetchWord(it, color_text, L"");
                str += std::to_wstring(it.x1);
                str += L",";
                str += std::to_wstring(it.y1);
                str += L"-";
                str += tempWord;
                str += L"/";
            }
        }
    }
    retstr = str;
}
// 在使用GetWords进行词组识别以后,可以用此接口进行识别词组数量的计算
void op::Op::GetWordResultCount(const wchar_t *result, long *ret) {
    internal::set_result(ret, 0L);
    if (!result || !ret)
        return;

    long cnt = 0;
    const wchar_t *p = result;
    while (*p) {
        if (*p == L'/')
            ++cnt;
        ++p;
    }
    internal::set_result(ret, cnt);
}
// 在使用GetWords进行词组识别以后,可以用此接口进行识别各个词组的坐标
void op::Op::GetWordResultPos(const wchar_t *result, long index, long *x, long *y, long *ret) {
    internal::set_result(ret, 0L);
    internal::set_result(x, 0L);
    internal::set_result(y, 0L);
    if (!result || index < 0)
        return;

    // GetWordsNoDict 的结果格式为: x,y-word/x,y-word/
    long cnt = 0;
    const wchar_t *p = result;
    while (*p && cnt <= index) {
        const wchar_t *item_end = wcschr(p, L'/');
        if (!item_end)
            item_end = p + wcslen(p);

        if (index == cnt) {
            long parsed_x = 0;
            long parsed_y = 0;
            if (parse_word_result_item(p, item_end, parsed_x, parsed_y, nullptr)) {
                internal::set_result(x, parsed_x);
                internal::set_result(y, parsed_y);
                internal::set_result(ret, 1L);
            }
            return;
        }

        if (*item_end == L'\0')
            return;
        p = item_end + 1;
        ++cnt;
    }
}
// 在使用GetWords进行词组识别以后,可以用此接口进行识别各个词组的内容
void op::Op::GetWordResultStr(const wchar_t *result, long index, std::wstring &ret_str) {
    ret_str.clear();
    if (!result || index < 0)
        return;

    // 坏格式直接返回空字符串，避免越过字符串结尾读取。
    long cnt = 0;
    const wchar_t *p = result;
    while (*p && cnt <= index) {
        const wchar_t *item_end = wcschr(p, L'/');
        if (!item_end)
            item_end = p + wcslen(p);

        if (index == cnt) {
            long parsed_x = 0;
            long parsed_y = 0;
            const wchar_t *sep = nullptr;
            if (parse_word_result_item(p, item_end, parsed_x, parsed_y, &sep))
                ret_str.assign(sep + 1, item_end);
            return;
        }

        if (*item_end == L'\0')
            return;
        p = item_end + 1;
        ++cnt;
    }
}
// 识别屏幕范围(x1,y1,x2,y2)内符合color_format的字符串,并且相似度为sim,sim取值范围(0.1-1.0),
void op::Op::Ocr(long x1, long y1, long x2, long y2, const wchar_t *color, double sim, std::wstring &retstr) {
    wstring str;
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            m_context->image_proc.OCR(color, sim, str);
        }
    }
    retstr = str;
}
// 回识别到的字符串，以及每个字符的坐标.
void op::Op::OcrEx(long x1, long y1, long x2, long y2, const wchar_t *color, double sim, std::wstring &retstr) {
    wstring str;
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            m_context->image_proc.OcrEx(color, sim, str);
        }
    }
    retstr = str;
}
// 在屏幕范围(x1,y1,x2,y2)内,查找string(可以是任意个字符串的组合),并返回符合color_format的坐标位置
void op::Op::FindStr(long x1, long y1, long x2, long y2, const wchar_t *strs, const wchar_t *color, double sim,
                    long *retx, long *rety, long *ret) {
    wstring str;
    long x = -1;
    long y = -1;
    internal::set_result(retx, x);
    internal::set_result(rety, y);
    internal::set_result(ret, 0L);
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            internal::set_result(ret, m_context->image_proc.FindStr(strs, color, sim, x, y));
            internal::set_result(retx, x);
            internal::set_result(rety, y);
        }
    }
}
// 返回符合color_format的所有坐标位置
void op::Op::FindStrEx(long x1, long y1, long x2, long y2, const wchar_t *strs, const wchar_t *color, double sim,
                      std::wstring &retstr) {
    wstring str;
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            m_context->image_proc.FindStrEx(strs, color, sim, str);
        }
    }
    retstr = str;
}

void op::Op::OcrAuto(long x1, long y1, long x2, long y2, double sim, std::wstring &retstr) {
    wstring str;
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            m_context->image_proc.OcrAuto(sim, str);
        }
    }
    retstr = str;
}

// 从文件中识别图片
void op::Op::OcrFromFile(const wchar_t *file_name, const wchar_t *color_format, double sim, std::wstring &retstr) {
    wstring str;
    m_context->image_proc.OcrFromFile(file_name, color_format, sim, str);
    retstr = str;
}
// 从文件中识别图片,无需指定颜色
void op::Op::OcrAutoFromFile(const wchar_t *file_name, double sim, std::wstring &retstr) {
    wstring str;
    m_context->image_proc.OcrAutoFromFile(file_name, sim, str);
    retstr = str;
}

void op::Op::FindLine(long x1, long y1, long x2, long y2, const wchar_t *color, double sim, wstring &retstr) {
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            m_context->image_proc.FindLine(color, sim, retstr);
        }
    }
}
