// winsock2.h 必须在 windows.h 之前（test_support.h 会拉进 windows.h），否则俩头里的
// sockaddr/fd_set 会重定义；mock_http_server.h 自己包含 winsock2.h，所以放最前面。
#include "mock_http_server.h"

#include "test_support.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

// OCR 的真实服务端是外部项目（github.com/WallBreaker2/op_ocr_engine），本机没有，
// 所以 tests/ocr_test.cpp 里那 25 条"识别结果"用例只能跳过。
// 但**客户端**（HttpOcrService：组包、HTTP、JSON 解析、超时、错误码）完全可以在本地验证 ——
// 这些用例用一个假服务端把客户端的每种响应都过一遍，任何崩溃/卡死/解析错误都会被抓到。

namespace {

// 假服务端已抽到 tests/mock_http_server.h（OCR/YOLO 两个契约测试共用一份实现）。
using MockOcrServer = test_support::MockHttpServer;

std::wstring MakeTestImageFile(const wchar_t *name) {
    const std::wstring path = test_support::GetTempBmpPath(name);
    const int width = 40;
    const int height = 20;
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 4, 0xFF);
    const auto file_bytes = test_support::BuildBmp32TopDown(width, height, pixels);
    FILE *file = nullptr;
    if (_wfopen_s(&file, path.c_str(), L"wb") != 0 || !file) {
        return std::wstring();
    }
    fwrite(file_bytes.data(), 1, file_bytes.size(), file);
    fclose(file);
    return path;
}

} // namespace

class OcrContractTest : public ::testing::Test {
  protected:
    // 这些用例走的是 HttpOcrService 单例，而 OcrFixture 会改同一份全局字库/端点状态，
    // 混在同一个进程里跑会互相污染（实测单独跑通过、全量跑失败）。
    // 所以主 op_test 入口用这个开关跳过它们，另注册一个独立 ctest 项单独跑。
    void SetUp() override {
        if (!test_support::GetEnvString(L"OP_SKIP_OCR_CONTRACT_TESTS").empty()) {
            GTEST_SKIP() << "OCR contract tests run in their own ctest entry.";
        }
    }

    void TearDown() override {
        // 恢复默认端点，避免污染其它用例。
        op::Op op;
        op.SetOcrEngine(L"", L"", L"");
    }

    op::Op op;
};

TEST_F(OcrContractTest, ParsesValidResponse) {
    MockOcrServer server;
    server.response_body =
        R"({"code":0,"results":[{"text":"OP42","bbox":[1,2,30,40],"confidence":0.98}]})";
    ASSERT_TRUE(server.Start());

    long engine_ret = op.SetOcrEngine(server.endpoint().c_str(), L"", L"--timeout=2000");
    EXPECT_EQ(engine_ret, 1) << "SetOcrEngine 应接受 http 端点";

    const std::wstring image = MakeTestImageFile(L"op_ocr_contract_valid.bmp");
    ASSERT_FALSE(image.empty());

    std::wstring text;
    op.OcrAutoFromFile(image.c_str(), 0.8, text);
    EXPECT_NE(text.find(L"OP42"), std::wstring::npos) << "合法响应应解析出 text（实际=" << text << L"）";
}

// 本地服务端零延迟时，一次 OCR 调用的耗时应该由网络栈和解析决定，
// 而不是被客户端的"等完成标志"方式决定。旧实现用 sleep(5ms) 轮询，成功路径也要多等
// 0~5ms（均值约 2.5ms），对小图 OCR 这种本来就只要几毫秒的调用是很明显的地板（§72.39）。
TEST_F(OcrContractTest, PostLatencyIsNotBoundByPollingGranularity) {
    MockOcrServer server;
    server.delay_ms = 0;
    server.response_body =
        R"({"code":0,"results":[{"text":"OP42","bbox":[1,2,30,40],"confidence":0.98}]})";
    ASSERT_TRUE(server.Start());
    ASSERT_EQ(op.SetOcrEngine(server.endpoint().c_str(), L"", L"--timeout=2000"), 1);

    const std::wstring image = MakeTestImageFile(L"op_ocr_contract_latency.bmp");
    ASSERT_FALSE(image.empty());

    std::wstring text;
    op.OcrAutoFromFile(image.c_str(), 0.8, text); // 预热：建连、首次分配

    const int kIters = 60;
    std::vector<double> samples;
    samples.reserve(kIters);
    for (int i = 0; i < kIters; ++i) {
        const auto t0 = std::chrono::steady_clock::now();
        op.OcrAutoFromFile(image.c_str(), 0.8, text);
        samples.push_back(
            std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count());
    }
    std::sort(samples.begin(), samples.end());
    const double mean = std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
    const double p50 = samples[samples.size() / 2];
    const double p95 = samples[static_cast<size_t>(samples.size() * 0.95)];
    std::printf("[BENCH] OcrAutoFromFile 本地零延迟服务端 %d 次：mean=%.1f us p50=%.1f us p95=%.1f us\n", kIters,
                mean, p50, p95);

    EXPECT_NE(text.find(L"OP42"), std::wstring::npos) << "计时期间调用仍应正常返回";
    // 轮询粒度是 5ms：旧实现的均值必然在 2.5ms 量级。这里只做宽松兜底，
    // 真正的证据是 [BENCH] 行（见 doc/defect_fixes.md §72.39）。
    EXPECT_LT(mean, 8000.0) << "单次 OCR 不该慢到 8ms 以上（本地服务端零延迟）";
}

