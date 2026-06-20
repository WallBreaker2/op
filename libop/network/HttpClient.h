#pragma once
#include "../runtime/Types.h"
#include <functional>
#include <string>
#include <vector>
#include <winhttp.h>

namespace op {

struct ParsedUrl {
    bool secure = false;
    INTERNET_PORT port = 80;
    std::wstring host;
    std::wstring path;
};

// --- string utilities ---
std::string to_lower_copy(std::string s);
bool starts_with_http(const std::string &url);
std::string trim_copy(const std::string &s);
std::string getenv_trimmed(const char *name);
int parse_positive_int(const std::string &value, int fallback);

// --- URL utilities ---
bool parse_url(const std::string &url, ParsedUrl &out);

// normalize_endpoint: appends default_path_suffix if the path is "/"
bool normalize_endpoint(std::string &endpoint, const char *default_path_suffix);

// resolve_endpoint_candidate: tries alias then "starts-with-http"
// try_alias(key, endpoint) -> true if key matched and endpoint was set
bool resolve_endpoint_candidate(const std::string &candidate, std::string &endpoint,
                                const std::function<bool(const std::string &, std::string &)> &try_alias);

// --- encoding & HTTP ---
bool base64_encode(const unsigned char *data, int size, std::string &out);
bool http_post_json(const ParsedUrl &url, const std::string &body, int timeout_ms, std::string &response,
                    DWORD &status_code, const wchar_t *user_agent);
std::string json_unescape(const std::string &s);

// --- shared endpoint configuration (used by both wrapper init methods) ---
// returns 0 on success, -1 on invalid endpoint
int configure_endpoint(std::string &m_endpoint, int &m_timeout_ms, const std::wstring &engine,
                       const std::wstring &dllName, const std::vector<std::string> &argvs,
                       const std::function<bool(const std::string &, std::string &)> &try_alias,
                       const char *default_path_suffix, const char *label, const char *default_endpoint_env_url,
                       const char *default_endpoint_env_backend, int default_timeout_ms_fallback);

} // namespace op
