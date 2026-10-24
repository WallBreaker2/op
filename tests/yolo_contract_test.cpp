// winsock2.h 必须先于 windows.h（test_support.h 会拉进后者），所以 mock 头放最前面。
#include "mock_http_server.h"

#include "test_support.h"

#include <psapi.h>

#pragma comment(lib, "psapi.lib")

#include <chrono>
#include <thread>
#include <atomic>
#include <iostream>
#include <string>
#include <vector>

// YOLO 客户端契约用例（与 OCR 同源：都走 op::http_post_json）。
// 真实检测服务是外部依赖，这里只验证客户端：解析、HTTP 错误、超时。

class YoloContractTest : public ::testing::Test {
  protected:
    void SetUp() override {
        if (!test_support::GetEnvString(L"OP_SKIP_OCR_CONTRACT_TESTS").empty()) {
            GTEST_SKIP() << "Client contract tests run in their own ctest entry.";
        }
    }

    void TearDown() override {
        op::Op op;
        op.SetYoloEngine(L"", L"", L"");
    }

    op::Op op;
};

namespace {

std::wstring MakeYoloImageFile(const wchar_t *name) {
    const std::wstring path = test_support::GetTempBmpPath(name);
    const int width = 32;
    const int height = 24;
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

TEST_F(YoloContractTest, ParsesValidResponse) {
    test_support::MockHttpServer server;
    server.response_body =
        R"({"code":0,"results":[{"label":"person","bbox":[1,2,30,40],"confidence":0.91,"class_id":0}]})";
    ASSERT_TRUE(server.Start());
    ASSERT_EQ(op.SetYoloEngine(server.endpoint(L"/api/v1/detect").c_str(), L"", L"--timeout=2000"), 1);

    const std::wstring image = MakeYoloImageFile(L"op_yolo_contract_valid.bmp");
    ASSERT_FALSE(image.empty());

    std::wstring json;
    long ret = 0;
    op.YoloDetectFromFile(image.c_str(), 0.5, 0.45, json, &ret);
    EXPECT_EQ(ret, 1) << "合法响应应返回 1 个目标（json=" << json << L"）";
    EXPECT_NE(json.find(L"person"), std::wstring::npos) << "应解析出 label（实际=" << json << L"）";
    EXPECT_NE(json.find(L"\"ok\":1"), std::wstring::npos) << "成功时应标记 ok:1（实际=" << json << L"）";
}

TEST_F(YoloContractTest, SurvivesHttpError) {
    test_support::MockHttpServer server;
    server.status = 500;
    server.response_body = R"({"error":"boom"})";
    ASSERT_TRUE(server.Start());
    ASSERT_EQ(op.SetYoloEngine(server.endpoint(L"/api/v1/detect").c_str(), L"", L"--timeout=2000"), 1);

    const std::wstring image = MakeYoloImageFile(L"op_yolo_contract_500.bmp");
    ASSERT_FALSE(image.empty());

    std::wstring json;
    long ret = -1;
    EXPECT_NO_FATAL_FAILURE(op.YoloDetectFromFile(image.c_str(), 0.5, 0.45, json, &ret));
    EXPECT_EQ(ret, 0) << "HTTP 500 时应返回 0 个目标";
    EXPECT_NE(json.find(L"\"ok\":0"), std::wstring::npos) << "失败时应标记 ok:0（实际=" << json << L"）";
}

// YOLO 与 OCR 共用 op::http_post_json，这条用例保证"自建硬上限"对 YOLO 这条路也生效。
TEST_F(YoloContractTest, RespectsTimeout) {
    test_support::MockHttpServer server;
    server.delay_ms = 3000;
    ASSERT_TRUE(server.Start());
    ASSERT_EQ(op.SetYoloEngine(server.endpoint(L"/api/v1/detect").c_str(), L"", L"--timeout=300"), 1);

    const std::wstring image = MakeYoloImageFile(L"op_yolo_contract_slow.bmp");
    ASSERT_FALSE(image.empty());

    const auto start = std::chrono::steady_clock::now();
    std::wstring json;
    long ret = -1;
    op.YoloDetectFromFile(image.c_str(), 0.5, 0.45, json, &ret);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();

    EXPECT_LT(elapsed, 2000) << "YOLO 客户端超时没有生效，一次调用卡了 " << elapsed << " ms";
    EXPECT_EQ(ret, 0) << "超时后应返回 0 个目标";
}

// 超时后工作线程仍在跑（WinHTTP 没有可靠取消手段），所以必须自证"不泄漏"：
// 连续制造超时，确认进程句柄数与私有字节不随次数增长。
// 记录基线 → 跑 N 次超时 → 等后台线程收尾 → 再测一次，用真实数字说话。
TEST_F(YoloContractTest, RepeatedTimeoutsDoNotLeakHandlesOrMemory) {
    struct Sample {
        DWORD handles = 0;
        SIZE_T private_bytes = 0;
    };
    auto sample = []() {
        Sample s;
        ::GetProcessHandleCount(::GetCurrentProcess(), &s.handles);
        PROCESS_MEMORY_COUNTERS_EX counters = {};
        if (::GetProcessMemoryInfo(::GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&counters),
                                   sizeof(counters))) {
            s.private_bytes = counters.PrivateUsage;
        }
        return s;
    };

    test_support::MockHttpServer server;
    server.delay_ms = 500; // 远大于客户端超时（100ms）
    ASSERT_TRUE(server.Start());
    ASSERT_EQ(op.SetYoloEngine(server.endpoint(L"/api/v1/detect").c_str(), L"", L"--timeout=100"), 1);

    const std::wstring image = MakeYoloImageFile(L"op_yolo_contract_stress.bmp");
    ASSERT_FALSE(image.empty());

    // 预热一次，避免把"首次加载 WinHTTP 的一次性分配"算进增长。
    {
        std::wstring warmup;
        long warmup_ret = 0;
        op.YoloDetectFromFile(image.c_str(), 0.5, 0.45, warmup, &warmup_ret);
    }
    const Sample before = sample();

    constexpr int kIterations = 20;
    int timed_out = 0;
    for (int i = 0; i < kIterations; ++i) {
        std::wstring json;
        long ret = -1;
        const auto start = std::chrono::steady_clock::now();
        op.YoloDetectFromFile(image.c_str(), 0.5, 0.45, json, &ret);
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - start)
                                 .count();
        if (elapsed < 400) {
            ++timed_out;
        }
        EXPECT_EQ(ret, 0) << "第 " << i << " 次超时应返回 0 个目标";
    }
    EXPECT_EQ(timed_out, kIterations) << "有调用没有在超时时间内返回（timed_out=" << timed_out << "）";

