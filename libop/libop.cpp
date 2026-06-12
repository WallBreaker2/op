// OpInterface.cpp: OpInterface 的实现

#include "libop.h"
#include "./ImageProc/ImageProc.h"
#include "./background/opBackground.h"
#include "./core/Cmder.h"
#include "./core/globalVar.h"
#include "./core/helpfunc.h"
#include "./core/opEnv.h"
#include "./core/optype.h"
#include "./core/window_layout.h"
#include "./imageProc/OcrWrapper.h"
#include "./opencv/OpenCvBridge.h"
#include "./opencv/OpenCvModule.h"
#include "./winapi/Injecter.h"
#include "./winapi/WinApi.h"
#include <tchar.h>

#include "./algorithm/AStar.hpp"
#include "./winapi/MemoryEx.h"
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <utility>
#include <vector>

#undef FindWindow
#undef FindWindowEx
#undef SetWindowText

const int small_block_size = 10;

std::atomic<int> libop::s_id(0);
const int SC_DATA_TOP = 0;
const int SC_DATA_BOTTOM = 1;

namespace {

struct key_combo_t {
    long vk = 0;
    std::vector<long> modifiers;
};

// 先按名称键查表，例如 ctrl、enter、f1。
bool is_named_vk(const std::map<std::wstring, long> &vkmap, const wchar_t *text, long &vk) {
    if (text == nullptr || text[0] == L'\0')
        return false;

    std::wstring key = text;
    wstring2lower(key);
    auto it = vkmap.find(key);
    if (it == vkmap.end())
        return false;

    vk = it->second;
    return true;
}

// 把单个字符拆成“主键 + 修饰键”组合。
bool resolve_char_key_combo(const wchar_t ch, key_combo_t &combo) {
    const SHORT mapped = ::VkKeyScanW(ch);
    if (mapped == -1)
        return false;

    combo = {};
    combo.vk = LOBYTE(mapped);

    const BYTE shift_state = HIBYTE(mapped);
    if (shift_state & 1)
        combo.modifiers.push_back(VK_SHIFT);
    if (shift_state & 2)
        combo.modifiers.push_back(VK_CONTROL);
    if (shift_state & 4)
        combo.modifiers.push_back(VK_MENU);

    return combo.vk != 0;
}

// 统一解析字符串按键，优先支持命名键，其次支持单字符。
bool resolve_text_key_combo(const std::map<std::wstring, long> &vkmap, const wchar_t *text, key_combo_t &combo) {
    long named_vk = 0;
    if (is_named_vk(vkmap, text, named_vk)) {
        combo = {};
        combo.vk = named_vk;
        return true;
    }

    if (text == nullptr || text[0] == L'\0' || text[1] != L'\0')
        return false;

    return resolve_char_key_combo(text[0], combo);
}

long key_combo_down(bkkeypad *keypad, const key_combo_t &combo) {
    // 先按修饰键，再按主键。
    for (long modifier : combo.modifiers) {
        if (keypad->KeyDown(modifier) != 1)
            return 0;
    }
    return keypad->KeyDown(combo.vk);
}

long key_combo_up(bkkeypad *keypad, const key_combo_t &combo) {
    long ret = keypad->KeyUp(combo.vk);
    if (ret != 1)
        return ret;

    // 抬键时反向释放修饰键。
    for (auto it = combo.modifiers.rbegin(); it != combo.modifiers.rend(); ++it) {
        if (keypad->KeyUp(*it) != 1)
            return 0;
    }
    return 1;
}

// 一次性完成组合键按下和释放。
long key_combo_press(bkkeypad *keypad, const key_combo_t &combo) {
    // 无修饰键时直接复用原有按键节奏，避免改变不同模式的时序。
    if (combo.modifiers.empty())
        return keypad->KeyPress(combo.vk);

    if (key_combo_down(keypad, combo) != 1)
        return 0;
    return key_combo_up(keypad, combo);
}

bool parse_layout_type(long value, window_layout::Type &type) {
    switch (value) {
    case 0:
        type = window_layout::Type::Grid;
        return true;
    case 1:
        type = window_layout::Type::Diagonal;
        return true;
    default:
        return false;
    }
}

bool parse_size_mode(long value, window_layout::SizeMode &mode) {
    switch (value) {
    case 0:
        mode = window_layout::SizeMode::Keep;
        return true;
    case 1:
        mode = window_layout::SizeMode::Uniform;
        return true;
    default:
        return false;
    }
}

bool parse_anchor_mode(long value, window_layout::AnchorMode &mode) {
    switch (value) {
    case 0:
        mode = window_layout::AnchorMode::Window;
        return true;
    case 1:
        mode = window_layout::AnchorMode::Client;
        return true;
    default:
        return false;
    }
}

bool parse_window_list(const wchar_t *hwnds, std::vector<HWND> &windows) {
    if (hwnds == nullptr || hwnds[0] == L'\0')
        return false;

    std::vector<std::wstring> items;
    split(hwnds, items, L"|");
    if (items.empty())
        return false;

    windows.clear();
    windows.reserve(items.size());
    for (const auto &item : items) {
        wchar_t *end = nullptr;
        const auto value = _wcstoi64(item.c_str(), &end, 0);
        if (end == item.c_str() || (end && *end != L'\0'))
            return false;
        windows.push_back(reinterpret_cast<HWND>(static_cast<LONG_PTR>(value)));
    }

    return !windows.empty();
}

std::wstring decode_command_output(const std::string &text) {
    if (text.empty())
        return L"";

    const int utf8_len =
        ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (utf8_len > 0) {
        std::wstring out(utf8_len, L'\0');
        ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), out.data(),
                              utf8_len);
        return out;
    }

    return _s2wstring(text);
}

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

bool parse_cv_threshold_mode(const wchar_t *mode_text, opcv::ThresholdMode &mode) {
    std::wstring value = mode_text ? mode_text : L"";
    wstring2lower(value);

    if (value == L"binary" || value.empty()) {
        mode = opcv::ThresholdMode::Binary;
    } else if (value == L"binary_inv" || value == L"inv") {
        mode = opcv::ThresholdMode::BinaryInv;
    } else if (value == L"otsu") {
        mode = opcv::ThresholdMode::Otsu;
    } else if (value == L"otsu_inv") {
        mode = opcv::ThresholdMode::OtsuInv;
    } else if (value == L"adaptive") {
        mode = opcv::ThresholdMode::Adaptive;
    } else if (value == L"adaptive_inv") {
        mode = opcv::ThresholdMode::AdaptiveInv;
    } else {
        return false;
    }
    return true;
}

bool parse_cv_color_space(const wchar_t *space_text, opcv::InRangeColorSpace &color_space) {
    std::wstring value = space_text ? space_text : L"";
    wstring2lower(value);

    if (value == L"bgr" || value.empty()) {
        color_space = opcv::InRangeColorSpace::Bgr;
    } else if (value == L"hsv") {
        color_space = opcv::InRangeColorSpace::Hsv;
    } else if (value == L"gray" || value == L"grey") {
        color_space = opcv::InRangeColorSpace::Gray;
    } else {
        return false;
    }
    return true;
}

bool parse_cv_morphology_mode(const wchar_t *mode_text, opcv::MorphologyMode &mode) {
    std::wstring value = mode_text ? mode_text : L"";
    wstring2lower(value);

    if (value == L"erode") {
        mode = opcv::MorphologyMode::Erode;
    } else if (value == L"dilate") {
        mode = opcv::MorphologyMode::Dilate;
    } else if (value == L"open" || value.empty()) {
        mode = opcv::MorphologyMode::Open;
    } else if (value == L"close") {
        mode = opcv::MorphologyMode::Close;
    } else {
        return false;
    }
    return true;
}

bool parse_cv_thin_mode(const wchar_t *mode_text, opcv::ThinMode &mode) {
    std::wstring value = mode_text ? mode_text : L"";
    wstring2lower(value);

    if (value == L"zhang_suen" || value == L"zhangsuen" || value.empty()) {
        mode = opcv::ThinMode::ZhangSuen;
    } else if (value == L"guo_hall" || value == L"guohall") {
        mode = opcv::ThinMode::GuoHall;
    } else if (value == L"morph") {
        mode = opcv::ThinMode::Morph;
    } else {
        return false;
    }
    return true;
}

bool parse_cv_blur_mode(const wchar_t *mode_text, opcv::BlurMode &mode) {
    std::wstring value = mode_text ? mode_text : L"";
    wstring2lower(value);

    if (value == L"gaussian" || value.empty()) {
        mode = opcv::BlurMode::Gaussian;
    } else if (value == L"median") {
        mode = opcv::BlurMode::Median;
    } else if (value == L"bilateral") {
        mode = opcv::BlurMode::Bilateral;
    } else if (value == L"box" || value == L"mean") {
        mode = opcv::BlurMode::Box;
    } else {
        return false;
    }
    return true;
}

bool parse_cv_number_list(const wchar_t *text, std::vector<double> &values) {
    values.clear();
    if (text == nullptr || text[0] == L'\0') {
        return false;
    }

    // 支持 "1,2,3" 和 "1|2|3"，方便 COM/Python 侧直接传字符串。
    std::wstring normalized = text;
    replacew(normalized, L"|", L",");

    std::vector<std::wstring> parts;
    split(normalized, parts, L",");
    for (const auto &part : parts) {
        if (part.empty()) {
            return false;
        }

        try {
            size_t parsed = 0;
            const double value = std::stod(part, &parsed);
            if (parsed != part.size()) {
                return false;
            }
            values.push_back(value);
        } catch (...) {
            return false;
        }
    }
    return !values.empty();
}

template <typename Preprocess>
void run_cv_file_preprocess(const wchar_t *src_file, const wchar_t *dst_file, long *ret, Preprocess &&preprocess) {
    *ret = 0;
    if (src_file == nullptr || dst_file == nullptr) {
        return;
    }

    opcv::ImageHandle source;
    opcv::ImageHandle output;
    if (!opcv::LoadImageFromFile(src_file, source) || !preprocess(source, output) ||
        !opcv::SaveImageToFile(output, dst_file)) {
        return;
    }

    *ret = 1;
}