// 大响应体：WinHTTP 会分多块投递（每块 8~16 KB），旧写法每块都新建临时串再 append。
// 改成"直接读进 response 尾部"之后，这里验证多块拼起来的正文没被截断/错位 ——
// 只看首尾两个标记，任何长度算错或游标写错都会让尾部那条消失。
TEST_F(OcrContractTest, ReadsMultiChunkBodyWithoutCorruption) {
    const int kResultCount = 3000; // ≈156 KB，必然跨多块
    std::string body = R"({"code":0,"results":[)";
    for (int i = 0; i < kResultCount; ++i) {
        if (i)
            body += ",";
        char item[96];
        std::snprintf(item, sizeof(item), R"({"text":"L%04d","bbox":[0,0,10,10],"confidence":0.95})", i);
        body += item;
    }
    body += "]}";

    MockOcrServer server;
    server.response_body = body;
    ASSERT_TRUE(server.Start());

    EXPECT_EQ(op.SetOcrEngine(server.endpoint().c_str(), L"", L"--timeout=5000"), 1);

    const std::wstring image = MakeTestImageFile(L"op_ocr_contract_multichunk.bmp");
    ASSERT_FALSE(image.empty());

    std::wstring text;
    op.OcrAutoFromFile(image.c_str(), 0.8, text);
    EXPECT_NE(text.find(L"L0000"), std::wstring::npos) << "首个结果应解析出来";
    EXPECT_NE(text.find(L"L2999"), std::wstring::npos)
        << "最后一条结果应解析出来（正文长度=" << body.size() << " 字节，尾部丢了说明多块拼接有错位）";
}

// 转义序列要能正确反解。
// 注：上一轮这条用例判红**不是客户端的问题**——客户端解析正常（探针实测
// raw_text=a\"b\\c → unescaped=a"b\c），空结果是因为 confidence=0.5 低于本用例传的 sim=0.8，
// 被 ImageSearchService::OCR 的置信度过滤丢掉了。把 confidence 提到 0.9 后按预期通过。
TEST_F(OcrContractTest, HandlesEscapedText) {
    MockOcrServer server;
    // JSON 里的转义：引号、反斜杠
    server.response_body =
        R"({"code":0,"results":[{"text":"a\"b\\c","bbox":[0,0,10,10],"confidence":0.9}]})";
    ASSERT_TRUE(server.Start());
    ASSERT_EQ(op.SetOcrEngine(server.endpoint().c_str(), L"", L"--timeout=2000"), 1);

    const std::wstring image = MakeTestImageFile(L"op_ocr_contract_escape.bmp");
    ASSERT_FALSE(image.empty());

    std::wstring text;
    op.OcrAutoFromFile(image.c_str(), 0.8, text);
    EXPECT_NE(text.find(L"a\"b\\c"), std::wstring::npos)
        << "转义序列应被反解（实际=" << text << L"）";
}

TEST_F(OcrContractTest, SurvivesHttpError) {
    MockOcrServer server;
    server.status = 500;
    server.response_body = R"({"error":"boom"})";
    ASSERT_TRUE(server.Start());
    ASSERT_EQ(op.SetOcrEngine(server.endpoint().c_str(), L"", L"--timeout=2000"), 1);

    const std::wstring image = MakeTestImageFile(L"op_ocr_contract_500.bmp");
    ASSERT_FALSE(image.empty());

    std::wstring text;
    EXPECT_NO_FATAL_FAILURE(op.OcrAutoFromFile(image.c_str(), 0.8, text));
    EXPECT_TRUE(text.empty()) << "HTTP 500 时应返回空结果而不是崩溃（实际=" << text << L"）";
}

TEST_F(OcrContractTest, SurvivesMalformedBody) {
    MockOcrServer server;
    server.response_body = "not a json at all";
    ASSERT_TRUE(server.Start());
    ASSERT_EQ(op.SetOcrEngine(server.endpoint().c_str(), L"", L"--timeout=2000"), 1);

    const std::wstring image = MakeTestImageFile(L"op_ocr_contract_bad.bmp");
    ASSERT_FALSE(image.empty());

    std::wstring text;
    EXPECT_NO_FATAL_FAILURE(op.OcrAutoFromFile(image.c_str(), 0.8, text));
    EXPECT_TRUE(text.empty()) << "无法解析的响应应返回空结果（实际=" << text << L"）";
}

