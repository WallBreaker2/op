#include "YoloWrapper.h"
#include "../core/helpfunc.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <wincrypt.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "crypt32.lib")

using std::cout;
using std::endl;

namespace {
constexpr const char *kYoloDefaultEndpoint = "http://127.0.0.1:8090/api/v1/detect";

struct ParsedUrl {
    bool secure = false;
    INTERNET_PORT port = 80;
    std::wstring host;
    std::wstring path;
};

std::string to_lower_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

bool starts_with_http(const std::string &url) {
    return url.rfind("http://", 0) == 0 || url.rfind("https://", 0) == 0;
}

std::string trim_copy(const std::string &s) {
    const auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string getenv_trimmed(const char *name) {
    const char *value = std::getenv(name);
    return value ? trim_copy(value) : "";
}

bool try_resolve_backend_alias(const std::string &candidate, std::string &endpoint) {
    const std::string key = to_lower_copy(trim_copy(candidate));
    if (key == "yolo" || key == "yolo11" || key == "yolov11" || key == "yolo_http" || key == "yolo_server") {
        endpoint = kYoloDefaultEndpoint;
        return true;
    }
    return false;
}

bool resolve_endpoint_candidate(const std::string &candidate, std::string &endpoint) {
    const std::string value = trim_copy(candidate);
    if (value.empty())
        return false;
    if (try_resolve_backend_alias(value, endpoint))
        return true;
    if (starts_with_http(value)) {
        endpoint = value;
        return true;
    }
    return false;
}

bool parse_url(const std::string &url, ParsedUrl &out) {
    const std::wstring wurl = _s2wstring(url);
    URL_COMPONENTS parts = {};
    parts.dwStructSize = sizeof(parts);
    parts.dwSchemeLength = (DWORD)-1;
    parts.dwHostNameLength = (DWORD)-1;
    parts.dwUrlPathLength = (DWORD)-1;
    parts.dwExtraInfoLength = (DWORD)-1;
    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &parts))
        return false;
    out.secure = (parts.nScheme == INTERNET_SCHEME_HTTPS);
    out.port = parts.nPort;
    out.host.assign(parts.lpszHostName, parts.dwHostNameLength);
    out.path.assign(parts.lpszUrlPath, parts.dwUrlPathLength);
    if (parts.dwExtraInfoLength > 0)
        out.path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    if (out.path.empty())
        out.path = L"/";
    return !out.host.empty();
}

bool normalize_endpoint(std::string &endpoint) {
    ParsedUrl parsed;
    if (!parse_url(endpoint, parsed))
        return false;
    if (parsed.path == L"/") {
        endpoint += endpoint.back() == '/' ? "api/v1/detect" : "/api/v1/detect";
    }
    return parse_url(endpoint, parsed);
}

int parse_positive_int(const std::string &value, int fallback) {
    if (value.empty())
        return fallback;
    const int parsed = atoi(value.c_str());
    return parsed > 0 ? parsed : fallback;
}

std::string resolve_default_endpoint() {
    std::string endpoint;
    if (resolve_endpoint_candidate(getenv_trimmed("OP_YOLO_URL"), endpoint))
        return endpoint;
    if (resolve_endpoint_candidate(getenv_trimmed("OP_YOLO_BACKEND"), endpoint))
        return endpoint;
    return kYoloDefaultEndpoint;
}

int resolve_default_timeout_ms() {
    return parse_positive_int(getenv_trimmed("OP_YOLO_TIMEOUT_MS"), 3000);
}

bool base64_encode(const byte *data, int size, std::string &out) {
    DWORD needed = 0;
    if (!CryptBinaryToStringA(data, static_cast<DWORD>(size), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr,
                              &needed))
        return false;
    if (needed == 0) {
        out.clear();
        return true;
    }
    out.assign(needed - 1, '\0');
    return CryptBinaryToStringA(data, static_cast<DWORD>(size), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &out[0],
                                &needed) == TRUE;
}

