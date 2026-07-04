#include "OpContext.h"
#include "OpCaptureHelpers.h"
#include "OpResult.h"

#include "image/Image.h"
#include "runtime/RuntimeUtils.h"
#include "yolo/YoloDetector.h"

#include <libop.h>

#include <string>
#include <vector>

namespace {

static std::wstring json_escape(const std::wstring &text) {
    std::wstring out;
    for (const auto ch : text) {
        switch (ch) {
        case L'\\':
            out += L"\\\\";
            break;
        case L'"':
            out += L"\\\"";
            break;
        case L'\n':
            out += L"\\n";
            break;
        case L'\r':
            out += L"\\r";
            break;
        case L'\t':
            out += L"\\t";
            break;
        default:
            out += ch;
            break;
        }
    }
    return out;
}

static void build_yolo_json(const op::vyolo_rec_t &items, std::wstring &retjson) {
    retjson = L"{\"code\":0,\"results\":[";
    bool first = true;
    for (const auto &it : items) {
        if (!first)
            retjson += L",";
        first = false;
        retjson += L"{\"class_id\":";
        retjson += std::to_wstring(it.class_id);
        retjson += L",\"label\":\"";
        retjson += json_escape(it.label);
        retjson += L"\",\"bbox\":[";
        retjson += std::to_wstring(it.left_top.x);
        retjson += L",";
        retjson += std::to_wstring(it.left_top.y);
        retjson += L",";
        retjson += std::to_wstring(it.right_bottom.x);
        retjson += L",";
        retjson += std::to_wstring(it.right_bottom.y);
        retjson += L"],\"confidence\":";
        retjson += std::to_wstring(it.confidence);
        retjson += L"}";
    }
    retjson += L"]}";
}

} // namespace

long op::Op::SetYoloEngine(const wchar_t *path_of_engine, const wchar_t *dll_name, const wchar_t *argv) {
    string argvs = argv ? _ws2string(argv) : "";
    vector<string> vstr;
    split(argvs, vstr, " ");
    const std::wstring engine = path_of_engine ? path_of_engine : L"";
    const std::wstring dll = dll_name ? dll_name : L"";
    return op::yolo::YoloDetector::getInstance()->init(engine, dll, vstr) == 0 ? 1 : 0;
}
void op::Op::YoloDetect(long x1, long y1, long x2, long y2, double conf, double iou, std::wstring &retjson, long *ret) {
    retjson.clear();
    internal::set_result(ret, 0L);
    internal::with_captured_region(m_context.get(), x1, y1, x2, y2, [&]() {
        vyolo_rec_t res;
        const int n =
            op::yolo::YoloDetector::getInstance()->detect(m_context->image_proc._src.pdata, m_context->image_proc._src.width,
                                               m_context->image_proc._src.height, 4, conf, iou, res);
        if (n < 0)
            return;
        for (auto &it : res) {
            it.left_top.x += static_cast<int>(x1);
            it.left_top.y += static_cast<int>(y1);
            it.right_bottom.x += static_cast<int>(x1);
            it.right_bottom.y += static_cast<int>(y1);
        }
        build_yolo_json(res, retjson);
        internal::set_result(ret, n);
    });
}

void op::Op::YoloDetectFromFile(const wchar_t *file_name, double conf, double iou, std::wstring &retjson, long *ret) {
    retjson.clear();
    internal::set_result(ret, 0L);
    std::wstring fullpath;
    if (!Path2GlobalPath(file_name ? file_name : L"", m_context->curr_path, fullpath))
        return;
    Image img;
    if (!img.read(fullpath.data()))
        return;
    vyolo_rec_t res;
    const int n = op::yolo::YoloDetector::getInstance()->detect(img.pdata, img.width, img.height, 4, conf, iou, res);
    if (n < 0)
        return;
    build_yolo_json(res, retjson);
    internal::set_result(ret, n);
}