std::wstring build_cv_components_json(const std::vector<opcv::RegionAnalysisResult> &results, bool ok) {
    std::wostringstream oss;
    oss << L"{\"ok\":" << (ok ? 1 : 0) << L",\"results\":[";
    for (size_t i = 0; i < results.size(); ++i) {
        if (i != 0) {
            oss << L",";
        }
        const auto &item = results[i];
        oss << L"{\"x\":" << item.x << L",\"y\":" << item.y << L",\"width\":" << item.width
            << L",\"height\":" << item.height << L",\"area\":" << static_cast<long long>(item.area) << L"}";
    }
    oss << L"]}";
    return oss.str();
}

std::wstring build_cv_contours_json(const std::vector<opcv::ContourAnalysisResult> &results, bool ok) {
    std::wostringstream oss;
    oss << L"{\"ok\":" << (ok ? 1 : 0) << L",\"results\":[";
    for (size_t i = 0; i < results.size(); ++i) {
        if (i != 0) {
            oss << L",";
        }
        const auto &item = results[i];
        oss << L"{\"x\":" << item.x << L",\"y\":" << item.y << L",\"width\":" << item.width
            << L",\"height\":" << item.height << L",\"area\":" << static_cast<long long>(item.area)
            << L",\"perimeter\":" << static_cast<long long>(item.perimeter) << L",\"points\":" << item.points
            << L"}";
    }
    oss << L"]}";
    return oss.str();
}

bool parse_cv_pipeline_step(const std::wstring &step, std::wstring &name, std::vector<std::wstring> &args) {
    name.clear();
    args.clear();
    if (step.empty()) {
        return false;
    }

    const size_t colon = step.find(L':');
    name = colon == std::wstring::npos ? step : step.substr(0, colon);
    wstring2lower(name);
    if (name.empty()) {
        return false;
    }

    if (colon != std::wstring::npos && colon + 1 < step.size()) {
        split(step.substr(colon + 1), args, L",");
    }
    return true;
}

bool parse_cv_pipeline_double_arg(const std::vector<std::wstring> &args, size_t index, double default_value,
                                  double &value) {
    value = default_value;
    if (index >= args.size() || args[index].empty()) {
        return true;
    }

    try {
        size_t parsed = 0;
        value = std::stod(args[index], &parsed);
        return parsed == args[index].size();
    } catch (...) {
        return false;
    }
}

bool parse_cv_pipeline_long_arg(const std::vector<std::wstring> &args, size_t index, long default_value, long &value) {
    value = default_value;
    if (index >= args.size() || args[index].empty()) {
        return true;
    }

    try {
        size_t parsed = 0;
        value = std::stol(args[index], &parsed);
        return parsed == args[index].size();
    } catch (...) {
        return false;
    }
}

bool cv_pipeline_apply_step(const std::wstring &name, const std::vector<std::wstring> &args,
                            const opcv::ImageHandle &source, opcv::ImageHandle &output) {
    // 每一步只接收当前图像并输出下一张图，保证流水线状态简单清晰。
    if (name == L"gray" || name == L"togray" || name == L"to_gray" || name == L"cvtogray") {
        return opcv::ToGray(source, output);
    }
    if (name == L"binary" || name == L"tobinary" || name == L"to_binary" || name == L"cvtobinary") {
        return opcv::ToBinary(source, output);
    }
    if (name == L"edge" || name == L"toedge" || name == L"to_edge" || name == L"cvtoedge") {
        return opcv::ToEdge(source, output);
    }
    if (name == L"outline" || name == L"tooutline" || name == L"to_outline" || name == L"cvtooutline") {
        return opcv::ToOutline(source, output);
    }
    if (name == L"denoise" || name == L"cvdenoise") {
        return opcv::Denoise(source, output);
    }
    if (name == L"equalize" || name == L"cvequalize") {
        return opcv::Equalize(source, output);
    }
    if (name == L"cropvalid" || name == L"crop_valid" || name == L"cvcropvalid") {
        return opcv::CropValid(source, output);
    }

    if (name == L"clahe") {
        double clip_limit = 2.0;
        long tile_grid_size = 8;
        return parse_cv_pipeline_double_arg(args, 0, 2.0, clip_limit) &&
               parse_cv_pipeline_long_arg(args, 1, 8, tile_grid_size) &&
               opcv::CLAHE(source, output, clip_limit, static_cast<int>(tile_grid_size));
    }
    if (name == L"blur") {
        opcv::BlurMode mode;
        long kernel_size = 3;
        const wchar_t *mode_text = args.empty() ? L"gaussian" : args[0].c_str();
        return parse_cv_blur_mode(mode_text, mode) && parse_cv_pipeline_long_arg(args, 1, 3, kernel_size) &&
               opcv::Blur(source, output, mode, static_cast<int>(kernel_size));
    }
    if (name == L"sharpen") {
        double strength = 1.0;
        return parse_cv_pipeline_double_arg(args, 0, 1.0, strength) && opcv::Sharpen(source, output, strength);
    }
    if (name == L"threshold") {
        opcv::ThresholdMode mode;
        const wchar_t *mode_text = args.empty() ? L"otsu" : args[0].c_str();
        double threshold = 0.0;
        double max_value = 255.0;
        return parse_cv_threshold_mode(mode_text, mode) && parse_cv_pipeline_double_arg(args, 1, 0.0, threshold) &&
               parse_cv_pipeline_double_arg(args, 2, 255.0, max_value) &&
               opcv::Threshold(source, output, threshold, max_value, mode);
    }
    if (name == L"inrange") {
        if (args.size() < 7) {
            return false;
        }

        opcv::InRangeColorSpace color_space;
        const std::wstring lower = args[1] + L"," + args[2] + L"," + args[3];
        const std::wstring upper = args[4] + L"," + args[5] + L"," + args[6];
        std::vector<double> lower_values;
        std::vector<double> upper_values;
        return parse_cv_color_space(args[0].c_str(), color_space) &&
               parse_cv_number_list(lower.c_str(), lower_values) && parse_cv_number_list(upper.c_str(), upper_values) &&
               opcv::InRange(source, output, color_space, lower_values, upper_values);
    }
    if (name == L"morph") {
        opcv::MorphologyMode mode;
        long kernel_size = 3;
        long iterations = 1;
        const wchar_t *mode_text = args.empty() ? L"open" : args[0].c_str();
        return parse_cv_morphology_mode(mode_text, mode) && parse_cv_pipeline_long_arg(args, 1, 3, kernel_size) &&
               parse_cv_pipeline_long_arg(args, 2, 1, iterations) &&
               opcv::Morphology(source, output, mode, static_cast<int>(kernel_size), static_cast<int>(iterations));
    }
    if (name == L"thin") {
        opcv::ThinMode mode;
        const wchar_t *mode_text = args.empty() ? L"zhang_suen" : args[0].c_str();
        return parse_cv_thin_mode(mode_text, mode) && opcv::Thin(source, output, mode);
    }
    if (name == L"resize") {
        long width = 0;
        long height = 0;
        return parse_cv_pipeline_long_arg(args, 0, 0, width) && parse_cv_pipeline_long_arg(args, 1, 0, height) &&
               opcv::Resize(source, static_cast<int>(width), static_cast<int>(height), output);
    }

    return false;
}

} // namespace
// using bytearray = std::vector<unsigned char>;
struct op_context {
    // 1. Windows API
    WinApi winapi;
    // background module
    opBackground bkproc;
    // image process
    ImageProc image_proc;
    // work path
    std::wstring curr_path;

    std::map<std::wstring, long> vkmap;
    std::vector<unsigned char> screenData;
    std::vector<unsigned char> screenDataBmp;
    std::wstring opPath;
    long screen_data_mode;
    int id;
};

libop::libop() : m_context(std::make_unique<op_context>()) {
    // 将进程默认 DPI 感知设置为系统 DPI 感知
    ::SetProcessDPIAware();

    m_context->screen_data_mode = SC_DATA_TOP;

    // 初始化目录
    wchar_t buff[MAX_PATH];
    ::GetCurrentDirectoryW(MAX_PATH, buff);
    m_context->curr_path = buff;
    m_context->image_proc._curr_path = m_context->curr_path;
    // 初始化键码表
    std::map<std::wstring, long> &_vkmap = m_context->vkmap;
    _vkmap[L"back"] = VK_BACK;
    _vkmap[L"ctrl"] = VK_CONTROL;
    _vkmap[L"lctrl"] = VK_LCONTROL;
    _vkmap[L"rctrl"] = VK_RCONTROL;
    _vkmap[L"alt"] = VK_MENU;
    _vkmap[L"lalt"] = VK_LMENU;
    _vkmap[L"ralt"] = VK_RMENU;
    _vkmap[L"shift"] = VK_SHIFT;
    _vkmap[L"lshift"] = VK_LSHIFT;
    _vkmap[L"rshift"] = VK_RSHIFT;
    _vkmap[L"win"] = VK_LWIN;
    _vkmap[L"lwin"] = VK_LWIN;
    _vkmap[L"rwin"] = VK_RWIN;
    _vkmap[L"space"] = VK_SPACE;
    _vkmap[L"cap"] = VK_CAPITAL;
    _vkmap[L"tab"] = VK_TAB;
    _vkmap[L"esc"] = VK_ESCAPE;
    _vkmap[L"enter"] = VK_RETURN;
    _vkmap[L"up"] = VK_UP;
    _vkmap[L"down"] = VK_DOWN;
    _vkmap[L"left"] = VK_LEFT;
    _vkmap[L"right"] = VK_RIGHT;
    _vkmap[L"menu"] = VK_APPS;
    _vkmap[L"print"] = VK_SNAPSHOT;
    _vkmap[L"insert"] = VK_INSERT;
    _vkmap[L"delete"] = VK_DELETE;
    _vkmap[L"pause"] = VK_PAUSE;
    _vkmap[L"scroll"] = VK_SCROLL;
    _vkmap[L"home"] = VK_HOME;
    _vkmap[L"end"] = VK_END;
    _vkmap[L"pgup"] = VK_PRIOR;
    _vkmap[L"pgdn"] = VK_NEXT;
    _vkmap[L"f1"] = VK_F1;
    _vkmap[L"f2"] = VK_F2;
    _vkmap[L"f3"] = VK_F3;
    _vkmap[L"f4"] = VK_F4;
    _vkmap[L"f5"] = VK_F5;
    _vkmap[L"f6"] = VK_F6;
    _vkmap[L"f7"] = VK_F7;
    _vkmap[L"f8"] = VK_F8;
    _vkmap[L"f9"] = VK_F9;
    _vkmap[L"f10"] = VK_F10;
    _vkmap[L"f11"] = VK_F11;
    _vkmap[L"f12"] = VK_F12;
    // Numpad keys
    _vkmap[L"num0"] = VK_NUMPAD0;
    _vkmap[L"num1"] = VK_NUMPAD1;
    _vkmap[L"num2"] = VK_NUMPAD2;
    _vkmap[L"num3"] = VK_NUMPAD3;
    _vkmap[L"num4"] = VK_NUMPAD4;
    _vkmap[L"num5"] = VK_NUMPAD5;
    _vkmap[L"num6"] = VK_NUMPAD6;
    _vkmap[L"num7"] = VK_NUMPAD7;
    _vkmap[L"num8"] = VK_NUMPAD8;
    _vkmap[L"num9"] = VK_NUMPAD9;
    _vkmap[L"numlock"] = VK_NUMLOCK;
    _vkmap[L"num."] = VK_DECIMAL;
    _vkmap[L"num*"] = VK_MULTIPLY;
    _vkmap[L"num+"] = VK_ADD;
    _vkmap[L"num-"] = VK_SUBTRACT;
    _vkmap[L"num/"] = VK_DIVIDE;

    m_context->opPath = opEnv::getBasePath();

    m_context->id = s_id++;
}

