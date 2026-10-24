#include "test_support.h"

#include "../libop/base/ThreadPool.h"

#include <chrono>
#include <vector>

using namespace std;

TEST(UtilsTest, ThreadPool) {
    ThreadPool pool(4);
    auto fut = pool.enqueue([] { return 42; });
    EXPECT_EQ(fut.get(), 42);
}

TEST(UtilsTest, RectDivideBlock) {
    op::rect_t rc(0, 0, 100, 100);
    vector<op::rect_t> blocks;
    rc.divideBlock(2, false, blocks);
    EXPECT_EQ(blocks.size(), 2u);
}

// Delay 期间不该把 CPU 烧满：原来是无等待忙等（PeekMessage 轮询），
// 200ms 的 Delay 会吃掉约 200ms CPU 时间。现在没有窗口消息时会阻塞等待。
// 用"CPU 时间 << 墙钟时间"来钉住这一点。
TEST(UtilsTest, DelayDoesNotBurnCpu) {
    auto filetime_ms = [](const FILETIME &a, const FILETIME &b) {
        ULARGE_INTEGER from = {};
        ULARGE_INTEGER to = {};
        from.LowPart = a.dwLowDateTime;
        from.HighPart = a.dwHighDateTime;
        to.LowPart = b.dwLowDateTime;
        to.HighPart = b.dwHighDateTime;
        return static_cast<long long>((to.QuadPart - from.QuadPart) / 10000ULL);
    };

    FILETIME create{}, exit{}, kernel_before{}, user_before{}, kernel_after{}, user_after{};
    ASSERT_TRUE(::GetProcessTimes(::GetCurrentProcess(), &create, &exit, &kernel_before, &user_before));

    const auto start = std::chrono::steady_clock::now();
    // Delay 本身不导出（op_test 链接的是 DLL 的导入库），走公开门面测同一条路径。
    op::Op patient;
    long ret = 0;
    patient.Delay(200, &ret);
    const auto wall_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

    ASSERT_TRUE(::GetProcessTimes(::GetCurrentProcess(), &create, &exit, &kernel_after, &user_after));
    const long long cpu_ms =
        filetime_ms(kernel_before, kernel_after) + filetime_ms(user_before, user_after);

    EXPECT_GE(wall_ms, 180) << "Delay 应至少等满指定时间";
    EXPECT_LT(cpu_ms * 2, wall_ms) << "Delay 期间 CPU 时间应远小于墙钟时间（cpu=" << cpu_ms << "ms wall=" << wall_ms
                                   << "ms）";
}