bool http_post_json(const ParsedUrl &url, const std::string &body, int timeout_ms, std::string &response,
                    DWORD &status_code) {
    status_code = 0;
    response.clear();
    HINTERNET hSession = WinHttpOpen(L"op-yolo-client/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
        return false;

    bool ok = false;
    HINTERNET hConnect = nullptr;
    HINTERNET hRequest = nullptr;
    do {
        hConnect = WinHttpConnect(hSession, url.host.c_str(), url.port, 0);
        if (!hConnect)
            break;
        hRequest = WinHttpOpenRequest(hConnect, L"POST", url.path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                      WINHTTP_DEFAULT_ACCEPT_TYPES, url.secure ? WINHTTP_FLAG_SECURE : 0);
        if (!hRequest)
            break;
        WinHttpSetTimeouts(hRequest, timeout_ms, timeout_ms, timeout_ms, timeout_ms);
        static const wchar_t *kHeaders = L"Content-Type: application/json\r\n";
        if (!WinHttpSendRequest(hRequest, kHeaders, static_cast<DWORD>(-1L), (LPVOID)body.data(),
                                static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0))
            break;
        if (!WinHttpReceiveResponse(hRequest, nullptr))
            break;
        DWORD size = sizeof(status_code);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                            &status_code, &size, WINHTTP_NO_HEADER_INDEX);
        while (true) {
            DWORD avail = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &avail))
                break;
            if (avail == 0) {
                ok = true;
                break;
            }
            std::string chunk(avail, '\0');
            DWORD read = 0;
            if (!WinHttpReadData(hRequest, &chunk[0], avail, &read))
                break;
            chunk.resize(read);
            response.append(chunk);
        }
    } while (false);

    if (hRequest)
        WinHttpCloseHandle(hRequest);
    if (hConnect)
        WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return ok;
}

std::string json_unescape(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '\\') {
            out.push_back(s[i]);
            continue;
        }
        if (i + 1 >= s.size())
            break;
        char c = s[++i];
        switch (c) {
        case '"': out.push_back('"'); break;
        case '\\': out.push_back('\\'); break;
        case '/': out.push_back('/'); break;
        case 'b': out.push_back('\b'); break;
        case 'f': out.push_back('\f'); break;
        case 'n': out.push_back('\n'); break;
        case 'r': out.push_back('\r'); break;
        case 't': out.push_back('\t'); break;
        default: out.push_back(c); break;
        }
    }
    return out;
}
} // namespace

YoloWrapper::YoloWrapper() : m_endpoint(resolve_default_endpoint()), m_timeout_ms(resolve_default_timeout_ms()) {
    if (!normalize_endpoint(m_endpoint))
        m_endpoint = kYoloDefaultEndpoint;
    cout << "YoloWrapper::YoloWrapper(), endpoint=" << m_endpoint << endl;
}

YoloWrapper::~YoloWrapper() {
    release();
}

YoloWrapper *YoloWrapper::getInstance() {
    static YoloWrapper sYoloEngine;
    return &sYoloEngine;
}

int YoloWrapper::init(const std::wstring &engine, const std::wstring &dllName, const vector<string> &argvs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string endpoint = m_endpoint.empty() ? resolve_default_endpoint() : m_endpoint;
    int timeout_ms = m_timeout_ms > 0 ? m_timeout_ms : resolve_default_timeout_ms();

    auto maybe_set_endpoint = [&](const std::string &candidate) {
        std::string resolved;
        if (resolve_endpoint_candidate(candidate, resolved))
            endpoint = resolved;
    };

    if (!engine.empty())
        maybe_set_endpoint(_ws2string(engine));
    if (!dllName.empty())
        maybe_set_endpoint(_ws2string(dllName));

    for (const auto &arg : argvs) {
        if (arg.rfind("--url=", 0) == 0) {
            maybe_set_endpoint(arg.substr(6));
            continue;
        }
        if (arg.rfind("--timeout=", 0) == 0) {
            const int v = atoi(arg.substr(10).c_str());
            if (v > 0)
                timeout_ms = v;
            continue;
        }
        if (starts_with_http(arg))
            endpoint = arg;
    }

    if (!normalize_endpoint(endpoint)) {
        cout << "SetYoloEngine invalid endpoint: " << endpoint << endl;
        return -1;
    }

    m_endpoint = endpoint;
    m_timeout_ms = timeout_ms;
    cout << "SetYoloEngine endpoint=" << m_endpoint << ", timeout=" << m_timeout_ms << "ms" << endl;
    return 0;
}

int YoloWrapper::release() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return 0;
}

int YoloWrapper::detect(byte *data, int w, int h, int bpp, double conf, double iou, vyolo_rec_t &result) {
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
    if (!http_post_json(parsed, req, m_timeout_ms, resp, status_code))
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
