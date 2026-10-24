#include "OcrService.h"
#include "../network/HttpClient.h"
#include "OcrResponseParser.h"
#include "../base/Utils.h"
#include <iostream>

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

    // 请求体一次成型：base64 直接编进 req 的尾部，省掉“临时串 + 再拷一遍”（§72.34）。
    std::string req;
    req.reserve(base64_encoded_size(pixel_bytes_size_t) + 96);
    req += "{\"image\":\"";
    if (!base64_encode_append(data, pixel_bytes_size_t, req)) {
        cout << "ocr base64 encode failed" << endl;
        return -3;
    }
    req += "\",\"width\":";
    req += std::to_string(w);
    req += ",\"height\":";
    req += std::to_string(h);
    req += ",\"bpp\":";
    req += std::to_string(bpp);
    req += "}";

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

    int server_code = 0;
    const int parse_ret = parse_ocr_response(resp, result, &server_code);
    if (parse_ret == -8) {
        cout << "ocr server code=" << server_code << ", body=" << resp << endl;
    } else if (parse_ret < 0) {
        cout << "ocr parse failed(" << parse_ret << "), body=" << resp << endl;
    }
    return parse_ret;
}

} // namespace op::ocr
