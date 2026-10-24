#include "test_support.h"

#include <cstdlib>
#include <string>

namespace {

// 把"被环境跳过"的用例在结束时明确列出来。
// gtest 会把 GTEST_SKIP() 算成"通过"，所以 ctest 里看不出来 —— 之前就因此漏掉了
// 两个真实问题（截屏套件被静默跳过、27 条 OpenCV 用例根本没编译）。
// 这个监听器只**打印**，不改变任何用例结果与退出码。
class SkipSummaryListener : public ::testing::EmptyTestEventListener {
  public:
    // §72.51（审计 R-02）：main() 要用这个数做严格门禁，所以留一个读取口。
    int skipped() const {
        return skipped_;
    }

    void OnTestIterationEnd(const ::testing::UnitTest &unit_test, int /*iteration*/) override {
        const int skipped = unit_test.skipped_test_count();
        skipped_ = skipped;
        if (skipped <= 0) {
            return;
        }
        printf("\n[ ENV-SKIP ] %d 条用例被环境跳过（gtest 计为通过，但并没有真正验证）：\n", skipped);
        for (int s = 0; s < unit_test.total_test_suite_count(); ++s) {
            const ::testing::TestSuite *suite = unit_test.GetTestSuite(s);
            if (!suite) {
                continue;
            }
            for (int t = 0; t < suite->total_test_count(); ++t) {
                const ::testing::TestInfo *info = suite->GetTestInfo(t);
                if (info && info->should_run() && info->result()->Skipped()) {
                    printf("    %s.%s\n", suite->name(), info->name());
                }
            }
        }
        printf("[ ENV-SKIP ] 以上用例需要额外环境（OCR/YOLO 服务、真实图片素材、特定显示后端等）\n\n");
        fflush(stdout);
    }

  private:
    int skipped_ = 0;
};

} // namespace

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new test_support::OpEnvironment);
    SkipSummaryListener *skip_listener = new SkipSummaryListener;
    ::testing::UnitTest::GetInstance()->listeners().Append(skip_listener);
    const int rc = RUN_ALL_TESTS();

    // §72.51（审计 R-02）：gtest 把 GTEST_SKIP() 算通过，所以整链回归可能是
    // "绿的但 x64 92 条 / x86 94 条根本没跑"。默认还是只打印（不改变现有 ctest
    // 的语义）；发布前 / CI 想严格的时候设 OP_REQUIRE_NO_SKIPS=1，只要有跳过
    // 就以非零退出，让跳过变成可见的失败。
    // 用 _dupenv_s 而不是 getenv：MSVC 把 getenv 标成 C4996，本项目零 warning。
    char *strict_raw = nullptr;
    size_t strict_len = 0;
    bool require_none = false;
    if (::_dupenv_s(&strict_raw, &strict_len, "OP_REQUIRE_NO_SKIPS") == 0 && strict_raw) {
        require_none = strict_len > 1 && strict_raw[0] != '0';
        std::free(strict_raw);
    }
    if (rc == 0 && require_none && skip_listener->skipped() > 0) {
        printf("\n[ ENV-SKIP ] OP_REQUIRE_NO_SKIPS=1：%d 条用例被跳过，按失败处理\n", skip_listener->skipped());
        fflush(stdout);
        return 1;
    }
    return rc;
}