    // 等后台工作线程把 WinHTTP 收尾（每个 500ms 延迟，留 1.2s 余量）。
    ::Sleep(1200);

    const Sample after = sample();
    const long long handle_growth = static_cast<long long>(after.handles) - static_cast<long long>(before.handles);
    const long long byte_growth = static_cast<long long>(after.private_bytes) - static_cast<long long>(before.private_bytes);

    // 把实测数字打出来（成功时也能看到），方便以后对比是否退化。
    std::cout << "[stress] handles " << before.handles << " -> " << after.handles << " (delta " << handle_growth
              << "), private_bytes " << before.private_bytes << " -> " << after.private_bytes << " (delta "
              << byte_growth << "), iterations " << kIterations << std::endl;

    EXPECT_LE(handle_growth, 8) << "连续超时后句柄增长过多（before=" << before.handles << " after=" << after.handles
                                << "）";
    EXPECT_LT(byte_growth, 16LL * 1024 * 1024) << "连续超时后私有字节增长过多（before=" << before.private_bytes
                                              << " after=" << after.private_bytes << "）";
}

// 并发超时：真实脚本会多线程调 OCR/YOLO。第 34 批给 http_post_json 加的"硬上限"是
// 工作线程 + 有界等待 + 超时后 detach，多线程同时超时时是否安全必须验证。
//
// 注：OCR/YOLO 服务都是单例且各自持锁，所以经门面的并发调用实际会**串行**；
// 本用例验证的是"串行化 + 反复超时 + 多线程入口"下不崩溃、不泄漏、总耗时不失控。
TEST_F(YoloContractTest, ConcurrentTimeoutsDoNotCrashOrLeak) {
    struct Sample {
        DWORD handles = 0;
        SIZE_T private_bytes = 0;
    };
    auto sample = []() {
        Sample s;
        ::GetProcessHandleCount(::GetCurrentProcess(), &s.handles);
        PROCESS_MEMORY_COUNTERS_EX counters = {};
        if (::GetProcessMemoryInfo(::GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&counters),
                                   sizeof(counters))) {
            s.private_bytes = counters.PrivateUsage;
        }
        return s;
    };

    test_support::MockHttpServer server;
    server.delay_ms = 500; // 远大于客户端超时
    ASSERT_TRUE(server.Start());
    ASSERT_EQ(op.SetYoloEngine(server.endpoint(L"/api/v1/detect").c_str(), L"", L"--timeout=100"), 1);

    const std::wstring image = MakeYoloImageFile(L"op_yolo_contract_concurrent.bmp");
    ASSERT_FALSE(image.empty());

    {
        std::wstring warmup;
        long warmup_ret = 0;
        op.YoloDetectFromFile(image.c_str(), 0.5, 0.45, warmup, &warmup_ret);
    }
    const Sample before = sample();

    constexpr int kThreads = 4;
    constexpr int kIterations = 5;
    std::atomic<int> failures{0};
    std::atomic<int> calls{0};

    const auto start = std::chrono::steady_clock::now();
    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&]() {
            for (int i = 0; i < kIterations; ++i) {
                std::wstring json;
                long ret = -1;
                op.YoloDetectFromFile(image.c_str(), 0.5, 0.45, json, &ret);
                if (ret != 0) {
                    failures.fetch_add(1);
                }
                calls.fetch_add(1);
            }
        });
    }
    for (auto &worker : workers) {
        worker.join();
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();

    EXPECT_EQ(calls.load(), kThreads * kIterations) << "并发调用次数不对";
    EXPECT_EQ(failures.load(), 0) << "超时应当返回 0 个目标，不该有其它失败";
    // 串行口径：每次约 timeout(100ms)+余量，20 次给 15s 上限。
    EXPECT_LT(elapsed, 15000) << "并发超时总耗时失控（" << elapsed << " ms）";

    ::Sleep(1500); // 让 detach 的工作线程收尾

    const Sample after = sample();
    const long long handle_growth = static_cast<long long>(after.handles) - static_cast<long long>(before.handles);
    const long long byte_growth =
        static_cast<long long>(after.private_bytes) - static_cast<long long>(before.private_bytes);

    std::cout << "[concurrent] calls=" << calls.load() << " failures=" << failures.load() << " elapsed=" << elapsed
              << "ms handles " << before.handles << " -> " << after.handles << " (delta " << handle_growth
              << "), private_bytes delta " << byte_growth << std::endl;

    EXPECT_LE(handle_growth, 16) << "并发超时后句柄增长过多（before=" << before.handles << " after=" << after.handles
                                 << "）";
    EXPECT_LT(byte_growth, 32LL * 1024 * 1024) << "并发超时后私有字节增长过多（delta=" << byte_growth << "）";
}

