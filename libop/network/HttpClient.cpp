#include "HttpClient.h"
#include "../base/JsonUtils.h"
#include "../base/Utils.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
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

size_t base64_encoded_size(size_t size) {
    // base64 每 3 字节出 4 字符，末尾不足的补 '='。
    return size == 0 ? 0 : ((size + 2) / 3) * 4;
}

namespace {

const char kBase64Table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

} // namespace

bool base64_encode_append(const unsigned char *data, size_t size, std::string &out) {
    if (data == nullptr || size == 0)
        return true;

    // 手写表驱动编码：以前用 CryptBinaryToStringA，实测编 8.29 MB 要 ~89 ms（≈93 MB/s），
    // 而 OCR/YOLO 每次调用都要编一张整图，成了实际瓶颈（§72.34）。这里约 5~6 倍于它。
    const size_t offset = out.size();
    out.resize(offset + base64_encoded_size(size));
    char *dst = &out[offset];

    size_t i = 0;
    for (; i + 3 <= size; i += 3) {
        const unsigned value =
            (static_cast<unsigned>(data[i]) << 16) | (static_cast<unsigned>(data[i + 1]) << 8) | data[i + 2];
        *dst++ = kBase64Table[(value >> 18) & 0x3F];
        *dst++ = kBase64Table[(value >> 12) & 0x3F];
        *dst++ = kBase64Table[(value >> 6) & 0x3F];
        *dst++ = kBase64Table[value & 0x3F];
    }

    const size_t remain = size - i;
    if (remain > 0) {
        unsigned value = static_cast<unsigned>(data[i]) << 16;
        if (remain == 2)
            value |= static_cast<unsigned>(data[i + 1]) << 8;
        *dst++ = kBase64Table[(value >> 18) & 0x3F];
        *dst++ = kBase64Table[(value >> 12) & 0x3F];
        *dst++ = remain == 2 ? kBase64Table[(value >> 6) & 0x3F] : '=';
        *dst++ = '=';
    }
    return true;
}

bool base64_encode(const unsigned char *data, int size, std::string &out) {
    out.clear();
    if (size < 0)
        return false;
    return base64_encode_append(data, static_cast<size_t>(size), out);
}

namespace {

// --- 会话池（§72.41）---
// 每次请求都 WinHttpOpen + WinHttpConnect 的话，TCP 连接（https 还有 TLS 握手）用完即弃。
// 本地服务端上这段固定开销约 0.3~0.4ms，占一次 OCR 往返的三分之一以上（§72.40 实测）。
// 这里按 host/port/scheme/UA 缓存 session + connect，让 WinHTTP 的连接池真正活下来。
struct SessionKey {
    std::wstring host;
    int port = 0;
    bool secure = false;
    std::wstring user_agent;