// 回归护栏：曾经 WinHTTP 的 receive/response 超时在本机完全不生效（配置 300ms、服务端 3s 后响应，
// 调用仍然等满 3s，三次独立复现）。现在 http_post_json 里有自建硬上限（工作线程 + 有界等待），
// 这条用例保证它不会再退回去。
TEST_F(OcrContractTest, RespectsTimeout) {
    MockOcrServer server;
    server.delay_ms = 3000; // 远大于下面的客户端超时
    ASSERT_TRUE(server.Start());
    ASSERT_EQ(op.SetOcrEngine(server.endpoint().c_str(), L"", L"--timeout=300"), 1);

    const std::wstring image = MakeTestImageFile(L"op_ocr_contract_slow.bmp");
    ASSERT_FALSE(image.empty());

    const auto start = std::chrono::steady_clock::now();
    std::wstring text;
    op.OcrAutoFromFile(image.c_str(), 0.8, text);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();

    EXPECT_LT(elapsed, 2000) << "客户端超时没有生效，一次调用卡了 " << elapsed << " ms";
    EXPECT_TRUE(text.empty());
}

// 置信度语义：ImageSearchService::OCR 只保留 confidence >= sim 的结果。
// 这条用例把"低于阈值被丢弃"钉住，避免以后误改过滤逻辑。
TEST_F(OcrContractTest, DropsResultsBelowSimilarity) {
    MockOcrServer server;
    server.response_body =
        R"({"code":0,"results":[{"text":"LOW","bbox":[0,0,10,10],"confidence":0.5}]})";
    ASSERT_TRUE(server.Start());
    ASSERT_EQ(op.SetOcrEngine(server.endpoint().c_str(), L"", L"--timeout=2000"), 1);

    const std::wstring image = MakeTestImageFile(L"op_ocr_contract_conf.bmp");
    ASSERT_FALSE(image.empty());

    std::wstring text;
    op.OcrAutoFromFile(image.c_str(), 0.8, text);
    EXPECT_TRUE(text.empty()) << "confidence 0.5 < sim 0.8 时应被丢弃（实际=" << text << L"）";

    text.clear();
    op.OcrAutoFromFile(image.c_str(), 0.4, text);
    EXPECT_NE(text.find(L"LOW"), std::wstring::npos)
        << "sim 降到 0.4 后同一结果应被保留（实际=" << text << L"）";
}

// 端点校验：这些畸形地址都要**明确拒绝**（返回 0），而不是崩、卡住、或者"看起来接受"。
// 这几条检查是门面可达的（SetOcrEngine 直接把用户串送进 configure_endpoint/parse_url），
// 所以必须有覆盖 —— 与 requestCapture 那种"被上层裁剪过、永远走不到"的防御不同。
TEST_F(OcrContractTest, RejectsMalformedEndpoints) {
    const wchar_t *bad_endpoints[] = {
        L"http://",                 // 没有 host
        L"http://:8080/api/v1/ocr", // 端口前没有 host
        L"https://127.0.0.1:99999/api/v1/ocr", // 端口越界
        L"ftp://127.0.0.1/api/v1/ocr",         // 非 http(s) 协议
        L"not-a-url-at-all",
    };
    for (const wchar_t *endpoint : bad_endpoints) {
        const long ret = op.SetOcrEngine(endpoint, L"", L"");
        EXPECT_EQ(ret, 0) << "畸形端点应被拒绝: " << endpoint;
    }

    // 拒绝之后服务仍然可用（不能被上一次坏配置带坏）。
    test_support::MockHttpServer server;
    server.response_body =
        R"({"code":0,"results":[{"text":"OK","bbox":[0,0,5,5],"confidence":0.9}]})";
    ASSERT_TRUE(server.Start());
    EXPECT_EQ(op.SetOcrEngine(server.endpoint().c_str(), L"", L"--timeout=2000"), 1)
        << "坏配置之后仍应能设置合法端点";
}
// --- §72.41 WinHTTP session/connect 复用 ---------------------------------------------------
// 走公开 API：连接池在 DLL 里，经由 OcrAutoFromFile 才测得到真正发布的那份代码。

namespace {
constexpr int kReusePostCount = 20;
// HttpOcrService 内部本来就是串行的（单例 + mutex），这几条线程验证的是：多条线程排队
// 共用同一份 session/connect 时，池里的 shared_ptr 保活没问题、结果不会串。
constexpr int kConcurrentThreads = 4;
constexpr int kPostsPerThread = 8;
const char *kOkBody = R"({"code":0,"results":[{"text":"OK","bbox":[0,0,10,10],"confidence":0.99}]})";
} // namespace

