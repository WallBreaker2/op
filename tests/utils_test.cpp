#include "test_support.h"

#include "../libop/imageProc/compute/ThreadPool.h"

#include <vector>

using namespace std;

TEST(UtilsTest, ThreadPool) {
    ThreadPool pool(4);
    auto fut = pool.enqueue([] { return 42; });
    EXPECT_EQ(fut.get(), 42);
}

TEST(UtilsTest, RectDivideBlock) {
    rect_t rc(0, 0, 100, 100);
    vector<rect_t> blocks;
    rc.divideBlock(2, false, blocks);
    EXPECT_EQ(blocks.size(), 2u);
}