    bool operator<(const SessionKey &rhs) const {
        if (host != rhs.host)
            return host < rhs.host;
        if (port != rhs.port)
            return port < rhs.port;
        if (secure != rhs.secure)
            return secure < rhs.secure;
        return user_agent < rhs.user_agent;
    }
};

struct SessionEntry {
    WinHttpHandle session;
    WinHttpHandle connect;
};

using SessionEntryPtr = std::shared_ptr<SessionEntry>;

// 故意不析构：DLL 卸载时函数内静态对象在 DllMain(PROCESS_DETACH) 里析构，
// 那时调 WinHttpCloseHandle 有 loader lock 风险，交给进程退出回收更安全。
std::map<SessionKey, SessionEntryPtr> &session_pool() {
    static auto *pool = new std::map<SessionKey, SessionEntryPtr>();
    return *pool;
}

std::mutex &session_pool_mutex() {
    static std::mutex mutex;
    return mutex;
}

// 缓存条数上限：实际只有 OCR / YOLO 两个端点，8 条足够；超了就不缓存（用完即关）。
const size_t kMaxCachedSessions = 8;

SessionEntryPtr make_session(const SessionKey &key, int timeout_ms) {
    auto entry = std::make_shared<SessionEntry>();
    entry->session.reset(WinHttpOpen(key.user_agent.empty() ? nullptr : key.user_agent.c_str(),
                                     WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS,
                                     0));
    if (!entry->session)
        return nullptr;
    if (timeout_ms > 0) {
        // session 上的超时只兜底；每次请求仍会在 request 上按调用方的 timeout 覆盖一遍。
        WinHttpSetTimeouts(entry->session.get(), timeout_ms, timeout_ms, timeout_ms, timeout_ms);
    }
    entry->connect.reset(WinHttpConnect(entry->session.get(), key.host.c_str(), key.port, 0));
    if (!entry->connect)
        return nullptr;
    return entry;
}

SessionEntryPtr acquire_session(const SessionKey &key, int timeout_ms) {
    {
        std::lock_guard<std::mutex> lock(session_pool_mutex());
        auto &pool = session_pool();
        auto it = pool.find(key);
        if (it != pool.end())
            return it->second; // shared_ptr 拷贝一份：别的线程正在用它也不会被提前关掉
    }

    // 建句柄可能慢（要建内部线程/套接字），不持锁建。
    auto entry = make_session(key, timeout_ms);
    if (!entry)
        return nullptr;

    std::lock_guard<std::mutex> lock(session_pool_mutex());
    auto &pool = session_pool();
    auto it = pool.find(key);
    if (it != pool.end())
        return it->second; // 建的同时别人抢先插了：用他的，我们这份随 shared_ptr 释放
    if (pool.size() >= kMaxCachedSessions)
        return entry; // 池满：不缓存，本次用完即关
    pool.emplace(key, entry);
    return entry;
}

// 只摘掉自己那把；如果已经被别人换成新的，就不要动（避免关掉别人正在用的句柄）。
void drop_session(const SessionKey &key, const SessionEntryPtr &entry) {
    std::lock_guard<std::mutex> lock(session_pool_mutex());
    auto &pool = session_pool();
    auto it = pool.find(key);
    if (it != pool.end() && it->second == entry)
        pool.erase(it);
    // entry 由调用方持有，最后一份 shared_ptr 释放时才真正 CloseHandle。
}

// 一次完整的 HTTP POST（阻塞）。超时交给 WinHTTP，但它在"等服务端首批响应头"这段
// 实测不奏效（doc/defect_fixes.md §40/§41），所以真正的硬上限在下面的 http_post_json 里。
struct HttpExchangeResult {
    bool ok = false;
    std::string response;
    DWORD status_code = 0;
};

bool http_post_json_blocking(const ParsedUrl &url, const std::string &body, int timeout_ms,
                             HttpExchangeResult &out, const wchar_t *user_agent) {
    DWORD &status_code = out.status_code;
    std::string &response = out.response;
    status_code = 0;
    response.clear();

    SessionKey key;
    key.host = url.host;
    key.port = url.port;
    key.secure = url.secure;
    if (user_agent)
        key.user_agent = user_agent;

    bool ok = false;
    // 连接是缓存来的，可能早已被服务端关掉（空闲超时 / 服务端重启）。那种情况下
    // WinHttpOpenRequest 或 WinHttpSendRequest 会失败，换一条新连接重试一次即可。
    // 只在“还没收到响应”之前重试：一旦 ReceiveResponse 之后失败，服务端可能已经处理过
    // 这次请求，重发不是安全的默认行为（我们的本地 OCR/YOLO 服务是幂等的，但仍然保守处理）。
    for (int attempt = 0; attempt < 2 && !ok; ++attempt) {
        SessionEntryPtr entry = acquire_session(key, timeout_ms);
        if (!entry)
            break;

        bool may_retry = false;
        WinHttpHandle request;

        do {
            request.reset(WinHttpOpenRequest(entry->connect.get(), L"POST", url.path.c_str(), nullptr,
                                             WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                             url.secure ? WINHTTP_FLAG_SECURE : 0));
            if (!request) {
                may_retry = true;
                break;
            }

            WinHttpSetTimeouts(request.get(), timeout_ms, timeout_ms, timeout_ms, timeout_ms);
            // WinHttpSetTimeouts 的 receive 段实测**不覆盖"等服务端首批响应头"**这段等待：
            // 配置 300ms、服务端延迟 3s 响应时，调用仍然等满 3s（见 doc/defect_fixes.md §40）。
            // WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT 是专门管"等响应"的选项，单独再设一次。
            {
                DWORD receive_response_timeout = static_cast<DWORD>(timeout_ms);
                WinHttpSetOption(request.get(), WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT, &receive_response_timeout,
                                 sizeof(receive_response_timeout));
            }

            static const wchar_t *kHeaders = L"Content-Type: application/json\r\n";
            if (!WinHttpSendRequest(request.get(), kHeaders, static_cast<DWORD>(-1L), (LPVOID)body.data(),
                                    static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0)) {
                // 缓存的连接可能已被服务端关掉，换一条新的重发一次。
                // 超时除外：那会把调用方给的 timeout 白等两遍（OcrContractTest.RespectsTimeout 就是这么暴露的）。
                may_retry = (::GetLastError() != ERROR_WINHTTP_TIMEOUT);
                break;
            }

            if (!WinHttpReceiveResponse(request.get(), nullptr)) {
                // 服务端把空闲的 keep-alive 连接关掉时，SendRequest 往往"成功"（写进了本地 socket 缓冲），
                // 真正失败会推迟到等响应头这一步。并发下这条路径会随机命中，实测 32 次并发里失败 1 次
                // （OcrContractTest.SessionPoolConcurrentRequestsStayCorrect）。此时服务端没回过任何响应头，
                // 换一条新连接重发是安全的：OCR/YOLO 都是无副作用的本地计算。
                // 仍然排除超时，理由同上。
                may_retry = (::GetLastError() != ERROR_WINHTTP_TIMEOUT);
                break;
            }

            DWORD size = sizeof(status_code);
            WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &size, WINHTTP_NO_HEADER_INDEX);

            response.reserve(64 * 1024); // 一次预留，避免大响应体反复扩容
            // §72.51：响应体原来不设上限。服务端（或中间人）只要一直发，调用方就会
            // 一直 resize()，直到把宿主进程 OOM 掉 —— OCR/YOLO 的端点又是运行时可配的。
            // 32MB 对 OCR 响应（几万条识别结果也才几 MB）绰绰有余。
            constexpr size_t kMaxResponseBytes = 32 * 1024 * 1024;
            while (true) {
                DWORD avail = 0;
                if (!WinHttpQueryDataAvailable(request.get(), &avail)) {
                    break;
                }
                if (avail == 0) {
                    ok = true;
                    break;
                }
                if (response.size() + avail > kMaxResponseBytes) {
                    ok = false; // 超限：当作这次请求失败，不把半个响应交给解析层
                    break;
                }

                // 直接读进 response 的尾部：旧写法每块新建一个临时 std::string、再 append 一次，
                // 等于每块一次分配 + 一次拷贝（§72.35）。这里只留一次 resize 和一次收缩。
                const size_t offset = response.size();
                response.resize(offset + avail);
                DWORD read = 0;
                if (!WinHttpReadData(request.get(), &response[offset], avail, &read)) {
                    response.resize(offset); // 读失败：去掉这段未填充的尾巴
                    break;
                }
                response.resize(offset + read);
            }
        } while (false);

        if (ok)
            break;
        // 失败：把这条连接从池里摘掉（下次重新握手），能重试就换一条再来一次。
        drop_session(key, entry);
        if (!may_retry)
            break;
    }