libop::~libop() {
}

std::wstring libop::Ver() {

    // Tool::setlog("address=%d,str=%s", ver, ver);
    return _T(OP_VERSION);
}

void libop::SetPath(const wchar_t *path, long *ret) {
    wstring fpath = path;
    replacew(fpath, L"/", L"\\");
    if (fpath.find(L'\\') != wstring::npos && ::PathFileExistsW(fpath.data())) {
        m_context->curr_path = fpath;
        m_context->image_proc._curr_path = m_context->curr_path;
        m_context->bkproc._curr_path = m_context->curr_path;
        *ret = 1;
    } else {

        if (!fpath.empty() && fpath[0] != L'\\')
            fpath = m_context->curr_path + L'\\' + fpath;
        else
            fpath = m_context->curr_path + fpath;
        if (::PathFileExistsW(fpath.data())) {
            m_context->curr_path = path;
            m_context->image_proc._curr_path = m_context->curr_path;
            m_context->bkproc._curr_path = m_context->curr_path;
            *ret = 1;
        } else {
            setlog("path '%s' not exists", fpath.data());
            *ret = 0;
        }
    }
}

void libop::GetPath(std::wstring &path) {
    path = m_context->curr_path;
}

void libop::GetBasePath(std::wstring &path) {
    path = opEnv::getBasePath();
}

void libop::GetID(long *ret) {
    *ret = m_context->id;
}

void libop::GetLastError(long *ret) {
    *ret = ::GetLastError();
}

void libop::SetShowErrorMsg(long show_type, long *ret) {
    opEnv::m_showErrorMsg = show_type;
    *ret = 1;
}

void libop::Sleep(long millseconds, long *ret) {
    ::Sleep(millseconds);
    *ret = 1;
}

void libop::InjectDll(const wchar_t *process_name, const wchar_t *dll_name, long *ret) {
    auto proc = _ws2string(process_name);
    auto dll = _ws2string(dll_name);
    LONG_PTR hwnd = 0;
    FindWindowByProcess(process_name, L"", L"", &hwnd);
    long pid;
    GetWindowProcessId(hwnd, &pid);
    *ret = 0;
    if (Injecter::EnablePrivilege(TRUE)) {
        long error_code = 0;
        *ret = Injecter::InjectDll(pid, dll_name, error_code);
    } else {
        setlog("EnablePrivilege false erro_code=%08X ", ::GetLastError());
    }
}

void libop::EnablePicCache(long enable, long *ret) {
    m_context->image_proc._enable_cache = enable;
    *ret = 1;
}

void libop::CapturePre(const wchar_t *file, LONG *ret) {
    *ret = m_context->image_proc.Capture(file);
}

void libop::SetScreenDataMode(long mode, long *ret) {
    m_context->screen_data_mode = mode;
    *ret = 1;
}

void libop::AStarFindPath(long mapWidth, long mapHeight, const wchar_t *disable_points, long beginX, long beginY,
                          long endX, long endY, std::wstring &path) {
    AStar as;
    using Vec2i = AStar::Vec2i;
    std::vector<Vec2i> walls;
    std::vector<wstring> vstr;
    Vec2i tp;
    split(disable_points, vstr, L"|");
    for (auto &it : vstr) {
        if (swscanf(it.c_str(), L"%d,%d", &tp.x, &tp.y) != 2)
            break;
        walls.push_back(tp);
    }
    std::list<Vec2i> paths;

    as.set_map(mapWidth, mapHeight, walls);
    as.findpath(beginX, beginY, endX, endY, paths);
    path.clear();
    wchar_t buf[20];
    for (auto it = paths.rbegin(); it != paths.rend(); ++it) {
        auto v = *it;
        wsprintf(buf, L"%d,%d", v.x, v.y);
        path += buf;
        path.push_back(L'|');
    }
    if (!path.empty())
        path.pop_back();
}

void libop::FindNearestPos(const wchar_t *all_pos, long type, long x, long y, std::wstring &ret) {
    const wchar_t *p = 0;
    wchar_t buf[256] = {0};
    wchar_t rs[256] = {0};
    double old = 1e9;
    long rx = -1, ry = -1;
    std::wstring s = std::regex_replace(all_pos, std::wregex(L","), L" ");
    p = s.data();
    while (*p) {
        long x2, y2;
        bool ok = false;
        if (type == 1) {

            if (swscanf(p, L"%d %d", &x2, &y2) == 2) {
                ok = true;
            }
        } else {
            if (swscanf(p, L"%s %d %d", buf, &x2, &y2) == 3) {
                ok = true;
            }
        }
        if (ok) {
            double compareDis = (x - x2) * (x - x2) + (y - y2) * (y - y2);
            if (compareDis < old) {
                rx = x2;
                ry = y2;
                old = compareDis;
                wcscpy(rs, buf);
            }
        }
        while (*p && *p != L'|')
            ++p;
        if (*p)
            ++p;
    }
    if (rs[0]) {
        wcscpy(buf, rs);
        wsprintf(rs, L"%s,%d,%d", buf, rx, ry);
    } else if (type == 1 && rx != -1) {
        wsprintf(rs, L"%d,%d", rx, ry);
    }
    ret = rs;
}

void libop::EnumWindow(LONG_PTR parent, const wchar_t *title, const wchar_t *class_name, long filter,
                       std::wstring &retstr) {

    std::vector<wchar_t> retstring(MAX_PATH * 200, 0);
    m_context->winapi.EnumWindow(reinterpret_cast<HWND>(parent), title, class_name, filter, retstring.data());
    //*retstr=_bstr_t(retstring);
    retstr = retstring.data();
}

void libop::EnumWindowByProcess(const wchar_t *process_name, const wchar_t *title, const wchar_t *class_name,
                                long filter, std::wstring &retstring) {

    std::vector<wchar_t> retstr(MAX_PATH * 200, 0);
    m_context->winapi.EnumWindow(nullptr, title, class_name, filter, retstr.data(), process_name);
    //*retstring=_bstr_t(retstr);

    retstring = retstr.data();
}

void libop::EnumProcess(const wchar_t *name, std::wstring &retstring) {
    std::vector<wchar_t> retstr(MAX_PATH * 200, 0);
    m_context->winapi.EnumProcess(name, retstr.data());
    //*retstring=_bstr_t(retstr);
    retstring = retstr.data();
}

void libop::ClientToScreen(LONG_PTR hwnd, long *x, long *y, long *bret) {
    *bret = m_context->winapi.ClientToScreen(reinterpret_cast<HWND>(hwnd), *x, *y);
}

void libop::FindWindow(const wchar_t *class_name, const wchar_t *title, LONG_PTR *rethwnd) {
    *rethwnd = reinterpret_cast<LONG_PTR>(m_context->winapi.FindWindow(class_name, title));
}

void libop::FindWindowByProcess(const wchar_t *process_name, const wchar_t *class_name, const wchar_t *title,
                                LONG_PTR *rethwnd) {

    HWND hwnd = nullptr;
    m_context->winapi.FindWindowByProcess(class_name, title, hwnd, process_name);
    *rethwnd = reinterpret_cast<LONG_PTR>(hwnd);
}

void libop::FindWindowByProcessId(long process_id, const wchar_t *class_name, const wchar_t *title, LONG_PTR *rethwnd) {
    HWND hwnd = nullptr;
    m_context->winapi.FindWindowByProcess(class_name, title, hwnd, NULL, process_id);
    *rethwnd = reinterpret_cast<LONG_PTR>(hwnd);
}

void libop::FindWindowEx(LONG_PTR parent, const wchar_t *class_name, const wchar_t *title, LONG_PTR *rethwnd) {
    *rethwnd =
        reinterpret_cast<LONG_PTR>(m_context->winapi.FindWindowEx(reinterpret_cast<HWND>(parent), class_name, title));
}

void libop::GetClientRect(LONG_PTR hwnd, long *x1, long *y1, long *x2, long *y2, long *nret) {
    *nret = m_context->winapi.GetClientRect(reinterpret_cast<HWND>(hwnd), *x1, *y1, *x2, *y2);
}

void libop::GetClientSize(LONG_PTR hwnd, long *width, long *height, long *nret) {
    *nret = m_context->winapi.GetClientSize(reinterpret_cast<HWND>(hwnd), *width, *height);
}

void libop::GetForegroundFocus(LONG_PTR *rethwnd) {
    *rethwnd = reinterpret_cast<LONG_PTR>(::GetFocus());
}

