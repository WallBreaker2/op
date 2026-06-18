#include "test_support.h"

using namespace test_support;

TEST(YoloTest, SetYoloEngineAcceptsBaseUrlAndAliasArgs) {
    libop op;
    EXPECT_EQ(1, op.SetYoloEngine(L"http://127.0.0.1:8090", L"", L"--timeout=5000"));
    EXPECT_EQ(1, op.SetYoloEngine(L"yolo", L"", L"--timeout=2000"));
}

TEST(YoloTest, SetYoloEngineRejectsInvalidUrl) {
    libop op;
    EXPECT_EQ(0, op.SetYoloEngine(L"http://", L"", L"--timeout=5000"));
}

TEST(YoloTest, YoloDetectFromFileReturnsEmptyForMissingFile) {
    libop op;
    std::wstring json = L"not-empty";
    long ret = 123;
    op.YoloDetectFromFile(L"__missing_yolo_input__.bmp", 0.25, 0.45, json, &ret);
    EXPECT_EQ(0, ret);
    EXPECT_TRUE(json.empty());
}
