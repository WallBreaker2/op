#include "test_support.h"

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new test_support::OpEnvironment);
    return RUN_ALL_TESTS();
}