void libop::GetForegroundWindow(LONG_PTR *rethwnd) {
    *rethwnd = reinterpret_cast<LONG_PTR>(::GetForegroundWindow());
}

void libop::GetMousePointWindow(LONG_PTR *rethwnd) {
    //::Sleep(2000);
    HWND hwnd = nullptr;
    m_context->winapi.GetMousePointWindow(hwnd);
    *rethwnd = reinterpret_cast<LONG_PTR>(hwnd);
}

void libop::GetPointWindow(long x, long y, LONG_PTR *rethwnd) {
    HWND hwnd = nullptr;
    m_context->winapi.GetMousePointWindow(hwnd, x, y);
    *rethwnd = reinterpret_cast<LONG_PTR>(hwnd);
}

void libop::GetProcessInfo(long pid, std::wstring &retstring) {
    wchar_t retstr[MAX_PATH] = {0};
    m_context->winapi.GetProcessInfo(pid, retstr);
    //* retstring=_bstr_t(retstr);

    retstring = retstr;
}

void libop::GetSpecialWindow(long flag, LONG_PTR *rethwnd) {
    *rethwnd = 0;
    if (flag == 0)
        *rethwnd = reinterpret_cast<LONG_PTR>(GetDesktopWindow());
    else if (flag == 1) {
        *rethwnd = reinterpret_cast<LONG_PTR>(::FindWindowW(L"Shell_TrayWnd", NULL));
    }
}

void libop::GetWindow(LONG_PTR hwnd, long flag, LONG_PTR *nret) {
    HWND target = nullptr;
    m_context->winapi.GetWindow(reinterpret_cast<HWND>(hwnd), flag, target);
    *nret = reinterpret_cast<LONG_PTR>(target);
}

void libop::GetWindowClass(LONG_PTR hwnd, std::wstring &retstring) {
    wchar_t classname[MAX_PATH] = {0};
    ::GetClassName(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), classname, MAX_PATH);
    //* retstring=_bstr_t(classname);

    retstring = classname;
}

void libop::GetWindowProcessId(LONG_PTR hwnd, long *nretpid) {
    DWORD pid = 0;
    ::GetWindowThreadProcessId(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), &pid);
    *nretpid = pid;
}

void libop::GetWindowProcessPath(LONG_PTR hwnd, std::wstring &retstring) {
    DWORD pid = 0;
    ::GetWindowThreadProcessId(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), &pid);
    wchar_t process_path[MAX_PATH] = {0};
    m_context->winapi.GetProcesspath(pid, process_path);
    //* retstring=_bstr_t(process_path);

    retstring = process_path;
}

void libop::GetWindowRect(LONG_PTR hwnd, long *x1, long *y1, long *x2, long *y2, long *nret) {
    RECT winrect;
    *nret = ::GetWindowRect(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), &winrect);
    *x1 = winrect.left;
    *y1 = winrect.top;
    *x2 = winrect.right;
    *y2 = winrect.bottom;
}

void libop::GetWindowState(LONG_PTR hwnd, long flag, long *rethwnd) {
    *rethwnd = m_context->winapi.GetWindowState(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), flag);
}

void libop::GetWindowTitle(LONG_PTR hwnd, std::wstring &rettitle) {
    wchar_t title[MAX_PATH] = {0};
    ::GetWindowTextW(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), title, MAX_PATH);
    //* rettitle=_bstr_t(title);

    rettitle = title;
}

void libop::MoveWindow(LONG_PTR hwnd, long x, long y, long *nret) {
    RECT winrect;
    HWND target = reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd));
    ::GetWindowRect(target, &winrect);
    int width = winrect.right - winrect.left;
    int hight = winrect.bottom - winrect.top;
    *nret = ::MoveWindow(target, x, y, width, hight, false);
}

void libop::ScreenToClient(LONG_PTR hwnd, long *x, long *y, long *nret) {
    POINT point;
    point.x = *x;
    point.y = *y;
    *nret = ::ScreenToClient(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), &point);
    *x = point.x;
    *y = point.y;
}

void libop::SendPaste(LONG_PTR hwnd, long *nret) {
    *nret = m_context->winapi.SendPaste(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)));
}

void libop::SetClientSize(LONG_PTR hwnd, long width, long hight, long *nret) {
    *nret = m_context->winapi.SetWindowSize(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), width, hight);
}

void libop::SetWindowState(LONG_PTR hwnd, long flag, long *nret) {
    *nret = m_context->winapi.SetWindowState(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), flag);
}

void libop::SetWindowSize(LONG_PTR hwnd, long width, long height, long *nret) {
    *nret = m_context->winapi.SetWindowSize(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), width, height, 1);
}

void libop::LayoutWindows(const wchar_t *hwnds, long layout_type, long columns, long start_x, long start_y, long gap_x,
                          long gap_y, long size_mode, long window_width, long window_height, long anchor_mode,
                          long *ret) {
    *ret = 0;

    std::vector<HWND> windows;
    if (!parse_window_list(hwnds, windows))
        return;

    window_layout::Options options;
    if (!parse_layout_type(layout_type, options.type))
        return;
    if (!parse_size_mode(size_mode, options.size_mode))
        return;
    if (!parse_anchor_mode(anchor_mode, options.anchor_mode))
        return;

    options.columns = columns;
    options.start_x = start_x;
    options.start_y = start_y;
    options.gap_x = gap_x;
    options.gap_y = gap_y;
    options.window_width = window_width;
    options.window_height = window_height;

    *ret = window_layout::Layout(windows, options);
}

void libop::SetWindowText(LONG_PTR hwnd, const wchar_t *title, long *nret) {
    //*nret=gWindowObj.TSSetWindowState(hwnd,flag);
    *nret = ::SetWindowTextW(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), title);
}

void libop::SetWindowTransparent(LONG_PTR hwnd, long trans, long *nret) {
    *nret = m_context->winapi.SetWindowTransparent(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), trans);
}

void libop::SendString(LONG_PTR hwnd, const wchar_t *str, long *ret) {
    *ret = m_context->winapi.SendString(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), str);
}

void libop::SendStringIme(LONG_PTR hwnd, const wchar_t *str, long *ret) {
    *ret = m_context->winapi.SendStringIme(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), str);
}

void libop::RunApp(const wchar_t *cmdline, long mode, unsigned long *pid, long *ret) {
    // 成功时返回新进程 pid，失败时返回 0。
    *ret = m_context->winapi.RunApp(cmdline, mode, pid);
}

void libop::WinExec(const wchar_t *cmdline, long cmdshow, long *ret) {
    auto str = _ws2string(cmdline);
    *ret = ::WinExec(str.c_str(), cmdshow) > 31 ? 1 : 0;
}

void libop::GetCmdStr(const wchar_t *cmd, long millseconds, std::wstring &retstr) {
    Cmder cd;
    auto str =
        cd.GetCmdStr(cmd ? std::wstring(cmd) : std::wstring(), millseconds <= 0 ? 5 : static_cast<DWORD>(millseconds));
    retstr = decode_command_output(str);
}

void libop::SetClipboard(const wchar_t *str, long *ret) {
    *ret = m_context->winapi.SetClipboard(str);
}

void libop::GetClipboard(std::wstring &ret) {
    m_context->winapi.GetClipboard(ret);
}

void libop::Delay(long mis, long *ret) {
    *ret = ::Delay(mis);
}

void libop::Delays(long mis_min, long mis_max, long *ret) {
    *ret = ::Delays(mis_min, mis_max);
}

void libop::BindWindow(LONG_PTR hwnd, const wchar_t *display, const wchar_t *mouse, const wchar_t *keypad, long mode,
                       long *ret) {
    BindWindowEx(hwnd, hwnd, display, mouse, keypad, mode, ret);
}

void libop::BindWindowEx(LONG_PTR display_hwnd, LONG_PTR input_hwnd, const wchar_t *display, const wchar_t *mouse,
                         const wchar_t *keypad, long mode, long *ret) {
    if (m_context->bkproc.IsBind())
        m_context->bkproc.UnBindWindow();
    *ret = m_context->bkproc.BindWindowEx(display_hwnd, input_hwnd, display, mouse, keypad, mode);
}

void libop::UnBindWindow(long *ret) {
    *ret = m_context->bkproc.UnBindWindow();
}

void libop::GetBindWindow(LONG_PTR *ret) {
    *ret = m_context->bkproc.GetBindWindow();
}

void libop::IsBind(long *ret) {
    *ret = m_context->bkproc.IsBind();
}

void libop::GetCursorPos(long *x, long *y, long *ret) {

    *ret = m_context->bkproc._bkmouse->GetCursorPos(*x, *y);
}

void libop::GetCursorShape(std::wstring &ret) {
    m_context->bkproc._bkmouse->GetCursorShape(ret);
}

void libop::MoveR(long x, long y, long *ret) {
    *ret = m_context->bkproc._bkmouse->MoveR(x, y);
}

void libop::MoveTo(long x, long y, long *ret) {
    *ret = m_context->bkproc._bkmouse->MoveTo(x, y);
}

void libop::MoveToEx(long x, long y, long w, long h, std::wstring &ret) {
    int dst_x = x;
    int dst_y = y;
    if (m_context->bkproc._bkmouse->MoveToEx(x, y, w, h, dst_x, dst_y)) {
        ret = std::to_wstring(dst_x) + L"," + std::to_wstring(dst_y);
    } else {
        ret.clear();
    }
}

void libop::LeftClick(long *ret) {
    *ret = m_context->bkproc._bkmouse->LeftClick();
}

void libop::LeftDoubleClick(long *ret) {
    *ret = m_context->bkproc._bkmouse->LeftDoubleClick();
}

void libop::LeftDown(long *ret) {
    *ret = m_context->bkproc._bkmouse->LeftDown();
}

void libop::LeftUp(long *ret) {
    *ret = m_context->bkproc._bkmouse->LeftUp();
}

void libop::MiddleClick(long *ret) {
    *ret = m_context->bkproc._bkmouse->MiddleClick();
}

void libop::MiddleDown(long *ret) {
    *ret = m_context->bkproc._bkmouse->MiddleDown();
}

void libop::MiddleUp(long *ret) {
    *ret = m_context->bkproc._bkmouse->MiddleUp();
}

