#include "OcrService.h"
#include "../network/HttpClient.h"
#include "../base/Utils.h"
#include <iostream>
#include <regex>

using std::cout;
using std::endl;

namespace op::ocr {

namespace {
constexpr const char *kTesseractDefaultEndpoint = "http://127.0.0.1:8080/api/v1/ocr";
constexpr const char *kPaddleOcrDefaultEndpoint = "http://127.0.0.1:8081/api/v1/ocr";
constexpr const char *kPaddleNcnnOcrDefaultEndpoint = "http://127.0.0.1:8082/api/v1/ocr";
constexpr const char *kOcrDefaultPathSuffix = "api/v1/ocr";

bool try_resolve_ocr_backend(const std::string &candidate, std::string &endpoint) {
    const std::string key = to_lower_copy(trim_copy(candidate));
    if (key.empty()) {
        return false;
    }
    if (key == "tesseract" || key == "tess") {
        endpoint = kTesseractDefaultEndpoint;
        return true;
    }
    if (key == "paddle_ncnn") {
        endpoint = kPaddleNcnnOcrDefaultEndpoint;
        return true;
    }
    if (key == "paddle" || key == "paddleocr" || key == "paddle_ocr") {
        endpoint = kPaddleOcrDefaultEndpoint;
        return true;
    }
    return false;
}

std::string resolve_default_endpoint() {
    std::string endpoint;
    if (resolve_endpoint_candidate(getenv_trimmed("OP_OCR_URL"), endpoint, try_resolve_ocr_backend)) {
        return endpoint;
    }
    if (resolve_endpoint_candidate(getenv_trimmed("OP_OCR_BACKEND"), endpoint, try_resolve_ocr_backend)) {
        return endpoint;
    }
    return kPaddleNcnnOcrDefaultEndpoint;
}

int resolve_default_timeout_ms() {
    return parse_positive_int(getenv_trimmed("OP_OCR_TIMEOUT_MS"), 3000);
}
} // namespace

HttpOcrService::HttpOcrService() : m_endpoint(resolve_default_endpoint()), m_timeout_ms(resolve_default_timeout_ms()) {
    if (!normalize_endpoint(m_endpoint, kOcrDefaultPathSuffix)) {
        // 配置写错时退回主 OCR 服务，避免静默切到旧 Tesseract 后端。
        m_endpoint = kPaddleNcnnOcrDefaultEndpoint;
    }
    cout << "HttpOcrService::HttpOcrService(), endpoint=" << m_endpoint << endl;
}

HttpOcrService::~HttpOcrService() {
    release();
}

HttpOcrService *HttpOcrService::getInstance() {
    static HttpOcrService sOcrEngine;
    return &sOcrEngine;
}

int HttpOcrService::init(const std::wstring &engine, const std::wstring &dllName, const vector<string> &argvs) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Resolve fallback endpoint/timeout if not yet set
    if (m_endpoint.empty() || m_timeout_ms <= 0) {
        std::string cur = m_endpoint.empty() ? resolve_default_endpoint() : m_endpoint;
        int timeout = m_timeout_ms > 0 ? m_timeout_ms : resolve_default_timeout_ms();
        m_endpoint = cur;
        m_timeout_ms = timeout;
    }

    return configure_endpoint(m_endpoint, m_timeout_ms, engine, dllName, argvs, try_resolve_ocr_backend,
                              kOcrDefaultPathSuffix, "SetOcrEngine", nullptr, nullptr,
                              3000);
}

int HttpOcrService::release() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return 0;
}

int HttpOcrService::ocr(byte *data, int w, int h, int bpp, vocr_rec_t &result) {
    const std::lock_guard<std::mutex> lock(m_mutex);
    result.clear();

    if (data == nullptr || w <= 0 || h <= 0 || (bpp != 1 && bpp != 3 && bpp != 4)) {
        return -1;
    }

    const size_t pixel_bytes_size_t = static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(bpp);
    if (pixel_bytes_size_t > 64 * 1024 * 1024) {
        return -2;
    }

    std::string image_b64;
    if (!base64_encode(data, static_cast<int>(pixel_bytes_size_t), image_b64)) {
        cout << "ocr base64 encode failed" << endl;
        return -3;
    }

    std::string req = "{\"image\":\"" + image_b64 + "\",\"width\":" + std::to_string(w) +
                      ",\"height\":" + std::to_string(h) + ",\"bpp\":" + std::to_string(bpp) + "}";

    ParsedUrl parsed;
    if (!parse_url(m_endpoint, parsed)) {
        cout << "ocr endpoint invalid: " << m_endpoint << endl;
        return -4;
    }

    std::string resp;
    DWORD status_code = 0;
    if (!http_post_json(parsed, req, m_timeout_ms, resp, status_code, L"op-ocr-client/1.0")) {
        cout << "ocr request failed: endpoint=" << m_endpoint << endl;
        return -5;
    }
    if (status_code != 200) {
        cout << "ocr http status=" << status_code << ", body=" << resp << endl;
        return -6;
    }

    static const std::regex code_re("\\\"code\\\"\\s*:\\s*(-?\\d+)");
    std::smatch code_match;
    if (!std::regex_search(resp, code_match, code_re)) {
        cout << "ocr parse response code failed" << endl;
        return -7;
    }
    const int code = atoi(code_match[1].str().c_str());
    if (code != 0) {
        cout << "ocr server code=" << code << ", body=" << resp << endl;
        return -8;
    }

    static const std::regex obj_re("\\{[^\\{\\}]*\\}");
    static const std::regex text_re("\\\"text\\\"\\s*:\\s*\\\"((?:\\\\.|[^\\\"\\\\])*)\\\"");
    static const std::regex bbox_re(
        "\\\"bbox\\\"\\s*:\\s*\\[\\s*(-?\\d+)\\s*,\\s*(-?\\d+)\\s*,\\s*(-?\\d+)\\s*,\\s*(-?\\d+)\\s*\\]");
    static const std::regex conf_re("\\\"confidence\\\"\\s*:\\s*([-+]?\\d*\\.?\\d+(?:[eE][-+]?\\d+)?)");

    int n = 0;
    for (std::sregex_iterator it(resp.begin(), resp.end(), obj_re), end; it != end; ++it) {
        const std::string obj = it->str();
        std::smatch text_m;
        std::smatch bbox_m;
        std::smatch conf_m;
        if (!std::regex_search(obj, text_m, text_re) || !std::regex_search(obj, bbox_m, bbox_re) ||
            !std::regex_search(obj, conf_m, conf_re)) {
            continue;
        }

        ocr_rec_t ts;
        ts.left_top = point_t(atoi(bbox_m[1].str().c_str()), atoi(bbox_m[2].str().c_str()));
        ts.right_bottom = point_t(atoi(bbox_m[3].str().c_str()), atoi(bbox_m[4].str().c_str()));
        ts.confidence = static_cast<float>(atof(conf_m[1].str().c_str()));
        const std::string text = json_unescape(text_m[1].str());
        ts.text = _s2wstring(utf8_to_ansi(text));
        result.push_back(ts);
        ++n;
    }

    if (n == 0) {
        static const std::regex empty_result_re("\\\"results\\\"\\s*:\\s*\\[\\s*\\]");
        if (!std::regex_search(resp, empty_result_re)) {
            cout << "ocr parse results failed, body=" << resp << endl;
            return -9;
        }
    }

    return n;
}

} // namespace op::ocr
