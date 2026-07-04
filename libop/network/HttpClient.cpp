#include "HttpClient.h"
#include "../runtime/JsonUtils.h"
#include "../runtime/RuntimeUtils.h"
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

namespace op {

namespace {

class WinHttpHandle {
  public:
    WinHttpHandle() noexcept = default;
    explicit WinHttpHandle(HINTERNET handle) noexcept : handle_(handle) {
    }

    ~WinHttpHandle() {
        reset();
    }

    WinHttpHandle(const WinHttpHandle &) = delete;
    WinHttpHandle &operator=(const WinHttpHandle &) = delete;

    HINTERNET get() const noexcept {
        return handle_;
    }

    explicit operator bool() const noexcept {
        return handle_ != nullptr;
    }

    void reset(HINTERNET handle = nullptr) noexcept {
        if (handle_) {
            WinHttpCloseHandle(handle_);
        }
        handle_ = handle;
    }

  private:
    HINTERNET handle_ = nullptr;
};

} // namespace

// --- string utilities ---

std::string to_lower_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
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
    if (value == nullptr) {
        return "";
    }
    return trim_copy(value);
}

int parse_positive_int(const std::string &value, int fallback) {
    if (value.empty()) {
        return fallback;
    }
    const int parsed = atoi(value.c_str());
    return parsed > 0 ? parsed : fallback;
}

// --- URL utilities ---

bool parse_url(const std::string &url, ParsedUrl &out) {
    const std::wstring wurl = _s2wstring(url);
    URL_COMPONENTS parts = {};
    parts.dwStructSize = sizeof(parts);
    parts.dwSchemeLength = (DWORD)-1;
    parts.dwHostNameLength = (DWORD)-1;
    parts.dwUrlPathLength = (DWORD)-1;
    parts.dwExtraInfoLength = (DWORD)-1;
    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &parts)) {
        return false;
    }
    out.secure = (parts.nScheme == INTERNET_SCHEME_HTTPS);
    out.port = parts.nPort;
    out.host.assign(parts.lpszHostName, parts.dwHostNameLength);
    out.path.assign(parts.lpszUrlPath, parts.dwUrlPathLength);
    if (parts.dwExtraInfoLength > 0) {
        out.path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    }
    if (out.path.empty()) {
        out.path = L"/";
    }
    return !out.host.empty();
}

bool normalize_endpoint(std::string &endpoint, const char *default_path_suffix) {
    ParsedUrl parsed;
    if (!parse_url(endpoint, parsed)) {
        return false;
    }
    if (parsed.path == L"/") {
        if (!endpoint.empty() && endpoint.back() == '/') {
            endpoint += default_path_suffix;
        } else {
            endpoint += '/';
            endpoint += default_path_suffix;
        }
    }
    return parse_url(endpoint, parsed);
}

bool resolve_endpoint_candidate(const std::string &candidate, std::string &endpoint,
                                const std::function<bool(const std::string &, std::string &)> &try_alias) {
    const std::string value = trim_copy(candidate);
    if (value.empty()) {
        return false;
    }
    if (try_alias(value, endpoint)) {
        return true;
    }
    if (starts_with_http(value)) {
        endpoint = value;
        return true;
    }
    return false;
}

// --- encoding & HTTP ---

bool base64_encode(const unsigned char *data, int size, std::string &out) {
    DWORD needed = 0;
    if (!CryptBinaryToStringA(data, static_cast<DWORD>(size), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr,
                              &needed)) {
        return false;
    }

    if (needed == 0) {
        out.clear();
        return true;
    }

    out.assign(needed - 1, '\0');
    return CryptBinaryToStringA(data, static_cast<DWORD>(size), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &out[0],
                                &needed) == TRUE;
}