void libop::RightClick(long *ret) {
    *ret = m_context->bkproc._bkmouse->RightClick();
}

void libop::RightDown(long *ret) {
    *ret = m_context->bkproc._bkmouse->RightDown();
}

void libop::RightUp(long *ret) {
    *ret = m_context->bkproc._bkmouse->RightUp();
}

void libop::WheelDown(long *ret) {
    *ret = m_context->bkproc._bkmouse->WheelDown();
}

void libop::WheelUp(long *ret) {
    *ret = m_context->bkproc._bkmouse->WheelUp();
}

void libop::SetMouseDelay(const wchar_t *type, long delay, long *ret) {
    *ret = 0;
    if (delay < 0)
        return;
    *ret = 1;
    if (wcscmp(type, L"normal") == 0)
        MOUSE_NORMAL_DELAY = delay;
    else if (wcscmp(type, L"windows") == 0)
        MOUSE_WINDOWS_DELAY = delay;
    else if (wcscmp(type, L"dx") == 0)
        MOUSE_DX_DELAY = delay;
    else
        *ret = 0;
}

void libop::GetKeyState(long vk_code, long *ret) {
    *ret = m_context->bkproc._keypad->GetKeyState(vk_code);
}

void libop::KeyDown(long vk_code, long *ret) {
    *ret = m_context->bkproc._keypad->KeyDown(vk_code);
}

void libop::KeyDownChar(const wchar_t *vk_code, long *ret) {
    *ret = 0;
    key_combo_t combo;
    if (resolve_text_key_combo(m_context->vkmap, vk_code, combo))
        *ret = key_combo_down(m_context->bkproc._keypad, combo);
}

void libop::KeyUp(long vk_code, long *ret) {
    *ret = m_context->bkproc._keypad->KeyUp(vk_code);
}

void libop::KeyUpChar(const wchar_t *vk_code, long *ret) {
    *ret = 0;
    key_combo_t combo;
    if (resolve_text_key_combo(m_context->vkmap, vk_code, combo))
        *ret = key_combo_up(m_context->bkproc._keypad, combo);
}

void libop::WaitKey(long vk_code, long time_out, long *ret) {
    unsigned long t = time_out < 0 ? 0xffffffffu : static_cast<unsigned long>(time_out);
    *ret = m_context->bkproc._keypad->WaitKey(vk_code, t);
}

void libop::KeyPress(long vk_code, long *ret) {

    *ret = m_context->bkproc._keypad->KeyPress(vk_code);
}

void libop::KeyPressChar(const wchar_t *vk_code, long *ret) {
    *ret = 0;
    key_combo_t combo;
    if (resolve_text_key_combo(m_context->vkmap, vk_code, combo))
        *ret = key_combo_press(m_context->bkproc._keypad, combo);
}

void libop::SetKeypadDelay(const wchar_t *type, long delay, long *ret) {
    *ret = 0;
    if (delay < 0)
        return;
    *ret = 1;
    if (wcscmp(type, L"normal") == 0)
        KEYPAD_NORMAL_DELAY = delay;
    else if (wcscmp(type, L"normal.hd") == 0)
        KEYPAD_NORMAL2_DELAY = delay;
    else if (wcscmp(type, L"windows") == 0)
        KEYPAD_WINDOWS_DELAY = delay;
    else if (wcscmp(type, L"dx") == 0)
        KEYPAD_DX_DELAY = delay;
    else
        *ret = 0;
}

void libop::KeyPressStr(const wchar_t *key_str, long delay, long *ret) {
    *ret = 0;
    auto nlen = wcslen(key_str);
    for (size_t i = 0; i < nlen; ++i) {
        key_combo_t combo;
        if (!resolve_char_key_combo(key_str[i], combo))
            return;

        *ret = key_combo_press(m_context->bkproc._keypad, combo);
        if (*ret == 0)
            return;
        // 连续输入时给控件留一点处理时间，避免字符连发被吞掉。
        ::Delay(delay > 0 ? delay : 1);
    }
}

// 抓取指定区域(x1, y1, x2, y2)的图像, 保存为file
void libop::Capture(long x1, long y1, long x2, long y2, const wchar_t *file_name, long *ret) {

    *ret = 0;

    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);

            *ret = m_context->image_proc.Capture(file_name);
        }
    }
}
// 比较指定坐标点(x,y)的颜色
void libop::CmpColor(long x, long y, const wchar_t *color, DOUBLE sim, long *ret) {
    // LONG rx = -1, ry = -1;
    long tx = x + small_block_size, ty = y + small_block_size;
    *ret = 0;
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x, y, tx, ty)) {
        if (!m_context->bkproc.requestCapture(x, y, small_block_size, small_block_size, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x, y);
            *ret = m_context->image_proc.CmpColor(x, y, color, sim);
        }
    }
}
// 查找指定区域内的颜色
void libop::FindColor(long x1, long y1, long x2, long y2, const wchar_t *color, DOUBLE sim, long dir, long *x, long *y,
                      long *ret) {

    *ret = 0;
    *x = *y = -1;

    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            *ret = m_context->image_proc.FindColor(color, sim, dir, *x, *y);
        }
    }
}
// 查找指定区域内的所有颜色
void libop::FindColorEx(long x1, long y1, long x2, long y2, const wchar_t *color, DOUBLE sim, long dir,
                        std::wstring &retstr) {
    // wstring str;
    retstr.clear();
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            m_context->image_proc.FindColoEx(color, sim, dir, retstr);
        }
    }
}
// 根据指定的多点查找颜色坐标
void libop::FindMultiColor(long x1, long y1, long x2, long y2, const wchar_t *first_color, const wchar_t *offset_color,
                           DOUBLE sim, long dir, long *x, long *y, long *ret) {

    *ret = 0;
    *x = *y = -1;

    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            *ret = m_context->image_proc.FindMultiColor(first_color, offset_color, sim, dir, *x, *y);
        }

        /*if (*ret) {
            rx += x1; ry += y1;
            rx -= m_context->bkproc._pbkdisplay->_client_x;
            ry -= m_context->bkproc._pbkdisplay->_client_y;
        }*/
    }
}
// 根据指定的多点查找所有颜色坐标
void libop::FindMultiColorEx(long x1, long y1, long x2, long y2, const wchar_t *first_color,
                             const wchar_t *offset_color, DOUBLE sim, long dir, std::wstring &retstr) {
    retstr.clear();
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            m_context->image_proc.FindMultiColorEx(first_color, offset_color, sim, dir, retstr);
        }
    }
    // retstr = str;
}
// 查找指定区域内的图片
void libop::FindPic(long x1, long y1, long x2, long y2, const wchar_t *files, const wchar_t *delta_color, DOUBLE sim,
                    long dir, long *x, long *y, long *ret) {

    *ret = 0;
    *x = *y = -1;

    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            *ret = m_context->image_proc.FindPic(files, delta_color, sim, dir, *x, *y);
        }

        /*if (*ret) {
            rx += x1; ry += y1;
            rx -= m_context->bkproc._pbkdisplay->_client_x;
            ry -= m_context->bkproc._pbkdisplay->_client_y;
        }*/
    }
}
// 查找多个图片
void libop::FindPicEx(long x1, long y1, long x2, long y2, const wchar_t *files, const wchar_t *delta_color, DOUBLE sim,
                      long dir, std::wstring &retstr) {

    // wstring str;
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            m_context->image_proc.FindPicEx(files, delta_color, sim, dir, retstr);
        }
    }
    // retstr = str;
}

void libop::FindPicExS(long x1, long y1, long x2, long y2, const wchar_t *files, const wchar_t *delta_color, double sim,
                       long dir, std::wstring &retstr) {
    // wstring str;
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            m_context->image_proc.FindPicEx(files, delta_color, sim, dir, retstr, false);
        }
    }
    // retstr = str;
}

void libop::FindColorBlock(long x1, long y1, long x2, long y2, const wchar_t *color, double sim, long count,
                           long height, long width, long *x, long *y, long *ret) {
    *ret = 0;
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            *ret = m_context->image_proc.FindColorBlock(color, sim, count, height, width, *x, *y);
        }
    }
}

void libop::FindColorBlockEx(long x1, long y1, long x2, long y2, const wchar_t *color, double sim, long count,
                             long height, long width, std::wstring &retstr) {

    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            m_context->image_proc.FindColorBlockEx(color, sim, count, height, width, retstr);
        }
    }
}

// 获取(x,y)的颜色
void libop::GetColor(long x, long y, std::wstring &ret) {
    color_t cr;
    auto tx = x + small_block_size, ty = y + small_block_size;
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x, y, tx, ty)) {
        if (m_context->bkproc.requestCapture(x, y, small_block_size, small_block_size, m_context->image_proc._src)) {
            m_context->image_proc.set_offset(x, y);
            cr = m_context->image_proc._src.at<color_t>(0, 0);
        } else {
            setlog("error requestCapture");
        }
    } else {
        // setlog("")
    }

    ret = cr.towstr();
}

void libop::GetColorNum(long x1, long y1, long x2, long y2, const wchar_t *color, double sim, long *ret) {
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            *ret = m_context->image_proc.GetColorNum(color, sim);
        }
    }
}

void libop::SetDisplayInput(const wchar_t *mode, long *ret) {
    *ret = m_context->bkproc.set_display_method(mode);
}

void libop::LoadPic(const wchar_t *file_name, long *ret) {
    *ret = m_context->image_proc.LoadPic(file_name);
}

void libop::FreePic(const wchar_t *file_name, long *ret) {
    *ret = m_context->image_proc.FreePic(file_name);
}

void libop::LoadMemPic(const wchar_t *file_name, void *data, long size, long *ret) {
    *ret = m_context->image_proc.LoadMemPic(file_name, data, size);
}

void libop::GetPicSize(const wchar_t *pic_name, long *width, long *height, long *ret) {
    *ret = m_context->image_proc.GetPicSize(pic_name, width, height);
}

