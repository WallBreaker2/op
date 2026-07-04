#include "YoloDetector.h"
#include "../network/HttpClient.h"
#include "../base/Utils.h"
#include <iostream>
#include <regex>

using std::cout;
using std::endl;

namespace op::yolo {

namespace {
constexpr const char *kYoloDefaultEndpoint = "http://127.0.0.1:8090/api/v1/detect";
constexpr const char *kYoloDefaultPathSuffix = "api/v1/detect";

bool try_resolve_yolo_backend(const std::string &candidate, std::string &endpoint) {
    const std::string key = to_lower_copy(trim_copy(candidate));
    if (key == "yolo" || key == "yolo11" || key == "yolov11" || key == "yolo_http" || key == "yolo_server") {
        endpoint = kYoloDefaultEndpoint;
        return true;
    }
    return false;
}

std::string resolve_default_endpoint() {
    std::string endpoint;
    if (resolve_endpoint_candidate(getenv_trimmed("OP_YOLO_URL"), endpoint, try_resolve_yolo_backend)) {
        return endpoint;
    }
    if (resolve_endpoint_candidate(getenv_trimmed("OP_YOLO_BACKEND"), endpoint, try_resolve_yolo_backend)) {
        return endpoint;
    }
    return kYoloDefaultEndpoint;
}

int resolve_default_timeout_ms() {
    return parse_positive_int(getenv_trimmed("OP_YOLO_TIMEOUT_MS"), 3000);
}
} // namespace

YoloDetector::YoloDetector() : m_endpoint(resolve_default_endpoint()), m_timeout_ms(resolve_default_timeout_ms()) {
    if (!normalize_endpoint(m_endpoint, kYoloDefaultPathSuffix))
        m_endpoint = kYoloDefaultEndpoint;
    cout << "YoloDetector::YoloDetector(), endpoint=" << m_endpoint << endl;
}

YoloDetector::~YoloDetector() {
    release();
}

YoloDetector *YoloDetector::getInstance() {
    static YoloDetector sYoloEngine;
    return &sYoloEngine;
}

int YoloDetector::init(const std::wstring &engine, const std::wstring &dllName, const vector<string> &argvs) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Resolve fallback endpoint/timeout if not yet set
    if (m_endpoint.empty() || m_timeout_ms <= 0) {
        std::string cur = m_endpoint.empty() ? resolve_default_endpoint() : m_endpoint;
        int timeout = m_timeout_ms > 0 ? m_timeout_ms : resolve_default_timeout_ms();
        m_endpoint = cur;
        m_timeout_ms = timeout;
    }

    return configure_endpoint(m_endpoint, m_timeout_ms, engine, dllName, argvs, try_resolve_yolo_backend,
                              kYoloDefaultPathSuffix, "SetYoloEngine", nullptr, nullptr, 3000);
}

int YoloDetector::release() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return 0;
}

int YoloDetector::detect(byte *data, int w, int h, int bpp, double conf, double iou, vyolo_rec_t &result) {
    const std::lock_guard<std::mutex> lock(m_mutex);
    result.clear();
    if (data == nullptr || w <= 0 || h <= 0 || (bpp != 1 && bpp != 3 && bpp != 4))
        return -1;
    const size_t pixel_bytes_size_t = static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(bpp);
    if (pixel_bytes_size_t > 64 * 1024 * 1024)
        return -2;

    std::string image_b64;
    if (!base64_encode(data, static_cast<int>(pixel_bytes_size_t), image_b64))
        return -3;

    std::string req = "{\"image\":\"" + image_b64 + "\",\"width\":" + std::to_string(w) +
                      ",\"height\":" + std::to_string(h) + ",\"bpp\":" + std::to_string(bpp) +
                      ",\"conf\":" + std::to_string(conf) + ",\"iou\":" + std::to_string(iou) + "}";

    ParsedUrl parsed;
    if (!parse_url(m_endpoint, parsed))
        return -4;

    std::string resp;
    DWORD status_code = 0;
    if (!http_post_json(parsed, req, m_timeout_ms, resp, status_code, L"op-yolo-client/1.0"))
        return -5;
    if (status_code != 200)
        return -6;

    static const std::regex code_re("\\\"code\\\"\\s*:\\s*(-?\\d+)");
    std::smatch code_match;
    if (!std::regex_search(resp, code_match, code_re))
        return -7;
    if (atoi(code_match[1].str().c_str()) != 0)
        return -8;

    static const std::regex obj_re("\\{[^\\{\\}]*\\}");
    static const std::regex label_re("\\\"(?:label|class_name|name)\\\"\\s*:\\s*\\\"((?:\\\\.|[^\\\"\\\\])*)\\\"");
    static const std::regex class_id_re("\\\"(?:class_id|cls)\\\"\\s*:\\s*(-?\\d+)");
    static const std::regex bbox_re("\\\"bbox\\\"\\s*:\\s*\\[\\s*(-?\\d+)\\s*,\\s*(-?\\d+)\\s*,\\s*(-?\\d+)\\s*,\\s*(-?\\d+)\\s*\\]");
    static const std::regex conf_re("\\\"(?:confidence|conf)\\\"\\s*:\\s*([-+]?\\d*\\.?\\d+(?:[eE][-+]?\\d+)?)");

    int n = 0;
    for (std::sregex_iterator it(resp.begin(), resp.end(), obj_re), end; it != end; ++it) {
        const std::string obj = it->str();
        std::smatch bbox_m;
        std::smatch conf_m;
        if (!std::regex_search(obj, bbox_m, bbox_re) || !std::regex_search(obj, conf_m, conf_re))
            continue;
        yolo_rec_t rec;
        std::smatch label_m;
        std::smatch class_id_m;
        if (std::regex_search(obj, label_m, label_re))
            rec.label = _s2wstring(utf8_to_ansi(json_unescape(label_m[1].str())));
        if (std::regex_search(obj, class_id_m, class_id_re))
            rec.class_id = atoi(class_id_m[1].str().c_str());
        rec.left_top = point_t(atoi(bbox_m[1].str().c_str()), atoi(bbox_m[2].str().c_str()));
        rec.right_bottom = point_t(atoi(bbox_m[3].str().c_str()), atoi(bbox_m[4].str().c_str()));
        rec.confidence = static_cast<float>(atof(conf_m[1].str().c_str()));
        result.push_back(rec);
        ++n;
    }

    if (n == 0) {
        static const std::regex empty_result_re("\\\"results\\\"\\s*:\\s*\\[\\s*\\]");
        if (!std::regex_search(resp, empty_result_re))
            return -9;
    }
    return n;
}

} // namespace op::yolo
