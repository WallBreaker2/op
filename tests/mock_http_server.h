#pragma once

// 极简假 HTTP 服务端（仅供测试）：
//   - 监听 127.0.0.1 上的系统分配端口；
//   - 每个连接一个线程（否则"服务端串行"会被误测成"客户端没超时"）；
//   - 可配置状态码、响应体、响应前延迟。
// 注意：winsock2.h 必须在 windows.h 之前，所以本头文件只允许在 test_support.h 之前包含。

#include <winsock2.h>
#include <ws2tcpip.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

namespace test_support {

struct MockHttpServer {
    std::string response_body = R"({"code":0,"results":[]})";
    int status = 200;
    int delay_ms = 0;

    bool Start() {
        WSADATA data = {};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            return false;
        }
        wsa_started_ = true;

        listen_socket_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listen_socket_ == INVALID_SOCKET) {
            return false;
        }
        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        if (::bind(listen_socket_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
            return false;
        }
        if (::listen(listen_socket_, 8) != 0) {
            return false;
        }
        sockaddr_in bound = {};
        int len = sizeof(bound);
        if (::getsockname(listen_socket_, reinterpret_cast<sockaddr *>(&bound), &len) != 0) {
            return false;
        }
        port_ = ntohs(bound.sin_port);
        worker_ = std::thread([this]() { serve(); });
        return port_ != 0;
    }

    std::wstring endpoint(const wchar_t *path = L"/api/v1/ocr") const {
        wchar_t buffer[160] = {};
        swprintf_s(buffer, L"http://127.0.0.1:%u%s", static_cast<unsigned>(port_), path);
        return buffer;
    }

    void Stop() {
        if (listen_socket_ != INVALID_SOCKET) {
            ::closesocket(listen_socket_);
            listen_socket_ = INVALID_SOCKET;
        }
        if (worker_.joinable()) {
            worker_.join();
        }
        if (wsa_started_) {
            WSACleanup();
            wsa_started_ = false;
        }
    }

    ~MockHttpServer() { Stop(); }

  private:
    void serve() {
        for (;;) {
            SOCKET client = ::accept(listen_socket_, nullptr, nullptr);
            if (client == INVALID_SOCKET) {
                return;
            }
            std::thread([this, client]() { handle(client); }).detach();
        }
    }

    void handle(SOCKET client) {
        // 必须把请求读完整（含 Content-Length 指定的 body）再回应；只读一小段就 send+close，
        // 客户端还在发 body 时会收到 RST，表现为"请求失败"——那是假服务端的问题。
        std::string request;
        char chunk[8192];
        size_t content_length = 0;
        bool headers_done = false;
        for (;;) {
            const int got = ::recv(client, chunk, static_cast<int>(sizeof(chunk)), 0);
            if (got <= 0) {
                break;
            }
            request.append(chunk, static_cast<size_t>(got));
            const size_t header_end = request.find("\r\n\r\n");
            if (header_end == std::string::npos) {
                continue;
            }
            if (!headers_done) {
                headers_done = true;
                std::string lower = request.substr(0, header_end);
                for (auto &ch : lower) {
                    ch = static_cast<char>(::tolower(static_cast<unsigned char>(ch)));
                }
                const size_t pos = lower.find("content-length:");
                if (pos != std::string::npos) {
                    content_length = static_cast<size_t>(::strtoul(lower.c_str() + pos + 15, nullptr, 10));
                }
            }
            if (request.size() - (header_end + 4) >= content_length) {
                break;
            }
        }

        if (delay_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
        char header[256] = {};
        const char *reason = status == 200 ? "OK" : "Internal Server Error";
        const int header_len = ::snprintf(header, sizeof(header),
                                          "HTTP/1.1 %d %s\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n"
                                          "Connection: close\r\n\r\n",
                                          status, reason, response_body.size());
        ::send(client, header, header_len, 0);
        if (!response_body.empty()) {
            ::send(client, response_body.data(), static_cast<int>(response_body.size()), 0);
        }
        ::shutdown(client, SD_BOTH);
        ::closesocket(client);
    }

    SOCKET listen_socket_ = INVALID_SOCKET;
    unsigned short port_ = 0;
    bool wsa_started_ = false;
    std::thread worker_;
};

} // namespace test_support
