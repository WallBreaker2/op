#include "OpenCvBridge.h"

#include "../core/helpfunc.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace opcv::bridge {

namespace {

std::wstring EscapeJsonString(const std::wstring &value) {
    std::wstring escaped;
    escaped.reserve(value.size() + 8);
    for (wchar_t ch : value) {
        switch (ch) {
        case L'\\':
            escaped += L"\\\\";
            break;
        case L'"':
            escaped += L"\\\"";
            break;
        case L'\b':
            escaped += L"\\b";
            break;
        case L'\f':
            escaped += L"\\f";
            break;
        case L'\n':
            escaped += L"\\n";
            break;
        case L'\r':
            escaped += L"\\r";
            break;
        case L'\t':
            escaped += L"\\t";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }
    return escaped;
}

std::wstring FormatJsonDouble(double value) {
    std::wostringstream oss;
    oss << std::fixed << std::setprecision(6) << value;
    return oss.str();
}

} // namespace

MatchColorMode ParseColorMode(long color_mode) {
    return color_mode == 0 ? MatchColorMode::Color : MatchColorMode::Gray;
}

std::wstring BuildMatchJson(const MatchResult &match, bool ok) {
    if (!ok) {
        return L"{\"ok\":0}";
    }

    std::wostringstream oss;
    oss << L"{\"ok\":1,\"x\":" << match.x << L",\"y\":" << match.y << L",\"width\":" << match.width
        << L",\"height\":" << match.height << L",\"score\":" << FormatJsonDouble(match.score) << L"}";
    return oss.str();
}

std::wstring BuildNamedMatchJson(const NamedMatchResult &match, bool ok) {
    if (!ok) {
        return L"{\"ok\":0}";
    }

    std::wostringstream oss;
    oss << L"{\"ok\":1,\"name\":\"" << EscapeJsonString(match.name) << L"\",\"x\":" << match.match.x << L",\"y\":"
        << match.match.y << L",\"width\":" << match.match.width << L",\"height\":" << match.match.height
        << L",\"score\":" << FormatJsonDouble(match.match.score) << L"}";
    return oss.str();
}

std::wstring BuildAllMatchesJson(const std::vector<NamedMatchResult> &matches, bool ok) {
    std::wostringstream oss;
    oss << L"{\"ok\":" << (ok ? 1 : 0) << L",\"results\":[";
    for (size_t i = 0; i < matches.size(); ++i) {
        if (i != 0) {
            oss << L",";
        }
        oss << L"{\"name\":\"" << EscapeJsonString(matches[i].name) << L"\",\"x\":" << matches[i].match.x << L",\"y\":"
            << matches[i].match.y << L",\"width\":" << matches[i].match.width << L",\"height\":"
            << matches[i].match.height << L",\"score\":" << FormatJsonDouble(matches[i].match.score) << L"}";
    }
    oss << L"]}";
    return oss.str();
}

bool ParseTemplateNames(const wchar_t *text, std::vector<std::wstring> &names) {
    names.clear();
    if (text == nullptr || text[0] == L'\0') {
        return false;
    }

    split(text, names, L"|");
    names.erase(
        std::remove_if(names.begin(), names.end(), [](const std::wstring &value) { return value.empty(); }),
        names.end());
    return !names.empty();
}

bool ParseScaleList(const wchar_t *text, std::vector<double> &scales) {
    scales.clear();
    if (text == nullptr || text[0] == L'\0') {
        return true;
    }

    std::vector<std::wstring> parts;
    split(text, parts, L"|");
    for (const auto &part : parts) {
        if (part.empty()) {
            continue;
        }
        try {
            const double scale = std::stod(part);
            if (scale > 0.0) {
                scales.push_back(scale);
            }
        } catch (...) {
            return false;
        }
    }
    return true;
}

bool CaptureRegion(
    opBackground &background,
    ImageProc &image_proc,
    long x,
    long y,
    long width,
    long height,
    ImageHandle &source,
    Region &region,
    int &origin_x,
    int &origin_y) {
    source = {};
    region = {};
    origin_x = 0;
    origin_y = 0;

    if (width <= 0 || height <= 0) {
        return false;
    }

    long x1 = x;
    long y1 = y;
    long x2 = x + width;
    long y2 = y + height;
    if (!background.check_bind() || !background.RectConvert(x1, y1, x2, y2)) {
        return false;
    }
    if (!background.requestCapture(x1, y1, x2 - x1, y2 - y1, image_proc._src)) {
        setlog("error requestCapture");
        return false;
    }

    image_proc.set_offset(x1, y1);
    auto &img = image_proc._src;
    if (img.empty() || img.pdata == nullptr || img.width <= 0 || img.height <= 0) {
        return false;
    }

    source.width = img.width;
    source.height = img.height;
    source.channels = 4;
    source.bytes.assign(img.pdata, img.pdata + static_cast<size_t>(img.size() * 4));

    region.x = 0;
    region.y = 0;
    region.width = img.width;
    region.height = img.height;
    origin_x = x1;
    origin_y = y1;
    return true;
}

} // namespace opcv::bridge