bool http_post_json(const ParsedUrl &url, const std::string &body, int timeout_ms, std::string &response,
                    DWORD &status_code, const wchar_t *user_agent) {
    status_code = 0;
    response.clear();

    WinHttpHandle session{
        WinHttpOpen(user_agent, WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)};
    if (!session)
        return false;

    bool ok = false;
    WinHttpHandle connect;
    WinHttpHandle request;

    do {
        connect.reset(WinHttpConnect(session.get(), url.host.c_str(), url.port, 0));
        if (!connect)
            break;

        request.reset(WinHttpOpenRequest(connect.get(), L"POST", url.path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                         WINHTTP_DEFAULT_ACCEPT_TYPES, url.secure ? WINHTTP_FLAG_SECURE : 0));
        if (!request)
            break;

        WinHttpSetTimeouts(request.get(), timeout_ms, timeout_ms, timeout_ms, timeout_ms);

        static const wchar_t *kHeaders = L"Content-Type: application/json\r\n";
        if (!WinHttpSendRequest(request.get(), kHeaders, static_cast<DWORD>(-1L), (LPVOID)body.data(),
                                static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0)) {
            break;
        }

        if (!WinHttpReceiveResponse(request.get(), nullptr))
            break;

        DWORD size = sizeof(status_code);
        WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &size, WINHTTP_NO_HEADER_INDEX);

        while (true) {
            DWORD avail = 0;
            if (!WinHttpQueryDataAvailable(request.get(), &avail)) {
                break;
            }
            if (avail == 0) {
                ok = true;
                break;
            }

            std::string chunk;
            chunk.resize(avail);
            DWORD read = 0;
            if (!WinHttpReadData(request.get(), &chunk[0], avail, &read)) {
                break;
            }
            chunk.resize(read);
            response.append(chunk);
        }
    } while (false);

    return ok;
}

std::string json_unescape(const std::string &s) {
    return internal::json::UnescapeString(s);
}

// --- shared endpoint configuration ---

int configure_endpoint(std::string &m_endpoint, int &m_timeout_ms, const std::wstring &engine,
                       const std::wstring &dllName, const std::vector<std::string> &argvs,
                       const std::function<bool(const std::string &, std::string &)> &try_alias,
                       const char *default_path_suffix, const char *label, const char *default_endpoint_env_url,
                       const char *default_endpoint_env_backend, int default_timeout_ms_fallback) {
    std::string endpoint = m_endpoint;
    int timeout_ms = m_timeout_ms;

    if (endpoint.empty()) {
        // resolve default from env or alias
        if (default_endpoint_env_url) {
            std::string url_env = getenv_trimmed(default_endpoint_env_url);
            if (!url_env.empty()) {
                endpoint = url_env;
            }
        }
        if (endpoint.empty() && default_endpoint_env_backend) {
            std::string backend_env = getenv_trimmed(default_endpoint_env_backend);
            if (!backend_env.empty()) {
                std::string tmp;
                if (try_alias(backend_env, tmp))
                    endpoint = tmp;
            }
        }
    }
    if (timeout_ms <= 0 && default_timeout_ms_fallback > 0) {
        // try resolve timeout from env (OCR uses OP_OCR_TIMEOUT_MS, YOLO uses OP_YOLO_TIMEOUT_MS)
        // the caller passes the default via the function — we just keep existing timeout_ms logic
    }

    auto maybe_set_endpoint = [&](const std::string &candidate) {
        std::string resolved;
        if (resolve_endpoint_candidate(candidate, resolved, try_alias)) {
            endpoint = resolved;
        }
    };

    if (!engine.empty()) {
        maybe_set_endpoint(_ws2string(engine));
    }
    if (!dllName.empty()) {
        maybe_set_endpoint(_ws2string(dllName));
    }

    for (const auto &arg : argvs) {
        if (arg.rfind("--url=", 0) == 0) {
            maybe_set_endpoint(arg.substr(6));
            continue;
        }
        if (arg.rfind("--timeout=", 0) == 0) {
            const int v = atoi(arg.substr(10).c_str());
            if (v > 0) {
                timeout_ms = v;
            }
            continue;
        }
        if (starts_with_http(arg)) {
            endpoint = arg;
        }
    }

    if (!normalize_endpoint(endpoint, default_path_suffix)) {
        cout << label << " invalid endpoint: " << endpoint << std::endl;
        return -1;
    }

    m_endpoint = endpoint;
    m_timeout_ms = timeout_ms;
    cout << label << " endpoint=" << m_endpoint << ", timeout=" << m_timeout_ms << "ms" << std::endl;
    return 0;
}

} // namespace op
