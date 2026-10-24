#include "OcrResponseParser.h"

#include "../base/JsonScanner.h"
#include "../base/Utils.h"

namespace op::ocr {

int parse_ocr_response(const std::string &body, vocr_rec_t &out, int *server_code) {
    out.clear();
    const char *const begin = body.data();
    const char *const end = begin + body.size();

    // 1) code 必须存在且为 0。
    const char *code_pos = nullptr;
    const int code_ret = json::check_code(begin, end, server_code, &code_pos);
    if (code_ret != 0)
        return code_ret;

    // 2) 逐个扁平对象扫结果。服务端每个结果都是 `{...}` 且不嵌套，所以"下一个 '{' 到下一个 '}'"就是边界。
    const char *p = code_pos;
    while (true) {
        const char *obj_begin = json::find_literal(p, end, "{");
        if (!obj_begin)
            break;
        const char *obj_end = json::find_literal(obj_begin + 1, end, "}");
        if (!obj_end)
            break;

        const char *value = nullptr;
        ocr_rec_t rec;
        if (json::locate_value(obj_begin, obj_end, "\"text\"", &value)) {
            std::string raw;
            const char *text_pos = value;
            const char *conf_pos = nullptr;
            double confidence = 0.0;
            if (json::scan_json_string(text_pos, obj_end, &raw) &&
                json::parse_bbox(obj_begin, obj_end, &rec.left_top, &rec.right_bottom) &&
                json::locate_value(obj_begin, obj_end, "\"confidence\"", &conf_pos) &&
                json::scan_double(conf_pos, obj_end, &confidence)) {
                rec.confidence = static_cast<float>(confidence);
                // 服务端给的是 UTF-8：直接转宽字符。旧实现走 "UTF-8 → GBK → ACP → 宽字符"，
                // 非 GBK 字符（韩文、emoji 等）会被压成 '?'，是实打实的丢字（§72.33）。
                rec.text = utf8_to_wstring(json::unescape(raw));
                out.push_back(rec);
            }
        }
        p = obj_end + 1;
    }

    if (!out.empty())
        return static_cast<int>(out.size());

    // 3) 没解析到结果时，只有明确写了 `"results":[]` 才算"空结果是正常的"。
    if (json::has_empty_results_array(begin, end))
        return 0;
    return -9;
}

} // namespace op::ocr
