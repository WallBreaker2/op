#include "YoloDetector.h"

#include "YoloResponseParser.h"
#include "../network/HttpClient.h"
#include "../base/Utils.h"
#include <iostream>

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

    // 请求体一次成型：base64 直接编进 req 的尾部，省掉“临时串 + 再拼一遍”（§72.34）。
    std::string req;
    req.reserve(base64_encoded_size(pixel_bytes_size_t) + 128);
    req += "{\"image\":\"";
    if (!base64_encode_append(data, pixel_bytes_size_t, req))
        return -3;
    req += "\",\"width\":";
    req += std::to_string(w);
    req += ",\"height\":";
    req += std::to_string(h);
    req += ",\"bpp\":";
    req += std::to_string(bpp);
    req += ",\"conf\":";
    req += std::to_string(conf);
    req += ",\"iou\":";
    req += std::to_string(iou);
    req += "}";

    ParsedUrl parsed;
    if (!parse_url(m_endpoint, parsed))
        return -4;

    std::string resp;
    DWORD status_code = 0;
    if (!http_post_json(parsed, req, m_timeout_ms, resp, status_code, L"op-yolo-client/1.0"))
        return -5;
    if (status_code != 200)
        return -6;

    int server_code = 0;
    const int parse_ret = parse_yolo_response(resp, result, &server_code);
    if (parse_ret == -8)
        cout << "yolo server code=" << server_code << ", body=" << resp << endl;
    else if (parse_ret < 0)
        cout << "yolo parse failed(" << parse_ret << "), body=" << resp << endl;
    return parse_ret;
}

} // namespace op::yolo