void libop::GetScreenData(long x1, long y1, long x2, long y2, size_t *data, long *ret) {
    *data = 0;
    *ret = 0;
    auto &img = m_context->image_proc._src;
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            m_context->screenData.resize(img.size() * 4);

            if (m_context->screen_data_mode == SC_DATA_BOTTOM) {
                for (int i = 0; i < img.height; i++) {
                    memcpy(m_context->screenData.data() + i * img.width * 4, img.ptr<char>(img.height - 1 - i),
                           img.width * 4);
                }
            } else {
                memcpy(m_context->screenData.data(), img.pdata, img.size() * 4);
            }
            *data = (size_t)m_context->screenData.data();
            *ret = 1;
        }
    }
}

void libop::GetScreenDataBmp(long x1, long y1, long x2, long y2, size_t *data, long *size, long *ret) {
    *data = 0;
    *ret = 0;
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("rerror requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            auto &img = m_context->image_proc._src;

            BITMAPFILEHEADER bfh = {0}; // bmp file header
            BITMAPINFOHEADER bih = {0}; // bmp info header
            const int szBfh = sizeof(BITMAPFILEHEADER);
            const int szBih = sizeof(BITMAPINFOHEADER);
            bfh.bfOffBits = szBfh + szBih;
            bfh.bfSize = bfh.bfOffBits + img.width * img.height * 4;
            bfh.bfType = static_cast<WORD>(0x4d42);

            bih.biBitCount = 32; // 每个像素字节大小
            bih.biCompression = BI_RGB;
            // bih.biHeight = -img.height;//高度 反
            bih.biHeight = m_context->screen_data_mode == SC_DATA_BOTTOM ? img.height : -img.height; // 高度
            bih.biPlanes = 1;
            bih.biSize = sizeof(BITMAPINFOHEADER);
            bih.biSizeImage = img.width * 4 * img.height; // 图像数据大小
            bih.biWidth = img.width;                      // 宽度

            m_context->screenDataBmp.resize(bfh.bfSize);
            /*	std::ofstream f;
        f.open("xx.bmp",std::ios::binary);
        if (f) {
            f.write((char*)&bfh, sizeof(bfh));
            f.write((char*)&bih, sizeof(bih));
            f.write((char*)img.pdata, img.size() * 4);
        }

        f.close();*/
            auto dst = m_context->screenDataBmp.data();

            memcpy(dst, &bfh, sizeof(bfh));
            memcpy(dst + sizeof(bfh), &bih, sizeof(bih));
            dst += sizeof(bfh) + sizeof(bih);
            if (m_context->screen_data_mode == SC_DATA_BOTTOM) {
                for (int i = 0; i < img.height; i++) {
                    memcpy(dst + i * img.width * 4, img.ptr<char>(img.height - 1 - i), img.width * 4);
                }
            } else {
                memcpy(dst, img.pdata, img.size() * 4);
            }

            // memcpy(dst + sizeof(bfh)+sizeof(bih), img.pdata, img.size()*4);
            *data = (size_t)m_context->screenDataBmp.data();
            *size = bfh.bfSize;
            *ret = 1;
        }
    }
}

void libop::GetScreenFrameInfo(long *frame_id, long *time) {
    FrameInfo info = {};
    if (m_context->bkproc.IsBind()) {
        m_context->bkproc._pbkdisplay->getFrameInfo(info);
    }
    *frame_id = info.frameId;
    *time = info.time;
}

void libop::MatchPicName(const wchar_t *pic_name, std::wstring &retstr) {
    retstr.clear();
    std::wstring s(pic_name);
    if (s.find(L'/') != s.npos || s.find(L'\\') != s.npos) {
        setlog("invalid pic_name");
    }

    s = std::regex_replace(s, std::wregex(L"(\\.|\\(|\\)|\\[|\\]|\\{|\\})"), L"\\$1");
    /*s = std::regex_replace(s, std::wregex(L"\\("), L"\\(");
    s = std::regex_replace(s, std::wregex(L"\\)"), L"\\)");
    s = std::regex_replace(s, std::wregex(L"\\["), L"\\[");
    s = std::regex_replace(s, std::wregex(L"\\]"), L"\\]");*/
    s = std::regex_replace(s, std::wregex(L"\\*"), L".*?");
    s = std::regex_replace(s, std::wregex(L"\\?"), L".?");

    // setlog(s.data());
    namespace fs = std::filesystem;
    fs::path path(m_context->curr_path);
    if (fs::exists(path)) {
        fs::directory_iterator iter(path);
        std::wstring tmp;
        std::wregex e(s);
        for (auto &it : iter) {
            if (it.status().type() == fs::file_type::regular) {
                tmp = it.path().filename();
                try {
                    if (std::regex_match(tmp, e)) {
                        retstr += tmp;
                        retstr += L"|";
                    }
                } catch (...) {
                    setlog("exception!");
                }
            }
        }
        if (!retstr.empty() && retstr.back() == L'|')
            retstr.pop_back();
    }
}

void libop::CvLoadTemplate(const wchar_t *name, const wchar_t *file_path, long *ret) {
    *ret = (name != nullptr && file_path != nullptr && opcv::LoadTemplate(name, file_path)) ? 1 : 0;
}

void libop::CvLoadMaskedTemplate(const wchar_t *name, const wchar_t *template_path, const wchar_t *mask_path,
                                 long *ret) {
    *ret = (name != nullptr && template_path != nullptr && mask_path != nullptr &&
            opcv::LoadMaskedTemplate(name, template_path, mask_path))
               ? 1
               : 0;
}

void libop::CvRemoveTemplate(const wchar_t *name, long *ret) {
    *ret = (name != nullptr && opcv::RemoveTemplate(name)) ? 1 : 0;
}

void libop::CvRemoveAllTemplates(long *ret) {
    opcv::RemoveAllTemplates();
    *ret = 1;
}

void libop::CvHasTemplate(const wchar_t *name, long *ret) {
    *ret = (name != nullptr && opcv::HasTemplate(name)) ? 1 : 0;
}

void libop::CvGetTemplateCount(long *ret) {
    *ret = opcv::GetTemplateCount();
}

void libop::CvGetAllTemplateNames(std::wstring &retstr) {
    retstr.clear();
    const auto names = opcv::GetAllTemplateNames();
    for (size_t i = 0; i < names.size(); ++i) {
        if (i != 0) {
            retstr += L"|";
        }
        retstr += names[i];
    }
}

void libop::CvGetOpenCvVersion(std::wstring &retstr) {
    retstr = opcv::GetOpenCvVersion();
}

void libop::CvLoadTemplateList(const wchar_t *template_list, long *ret) {
    *ret = 0;
    if (template_list == nullptr || template_list[0] == L'\0') {
        return;
    }

    std::vector<std::wstring> items;
    split(template_list, items, L"|");

    std::vector<std::pair<std::wstring, std::wstring>> templates;
    templates.reserve(items.size());
    for (const auto &item : items) {
        if (item.empty()) {
            continue;
        }

        const size_t comma_pos = item.find(L',');
        if (comma_pos == std::wstring::npos || comma_pos == 0 || comma_pos + 1 >= item.size()) {
            return;
        }

        const std::wstring name = item.substr(0, comma_pos);
        const std::wstring path = item.substr(comma_pos + 1);
        if (name.empty() || path.empty()) {
            return;
        }
        templates.emplace_back(name, path);
    }

    if (templates.empty()) {
        return;
    }

    *ret = opcv::LoadTemplateList(templates) ? 1 : 0;
}

void libop::CvToGray(const wchar_t *src_file, const wchar_t *dst_file, long *ret) {
    run_cv_file_preprocess(src_file, dst_file, ret, [](const opcv::ImageHandle &source, opcv::ImageHandle &output) {
        return opcv::ToGray(source, output);
    });
}

void libop::CvToBinary(const wchar_t *src_file, const wchar_t *dst_file, long *ret) {
    run_cv_file_preprocess(src_file, dst_file, ret, [](const opcv::ImageHandle &source, opcv::ImageHandle &output) {
        return opcv::ToBinary(source, output);
    });
}

void libop::CvToEdge(const wchar_t *src_file, const wchar_t *dst_file, long *ret) {
    run_cv_file_preprocess(src_file, dst_file, ret, [](const opcv::ImageHandle &source, opcv::ImageHandle &output) {
        return opcv::ToEdge(source, output);
    });
}

void libop::CvToOutline(const wchar_t *src_file, const wchar_t *dst_file, long *ret) {
    run_cv_file_preprocess(src_file, dst_file, ret, [](const opcv::ImageHandle &source, opcv::ImageHandle &output) {
        return opcv::ToOutline(source, output);
    });
}

void libop::CvDenoise(const wchar_t *src_file, const wchar_t *dst_file, long *ret) {
    run_cv_file_preprocess(src_file, dst_file, ret, [](const opcv::ImageHandle &source, opcv::ImageHandle &output) {
        return opcv::Denoise(source, output);
    });
}

void libop::CvEqualize(const wchar_t *src_file, const wchar_t *dst_file, long *ret) {
    run_cv_file_preprocess(src_file, dst_file, ret, [](const opcv::ImageHandle &source, opcv::ImageHandle &output) {
        return opcv::Equalize(source, output);
    });
}

void libop::CvCLAHE(const wchar_t *src_file, const wchar_t *dst_file, double clip_limit, long tile_grid_size,
                    long *ret) {
    run_cv_file_preprocess(src_file, dst_file, ret, [&](const opcv::ImageHandle &source, opcv::ImageHandle &output) {
        return opcv::CLAHE(source, output, clip_limit, static_cast<int>(tile_grid_size));
    });
}

void libop::CvBlur(const wchar_t *src_file, const wchar_t *dst_file, const wchar_t *mode, long kernel_size, long *ret) {
    opcv::BlurMode blur_mode;
    if (!parse_cv_blur_mode(mode, blur_mode)) {
        *ret = 0;
        return;
    }

    run_cv_file_preprocess(src_file, dst_file, ret, [&](const opcv::ImageHandle &source, opcv::ImageHandle &output) {
        return opcv::Blur(source, output, blur_mode, static_cast<int>(kernel_size));
    });
}

void libop::CvSharpen(const wchar_t *src_file, const wchar_t *dst_file, double strength, long *ret) {
    run_cv_file_preprocess(src_file, dst_file, ret, [&](const opcv::ImageHandle &source, opcv::ImageHandle &output) {
        return opcv::Sharpen(source, output, strength);
    });
}

