#include "YoloResponseParser.h"

#include "../base/JsonScanner.h"
#include "../base/Utils.h"

namespace op::yolo {

namespace {

// 服务端对同类字段有几种写法，按历史实现的顺序逐个试。
const char *const kLabelKeys[] = {"\"label\"", "\"class_name\"", "\"name\""};
const char *const kClassIdKeys[] = {"\"class_id\"", "\"cls\""};
const char *const kConfidenceKeys[] = {"\"confidence\"", "\"conf\""};

} // namespace

int parse_yolo_response(const std::string &body, vyolo_rec_t &out, int *server_code) {
    out.clear();
    const char *const begin = body.data();
    const char *const end = begin + body.size();

    const char *code_pos = nullptr;
    const int code_ret = json::check_code(begin, end, server_code, &code_pos);
    if (code_ret != 0)
        return code_ret;

    // 每个检测结果都是扁平对象，不嵌套；"下一个 '{' 到下一个 '}'"就是一条的边界。
    const char *p = code_pos;
    int count = 0;
    while (true) {
        const char *obj_begin = json::find_literal(p, end, "{");
        if (!obj_begin)
            break;
        const char *obj_end = json::find_literal(obj_begin + 1, end, "}");
        if (!obj_end)
            break;

        yolo_rec_t rec;
        // bbox 与 confidence 是必需字段，缺任何一个就跳过这条（与旧实现一致）。
        const char *conf_pos = nullptr;
        double confidence = 0.0;
        if (json::parse_bbox(obj_begin, obj_end, &rec.left_top, &rec.right_bottom) &&
            json::locate_first_value(obj_begin, obj_end, kConfidenceKeys, 2, &conf_pos) &&
            json::scan_double(conf_pos, obj_end, &confidence)) {
            rec.confidence = static_cast<float>(confidence);

            const char *label_pos = nullptr;
            if (json::locate_first_value(obj_begin, obj_end, kLabelKeys, 3, &label_pos)) {
                std::string raw;
                const char *label_scan = label_pos;
                if (json::scan_json_string(label_scan, obj_end, &raw)) {
                    // 旧实现走 "UTF-8 → GBK → ACP → 宽字符"，非 GBK 标签（韩文、泰文等）
                    // 会被压成 '?'；这里直接 UTF-8 转宽字符（§72.34）。
                    rec.label = utf8_to_wstring(json::unescape(raw));
                }
            }

            const char *class_pos = nullptr;
            if (json::locate_first_value(obj_begin, obj_end, kClassIdKeys, 2, &class_pos)) {
                long long class_id = 0;
                if (json::scan_long(class_pos, obj_end, &class_id))
                    rec.class_id = static_cast<int>(class_id);
            }

            out.push_back(rec);
            ++count;
        }
        p = obj_end + 1;
    }

    if (count > 0)
        return count;
    if (json::has_empty_results_array(begin, end))
        return 0;
    return -9;
}

} // namespace op::yolo