TEST_F(OcrContractTest, SessionPoolReusesConnectionAcrossSequentialPosts) {
    MockOcrServer server;
    server.response_body = kOkBody;
    ASSERT_TRUE(server.Start());
    ASSERT_EQ(op.SetOcrEngine(server.endpoint().c_str(), L"", L"--timeout=5000"), 1);

    const std::wstring image = MakeTestImageFile(L"op_ocr_session_reuse.bmp");
    ASSERT_FALSE(image.empty());

    for (int i = 0; i < kReusePostCount; ++i) {
        std::wstring text;
        op.OcrAutoFromFile(image.c_str(), 0.8, text);
        ASSERT_EQ(text, L"OK") << "第 " << i << " 次请求失败或结果被污染";
    }
}

TEST_F(OcrContractTest, SessionPoolRecoversAfterTimedOutRequest) {
    MockOcrServer server;
    server.response_body = kOkBody;
    server.delay_ms = 800; // 远超下面的 200ms 超时
    ASSERT_TRUE(server.Start());
    ASSERT_EQ(op.SetOcrEngine(server.endpoint().c_str(), L"", L"--timeout=200"), 1);

    const std::wstring image = MakeTestImageFile(L"op_ocr_session_timeout.bmp");
    ASSERT_FALSE(image.empty());

    std::wstring timed_out;
    const auto started = std::chrono::steady_clock::now();
    op.OcrAutoFromFile(image.c_str(), 0.8, timed_out);
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count();
    EXPECT_TRUE(timed_out.empty()) << "服务端延迟 800ms、超时 200ms，这次不该拿到结果";
    EXPECT_LT(elapsed, 3000) << "超时应在 timeout 量级返回，实际 " << elapsed << "ms";

    // 上一次超时之后，连接池里那条连接可能已经被摘掉；下一次必须能重新建连拿到结果。
    server.delay_ms = 0;
    std::wstring recovered;
    op.OcrAutoFromFile(image.c_str(), 0.8, recovered);
    EXPECT_EQ(recovered, L"OK") << "恢复后的结果应正确（实际=" << recovered << L"）";
}

TEST_F(OcrContractTest, SessionPoolConcurrentRequestsStayCorrect) {
    MockOcrServer server;
    server.response_body = kOkBody;
    ASSERT_TRUE(server.Start());
    ASSERT_EQ(op.SetOcrEngine(server.endpoint().c_str(), L"", L"--timeout=5000"), 1);

    const std::wstring image = MakeTestImageFile(L"op_ocr_session_concurrent.bmp");
    ASSERT_FALSE(image.empty());

    std::atomic<int> ok_count{0};
    std::atomic<int> bad_count{0};
    // 临时诊断：并发下偶发一次失败（32 次里约 1 次），且没有任何 ocr 日志分支被打到，
    // 需要抓出“失败那一次到底拿到了什么”。
    std::mutex bad_mutex;
    std::string first_bad;

    std::vector<std::thread> workers;
    workers.reserve(kConcurrentThreads);
    for (int i = 0; i < kConcurrentThreads; ++i) {
        workers.emplace_back([&image, &ok_count, &bad_count, &bad_mutex, &first_bad]() {
            // 每个线程一份自己的 op_context：ImageSearchService::_src 是“当前图”缓冲，
            // 同一个 Op 实例本来就不能多线程同时用（OcrAutoFromFile 会写 _src）。
            // HTTP 的 session/connect 池是进程级 static，所以这里仍然在测池的并发安全。
            op::Op local_op;
            long local_ret = 0;
            local_op.SetShowErrorMsg(3, &local_ret);
            for (int n = 0; n < kPostsPerThread; ++n) {
                std::wstring text;
                local_op.OcrAutoFromFile(image.c_str(), 0.8, text);
                if (text == L"OK") {
                    ++ok_count;
                } else {
                    std::lock_guard<std::mutex> lock(bad_mutex);
                    if (first_bad.empty()) {
                        // 显式窄化：std::string(wide.begin(), wide.end()) 会触发 C4244（wchar_t→char），
                        // 而且非 ASCII 会被截成垃圾字节。失败消息里只需要可读的 ASCII。
                        std::string narrowed;
                        for (const wchar_t c : text)
                            narrowed.push_back(c >= 0x20 && c < 0x7F ? static_cast<char>(c) : '?');
                        first_bad = text.empty() ? "<empty>" : narrowed;
                    }
                    ++bad_count;
                }
            }
        });
    }
    for (auto &worker : workers)
        worker.join();

    EXPECT_EQ(bad_count.load(), 0) << "并发请求不该失败或串结果；首个失败结果=" << first_bad;
    EXPECT_EQ(ok_count.load(), kConcurrentThreads * kPostsPerThread);
}