void libop::CvCropValid(const wchar_t *src_file, const wchar_t *dst_file, long *ret) {
    run_cv_file_preprocess(src_file, dst_file, ret, [](const opcv::ImageHandle &source, opcv::ImageHandle &output) {
        return opcv::CropValid(source, output);
    });
}

void libop::CvConnectedComponents(const wchar_t *src_file, double min_area, std::wstring &retjson, long *ret) {
    *ret = 0;
    retjson = build_cv_components_json({}, false);
    if (src_file == nullptr) {
        return;
    }

    opcv::ImageHandle source;
    std::vector<opcv::RegionAnalysisResult> results;
    const bool ok = opcv::LoadImageFromFile(src_file, source) && opcv::ConnectedComponents(source, min_area, results);
    retjson = build_cv_components_json(results, ok);
    *ret = ok ? 1 : 0;
}

void libop::CvFindContours(const wchar_t *src_file, double min_area, std::wstring &retjson, long *ret) {
    *ret = 0;
    retjson = build_cv_contours_json({}, false);
    if (src_file == nullptr) {
        return;
    }

    opcv::ImageHandle source;
    std::vector<opcv::ContourAnalysisResult> results;
    const bool ok = opcv::LoadImageFromFile(src_file, source) && opcv::FindContours(source, min_area, results);
    retjson = build_cv_contours_json(results, ok);
    *ret = ok ? 1 : 0;
}

void libop::CvPreprocessPipeline(const wchar_t *src_file, const wchar_t *dst_file, const wchar_t *pipeline, long *ret) {
    *ret = 0;
    if (src_file == nullptr || dst_file == nullptr || pipeline == nullptr || pipeline[0] == L'\0') {
        return;
    }

    opcv::ImageHandle current;
    if (!opcv::LoadImageFromFile(src_file, current)) {
        return;
    }

    std::vector<std::wstring> steps;
    split(pipeline, steps, L"|");

    bool applied = false;
    for (const auto &step : steps) {
        std::wstring name;
        std::vector<std::wstring> args;
        if (step.empty()) {
            continue;
        }
        if (!parse_cv_pipeline_step(step, name, args)) {
            return;
        }

        opcv::ImageHandle next;
        if (!cv_pipeline_apply_step(name, args, current, next)) {
            return;
        }

        current = std::move(next);
        applied = true;
    }

    if (!applied || !opcv::SaveImageToFile(current, dst_file)) {
        return;
    }

    *ret = 1;
}

void libop::CvCrop(const wchar_t *src_file, long x, long y, long width, long height, const wchar_t *dst_file,
                   long *ret) {
    *ret = 0;
    if (src_file == nullptr || dst_file == nullptr) {
        return;
    }

    opcv::ImageHandle source;
    opcv::ImageHandle output;
    opcv::Region region;
    region.x = static_cast<int>(x);
    region.y = static_cast<int>(y);
    region.width = static_cast<int>(width);
    region.height = static_cast<int>(height);
    if (!opcv::LoadImageFromFile(src_file, source) || !opcv::Crop(source, region, output) ||
        !opcv::SaveImageToFile(output, dst_file)) {
        return;
    }

    *ret = 1;
}

void libop::CvResize(const wchar_t *src_file, long width, long height, const wchar_t *dst_file, long *ret) {
    *ret = 0;
    if (src_file == nullptr || dst_file == nullptr) {
        return;
    }

    opcv::ImageHandle source;
    opcv::ImageHandle output;
    if (!opcv::LoadImageFromFile(src_file, source) ||
        !opcv::Resize(source, static_cast<int>(width), static_cast<int>(height), output) ||
        !opcv::SaveImageToFile(output, dst_file)) {
        return;
    }

    *ret = 1;
}

void libop::CvThreshold(const wchar_t *src_file, const wchar_t *dst_file, double threshold, double max_value,
                        const wchar_t *mode, long *ret) {
    *ret = 0;
    if (src_file == nullptr || dst_file == nullptr) {
        return;
    }

    opcv::ThresholdMode threshold_mode;
    if (!parse_cv_threshold_mode(mode, threshold_mode)) {
        return;
    }

    opcv::ImageHandle source;
    opcv::ImageHandle output;
    if (!opcv::LoadImageFromFile(src_file, source) ||
        !opcv::Threshold(source, output, threshold, max_value, threshold_mode) ||
        !opcv::SaveImageToFile(output, dst_file)) {
        return;
    }

    *ret = 1;
}

void libop::CvInRange(const wchar_t *src_file, const wchar_t *dst_file, const wchar_t *color_space,
                      const wchar_t *lower, const wchar_t *upper, long *ret) {
    *ret = 0;
    if (src_file == nullptr || dst_file == nullptr) {
        return;
    }

    opcv::InRangeColorSpace parsed_color_space;
    std::vector<double> lower_values;
    std::vector<double> upper_values;
    if (!parse_cv_color_space(color_space, parsed_color_space) || !parse_cv_number_list(lower, lower_values) ||
        !parse_cv_number_list(upper, upper_values)) {
        return;
    }

    opcv::ImageHandle source;
    opcv::ImageHandle output;
    if (!opcv::LoadImageFromFile(src_file, source) ||
        !opcv::InRange(source, output, parsed_color_space, lower_values, upper_values) ||
        !opcv::SaveImageToFile(output, dst_file)) {
        return;
    }

    *ret = 1;
}

void libop::CvMorphology(const wchar_t *src_file, const wchar_t *dst_file, const wchar_t *mode, long kernel_size,
                         long iterations, long *ret) {
    *ret = 0;
    if (src_file == nullptr || dst_file == nullptr) {
        return;
    }

    opcv::MorphologyMode morphology_mode;
    if (!parse_cv_morphology_mode(mode, morphology_mode)) {
        return;
    }

    opcv::ImageHandle source;
    opcv::ImageHandle output;
    if (!opcv::LoadImageFromFile(src_file, source) ||
        !opcv::Morphology(source, output, morphology_mode, static_cast<int>(kernel_size), static_cast<int>(iterations)) ||
        !opcv::SaveImageToFile(output, dst_file)) {
        return;
    }

    *ret = 1;
}

void libop::CvThin(const wchar_t *src_file, const wchar_t *dst_file, const wchar_t *mode, long *ret) {
    *ret = 0;
    if (src_file == nullptr || dst_file == nullptr) {
        return;
    }

    opcv::ThinMode thin_mode;
    if (!parse_cv_thin_mode(mode, thin_mode)) {
        return;
    }

    opcv::ImageHandle source;
    opcv::ImageHandle output;
    if (!opcv::LoadImageFromFile(src_file, source) || !opcv::Thin(source, output, thin_mode) ||
        !opcv::SaveImageToFile(output, dst_file)) {
        return;
    }

    *ret = 1;
}

void libop::CvMatchTemplate(long x, long y, long width, long height, const wchar_t *template_name, double threshold,
                            long dir, long strip_mode, long method, long color_mode, std::wstring &retjson, long *ret) {
    retjson = L"{\"ok\":0}";
    *ret = 0;
    if (template_name == nullptr) {
        return;
    }

    opcv::ImageHandle source;
    opcv::Region region;
    int origin_x = 0;
    int origin_y = 0;
    if (!opcv::bridge::CaptureRegion(m_context->bkproc, m_context->image_proc, x, y, width, height, source, region,
                                     origin_x, origin_y)) {
        return;
    }

    opcv::MatchResult match;
    const bool ok =
        opcv::MatchTemplate(source, template_name, region, threshold, match, static_cast<opcv::SearchDirection>(dir),
                            static_cast<opcv::StripMode>(strip_mode), method, opcv::bridge::ParseColorMode(color_mode));
    if (ok) {
        match.x += origin_x;
        match.y += origin_y;
    }
    retjson = opcv::bridge::BuildMatchJson(match, ok);
    *ret = ok ? 1 : 0;
}

void libop::CvMatchTemplateScale(long x, long y, long width, long height, const wchar_t *template_name,
                                 const wchar_t *scales, double threshold, long method, long color_mode,
                                 std::wstring &retjson, long *ret) {
    retjson = L"{\"ok\":0}";
    *ret = 0;
    if (template_name == nullptr) {
        return;
    }

    std::vector<double> scale_values;
    if (!opcv::bridge::ParseScaleList(scales, scale_values)) {
        return;
    }

    opcv::ImageHandle source;
    opcv::Region region;
    int origin_x = 0;
    int origin_y = 0;
    if (!opcv::bridge::CaptureRegion(m_context->bkproc, m_context->image_proc, x, y, width, height, source, region,
                                     origin_x, origin_y)) {
        return;
    }

    opcv::MatchResult match;
    const bool ok = opcv::MatchTemplateScale(source, template_name, region, scale_values, threshold, match, method,
                                             opcv::bridge::ParseColorMode(color_mode));
    if (ok) {
        match.x += origin_x;
        match.y += origin_y;
    }
    retjson = opcv::bridge::BuildMatchJson(match, ok);
    *ret = ok ? 1 : 0;
}

void libop::CvMatchAnyTemplate(long x, long y, long width, long height, const wchar_t *template_names, double threshold,
                               long dir, long strip_mode, long method, long color_mode, std::wstring &retjson,
                               long *ret) {
    retjson = L"{\"ok\":0}";
    *ret = 0;

    std::vector<std::wstring> names;
    if (!opcv::bridge::ParseTemplateNames(template_names, names)) {
        return;
    }

    opcv::ImageHandle source;
    opcv::Region region;
    int origin_x = 0;
    int origin_y = 0;
    if (!opcv::bridge::CaptureRegion(m_context->bkproc, m_context->image_proc, x, y, width, height, source, region,
                                     origin_x, origin_y)) {
        return;
    }

    opcv::NamedMatchResult match;
    const bool ok = opcv::MatchAnyTemplate(
        source, names, region, threshold, match, static_cast<opcv::SearchDirection>(dir),
        static_cast<opcv::StripMode>(strip_mode), method, opcv::bridge::ParseColorMode(color_mode));
    if (ok) {
        match.match.x += origin_x;
        match.match.y += origin_y;
    }
    retjson = opcv::bridge::BuildNamedMatchJson(match, ok);
    *ret = ok ? 1 : 0;
}