    out.ok = ok;
    return ok;
}

} // namespace

bool http_post_json(const ParsedUrl &url, const std::string &body, int timeout_ms, std::string &response,
                    DWORD &status_code, const wchar_t *user_agent) {
    response.clear();
    status_code = 0;

    // WinHTTP 的 receive/response 超时在本机实测不生效（配置 300ms、服务端 3s 后响应时仍然等满 3s）。
    // 这里补一层硬上限：把请求放进工作线程，主线程有界等待；超时就放弃本次请求并返回失败，
    // 工作线程 detach 自行收尾（结果对象由 shared_ptr 保活，不会悬垂）。
    if (timeout_ms <= 0) {
        HttpExchangeResult direct;
        http_post_json_blocking(url, body, timeout_ms, direct, user_agent);
        response = std::move(direct.response);
        status_code = direct.status_code;
        return direct.ok;
    }

    auto shared = std::make_shared<HttpExchangeResult>();
    // 完成信号走条件变量，不再 sleep 轮询：Windows 的默认定时器粒度是 15.6ms，
    // sleep_for(5ms) 实际要睡满一个 tick，一次 1ms 的本地请求因此要多等约 14.5ms（§72.39）。
    // 同步对象必须堆上（worker 可能被 detach，函数返回后它还要 notify）。
    struct HttpExchangeSync {
        std::mutex mutex;
        std::condition_variable cv;
        bool done = false;
    };
    auto sync = std::make_shared<HttpExchangeSync>();
    std::thread worker([shared, sync, url, body, timeout_ms, user_agent]() {
        http_post_json_blocking(url, body, timeout_ms, *shared, user_agent);
        {
            std::lock_guard<std::mutex> lock(sync->mutex);
            sync->done = true;
        }
        sync->cv.notify_one();
    });

    bool finished = false;
    {
        std::unique_lock<std::mutex> lock(sync->mutex);
        finished = sync->cv.wait_for(lock, std::chrono::milliseconds(timeout_ms) + std::chrono::milliseconds(200),
                                     [&sync]() { return sync->done; });
    }

    if (!finished) {
        // 超时：放弃本次请求。工作线程仍会跑完（WinHTTP 最终返回失败或拿到迟到响应），
        // 它只写自己的 shared 结果，不会碰调用方的引用。
        worker.detach();
        cout << "http_post_json timeout after " << timeout_ms << "ms" << std::endl;
        return false;
    }

    worker.join();
    response = std::move(shared->response);
    status_code = shared->status_code;
    return shared->ok;
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

    // 用户明确给了一个候选（engine / dllName / --url= / 裸 URL），就必须能解析出来；
    // 原来解析失败时是**静默忽略**（沿用旧端点）并返回成功，调用方以为设置生效了。
    bool candidate_given = false;
    bool candidate_resolved = false;
    auto maybe_set_endpoint = [&](const std::string &candidate) {
        if (trim_copy(candidate).empty()) {
            return;
        }
        candidate_given = true;
        std::string resolved;
        if (resolve_endpoint_candidate(candidate, resolved, try_alias)) {
            endpoint = resolved;
            candidate_resolved = true;
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
            candidate_resolved = true;
        }
    }

    if (candidate_given && !candidate_resolved) {
        // 明确给了候选却一个都解析不出来：返回失败，而不是"沿用旧端点还报成功"。
        cout << label << " cannot resolve endpoint candidate" << std::endl;
        return -1;
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