void libop::CvMatchAllTemplates(long x, long y, long width, long height, const wchar_t *template_names,
                                double threshold, long dir, long strip_mode, long method, long color_mode,
                                std::wstring &retjson, long *ret) {
    retjson = L"{\"ok\":0,\"results\":[]}";
    *ret = 0;

    std::vector<std::wstring> names;
    if (!opcv::bridge::ParseTemplateNames(template_names, names)) {
        return;
    }

    opcv::ImageHandle source;
    opcv::Region region;
    int origin_x = 0;
    int origin_y = 0;
    if (!opcv::bridge::CaptureRegion(m_context->bkproc, m_context->image_proc, x, y, width, height, source, region,
                                     origin_x, origin_y)) {
        return;
    }

    std::vector<opcv::NamedMatchResult> matches;
    const bool ok = opcv::MatchAllTemplates(
        source, names, region, threshold, matches, static_cast<opcv::SearchDirection>(dir),
        static_cast<opcv::StripMode>(strip_mode), method, opcv::bridge::ParseColorMode(color_mode));
    if (ok) {
        for (auto &match : matches) {
            match.match.x += origin_x;
            match.match.y += origin_y;
        }
    }
    retjson = opcv::bridge::BuildAllMatchesJson(matches, ok);
    *ret = ok ? 1 : 0;
}

void libop::CvFeatureMatchTemplate(long x, long y, long width, long height, const wchar_t *template_name,
                                   double threshold, std::wstring &retjson, long *ret) {
    retjson = L"{\"ok\":0}";
    *ret = 0;
    if (template_name == nullptr) {
        return;
    }

    opcv::ImageHandle source;
    opcv::Region region;
    int origin_x = 0;
    int origin_y = 0;
    if (!opcv::bridge::CaptureRegion(m_context->bkproc, m_context->image_proc, x, y, width, height, source, region,
                                     origin_x, origin_y)) {
        return;
    }

    opcv::MatchResult match;
    const bool ok = opcv::FeatureMatchTemplate(source, template_name, region, threshold, match);
    if (ok) {
        match.x += origin_x;
        match.y += origin_y;
    }
    retjson = opcv::bridge::BuildMatchJson(match, ok);
    *ret = ok ? 1 : 0;
}

void libop::CvEdgeMatchTemplate(long x, long y, long width, long height, const wchar_t *template_name, double threshold,
                                std::wstring &retjson, long *ret) {
    retjson = L"{\"ok\":0}";
    *ret = 0;
    if (template_name == nullptr) {
        return;
    }

    opcv::ImageHandle source;
    opcv::Region region;
    int origin_x = 0;
    int origin_y = 0;
    if (!opcv::bridge::CaptureRegion(m_context->bkproc, m_context->image_proc, x, y, width, height, source, region,
                                     origin_x, origin_y)) {
        return;
    }

    opcv::MatchResult match;
    const bool ok = opcv::EdgeMatchTemplate(source, template_name, region, threshold, match);
    if (ok) {
        match.x += origin_x;
        match.y += origin_y;
    }
    retjson = opcv::bridge::BuildMatchJson(match, ok);
    *ret = ok ? 1 : 0;
}

void libop::CvShapeMatchTemplate(long x, long y, long width, long height, const wchar_t *template_name,
                                 double threshold, std::wstring &retjson, long *ret) {
    retjson = L"{\"ok\":0}";
    *ret = 0;
    if (template_name == nullptr) {
        return;
    }

    opcv::ImageHandle source;
    opcv::Region region;
    int origin_x = 0;
    int origin_y = 0;
    if (!opcv::bridge::CaptureRegion(m_context->bkproc, m_context->image_proc, x, y, width, height, source, region,
                                     origin_x, origin_y)) {
        return;
    }

    opcv::MatchResult match;
    const bool ok = opcv::ShapeMatchTemplate(source, template_name, region, threshold, match);
    if (ok) {
        match.x += origin_x;
        match.y += origin_y;
    }
    retjson = opcv::bridge::BuildMatchJson(match, ok);
    *ret = ok ? 1 : 0;
}

long libop::SetOcrEngine(const wchar_t *path_of_engine, const wchar_t *dll_name, const wchar_t *argv) {
    string argvs = argv ? _ws2string(argv) : "";
    vector<string> vstr;
    split(argvs, vstr, " ");
    const std::wstring engine = path_of_engine ? path_of_engine : L"";
    const std::wstring dll = dll_name ? dll_name : L"";
    return OcrWrapper::getInstance()->init(engine, dll, vstr) == 0 ? 1 : 0;
}
// 设置字库文件
void libop::SetDict(long idx, const wchar_t *file_name, long *ret) {
    *ret = m_context->image_proc.SetDict(idx, file_name);
}

void libop::GetDict(long idx, long font_index, std::wstring &retstr) {
    retstr = m_context->image_proc.GetDict(idx, font_index);
}

// 设置内存字库文件
void libop::SetMemDict(long idx, const wchar_t *data, long size, long *ret) {
    *ret = m_context->image_proc.SetMemDict(idx, (void *)data, size);
}

// 使用哪个字库文件进行识别
void libop::UseDict(long idx, long *ret) {
    *ret = m_context->image_proc.UseDict(idx);
}

// 给指定的字库中添加一条字库信息
void libop::AddDict(long idx, const wchar_t *dict_info, long *ret) {
    *ret = m_context->image_proc.AddDict(idx, dict_info);
}
void libop::SaveDict(long idx, const wchar_t *file_name, long *ret) {
    *ret = m_context->image_proc.SaveDict(idx, file_name);
}
// 清空指定的字库
void libop::ClearDict(long idx, long *ret) {
    *ret = m_context->image_proc.ClearDict(idx);
}
// 获取指定的字库中的字符数量
void libop::GetDictCount(long idx, long *ret) {
    *ret = m_context->image_proc.GetDictCount(idx);
}
// 获取当前使用的字库序号
void libop::GetNowDict(long *ret) {
    *ret = m_context->image_proc.GetNowDict();
}
// 根据指定的范围,以及指定的颜色描述，提取点阵信息，类似于大漠工具里的单独提取
void libop::FetchWord(long x1, long y1, long x2, long y2, const wchar_t *color, const wchar_t *word,
                      std::wstring &retstr) {
    wstring str;
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            rect_t rc;
            rc.x1 = rc.y1 = 0;
            rc.x2 = x2 - x1;
            rc.y2 = y2 - y1;
            str = m_context->image_proc.FetchWord(rc, color, word);
        }
    }
    retstr = str;
}
// 识别这个范围内所有满足条件的词组，这个识别函数不会用到字库. 只是识别大概形状的位置
void libop::GetWordsNoDict(long x1, long y1, long x2, long y2, const wchar_t *color, std::wstring &retstr) {
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
void libop::GetWordResultCount(const wchar_t *result, long *ret) {
    if (ret)
        *ret = 0;
    if (!result || !ret)
        return;

    long cnt = 0;
    const wchar_t *p = result;
    while (*p) {
        if (*p == L'/')
            ++cnt;
        ++p;
    }
    *ret = cnt;
}
// 在使用GetWords进行词组识别以后,可以用此接口进行识别各个词组的坐标
void libop::GetWordResultPos(const wchar_t *result, long index, long *x, long *y, long *ret) {
    if (ret)
        *ret = 0;
    if (x)
        *x = 0;
    if (y)
        *y = 0;
    if (!result || !x || !y || !ret || index < 0)
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
                *x = parsed_x;
                *y = parsed_y;
                *ret = 1;
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
void libop::GetWordResultStr(const wchar_t *result, long index, std::wstring &ret_str) {
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
void libop::Ocr(long x1, long y1, long x2, long y2, const wchar_t *color, DOUBLE sim, std::wstring &retstr) {
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
void libop::OcrEx(long x1, long y1, long x2, long y2, const wchar_t *color, DOUBLE sim, std::wstring &retstr) {
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
void libop::FindStr(long x1, long y1, long x2, long y2, const wchar_t *strs, const wchar_t *color, DOUBLE sim,
                    long *retx, long *rety, long *ret) {
    wstring str;
    *retx = *rety = -1;
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            *ret = m_context->image_proc.FindStr(strs, color, sim, *retx, *rety);
        }
    }
}
// 返回符合color_format的所有坐标位置
void libop::FindStrEx(long x1, long y1, long x2, long y2, const wchar_t *strs, const wchar_t *color, DOUBLE sim,
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

void libop::OcrAuto(long x1, long y1, long x2, long y2, DOUBLE sim, std::wstring &retstr) {
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
void libop::OcrFromFile(const wchar_t *file_name, const wchar_t *color_format, DOUBLE sim, std::wstring &retstr) {
    wstring str;
    m_context->image_proc.OcrFromFile(file_name, color_format, sim, str);
    retstr = str;
}
// 从文件中识别图片,无需指定颜色
void libop::OcrAutoFromFile(const wchar_t *file_name, DOUBLE sim, std::wstring &retstr) {
    wstring str;
    m_context->image_proc.OcrAutoFromFile(file_name, sim, str);
    retstr = str;
}

void libop::FindLine(long x1, long y1, long x2, long y2, const wchar_t *color, double sim, wstring &retstr) {
    if (m_context->bkproc.check_bind() && m_context->bkproc.RectConvert(x1, y1, x2, y2)) {
        if (!m_context->bkproc.requestCapture(x1, y1, x2 - x1, y2 - y1, m_context->image_proc._src)) {
            setlog("error requestCapture");
        } else {
            m_context->image_proc.set_offset(x1, y1);
            m_context->image_proc.FindLine(color, sim, retstr);
        }
    }
}

void libop::WriteData(LONG_PTR hwnd, const wchar_t *address, const wchar_t *data, long size, long *ret) {
    *ret = 0;
    MemoryEx mem;
    *ret = mem.WriteData(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address, data, size);
}
// 读取数据
void libop::ReadData(LONG_PTR hwnd, const wchar_t *address, long size, std::wstring &retstr) {
    MemoryEx mem;
    retstr = mem.ReadData(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address, size);
}
