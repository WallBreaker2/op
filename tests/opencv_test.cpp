#include "test_support.h"

#include "op_c_api.h"

#include "../libop/opencv/TemplateMatcher.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <fstream>
#include <numeric>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

using namespace std;

namespace {

// 把测试字节写到本地文件，供图像加载接口读取。
bool WriteBytesToFile(const std::wstring &path, const std::vector<uchar> &bytes) {
    std::ofstream output(path, std::ios::binary);
    if (!output.is_open()) {
        return false;
    }
    output.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return output.good();
}

// 使用 OpenCV 写出带 alpha 的 PNG，供自动 alpha mask 测试使用。
bool WritePngFile(const std::wstring &path, const cv::Mat &image) {
    const std::string narrow = std::filesystem::path(path).string();
    return cv::imwrite(narrow, image);
}

std::wstring FindOpenCvSampleImage(const std::wstring &file_name) {
    const auto cwd = std::filesystem::current_path();
    const std::vector<std::filesystem::path> candidates = {
        cwd / L"build" / L"_deps" / L"opencv" / L"opencv-4.13.0" / L"samples" / L"data" / file_name,
        cwd.parent_path() / L"build" / L"_deps" / L"opencv" / L"opencv-4.13.0" / L"samples" / L"data" / file_name,
        cwd.parent_path().parent_path() / L"build" / L"_deps" / L"opencv" / L"opencv-4.13.0" / L"samples" /
            L"data" / file_name,
    };

    for (const auto &candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate.wstring();
        }
    }
    return L"";
}

std::wstring FindDemoAsset(const std::wstring &file_name) {
    const auto cwd = std::filesystem::current_path();
    const std::vector<std::filesystem::path> candidates = {
        cwd / L"assets" / file_name,
        cwd.parent_path() / L"assets" / file_name,
        cwd.parent_path().parent_path() / L"assets" / file_name,
        cwd / L"doc" / L"op.wiki" / L"demo" / L"assets" / file_name,
        cwd.parent_path() / L"doc" / L"op.wiki" / L"demo" / L"assets" / file_name,
        cwd.parent_path().parent_path() / L"doc" / L"op.wiki" / L"demo" / L"assets" / file_name,
    };

    for (const auto &candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate.wstring();
        }
    }
    return L"";
}

opcv::ImageHandle ImageFromMat(const cv::Mat &image) {
    opcv::ImageHandle handle;
    if (image.empty()) {
        return handle;
    }

    handle.width = image.cols;
    handle.height = image.rows;
    handle.channels = image.channels();
    handle.bytes.assign(image.data, image.data + static_cast<size_t>(image.total() * image.elemSize()));
    return handle;
}

long long MeasureMilliseconds(const std::function<bool()> &operation, bool &ok) {
    const auto start = std::chrono::steady_clock::now();
    ok = operation();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    return elapsed.count();
}

void RecordMeasuredMatch(const std::string &name, long long elapsed_ms, double score) {
    ::testing::Test::RecordProperty(name + "_elapsed_ms", elapsed_ms);
    ::testing::Test::RecordProperty(name + "_score", score);
}

struct BenchmarkStats {
    long long min_ms = 0;
    long long median_ms = 0;
    long long p95_ms = 0;
    double avg_ms = 0.0;
};

BenchmarkStats BuildBenchmarkStats(std::vector<long long> samples) {
    BenchmarkStats stats;
    if (samples.empty()) {
        return stats;
    }

    std::sort(samples.begin(), samples.end());
    stats.min_ms = samples.front();
    stats.median_ms = samples[samples.size() / 2];
    const size_t p95_index = std::min(samples.size() - 1, static_cast<size_t>((samples.size() * 95 + 99) / 100 - 1));
    stats.p95_ms = samples[p95_index];
    stats.avg_ms = static_cast<double>(std::accumulate(samples.begin(), samples.end(), 0LL)) /
                   static_cast<double>(samples.size());
    return stats;
}

void RecordBenchmarkStats(const std::string &name, const BenchmarkStats &stats) {
    ::testing::Test::RecordProperty(name + "_min_ms", stats.min_ms);
    ::testing::Test::RecordProperty(name + "_median_ms", stats.median_ms);
    ::testing::Test::RecordProperty(name + "_avg_ms", stats.avg_ms);
    ::testing::Test::RecordProperty(name + "_p95_ms", stats.p95_ms);
}

BenchmarkStats MeasureBenchmark(
    const std::function<bool()> &operation,
    bool &ok,
    int warmup_runs = 1,
    int measured_runs = 5) {
    ok = true;
    for (int index = 0; index < warmup_runs; ++index) {
        if (!operation()) {
            ok = false;
            return {};
        }
    }

    std::vector<long long> samples;
    samples.reserve(static_cast<size_t>(measured_runs));
    for (int index = 0; index < measured_runs; ++index) {
        bool run_ok = false;
        const long long elapsed_ms = MeasureMilliseconds(operation, run_ok);
        if (!run_ok) {
            ok = false;
            samples.clear();
            return {};
        }
        samples.push_back(elapsed_ms);
    }
    return BuildBenchmarkStats(std::move(samples));
}

struct RealImageCase {
    std::wstring label;
    std::wstring source_file;
    std::wstring template_file;
    cv::Rect template_rect;
};

std::vector<RealImageCase> RealPhotoTemplateCases() {
    return {
        {L"landscape", L"opencv_real_source.jpg", L"opencv_real_template.png", cv::Rect(860, 130, 96, 96)},
        {L"city", L"opencv_real_city_source.jpg", L"opencv_real_city_template.png", cv::Rect(700, 260, 96, 96)},
        {L"fruit", L"opencv_real_fruit_source.jpg", L"opencv_real_fruit_template.png", cv::Rect(520, 250, 96, 96)},
        {L"sign", L"opencv_real_sign_source.jpg", L"opencv_real_sign_template.png", cv::Rect(585, 190, 96, 96)},
        {L"space", L"opencv_real_space_source.jpg", L"opencv_real_space_template.png", cv::Rect(560, 300, 96, 96)},
        {L"texture", L"opencv_real_texture_source.jpg", L"opencv_real_texture_template.png", cv::Rect(640, 320, 96, 96)},
    };
}

std::vector<RealImageCase> GameSceneTemplateCases() {
    return {
        {L"scene", L"opencv_game_scene_source.png", L"opencv_game_scene_template.png", cv::Rect(300, 488, 128, 128)},
        {L"coin", L"opencv_game_scene_source.png", L"opencv_game_coin_template.png", cv::Rect(170, 185, 70, 70)},
        {L"gem", L"opencv_game_scene_source.png", L"opencv_game_gem_template.png", cv::Rect(455, 325, 70, 70)},
        {L"scale", L"opencv_game_scale_source.png", L"opencv_game_coin_template.png", cv::Rect(1040, 120, 105, 105)},
        {L"feature", L"opencv_game_feature_source.png", L"opencv_game_feature_template.png", cv::Rect(837, 102, 156, 156)},
        {L"edge", L"opencv_game_scene_source.png", L"opencv_game_edge_template.png", cv::Rect(1060, 240, 90, 114)},
        {L"shape", L"opencv_game_shape_source.png", L"opencv_game_shape_template.png", cv::Rect(420, 65, 240, 275)},
    };
}

bool HasDemoAssetPair(const RealImageCase &item) {
    return !FindDemoAsset(item.source_file).empty() && !FindDemoAsset(item.template_file).empty();
}

void ExpectNearRect(const opcv::MatchResult &match, const cv::Rect &expected, int tolerance) {
    EXPECT_NEAR(match.x, expected.x, tolerance);
    EXPECT_NEAR(match.y, expected.y, tolerance);
    EXPECT_NEAR(match.width, expected.width, tolerance);
    EXPECT_NEAR(match.height, expected.height, tolerance);
}

// 构造测试用的大图和模板文件。
void BuildTemplateTestData(
    std::vector<uchar> &source_bgra,
    int &source_width,
    int &source_height,
    int &template_x,
    int &template_y,
    int &template_width,
    int &template_height,
    std::wstring &template_path) {
    source_width = 8;
    source_height = 8;
    source_bgra.assign(static_cast<size_t>(source_width * source_height * 4), 0);

    for (int y = 0; y < source_height; ++y) {
        for (int x = 0; x < source_width; ++x) {
            const size_t index = static_cast<size_t>((y * source_width + x) * 4);
            source_bgra[index + 0] = static_cast<uchar>((x * x * 17 + y * 29 + x * y * 7 + 23) % 251);
            source_bgra[index + 1] = static_cast<uchar>((x * 31 + y * y * 19 + x * y * 11 + 47) % 253);
            source_bgra[index + 2] = static_cast<uchar>((x * 13 + y * 17 + x * x * 5 + y * y * 3 + 71) % 255);
            source_bgra[index + 3] = 255;
        }
    }

    template_x = 3;
    template_y = 2;
    template_width = 3;
    template_height = 3;

    std::vector<uchar> template_bgra(static_cast<size_t>(template_width * template_height * 4), 0);
    for (int y = 0; y < template_height; ++y) {
        for (int x = 0; x < template_width; ++x) {
            const size_t source_index =
                static_cast<size_t>(((template_y + y) * source_width + (template_x + x)) * 4);
            const size_t template_index = static_cast<size_t>((y * template_width + x) * 4);
            for (int channel = 0; channel < 4; ++channel) {
                template_bgra[template_index + channel] = source_bgra[source_index + channel];
            }
        }
    }

    const auto template_bmp = test_support::BuildBmp32TopDown(template_width, template_height, template_bgra);
    template_path = test_support::GetTempBmpPath(L"opencv_template_match.bmp");
    ASSERT_TRUE(WriteBytesToFile(template_path, template_bmp));
}

// 构造更适合 ORB 的随机纹理块测试数据。
void BuildFeaturePatternData(
    std::vector<uchar> &source_bgra,
    int &source_width,
    int &source_height,
    int &template_x,
    int &template_y,
    int &template_width,
    int &template_height,
    std::wstring &template_path) {
    source_width = 64;
    source_height = 64;
    cv::Mat source(source_height, source_width, CV_8UC4, cv::Scalar(0, 0, 0, 255));

    template_x = 18;
    template_y = 16;
    template_width = 24;
    template_height = 24;

    const cv::Rect roi(template_x, template_y, template_width, template_height);
    cv::rectangle(source, roi, cv::Scalar(40, 40, 40, 255), cv::FILLED);
    cv::rectangle(source, roi, cv::Scalar(255, 255, 255, 255), 2);
    cv::line(
        source,
        cv::Point(template_x + 3, template_y + 3),
        cv::Point(template_x + template_width - 4, template_y + template_height - 4),
        cv::Scalar(255, 255, 255, 255),
        2);
    cv::line(
        source,
        cv::Point(template_x + template_width - 4, template_y + 3),
        cv::Point(template_x + 3, template_y + template_height - 4),
        cv::Scalar(200, 200, 200, 255),
        2);
    cv::circle(
        source,
        cv::Point(template_x + template_width / 2, template_y + template_height / 2),
        4,
        cv::Scalar(255, 255, 255, 255),
        2);
    cv::rectangle(
        source,
        cv::Rect(template_x + 4, template_y + 9, 5, 5),
        cv::Scalar(255, 255, 255, 255),
        cv::FILLED);
    cv::rectangle(
        source,
        cv::Rect(template_x + 15, template_y + 6, 4, 8),
        cv::Scalar(180, 180, 180, 255),
        cv::FILLED);
    cv::circle(
        source,
        cv::Point(template_x + 7, template_y + 18),
        2,
        cv::Scalar(255, 255, 255, 255),
        cv::FILLED);
    cv::circle(
        source,
        cv::Point(template_x + 17, template_y + 18),
        2,
        cv::Scalar(220, 220, 220, 255),
        cv::FILLED);

    source_bgra.assign(source.data, source.data + static_cast<size_t>(source.total() * source.elemSize()));

    std::vector<uchar> template_bgra(static_cast<size_t>(template_width * template_height * 4), 0);
    for (int y = 0; y < template_height; ++y) {
        for (int x = 0; x < template_width; ++x) {
            const size_t source_index =
                static_cast<size_t>(((template_y + y) * source_width + (template_x + x)) * 4);
            const size_t template_index = static_cast<size_t>((y * template_width + x) * 4);
            for (int channel = 0; channel < 4; ++channel) {
                template_bgra[template_index + channel] = source_bgra[source_index + channel];
            }
        }
    }

    const auto template_bmp = test_support::BuildBmp32TopDown(template_width, template_height, template_bgra);
    template_path = test_support::GetTempBmpPath(L"opencv_feature_template.bmp");
    ASSERT_TRUE(WriteBytesToFile(template_path, template_bmp));
}

void SetBgraPixel(
    std::vector<uchar> &pixels,
    int width,
    int x,
    int y,
    uchar b,
    uchar g,
    uchar r,
    uchar a = 255) {
    const size_t index = static_cast<size_t>((y * width + x) * 4);
    pixels[index + 0] = b;
    pixels[index + 1] = g;
    pixels[index + 2] = r;
    pixels[index + 3] = a;
}

void BuildLargePyramidTemplateData(
    std::vector<uchar> &source_bgra,
    int &source_width,
    int &source_height,
    int &template_width,
    int &template_height,
    std::vector<cv::Point> hit_points,
    std::wstring &template_path,
    const std::wstring &template_file_name) {
    source_width = 960;
    source_height = 540;
    template_width = 32;
    template_height = 32;
    source_bgra.assign(static_cast<size_t>(source_width * source_height * 4), 0);

    for (int y = 0; y < source_height; ++y) {
        for (int x = 0; x < source_width; ++x) {
            SetBgraPixel(
                source_bgra,
                source_width,
                x,
                y,
                static_cast<uchar>((x * 3 + y * 5 + 17) % 97),
                static_cast<uchar>((x * 7 + y * 2 + 29) % 89),
                static_cast<uchar>((x * 5 + y * 11 + 43) % 83));
        }
    }

    std::vector<uchar> template_bgra(static_cast<size_t>(template_width * template_height * 4), 0);
    for (int y = 0; y < template_height; ++y) {
        for (int x = 0; x < template_width; ++x) {
            uchar b = static_cast<uchar>((x * 17 + y * 31 + x * y * 7 + 53) % 251);
            uchar g = static_cast<uchar>((x * x * 11 + y * 13 + x * y * 5 + 89) % 253);
            uchar r = static_cast<uchar>((x * 23 + y * y * 17 + (x ^ y) * 29 + 131) % 255);
            if ((x + y) % 7 == 0) {
                b = static_cast<uchar>(240 - (x * 3) % 41);
                g = static_cast<uchar>(30 + (y * 5) % 61);
                r = static_cast<uchar>(180 + (x * y) % 53);
            }
            SetBgraPixel(template_bgra, template_width, x, y, b, g, r);
        }
    }

    for (const auto &point : hit_points) {
        ASSERT_GE(point.x, 0);
        ASSERT_GE(point.y, 0);
        ASSERT_LE(point.x + template_width, source_width);
        ASSERT_LE(point.y + template_height, source_height);
        for (int y = 0; y < template_height; ++y) {
            for (int x = 0; x < template_width; ++x) {
                const size_t src_index = static_cast<size_t>((y * template_width + x) * 4);
                SetBgraPixel(
                    source_bgra,
                    source_width,
                    point.x + x,
                    point.y + y,
                    template_bgra[src_index + 0],
                    template_bgra[src_index + 1],
                    template_bgra[src_index + 2],
                    template_bgra[src_index + 3]);
            }
        }
    }

    const auto template_bmp = test_support::BuildBmp32TopDown(template_width, template_height, template_bgra);
    template_path = test_support::GetTempBmpPath(template_file_name.c_str());
    ASSERT_TRUE(WriteBytesToFile(template_path, template_bmp));
}

} // namespace

TEST(OpenCvTest, ReportsVersion) {
    const std::wstring version = opcv::GetOpenCvVersion();
    EXPECT_FALSE(version.empty());
}

TEST(OpenCvTest, CApiJsonMethodsReturnFailureJsonForInvalidHandle) {
    EXPECT_EQ(L"{\"ok\":0}", std::wstring(OpCvMatchTemplate(nullptr, 0, 0, 10, 10, L"missing", 0.9, 0, 0, 0, 0)));
    EXPECT_EQ(L"{\"ok\":0,\"results\":[]}",
              std::wstring(OpCvConnectedComponents(nullptr, L"missing.bmp", 1.0)));
    EXPECT_EQ(L"{\"ok\":0,\"results\":[]}",
              std::wstring(OpCvMatchAllTemplates(nullptr, 0, 0, 10, 10, L"missing", 0.9, 0, 0, 0, 0)));
}

TEST(OpenCvTest, TemplateStoreAndMatchApis) {
    opcv::RemoveAllTemplates();

    std::vector<uchar> source_bgra;
    int source_width = 0;
    int source_height = 0;
    int template_x = 0;
    int template_y = 0;
    int template_width = 0;
    int template_height = 0;
    std::wstring template_path;
    BuildTemplateTestData(
        source_bgra,
        source_width,
        source_height,
        template_x,
        template_y,
        template_width,
        template_height,
        template_path);

    opcv::ImageHandle source_image;
    source_image.bytes = source_bgra;
    source_image.width = source_width;
    source_image.height = source_height;
    source_image.channels = 4;

    opcv::Region full_region{0, 0, source_width, source_height};

    ASSERT_TRUE(opcv::LoadTemplate(L"btn_ok", template_path));
    EXPECT_TRUE(opcv::HasTemplate(L"btn_ok"));
    EXPECT_EQ(opcv::GetTemplateCount(), 1);

    const auto names = opcv::GetAllTemplateNames();
    ASSERT_EQ(names.size(), 1u);
    EXPECT_EQ(names[0], L"btn_ok");

    opcv::MatchResult match;
    ASSERT_TRUE(opcv::MatchTemplate(source_image, L"btn_ok", full_region, 0.95, match));
    EXPECT_EQ(match.x, template_x);
    EXPECT_EQ(match.y, template_y);
    EXPECT_EQ(match.width, template_width);
    EXPECT_EQ(match.height, template_height);

    opcv::MatchResult match_ccoeff;
    ASSERT_TRUE(opcv::MatchTemplate(
        source_image, L"btn_ok", full_region, 0.95, match_ccoeff, opcv::SearchDirection::LeftToRight,
        opcv::StripMode::None, 5));
    EXPECT_EQ(match_ccoeff.x, template_x);
    EXPECT_EQ(match_ccoeff.y, template_y);

    opcv::MatchResult match_color;
    ASSERT_TRUE(opcv::MatchTemplate(
        source_image,
        L"btn_ok",
        full_region,
        0.95,
        match_color,
        opcv::SearchDirection::LeftToRight,
        opcv::StripMode::None,
        1,
        opcv::MatchColorMode::Color));
    EXPECT_EQ(match_color.x, template_x);
    EXPECT_EQ(match_color.y, template_y);

    opcv::Region region;
    region.x = 2;
    region.y = 1;
    region.width = 4;
    region.height = 4;
    opcv::MatchResult region_match;
    ASSERT_TRUE(opcv::MatchTemplate(source_image, L"btn_ok", region, 0.95, region_match));
    EXPECT_EQ(region_match.x, template_x);
    EXPECT_EQ(region_match.y, template_y);

    ASSERT_TRUE(opcv::LoadTemplateList({{L"btn_ok_copy", template_path}}));
    EXPECT_EQ(opcv::GetTemplateCount(), 2);

    opcv::NamedMatchResult any_match;
    ASSERT_TRUE(opcv::MatchAnyTemplate(
        source_image,
        {L"btn_ok_copy", L"btn_ok"},
        full_region,
        0.95,
        any_match));
    EXPECT_FALSE(any_match.name.empty());
    EXPECT_EQ(any_match.match.x, template_x);
    EXPECT_EQ(any_match.match.y, template_y);

    std::vector<opcv::NamedMatchResult> all_matches;
    ASSERT_TRUE(opcv::MatchAllTemplates(
        source_image,
        {L"btn_ok", L"btn_ok_copy"},
        full_region,
        0.95,
        all_matches));
    ASSERT_EQ(all_matches.size(), 2u);

    EXPECT_TRUE(opcv::RemoveTemplate(L"btn_ok_copy"));
    EXPECT_FALSE(opcv::HasTemplate(L"btn_ok_copy"));

    opcv::RemoveAllTemplates();
    EXPECT_EQ(opcv::GetTemplateCount(), 0);

    ::DeleteFileW(template_path.c_str());
}

TEST(OpenCvTest, MatchTemplateReturnsFirstThresholdHit) {
    opcv::RemoveAllTemplates();

    const int source_width = 8;
    const int source_height = 3;
    std::vector<uchar> source_bgra(static_cast<size_t>(source_width * source_height * 4), 0);

    auto set_pixel = [&](int x, int y, uchar b, uchar g, uchar r) {
        const size_t index = static_cast<size_t>((y * source_width + x) * 4);
        source_bgra[index + 0] = b;
        source_bgra[index + 1] = g;
        source_bgra[index + 2] = r;
        source_bgra[index + 3] = 255;
    };

    for (int y = 0; y < source_height; ++y) {
        for (int x = 0; x < source_width; ++x) {
            set_pixel(x, y, 10, 20, 30);
        }
    }

    const int template_width = 2;
    const int template_height = 2;
    std::vector<uchar> template_bgra(static_cast<size_t>(template_width * template_height * 4), 0);
    auto set_template_pixel = [&](int x, int y, uchar b, uchar g, uchar r) {
        const size_t index = static_cast<size_t>((y * template_width + x) * 4);
        template_bgra[index + 0] = b;
        template_bgra[index + 1] = g;
        template_bgra[index + 2] = r;
        template_bgra[index + 3] = 255;
    };

    set_template_pixel(0, 0, 40, 80, 120);
    set_template_pixel(1, 0, 60, 100, 140);
    set_template_pixel(0, 1, 90, 130, 170);
    set_template_pixel(1, 1, 110, 150, 190);

    const int first_x = 1;
    const int first_y = 1;
    const int best_x = 5;
    const int best_y = 1;

    for (int y = 0; y < template_height; ++y) {
        for (int x = 0; x < template_width; ++x) {
            const size_t templ_index = static_cast<size_t>((y * template_width + x) * 4);
            set_pixel(
                first_x + x,
                first_y + y,
                template_bgra[templ_index + 0],
                template_bgra[templ_index + 1],
                template_bgra[templ_index + 2]);

            set_pixel(
                best_x + x,
                best_y + y,
                template_bgra[templ_index + 0],
                template_bgra[templ_index + 1],
                template_bgra[templ_index + 2]);
        }
    }

    set_pixel(first_x + 1, first_y + 1, 103, 143, 183);

    const auto template_bmp = test_support::BuildBmp32TopDown(template_width, template_height, template_bgra);
    const std::wstring template_path = test_support::GetTempBmpPath(L"opencv_first_hit_template.bmp");
    ASSERT_TRUE(WriteBytesToFile(template_path, template_bmp));
    ASSERT_TRUE(opcv::LoadTemplate(L"first_hit", template_path));

    opcv::ImageHandle source_image;
    source_image.bytes = source_bgra;
    source_image.width = source_width;
    source_image.height = source_height;
    source_image.channels = 4;

    opcv::MatchResult match;
    ASSERT_TRUE(opcv::MatchTemplate(
        source_image,
        L"first_hit",
        {0, 0, source_width, source_height},
        0.95,
        match,
        opcv::SearchDirection::LeftToRight,
        opcv::StripMode::None,
        1,
        opcv::MatchColorMode::Color));
    EXPECT_EQ(match.x, first_x);
    EXPECT_EQ(match.y, first_y);

    std::vector<opcv::NamedMatchResult> all_matches;
    ASSERT_TRUE(opcv::MatchAllTemplates(
        source_image,
        {L"first_hit"},
        {0, 0, source_width, source_height},
        0.95,
        all_matches,
        opcv::SearchDirection::LeftToRight,
        opcv::StripMode::None,
        1,
        opcv::MatchColorMode::Color));
    ASSERT_EQ(all_matches.size(), 2u);

    ::DeleteFileW(template_path.c_str());
    opcv::RemoveAllTemplates();
}

TEST(OpenCvTest, MatchTemplateFollowsSearchDirection) {
    opcv::RemoveAllTemplates();

    const int source_width = 8;
    const int source_height = 3;
    std::vector<uchar> source_bgra(static_cast<size_t>(source_width * source_height * 4), 0);

    auto set_pixel = [&](int x, int y, uchar b, uchar g, uchar r) {
        const size_t index = static_cast<size_t>((y * source_width + x) * 4);
        source_bgra[index + 0] = b;
        source_bgra[index + 1] = g;
        source_bgra[index + 2] = r;
        source_bgra[index + 3] = 255;
    };

    for (int y = 0; y < source_height; ++y) {
        for (int x = 0; x < source_width; ++x) {
            set_pixel(x, y, 15, 25, 35);
        }
    }

    const int template_width = 2;
    const int template_height = 2;
    std::vector<uchar> template_bgra(static_cast<size_t>(template_width * template_height * 4), 0);
    auto set_template_pixel = [&](int x, int y, uchar b, uchar g, uchar r) {
        const size_t index = static_cast<size_t>((y * template_width + x) * 4);
        template_bgra[index + 0] = b;
        template_bgra[index + 1] = g;
        template_bgra[index + 2] = r;
        template_bgra[index + 3] = 255;
    };

    set_template_pixel(0, 0, 40, 80, 120);
    set_template_pixel(1, 0, 60, 100, 140);
    set_template_pixel(0, 1, 90, 130, 170);
    set_template_pixel(1, 1, 110, 150, 190);

    const int left_x = 1;
    const int right_x = 5;
    const int top_y = 0;
    const int bottom_y = 1;

    for (int y = 0; y < template_height; ++y) {
        for (int x = 0; x < template_width; ++x) {
            const size_t templ_index = static_cast<size_t>((y * template_width + x) * 4);
            set_pixel(left_x + x, bottom_y + y, template_bgra[templ_index + 0], template_bgra[templ_index + 1],
                      template_bgra[templ_index + 2]);
            set_pixel(right_x + x, top_y + y, template_bgra[templ_index + 0], template_bgra[templ_index + 1],
                      template_bgra[templ_index + 2]);
        }
    }

    const auto template_bmp = test_support::BuildBmp32TopDown(template_width, template_height, template_bgra);
    const std::wstring template_path = test_support::GetTempBmpPath(L"opencv_dir_template.bmp");
    ASSERT_TRUE(WriteBytesToFile(template_path, template_bmp));
    ASSERT_TRUE(opcv::LoadTemplate(L"dir_hit", template_path));

    opcv::ImageHandle source_image;
    source_image.bytes = source_bgra;
    source_image.width = source_width;
    source_image.height = source_height;
    source_image.channels = 4;

    opcv::MatchResult match;
    ASSERT_TRUE(opcv::MatchTemplate(source_image, L"dir_hit", {0, 0, source_width, source_height}, 0.95, match,
                                    opcv::SearchDirection::LeftToRight, opcv::StripMode::None, 1,
                                    opcv::MatchColorMode::Color));
    EXPECT_EQ(match.x, left_x);
    EXPECT_EQ(match.y, bottom_y);

    ASSERT_TRUE(opcv::MatchTemplate(source_image, L"dir_hit", {0, 0, source_width, source_height}, 0.95, match,
                                    opcv::SearchDirection::RightToLeft, opcv::StripMode::None, 1,
                                    opcv::MatchColorMode::Color));
    EXPECT_EQ(match.x, right_x);
    EXPECT_EQ(match.y, top_y);

    ASSERT_TRUE(opcv::MatchTemplate(source_image, L"dir_hit", {0, 0, source_width, source_height}, 0.95, match,
                                    opcv::SearchDirection::TopToBottom, opcv::StripMode::None, 1,
                                    opcv::MatchColorMode::Color));
    EXPECT_EQ(match.x, right_x);
    EXPECT_EQ(match.y, top_y);

    ASSERT_TRUE(opcv::MatchTemplate(source_image, L"dir_hit", {0, 0, source_width, source_height}, 0.95, match,
                                    opcv::SearchDirection::BottomToTop, opcv::StripMode::None, 1,
                                    opcv::MatchColorMode::Color));
    EXPECT_EQ(match.x, left_x);
    EXPECT_EQ(match.y, bottom_y);

    ::DeleteFileW(template_path.c_str());
    opcv::RemoveAllTemplates();
}

TEST(OpenCvTest, MatchTemplateHorizontalStripFindsAnyHit) {
    opcv::RemoveAllTemplates();

    const int source_width = 8;
    const int source_height = 6;
    std::vector<uchar> source_bgra(static_cast<size_t>(source_width * source_height * 4), 0);

    auto set_pixel = [&](int x, int y, uchar b, uchar g, uchar r) {
        const size_t index = static_cast<size_t>((y * source_width + x) * 4);
        source_bgra[index + 0] = b;
        source_bgra[index + 1] = g;
        source_bgra[index + 2] = r;
        source_bgra[index + 3] = 255;
    };

    for (int y = 0; y < source_height; ++y) {
        for (int x = 0; x < source_width; ++x) {
            set_pixel(x, y, 20, 30, 40);
        }
    }

    const int template_width = 2;
    const int template_height = 2;
    std::vector<uchar> template_bgra(static_cast<size_t>(template_width * template_height * 4), 0);
    auto set_template_pixel = [&](int x, int y, uchar b, uchar g, uchar r) {
        const size_t index = static_cast<size_t>((y * template_width + x) * 4);
        template_bgra[index + 0] = b;
        template_bgra[index + 1] = g;
        template_bgra[index + 2] = r;
        template_bgra[index + 3] = 255;
    };

    set_template_pixel(0, 0, 40, 80, 120);
    set_template_pixel(1, 0, 60, 100, 140);
    set_template_pixel(0, 1, 90, 130, 170);
    set_template_pixel(1, 1, 110, 150, 190);

    const int in_strip_x = 4;
    const int in_strip_y = 2;
    const int out_strip_x = 1;
    const int out_strip_y = 4;

    for (int y = 0; y < template_height; ++y) {
        for (int x = 0; x < template_width; ++x) {
            const size_t templ_index = static_cast<size_t>((y * template_width + x) * 4);
            set_pixel(in_strip_x + x, in_strip_y + y, template_bgra[templ_index + 0], template_bgra[templ_index + 1],
                      template_bgra[templ_index + 2]);
            set_pixel(out_strip_x + x, out_strip_y + y, template_bgra[templ_index + 0], template_bgra[templ_index + 1],
                      template_bgra[templ_index + 2]);
        }
    }

    const auto template_bmp = test_support::BuildBmp32TopDown(template_width, template_height, template_bgra);
    const std::wstring template_path = test_support::GetTempBmpPath(L"opencv_strip_horizontal.bmp");
    ASSERT_TRUE(WriteBytesToFile(template_path, template_bmp));
    ASSERT_TRUE(opcv::LoadTemplate(L"strip_hit", template_path));

    opcv::ImageHandle source_image;
    source_image.bytes = source_bgra;
    source_image.width = source_width;
    source_image.height = source_height;
    source_image.channels = 4;

    opcv::MatchResult match;
    ASSERT_TRUE(opcv::MatchTemplate(source_image, L"strip_hit", {0, 0, source_width, source_height}, 0.95, match,
                                    opcv::SearchDirection::LeftToRight, opcv::StripMode::Horizontal, 1,
                                    opcv::MatchColorMode::Color));
    EXPECT_TRUE((match.x == in_strip_x && match.y == in_strip_y) || (match.x == out_strip_x && match.y == out_strip_y));

    ::DeleteFileW(template_path.c_str());
    opcv::RemoveAllTemplates();
}

TEST(OpenCvTest, MatchTemplateRejectsInvalidStripDirectionCombo) {
    opcv::RemoveAllTemplates();

    const int source_width = 6;
    const int source_height = 6;
    std::vector<uchar> source_bgra(static_cast<size_t>(source_width * source_height * 4), 0);
    std::vector<uchar> template_bgra(static_cast<size_t>(2 * 2 * 4), 0);

    auto set_pixel = [&](std::vector<uchar> &buffer, int width, int x, int y, uchar b, uchar g, uchar r) {
        const size_t index = static_cast<size_t>((y * width + x) * 4);
        buffer[index + 0] = b;
        buffer[index + 1] = g;
        buffer[index + 2] = r;
        buffer[index + 3] = 255;
    };

    set_pixel(template_bgra, 2, 0, 0, 40, 80, 120);
    set_pixel(template_bgra, 2, 1, 0, 60, 100, 140);
    set_pixel(template_bgra, 2, 0, 1, 90, 130, 170);
    set_pixel(template_bgra, 2, 1, 1, 110, 150, 190);

    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            const size_t templ_index = static_cast<size_t>((y * 2 + x) * 4);
            set_pixel(source_bgra, source_width, 2 + x, 2 + y, template_bgra[templ_index + 0], template_bgra[templ_index + 1],
                      template_bgra[templ_index + 2]);
        }
    }

    const auto template_bmp = test_support::BuildBmp32TopDown(2, 2, template_bgra);
    const std::wstring template_path = test_support::GetTempBmpPath(L"opencv_strip_invalid.bmp");
    ASSERT_TRUE(WriteBytesToFile(template_path, template_bmp));
    ASSERT_TRUE(opcv::LoadTemplate(L"strip_invalid", template_path));

    opcv::ImageHandle source_image;
    source_image.bytes = source_bgra;
    source_image.width = source_width;
    source_image.height = source_height;
    source_image.channels = 4;

    opcv::MatchResult match;
    EXPECT_FALSE(opcv::MatchTemplate(source_image, L"strip_invalid", {0, 0, source_width, source_height}, 0.95, match,
                                     opcv::SearchDirection::TopToBottom, opcv::StripMode::Horizontal, 1,
                                     opcv::MatchColorMode::Color));

    ::DeleteFileW(template_path.c_str());
    opcv::RemoveAllTemplates();
}

TEST(OpenCvTest, MatchAnyTemplateFollowsSearchDirectionPerTemplate) {
    opcv::RemoveAllTemplates();

    const int source_width = 9;
    const int source_height = 3;
    std::vector<uchar> source_bgra(static_cast<size_t>(source_width * source_height * 4), 0);

    auto set_pixel = [&](int x, int y, uchar b, uchar g, uchar r) {
        const size_t index = static_cast<size_t>((y * source_width + x) * 4);
        source_bgra[index + 0] = b;
        source_bgra[index + 1] = g;
        source_bgra[index + 2] = r;
        source_bgra[index + 3] = 255;
    };

    for (int y = 0; y < source_height; ++y) {
        for (int x = 0; x < source_width; ++x) {
            set_pixel(x, y, 12, 18, 24);
        }
    }

    const int template_width = 2;
    const int template_height = 2;
    std::vector<uchar> templ_a_bgra(static_cast<size_t>(template_width * template_height * 4), 0);
    std::vector<uchar> templ_b_bgra(static_cast<size_t>(template_width * template_height * 4), 0);

    auto fill_template = [&](std::vector<uchar> &templ, const std::array<uchar, 12> &values) {
        for (size_t i = 0; i < values.size(); ++i) {
            templ[i] = values[i];
        }
        templ[3] = 255;
        templ[7] = 255;
        templ[11] = 255;
        templ[15] = 255;
    };

    fill_template(templ_a_bgra, {30, 60, 90, 0, 50, 80, 110, 0, 70, 100, 130, 0});
    fill_template(templ_b_bgra, {100, 40, 20, 0, 120, 60, 30, 0, 140, 80, 40, 0});

    const int templ_a_x = 1;
    const int templ_a_y = 1;
    const int templ_b_x = 5;
    const int templ_b_y = 1;

    for (int y = 0; y < template_height; ++y) {
        for (int x = 0; x < template_width; ++x) {
            const size_t index = static_cast<size_t>((y * template_width + x) * 4);
            set_pixel(
                templ_a_x + x,
                templ_a_y + y,
                templ_a_bgra[index + 0],
                templ_a_bgra[index + 1],
                templ_a_bgra[index + 2]);
            set_pixel(
                templ_b_x + x,
                templ_b_y + y,
                templ_b_bgra[index + 0],
                templ_b_bgra[index + 1],
                templ_b_bgra[index + 2]);
        }
    }

    set_pixel(templ_a_x + 1, templ_a_y + 1, 67, 97, 127);

    const std::wstring templ_a_path = test_support::GetTempBmpPath(L"opencv_any_first_a.bmp");
    const std::wstring templ_b_path = test_support::GetTempBmpPath(L"opencv_any_first_b.bmp");
    ASSERT_TRUE(WriteBytesToFile(templ_a_path, test_support::BuildBmp32TopDown(template_width, template_height, templ_a_bgra)));
    ASSERT_TRUE(WriteBytesToFile(templ_b_path, test_support::BuildBmp32TopDown(template_width, template_height, templ_b_bgra)));
    ASSERT_TRUE(opcv::LoadTemplate(L"templ_a", templ_a_path));
    ASSERT_TRUE(opcv::LoadTemplate(L"templ_b", templ_b_path));

    opcv::ImageHandle source_image;
    source_image.bytes = source_bgra;
    source_image.width = source_width;
    source_image.height = source_height;
    source_image.channels = 4;

    opcv::NamedMatchResult match;
    ASSERT_TRUE(opcv::MatchAnyTemplate(
        source_image,
        {L"templ_a", L"templ_b"},
        {0, 0, source_width, source_height},
        0.95,
        match,
        opcv::SearchDirection::LeftToRight,
        opcv::StripMode::None,
        1,
        opcv::MatchColorMode::Color));
    EXPECT_TRUE(
        (match.name == L"templ_a" && match.match.x == templ_a_x && match.match.y == templ_a_y) ||
        (match.name == L"templ_b" && match.match.x == templ_b_x && match.match.y == templ_b_y));

    ASSERT_TRUE(opcv::MatchAnyTemplate(
        source_image,
        {L"templ_a", L"templ_b"},
        {0, 0, source_width, source_height},
        0.95,
        match,
        opcv::SearchDirection::RightToLeft,
        opcv::StripMode::None,
        1,
        opcv::MatchColorMode::Color));
    EXPECT_EQ(match.name, L"templ_b");
    EXPECT_EQ(match.match.x, templ_b_x);
    EXPECT_EQ(match.match.y, templ_b_y);

    ::DeleteFileW(templ_a_path.c_str());
    ::DeleteFileW(templ_b_path.c_str());
    opcv::RemoveAllTemplates();
}

TEST(OpenCvTest, MatchAnyTemplateReturnsAnyFirstCompletedHit) {
    opcv::RemoveAllTemplates();

    const int source_width = 10;
    const int source_height = 3;
    std::vector<uchar> source_bgra(static_cast<size_t>(source_width * source_height * 4), 0);

    auto set_pixel = [&](int x, int y, uchar b, uchar g, uchar r) {
        const size_t index = static_cast<size_t>((y * source_width + x) * 4);
        source_bgra[index + 0] = b;
        source_bgra[index + 1] = g;
        source_bgra[index + 2] = r;
        source_bgra[index + 3] = 255;
    };

    for (int y = 0; y < source_height; ++y) {
        for (int x = 0; x < source_width; ++x) {
            set_pixel(x, y, 12, 18, 24);
        }
    }

    const int template_width = 2;
    const int template_height = 2;
    std::vector<uchar> templ_a_bgra(static_cast<size_t>(template_width * template_height * 4), 0);
    std::vector<uchar> templ_b_bgra(static_cast<size_t>(template_width * template_height * 4), 0);

    auto fill_template = [&](std::vector<uchar> &templ, const std::array<uchar, 12> &values) {
        for (size_t i = 0; i < values.size(); ++i) {
            templ[i] = values[i];
        }
        templ[3] = 255;
        templ[7] = 255;
        templ[11] = 255;
        templ[15] = 255;
    };

    fill_template(templ_a_bgra, {30, 60, 90, 0, 50, 80, 110, 0, 70, 100, 130, 0});
    fill_template(templ_b_bgra, {100, 40, 20, 0, 120, 60, 30, 0, 140, 80, 40, 0});

    const int templ_a_x = 1;
    const int templ_a_y = 1;
    const int templ_b_x = 7;
    const int templ_b_y = 0;

    for (int y = 0; y < template_height; ++y) {
        for (int x = 0; x < template_width; ++x) {
            const size_t index = static_cast<size_t>((y * template_width + x) * 4);
            set_pixel(templ_a_x + x, templ_a_y + y, templ_a_bgra[index + 0], templ_a_bgra[index + 1],
                      templ_a_bgra[index + 2]);
            set_pixel(templ_b_x + x, templ_b_y + y, templ_b_bgra[index + 0], templ_b_bgra[index + 1],
                      templ_b_bgra[index + 2]);
        }
    }

    const std::wstring templ_a_path = test_support::GetTempBmpPath(L"opencv_any_dir_a.bmp");
    const std::wstring templ_b_path = test_support::GetTempBmpPath(L"opencv_any_dir_b.bmp");
    ASSERT_TRUE(WriteBytesToFile(templ_a_path, test_support::BuildBmp32TopDown(template_width, template_height, templ_a_bgra)));
    ASSERT_TRUE(WriteBytesToFile(templ_b_path, test_support::BuildBmp32TopDown(template_width, template_height, templ_b_bgra)));
    ASSERT_TRUE(opcv::LoadTemplate(L"templ_a_dir", templ_a_path));
    ASSERT_TRUE(opcv::LoadTemplate(L"templ_b_dir", templ_b_path));

    opcv::ImageHandle source_image;
    source_image.bytes = source_bgra;
    source_image.width = source_width;
    source_image.height = source_height;
    source_image.channels = 4;

    opcv::NamedMatchResult match;
    ASSERT_TRUE(opcv::MatchAnyTemplate(source_image, {L"templ_a_dir", L"templ_b_dir"}, {0, 0, source_width, source_height},
                                       0.95, match, opcv::SearchDirection::RightToLeft, opcv::StripMode::None, 1,
                                       opcv::MatchColorMode::Color));
    if (match.name == L"templ_a_dir") {
        EXPECT_EQ(match.match.x, templ_a_x);
        EXPECT_EQ(match.match.y, templ_a_y);
    } else if (match.name == L"templ_b_dir") {
        EXPECT_EQ(match.match.x, templ_b_x);
        EXPECT_EQ(match.match.y, templ_b_y);
    } else {
        ADD_FAILURE() << "unexpected template name";
    }

    ::DeleteFileW(templ_a_path.c_str());
    ::DeleteFileW(templ_b_path.c_str());
    opcv::RemoveAllTemplates();
}

TEST(OpenCvTest, MatchTemplateScaleFindsScaledTemplate) {
    opcv::RemoveAllTemplates();

    const int base_width = 4;
    const int base_height = 4;
    std::vector<uchar> templ_bgra(static_cast<size_t>(base_width * base_height * 4), 0);
    for (int y = 0; y < base_height; ++y) {
        for (int x = 0; x < base_width; ++x) {
            const size_t index = static_cast<size_t>((y * base_width + x) * 4);
            const uchar value = static_cast<uchar>((x * 37 + y * 53 + x * y * 19 + 41) % 255);
            templ_bgra[index + 0] = value;
            templ_bgra[index + 1] = static_cast<uchar>((value + 67) % 255);
            templ_bgra[index + 2] = static_cast<uchar>((value + 131) % 255);
            templ_bgra[index + 3] = 255;
        }
    }

    const auto template_bmp = test_support::BuildBmp32TopDown(base_width, base_height, templ_bgra);
    const std::wstring template_path = test_support::GetTempBmpPath(L"opencv_template_scale.bmp");
    ASSERT_TRUE(WriteBytesToFile(template_path, template_bmp));
    ASSERT_TRUE(opcv::LoadTemplate(L"scale_target", template_path));

    cv::Mat templ_mat(base_height, base_width, CV_8UC4, templ_bgra.data());
    cv::Mat scaled_mat;
    cv::resize(templ_mat, scaled_mat, cv::Size(6, 6), 0.0, 0.0, cv::INTER_LINEAR);

    const int source_width = 18;
    const int source_height = 18;
    std::vector<uchar> source_bgra(static_cast<size_t>(source_width * source_height * 4), 0);
    for (int y = 0; y < source_height; ++y) {
        for (int x = 0; x < source_width; ++x) {
            const size_t index = static_cast<size_t>((y * source_width + x) * 4);
            source_bgra[index + 0] = static_cast<uchar>((x * 11 + y * 7 + 17) % 255);
            source_bgra[index + 1] = static_cast<uchar>((x * 5 + y * 13 + 29) % 255);
            source_bgra[index + 2] = static_cast<uchar>((x * 3 + y * 19 + 43) % 255);
            source_bgra[index + 3] = 255;
        }
    }

    const int target_x = 7;
    const int target_y = 5;
    for (int y = 0; y < scaled_mat.rows; ++y) {
        for (int x = 0; x < scaled_mat.cols; ++x) {
            const cv::Vec4b pixel = scaled_mat.at<cv::Vec4b>(y, x);
            const size_t index = static_cast<size_t>(((target_y + y) * source_width + (target_x + x)) * 4);
            source_bgra[index + 0] = pixel[0];
            source_bgra[index + 1] = pixel[1];
            source_bgra[index + 2] = pixel[2];
            source_bgra[index + 3] = pixel[3];
        }
    }

    opcv::ImageHandle source_image;
    source_image.bytes = source_bgra;
    source_image.width = source_width;
    source_image.height = source_height;
    source_image.channels = 4;

    opcv::MatchResult result;
    ASSERT_TRUE(opcv::MatchTemplateScale(
        source_image,
        L"scale_target",
        {0, 0, source_width, source_height},
        {1.0, 1.5},
        0.95,
        result));
    EXPECT_EQ(result.x, target_x);
    EXPECT_EQ(result.y, target_y);
    EXPECT_EQ(result.width, 6);
    EXPECT_EQ(result.height, 6);

    opcv::RemoveAllTemplates();
    ::DeleteFileW(template_path.c_str());
}

TEST(OpenCvTest, MatchTemplateAutomaticallyUsesPyramidOnLargeRegion) {
    opcv::RemoveAllTemplates();

    std::vector<uchar> source_bgra;
    int source_width = 0;
    int source_height = 0;
    int template_width = 0;
    int template_height = 0;
    const cv::Point hit_point(216, 142);
    std::wstring template_path;
    BuildLargePyramidTemplateData(
        source_bgra,
        source_width,
        source_height,
        template_width,
        template_height,
        {hit_point},
        template_path,
        L"opencv_pyramid_single.bmp");

    ASSERT_TRUE(opcv::LoadTemplate(L"pyramid_single", template_path));

    opcv::ImageHandle source_image;
    source_image.bytes = source_bgra;
    source_image.width = source_width;
    source_image.height = source_height;
    source_image.channels = 4;

    opcv::MatchResult match;
    ASSERT_TRUE(opcv::MatchTemplate(
        source_image,
        L"pyramid_single",
        {0, 0, source_width, source_height},
        0.999,
        match,
        opcv::SearchDirection::LeftToRight,
        opcv::StripMode::None,
        1,
        opcv::MatchColorMode::Color));
    EXPECT_EQ(match.x, hit_point.x);
    EXPECT_EQ(match.y, hit_point.y);
    EXPECT_EQ(match.width, template_width);
    EXPECT_EQ(match.height, template_height);

    opcv::RemoveAllTemplates();
    ::DeleteFileW(template_path.c_str());
}

TEST(OpenCvTest, MatchAnyTemplateAutomaticallyUsesPyramidOnLargeRegion) {
    opcv::RemoveAllTemplates();

    std::vector<uchar> source_bgra;
    int source_width = 0;
    int source_height = 0;
    int template_width = 0;
    int template_height = 0;
    const cv::Point hit_point(352, 248);
    std::wstring template_path;
    BuildLargePyramidTemplateData(
        source_bgra,
        source_width,
        source_height,
        template_width,
        template_height,
        {hit_point},
        template_path,
        L"opencv_pyramid_any.bmp");

    ASSERT_TRUE(opcv::LoadTemplate(L"pyramid_any", template_path));

    opcv::ImageHandle source_image;
    source_image.bytes = source_bgra;
    source_image.width = source_width;
    source_image.height = source_height;
    source_image.channels = 4;

    opcv::NamedMatchResult match;
    ASSERT_TRUE(opcv::MatchAnyTemplate(
        source_image,
        {L"missing_template", L"pyramid_any"},
        {0, 0, source_width, source_height},
        0.999,
        match,
        opcv::SearchDirection::LeftToRight,
        opcv::StripMode::None,
        1,
        opcv::MatchColorMode::Color));
    EXPECT_EQ(match.name, L"pyramid_any");
    EXPECT_EQ(match.match.x, hit_point.x);
    EXPECT_EQ(match.match.y, hit_point.y);
    EXPECT_EQ(match.match.width, template_width);
    EXPECT_EQ(match.match.height, template_height);

    opcv::RemoveAllTemplates();
    ::DeleteFileW(template_path.c_str());
}

TEST(OpenCvTest, MatchAllTemplatesAutomaticallyUsesPyramidOnLargeRegion) {
    opcv::RemoveAllTemplates();

    std::vector<uchar> source_bgra;
    int source_width = 0;
    int source_height = 0;
    int template_width = 0;
    int template_height = 0;
    const cv::Point first_hit(96, 88);
    const cv::Point second_hit(420, 302);
    std::wstring template_path;
    BuildLargePyramidTemplateData(
        source_bgra,
        source_width,
        source_height,
        template_width,
        template_height,
        {first_hit, second_hit},
        template_path,
        L"opencv_pyramid_all.bmp");

    ASSERT_TRUE(opcv::LoadTemplate(L"pyramid_all", template_path));

    opcv::ImageHandle source_image;
    source_image.bytes = source_bgra;
    source_image.width = source_width;
    source_image.height = source_height;
    source_image.channels = 4;

    std::vector<opcv::NamedMatchResult> results;
    ASSERT_TRUE(opcv::MatchAllTemplates(
        source_image,
        {L"pyramid_all"},
        {0, 0, source_width, source_height},
        0.999,
        results,
        opcv::SearchDirection::LeftToRight,
        opcv::StripMode::None,
        1,
        opcv::MatchColorMode::Color));
    ASSERT_EQ(results.size(), 2u);

    std::sort(
        results.begin(),
        results.end(),
        [](const opcv::NamedMatchResult &lhs, const opcv::NamedMatchResult &rhs) {
            if (lhs.match.y != rhs.match.y) {
                return lhs.match.y < rhs.match.y;
            }
            return lhs.match.x < rhs.match.x;
        });

    EXPECT_EQ(results[0].name, L"pyramid_all");
    EXPECT_EQ(results[0].match.x, first_hit.x);
    EXPECT_EQ(results[0].match.y, first_hit.y);
    EXPECT_EQ(results[0].match.width, template_width);
    EXPECT_EQ(results[0].match.height, template_height);
    EXPECT_EQ(results[1].name, L"pyramid_all");
    EXPECT_EQ(results[1].match.x, second_hit.x);
    EXPECT_EQ(results[1].match.y, second_hit.y);
    EXPECT_EQ(results[1].match.width, template_width);
    EXPECT_EQ(results[1].match.height, template_height);

    opcv::RemoveAllTemplates();
    ::DeleteFileW(template_path.c_str());
}

TEST(OpenCvTest, MatchTemplateOnRealPhotoSample) {
    opcv::RemoveAllTemplates();

    std::vector<RealImageCase> cases = RealPhotoTemplateCases();

    cases.erase(
        std::remove_if(
            cases.begin(),
            cases.end(),
            [](const RealImageCase &item) { return !HasDemoAssetPair(item); }),
        cases.end());

    std::wstring generated_template_path;
    opcv::ImageHandle generated_source_image;
    if (cases.empty()) {
        const std::wstring sample_path = FindOpenCvSampleImage(L"graf1.png");
        if (sample_path.empty()) {
            GTEST_SKIP() << "No real photo sample asset is available.";
        }

        cv::Mat sample = cv::imread(std::filesystem::path(sample_path).string(), cv::IMREAD_COLOR);
        ASSERT_FALSE(sample.empty());

        cv::Mat source;
        cv::resize(sample, source, cv::Size(1280, 1024), 0.0, 0.0, cv::INTER_LINEAR);

        generated_template_path = test_support::GetTempBmpPath(L"opencv_real_photo_template.png");
        ASSERT_TRUE(WritePngFile(generated_template_path, source(cv::Rect(480, 360, 96, 96)).clone()));
        generated_source_image = ImageFromMat(source);
        cases.push_back({L"opencv_sample", L"", generated_template_path, cv::Rect(480, 360, 96, 96)});
    }

    for (const auto &item : cases) {
        opcv::RemoveAllTemplates();

        opcv::ImageHandle source_image;
        std::wstring template_path;
        if (generated_source_image.width > 0 && item.source_file.empty()) {
            source_image = generated_source_image;
            template_path = generated_template_path;
        } else {
            ASSERT_TRUE(opcv::LoadImageFromFile(FindDemoAsset(item.source_file), source_image))
                << std::filesystem::path(item.source_file).string();
            template_path = FindDemoAsset(item.template_file);
        }

        const std::wstring template_name = std::wstring(L"real_photo_") + item.label;
        ASSERT_TRUE(opcv::LoadTemplate(template_name, template_path))
            << std::filesystem::path(item.template_file).string();

        opcv::MatchResult match;
        bool ok = false;
        const long long elapsed_ms = MeasureMilliseconds(
            [&]() {
                return opcv::MatchTemplate(
                    source_image,
                    template_name,
                    {0, 0, source_image.width, source_image.height},
                    0.98,
                    match,
                    opcv::SearchDirection::LeftToRight,
                    opcv::StripMode::None,
                    1,
                    opcv::MatchColorMode::Color);
            },
            ok);

        const std::string property_prefix =
            "real_photo_" + std::filesystem::path(item.label).string();
        RecordProperty(property_prefix + "_source_width", source_image.width);
        RecordProperty(property_prefix + "_source_height", source_image.height);
        RecordProperty(property_prefix + "_template_width", item.template_rect.width);
        RecordProperty(property_prefix + "_template_height", item.template_rect.height);
        RecordProperty(property_prefix + "_match_elapsed_ms", elapsed_ms);

        ASSERT_TRUE(ok) << std::filesystem::path(item.label).string();
        EXPECT_EQ(match.x, item.template_rect.x) << std::filesystem::path(item.label).string();
        EXPECT_EQ(match.y, item.template_rect.y) << std::filesystem::path(item.label).string();
        EXPECT_EQ(match.width, item.template_rect.width) << std::filesystem::path(item.label).string();
        EXPECT_EQ(match.height, item.template_rect.height) << std::filesystem::path(item.label).string();
    }

    opcv::RemoveAllTemplates();
    if (!generated_template_path.empty()) {
        ::DeleteFileW(generated_template_path.c_str());
    }
}

TEST(OpenCvTest, AllMatchingMethodsWorkOnRealImageAssets) {
    opcv::RemoveAllTemplates();

    auto cases = RealPhotoTemplateCases();
    cases.erase(
        std::remove_if(
            cases.begin(),
            cases.end(),
            [](const RealImageCase &item) { return !HasDemoAssetPair(item); }),
        cases.end());
    if (cases.size() < 3) {
        GTEST_SKIP() << "Real image template assets are not available.";
    }

    const auto load_source = [](const RealImageCase &item, opcv::ImageHandle &source) {
        return opcv::LoadImageFromFile(FindDemoAsset(item.source_file), source);
    };
    const auto load_template = [](const std::wstring &name, const RealImageCase &item) {
        return opcv::LoadTemplate(name, FindDemoAsset(item.template_file));
    };

    opcv::ImageHandle direct_source;
    ASSERT_TRUE(load_source(cases[0], direct_source));
    opcv::ImageHandle direct_template;
    ASSERT_TRUE(opcv::LoadImageFromFile(FindDemoAsset(cases[0].template_file), direct_template));
    std::vector<opcv::MatchCandidate> candidates;
    bool direct_ok = false;
    const long long direct_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::MatchTemplate(
                direct_source,
                direct_template,
                {0, 0, direct_source.width, direct_source.height},
                candidates,
                1,
                opcv::MatchColorMode::Color);
        },
        direct_ok);
    ASSERT_TRUE(direct_ok);
    opcv::MatchResult direct_best;
    ASSERT_TRUE(opcv::GetBestMatch(candidates, direct_template.width, direct_template.height, direct_best));
    ASSERT_TRUE(opcv::IsMatchAboveThreshold(direct_best, 0.98));
    EXPECT_EQ(direct_best.x, cases[0].template_rect.x);
    EXPECT_EQ(direct_best.y, cases[0].template_rect.y);
    RecordMeasuredMatch("real_direct", direct_elapsed_ms, direct_best.score);

    ASSERT_TRUE(load_template(L"real_single", cases[1]));
    opcv::ImageHandle single_source;
    ASSERT_TRUE(load_source(cases[1], single_source));
    opcv::MatchResult single_match;
    bool single_ok = false;
    const long long single_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::MatchTemplate(
                single_source,
                L"real_single",
                {0, 0, single_source.width, single_source.height},
                0.97,
                single_match,
                opcv::SearchDirection::LeftToRight,
                opcv::StripMode::None,
                1,
                opcv::MatchColorMode::Color);
        },
        single_ok);
    ASSERT_TRUE(single_ok);
    ExpectNearRect(single_match, cases[1].template_rect, 1);
    RecordMeasuredMatch("real_single", single_elapsed_ms, single_match.score);

    ASSERT_TRUE(load_template(L"real_any", cases[2]));
    opcv::ImageHandle any_source;
    ASSERT_TRUE(load_source(cases[2], any_source));
    opcv::NamedMatchResult any_match;
    bool any_ok = false;
    const long long any_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::MatchAnyTemplate(
                any_source,
                {L"missing_real_template", L"real_any"},
                {0, 0, any_source.width, any_source.height},
                0.98,
                any_match,
                opcv::SearchDirection::LeftToRight,
                opcv::StripMode::None,
                1,
                opcv::MatchColorMode::Color);
        },
        any_ok);
    ASSERT_TRUE(any_ok);
    EXPECT_EQ(any_match.name, L"real_any");
    ExpectNearRect(any_match.match, cases[2].template_rect, 1);
    RecordMeasuredMatch("real_any", any_elapsed_ms, any_match.match.score);

    ASSERT_TRUE(load_template(L"real_all", cases[3]));
    opcv::ImageHandle all_source;
    ASSERT_TRUE(load_source(cases[3], all_source));
    std::vector<opcv::NamedMatchResult> all_matches;
    bool all_ok = false;
    const long long all_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::MatchAllTemplates(
                all_source,
                {L"real_all"},
                {0, 0, all_source.width, all_source.height},
                0.98,
                all_matches,
                opcv::SearchDirection::LeftToRight,
                opcv::StripMode::None,
                1,
                opcv::MatchColorMode::Color);
        },
        all_ok);
    ASSERT_TRUE(all_ok);
    ASSERT_FALSE(all_matches.empty());
    const auto all_it = std::find_if(
        all_matches.begin(),
        all_matches.end(),
        [&](const opcv::NamedMatchResult &item) {
            return item.name == L"real_all" && std::abs(item.match.x - cases[3].template_rect.x) <= 1 &&
                   std::abs(item.match.y - cases[3].template_rect.y) <= 1;
        });
    ASSERT_NE(all_it, all_matches.end());
    ExpectNearRect(all_it->match, cases[3].template_rect, 1);
    RecordMeasuredMatch("real_all", all_elapsed_ms, all_it->match.score);
    RecordProperty("real_all_matches", static_cast<int>(all_matches.size()));

    const std::wstring scale_source_path = FindDemoAsset(L"opencv_real_scale_source.png");
    const std::wstring scale_template_path = FindDemoAsset(L"opencv_real_scale_template.png");
    if (scale_source_path.empty() || scale_template_path.empty()) {
        GTEST_SKIP() << "Real scale matching assets are not available.";
    }
    opcv::ImageHandle scale_source;
    ASSERT_TRUE(opcv::LoadImageFromFile(scale_source_path, scale_source));
    ASSERT_TRUE(opcv::LoadTemplate(L"real_scale", scale_template_path));
    opcv::MatchResult scale_match;
    bool scale_ok = false;
    const long long scale_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::MatchTemplateScale(
                scale_source,
                L"real_scale",
                {0, 0, scale_source.width, scale_source.height},
                {1.0, 1.5},
                0.98,
                scale_match,
                1,
                opcv::MatchColorMode::Color);
        },
        scale_ok);
    ASSERT_TRUE(scale_ok);
    ExpectNearRect(scale_match, cv::Rect(80, 420, 144, 144), 1);
    RecordMeasuredMatch("real_scale", scale_elapsed_ms, scale_match.score);

    const std::wstring stop_source_path = FindDemoAsset(L"opencv_real_stop_source.jpg");
    const std::wstring stop_template_path = FindDemoAsset(L"opencv_real_stop_template.png");
    if (stop_source_path.empty() || stop_template_path.empty()) {
        GTEST_SKIP() << "Real stop sign matching assets are not available.";
    }
    opcv::ImageHandle stop_source;
    ASSERT_TRUE(opcv::LoadImageFromFile(stop_source_path, stop_source));
    ASSERT_TRUE(opcv::LoadTemplate(L"real_stop", stop_template_path));

    opcv::MatchResult feature_match;
    bool feature_ok = false;
    const long long feature_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::FeatureMatchTemplate(
                stop_source,
                L"real_stop",
                {0, 0, stop_source.width, stop_source.height},
                0.08,
                feature_match);
        },
        feature_ok);
    ASSERT_TRUE(feature_ok);
    ExpectNearRect(feature_match, cv::Rect(412, 222, 445, 449), 3);
    RecordMeasuredMatch("real_feature", feature_elapsed_ms, feature_match.score);

    opcv::MatchResult edge_match;
    bool edge_ok = false;
    const long long edge_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::EdgeMatchTemplate(
                stop_source,
                L"real_stop",
                {0, 0, stop_source.width, stop_source.height},
                0.90,
                edge_match);
        },
        edge_ok);
    ASSERT_TRUE(edge_ok);
    ExpectNearRect(edge_match, cv::Rect(412, 222, 445, 449), 2);
    RecordMeasuredMatch("real_edge", edge_elapsed_ms, edge_match.score);

    const std::wstring shape_source_path = FindDemoAsset(L"opencv_real_shape_source.png");
    const std::wstring shape_template_path = FindDemoAsset(L"opencv_real_shape_template.png");
    if (shape_source_path.empty() || shape_template_path.empty()) {
        GTEST_SKIP() << "Real shape matching assets are not available.";
    }
    opcv::ImageHandle shape_source;
    ASSERT_TRUE(opcv::LoadImageFromFile(shape_source_path, shape_source));
    ASSERT_TRUE(opcv::LoadTemplate(L"real_shape", shape_template_path));
    opcv::MatchResult shape_match;
    bool shape_ok = false;
    const long long shape_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::ShapeMatchTemplate(
                shape_source,
                L"real_shape",
                {0, 0, shape_source.width, shape_source.height},
                0.98,
                shape_match);
        },
        shape_ok);
    ASSERT_TRUE(shape_ok);
    ExpectNearRect(shape_match, cv::Rect(520, 140, 421, 425), 1);
    RecordMeasuredMatch("real_shape", shape_elapsed_ms, shape_match.score);

    opcv::RemoveAllTemplates();
}

TEST(OpenCvTest, AllMatchingMethodsWorkOnGameSceneAssets) {
    opcv::RemoveAllTemplates();

    auto cases = GameSceneTemplateCases();
    cases.erase(
        std::remove_if(
            cases.begin(),
            cases.end(),
            [](const RealImageCase &item) { return !HasDemoAssetPair(item); }),
        cases.end());
    if (cases.size() < 7) {
        GTEST_SKIP() << "Game scene matching assets are not available.";
    }

    const auto find_case = [&](const std::wstring &label) -> const RealImageCase & {
        const auto it = std::find_if(
            cases.begin(),
            cases.end(),
            [&](const RealImageCase &item) { return item.label == label; });
        EXPECT_NE(it, cases.end());
        return *it;
    };
    const auto load_source = [](const RealImageCase &item, opcv::ImageHandle &source) {
        return opcv::LoadImageFromFile(FindDemoAsset(item.source_file), source);
    };
    const auto load_template = [](const std::wstring &name, const RealImageCase &item) {
        return opcv::LoadTemplate(name, FindDemoAsset(item.template_file));
    };

    const RealImageCase &scene_case = find_case(L"scene");
    opcv::ImageHandle scene_source;
    ASSERT_TRUE(load_source(scene_case, scene_source));
    ASSERT_TRUE(load_template(L"game_scene", scene_case));
    opcv::MatchResult scene_match;
    bool scene_ok = false;
    const long long scene_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::MatchTemplate(
                scene_source,
                L"game_scene",
                {0, 0, scene_source.width, scene_source.height},
                0.98,
                scene_match,
                opcv::SearchDirection::LeftToRight,
                opcv::StripMode::None,
                1,
                opcv::MatchColorMode::Color);
        },
        scene_ok);
    ASSERT_TRUE(scene_ok);
    ExpectNearRect(scene_match, scene_case.template_rect, 1);
    RecordProperty("game_scene_match_elapsed_ms", scene_elapsed_ms);
    RecordProperty("game_scene_score", scene_match.score);

    const RealImageCase &coin_case = find_case(L"coin");
    ASSERT_TRUE(load_template(L"game_coin", coin_case));
    opcv::NamedMatchResult any_match;
    bool coin_any_ok = false;
    const long long coin_any_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::MatchAnyTemplate(
                scene_source,
                {L"missing_game_template", L"game_coin"},
                {0, 0, scene_source.width, scene_source.height},
                0.98,
                any_match,
                opcv::SearchDirection::LeftToRight,
                opcv::StripMode::None,
                cv::TM_CCORR_NORMED,
                opcv::MatchColorMode::Color);
        },
        coin_any_ok);
    ASSERT_TRUE(coin_any_ok);
    EXPECT_EQ(any_match.name, L"game_coin");
    ExpectNearRect(any_match.match, coin_case.template_rect, 1);
    RecordMeasuredMatch("game_coin_any", coin_any_elapsed_ms, any_match.match.score);

    std::vector<opcv::NamedMatchResult> all_coin_matches;
    bool coin_all_ok = false;
    const long long coin_all_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::MatchAllTemplates(
                scene_source,
                {L"game_coin"},
                {0, 0, scene_source.width, scene_source.height},
                0.98,
                all_coin_matches,
                opcv::SearchDirection::LeftToRight,
                opcv::StripMode::None,
                cv::TM_CCORR_NORMED,
                opcv::MatchColorMode::Color);
        },
        coin_all_ok);
    ASSERT_TRUE(coin_all_ok);
    EXPECT_GE(all_coin_matches.size(), 4u);
    RecordProperty("game_coin_all_elapsed_ms", coin_all_elapsed_ms);
    RecordProperty("game_coin_all_matches", static_cast<int>(all_coin_matches.size()));

    const RealImageCase &gem_case = find_case(L"gem");
    ASSERT_TRUE(load_template(L"game_gem", gem_case));
    opcv::NamedMatchResult gem_any_match;
    bool gem_any_ok = false;
    const long long gem_any_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::MatchAnyTemplate(
                scene_source,
                {L"game_gem"},
                {0, 0, scene_source.width, scene_source.height},
                0.98,
                gem_any_match,
                opcv::SearchDirection::LeftToRight,
                opcv::StripMode::None,
                cv::TM_CCORR_NORMED,
                opcv::MatchColorMode::Color);
        },
        gem_any_ok);
    ASSERT_TRUE(gem_any_ok);
    ExpectNearRect(gem_any_match.match, gem_case.template_rect, 1);
    RecordMeasuredMatch("game_gem_any", gem_any_elapsed_ms, gem_any_match.match.score);

    const RealImageCase &scale_case = find_case(L"scale");
    opcv::ImageHandle scale_source;
    ASSERT_TRUE(load_source(scale_case, scale_source));
    opcv::MatchResult scale_match;
    bool scale_ok = false;
    const long long scale_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::MatchTemplateScale(
                scale_source,
                L"game_coin",
                {1000, 90, 190, 160},
                {1.5},
                0.96,
                scale_match,
                cv::TM_CCORR_NORMED,
                opcv::MatchColorMode::Color);
        },
        scale_ok);
    ASSERT_TRUE(scale_ok);
    ExpectNearRect(scale_match, scale_case.template_rect, 2);
    RecordMeasuredMatch("game_scale", scale_elapsed_ms, scale_match.score);

    const RealImageCase &feature_case = find_case(L"feature");
    opcv::ImageHandle feature_source;
    ASSERT_TRUE(load_source(feature_case, feature_source));
    ASSERT_TRUE(load_template(L"game_feature", feature_case));
    opcv::MatchResult feature_match;
    bool feature_ok = false;
    const long long feature_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::FeatureMatchTemplate(
                feature_source,
                L"game_feature",
                {780, 40, 280, 270},
                0.25,
                feature_match);
        },
        feature_ok);
    ASSERT_TRUE(feature_ok);
    ExpectNearRect(feature_match, feature_case.template_rect, 24);
    RecordMeasuredMatch("game_feature", feature_elapsed_ms, feature_match.score);

    const RealImageCase &edge_case = find_case(L"edge");
    ASSERT_TRUE(load_template(L"game_edge", edge_case));
    opcv::MatchResult edge_match;
    bool edge_ok = false;
    const long long edge_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::EdgeMatchTemplate(
                scene_source,
                L"game_edge",
                {980, 180, 240, 220},
                0.80,
                edge_match);
        },
        edge_ok);
    ASSERT_TRUE(edge_ok);
    ExpectNearRect(edge_match, edge_case.template_rect, 2);
    RecordMeasuredMatch("game_edge", edge_elapsed_ms, edge_match.score);

    const RealImageCase &shape_case = find_case(L"shape");
    opcv::ImageHandle shape_source;
    ASSERT_TRUE(load_source(shape_case, shape_source));
    ASSERT_TRUE(load_template(L"game_shape", shape_case));
    opcv::MatchResult shape_match;
    bool shape_ok = false;
    const long long shape_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::ShapeMatchTemplate(
                shape_source,
                L"game_shape",
                {0, 0, shape_source.width, shape_source.height},
                0.70,
                shape_match);
        },
        shape_ok);
    ASSERT_TRUE(shape_ok);
    ExpectNearRect(shape_match, shape_case.template_rect, 5);
    RecordProperty("game_shape_elapsed_ms", shape_elapsed_ms);
    RecordProperty("game_shape_score", shape_match.score);

    opcv::RemoveAllTemplates();
}

TEST(OpenCvTest, HardRealImageCasesExposeMatchingBoundaries) {
    opcv::RemoveAllTemplates();

    const auto load_image = [](const std::wstring &file_name, opcv::ImageHandle &image) {
        const std::wstring path = FindDemoAsset(file_name);
        return !path.empty() && opcv::LoadImageFromFile(path, image);
    };
    const auto load_template = [](const std::wstring &name, const std::wstring &file_name) {
        const std::wstring path = FindDemoAsset(file_name);
        return !path.empty() && opcv::LoadTemplate(name, path);
    };

    opcv::ImageHandle scale_source;
    if (!load_image(L"opencv_hard_scale_source.png", scale_source) ||
        !load_template(L"hard_scale", L"opencv_hard_scale_template.png")) {
        GTEST_SKIP() << "Hard real-image assets are not available.";
    }

    opcv::MatchResult scale_small;
    bool scale_small_ok = false;
    const long long scale_small_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::MatchTemplateScale(
                scale_source,
                L"hard_scale",
                {0, 0, scale_source.width, scale_source.height},
                {},
                0.98,
                scale_small,
                1,
                opcv::MatchColorMode::Color);
        },
        scale_small_ok);
    ASSERT_TRUE(scale_small_ok);
    ExpectNearRect(scale_small, cv::Rect(90, 450, 72, 72), 1);
    RecordProperty("hard_scale_auto_elapsed_ms", scale_small_elapsed_ms);
    RecordProperty("hard_scale_auto_score", scale_small.score);

    opcv::MatchResult scale_large;
    bool scale_large_ok = false;
    const long long scale_large_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::MatchTemplateScale(
                scale_source,
                L"hard_scale",
                {0, 0, scale_source.width, scale_source.height},
                {1.35},
                0.98,
                scale_large,
                1,
                opcv::MatchColorMode::Color);
        },
        scale_large_ok);
    ASSERT_TRUE(scale_large_ok);
    ExpectNearRect(scale_large, cv::Rect(940, 390, 130, 130), 1);
    RecordProperty("hard_scale_135_elapsed_ms", scale_large_elapsed_ms);
    RecordProperty("hard_scale_135_score", scale_large.score);

    opcv::MatchResult scale_auto_large;
    bool scale_auto_large_ok = false;
    const long long scale_auto_large_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::MatchTemplateScale(
                scale_source,
                L"hard_scale",
                {820, 300, 360, 260},
                {},
                0.98,
                scale_auto_large,
                1,
                opcv::MatchColorMode::Color);
        },
        scale_auto_large_ok);
    ASSERT_TRUE(scale_auto_large_ok);
    ExpectNearRect(scale_auto_large, cv::Rect(947, 396, 120, 120), 2);
    RecordProperty("hard_scale_auto_large_elapsed_ms", scale_auto_large_elapsed_ms);
    RecordProperty("hard_scale_auto_large_score", scale_auto_large.score);

    opcv::ImageHandle brightness_source;
    ASSERT_TRUE(load_image(L"opencv_hard_brightness_source.png", brightness_source));
    ASSERT_TRUE(load_template(L"hard_brightness", L"opencv_hard_brightness_template.png"));
    opcv::MatchResult brightness_match;
    bool brightness_ok = false;
    const long long brightness_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::MatchTemplate(
                brightness_source,
                L"hard_brightness",
                {0, 0, brightness_source.width, brightness_source.height},
                0.90,
                brightness_match,
                opcv::SearchDirection::LeftToRight,
                opcv::StripMode::None,
                1,
                opcv::MatchColorMode::Color);
        },
        brightness_ok);
    ASSERT_TRUE(brightness_ok);
    ExpectNearRect(brightness_match, cv::Rect(460, 430, 96, 96), 1);
    RecordProperty("hard_brightness_elapsed_ms", brightness_elapsed_ms);
    RecordProperty("hard_brightness_score", brightness_match.score);

    opcv::ImageHandle repeated_source;
    ASSERT_TRUE(load_image(L"opencv_hard_repeated_source.png", repeated_source));
    ASSERT_TRUE(load_template(L"hard_repeated", L"opencv_hard_repeated_template.png"));
    std::vector<opcv::NamedMatchResult> repeated_matches;
    bool repeated_ok = false;
    const long long repeated_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::MatchAllTemplates(
                repeated_source,
                {L"hard_repeated"},
                {0, 0, repeated_source.width, repeated_source.height},
                0.90,
                repeated_matches,
                opcv::SearchDirection::LeftToRight,
                opcv::StripMode::None,
                1,
                opcv::MatchColorMode::Color);
        },
        repeated_ok);
    ASSERT_TRUE(repeated_ok);
    EXPECT_GE(repeated_matches.size(), 5u);
    RecordProperty("hard_repeated_elapsed_ms", repeated_elapsed_ms);
    RecordProperty("hard_repeated_matches", static_cast<int>(repeated_matches.size()));

    opcv::ImageHandle rotation_source;
    ASSERT_TRUE(load_image(L"opencv_hard_rotation_source.png", rotation_source));
    ASSERT_TRUE(load_template(L"hard_rotation", L"opencv_hard_rotation_template.png"));
    opcv::MatchResult rotation_template_match;
    bool rotation_template_ok = false;
    const long long rotation_template_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::MatchTemplate(
                rotation_source,
                L"hard_rotation",
                {0, 0, rotation_source.width, rotation_source.height},
                0.90,
                rotation_template_match,
                opcv::SearchDirection::LeftToRight,
                opcv::StripMode::None,
                1,
                opcv::MatchColorMode::Color);
        },
        rotation_template_ok);
    EXPECT_FALSE(rotation_template_ok);
    RecordProperty("hard_rotation_template_elapsed_ms", rotation_template_elapsed_ms);

    opcv::MatchResult rotation_edge_match;
    bool rotation_edge_ok = false;
    const long long rotation_edge_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::EdgeMatchTemplate(
                rotation_source,
                L"hard_rotation",
                {0, 0, rotation_source.width, rotation_source.height},
                0.50,
                rotation_edge_match);
        },
        rotation_edge_ok);
    EXPECT_FALSE(rotation_edge_ok);
    RecordProperty("hard_rotation_edge_elapsed_ms", rotation_edge_elapsed_ms);

    opcv::MatchResult rotation_feature_match;
    bool rotation_feature_ok = false;
    const long long rotation_feature_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::FeatureMatchTemplate(
                rotation_source,
                L"hard_rotation",
                {0, 0, rotation_source.width, rotation_source.height},
                0.25,
                rotation_feature_match);
        },
        rotation_feature_ok);
    ASSERT_TRUE(rotation_feature_ok);
    EXPECT_NEAR(rotation_feature_match.x, 610, 24);
    EXPECT_NEAR(rotation_feature_match.y, 130, 24);
    RecordProperty("hard_rotation_feature_elapsed_ms", rotation_feature_elapsed_ms);
    RecordProperty("hard_rotation_feature_score", rotation_feature_match.score);

    opcv::ImageHandle occlusion_source;
    ASSERT_TRUE(load_image(L"opencv_hard_occlusion_source.png", occlusion_source));
    ASSERT_TRUE(load_template(L"hard_occlusion", L"opencv_hard_occlusion_template.png"));
    opcv::MatchResult occlusion_match;
    bool occlusion_ok = false;
    const long long occlusion_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::MatchTemplate(
                occlusion_source,
                L"hard_occlusion",
                {0, 0, occlusion_source.width, occlusion_source.height},
                0.90,
                occlusion_match,
                opcv::SearchDirection::LeftToRight,
                opcv::StripMode::None,
                1,
                opcv::MatchColorMode::Color);
        },
        occlusion_ok);
    EXPECT_FALSE(occlusion_ok);
    RecordProperty("hard_occlusion_elapsed_ms", occlusion_elapsed_ms);

    opcv::ImageHandle similar_shape_source;
    ASSERT_TRUE(load_image(L"opencv_hard_similar_shapes_source.png", similar_shape_source));
    ASSERT_TRUE(load_template(L"hard_similar_shape", L"opencv_hard_similar_shapes_template.png"));
    opcv::MatchResult similar_shape_match;
    bool similar_shape_ok = false;
    const long long similar_shape_elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::ShapeMatchTemplate(
                similar_shape_source,
                L"hard_similar_shape",
                {0, 0, similar_shape_source.width, similar_shape_source.height},
                0.55,
                similar_shape_match);
        },
        similar_shape_ok);
    ASSERT_TRUE(similar_shape_ok);
    RecordProperty("hard_similar_shape_elapsed_ms", similar_shape_elapsed_ms);
    RecordProperty("hard_similar_shape_x", similar_shape_match.x);
    RecordProperty("hard_similar_shape_y", similar_shape_match.y);
    RecordProperty("hard_similar_shape_width", similar_shape_match.width);
    RecordProperty("hard_similar_shape_height", similar_shape_match.height);
    ExpectNearRect(similar_shape_match, cv::Rect(520, 140, 413, 423), 4);

    opcv::RemoveAllTemplates();
}

TEST(OpenCvTest, MatchTemplateOnConfiguredRealImage) {
    opcv::RemoveAllTemplates();

    const auto cases = RealPhotoTemplateCases();
    ASSERT_GT(cases.size(), 1u);
    const RealImageCase &default_case = cases[1];

    std::wstring source_path = test_support::GetEnvString(L"OP_OPENCV_REAL_SOURCE");
    std::wstring template_path = test_support::GetEnvString(L"OP_OPENCV_REAL_TEMPLATE");
    cv::Rect expected_rect = default_case.template_rect;
    if (source_path.empty()) {
        source_path = FindDemoAsset(default_case.source_file);
    }
    if (template_path.empty()) {
        template_path = FindDemoAsset(default_case.template_file);
    }
    if (source_path.empty() || template_path.empty()) {
        GTEST_SKIP() << "Default real image matching assets are not available.";
    }

    opcv::ImageHandle source_image;
    ASSERT_TRUE(opcv::LoadImageFromFile(source_path, source_image));
    ASSERT_TRUE(opcv::LoadTemplate(L"configured_real_template", template_path));

    opcv::MatchResult match;
    bool ok = false;
    const long long elapsed_ms = MeasureMilliseconds(
        [&]() {
            return opcv::MatchTemplate(
                source_image,
                L"configured_real_template",
                {0, 0, source_image.width, source_image.height},
                0.95,
                match,
                opcv::SearchDirection::LeftToRight,
                opcv::StripMode::None,
                1,
                opcv::MatchColorMode::Color);
        },
        ok);

    RecordProperty("configured_real_source_width", source_image.width);
    RecordProperty("configured_real_source_height", source_image.height);
    RecordProperty("configured_real_match_elapsed_ms", elapsed_ms);

    ASSERT_TRUE(ok);

    const std::wstring expected_x_text = test_support::GetEnvString(L"OP_OPENCV_REAL_EXPECT_X");
    const std::wstring expected_y_text = test_support::GetEnvString(L"OP_OPENCV_REAL_EXPECT_Y");
    if (!expected_x_text.empty() && !expected_y_text.empty()) {
        expected_rect.x = std::stoi(expected_x_text);
        expected_rect.y = std::stoi(expected_y_text);
    }
    ExpectNearRect(match, expected_rect, 1);

    opcv::RemoveAllTemplates();
}

TEST(OpenCvTest, MatchAllTemplatesReturnsAllThresholdMatches) {
    opcv::RemoveAllTemplates();

    const int source_width = 7;
    const int source_height = 4;
    std::vector<uchar> source_bgra(static_cast<size_t>(source_width * source_height * 4), 0);

    auto set_pixel = [&](int x, int y, uchar b, uchar g, uchar r) {
        const size_t index = static_cast<size_t>((y * source_width + x) * 4);
        source_bgra[index + 0] = b;
        source_bgra[index + 1] = g;
        source_bgra[index + 2] = r;
        source_bgra[index + 3] = 255;
    };

    // 构造一个 2x2 模板，并在大图中放入两个完全一致的副本。
    set_pixel(1, 1, 10, 40, 70);
    set_pixel(2, 1, 20, 50, 80);
    set_pixel(1, 2, 30, 60, 90);
    set_pixel(2, 2, 40, 70, 100);

    set_pixel(4, 1, 10, 40, 70);
    set_pixel(5, 1, 20, 50, 80);
    set_pixel(4, 2, 30, 60, 90);
    set_pixel(5, 2, 40, 70, 100);

    std::vector<uchar> templ_bgra(2 * 2 * 4, 0);
    auto copy_template_pixel = [&](int src_x, int src_y, int dst_x, int dst_y) {
        const size_t src_index = static_cast<size_t>((src_y * source_width + src_x) * 4);
        const size_t dst_index = static_cast<size_t>((dst_y * 2 + dst_x) * 4);
        for (int channel = 0; channel < 4; ++channel) {
            templ_bgra[dst_index + channel] = source_bgra[src_index + channel];
        }
    };
    copy_template_pixel(1, 1, 0, 0);
    copy_template_pixel(2, 1, 1, 0);
    copy_template_pixel(1, 2, 0, 1);
    copy_template_pixel(2, 2, 1, 1);

    const auto template_bmp = test_support::BuildBmp32TopDown(2, 2, templ_bgra);
    const std::wstring template_path = test_support::GetTempBmpPath(L"opencv_template_all_matches.bmp");
    ASSERT_TRUE(WriteBytesToFile(template_path, template_bmp));

    opcv::ImageHandle source_image;
    source_image.bytes = source_bgra;
    source_image.width = source_width;
    source_image.height = source_height;
    source_image.channels = 4;
    opcv::Region full_region{0, 0, source_width, source_height};

    ASSERT_TRUE(opcv::LoadTemplate(L"multi_hit", template_path));

    std::vector<opcv::NamedMatchResult> results;
    ASSERT_TRUE(opcv::MatchAllTemplates(source_image, {L"multi_hit"}, full_region, 0.999, results));
    ASSERT_EQ(results.size(), 2u);

    std::sort(
        results.begin(),
        results.end(),
        [](const opcv::NamedMatchResult &lhs, const opcv::NamedMatchResult &rhs) {
            if (lhs.match.y != rhs.match.y) {
                return lhs.match.y < rhs.match.y;
            }
            return lhs.match.x < rhs.match.x;
        });

    EXPECT_EQ(results[0].name, L"multi_hit");
    EXPECT_EQ(results[0].match.x, 1);
    EXPECT_EQ(results[0].match.y, 1);
    EXPECT_EQ(results[1].name, L"multi_hit");
    EXPECT_EQ(results[1].match.x, 4);
    EXPECT_EQ(results[1].match.y, 1);

    opcv::RemoveAllTemplates();
    ::DeleteFileW(template_path.c_str());
}

TEST(OpenCvTest, MultiTemplateRegionApisOnlyMatchInsideRegion) {
    opcv::RemoveAllTemplates();

    const int source_width = 8;
    const int source_height = 5;
    std::vector<uchar> source_bgra(static_cast<size_t>(source_width * source_height * 4), 0);

    auto set_pixel = [&](int x, int y, uchar b, uchar g, uchar r) {
        const size_t index = static_cast<size_t>((y * source_width + x) * 4);
        source_bgra[index + 0] = b;
        source_bgra[index + 1] = g;
        source_bgra[index + 2] = r;
        source_bgra[index + 3] = 255;
    };

    // 模板 A: 在 ROI 内和 ROI 外各放一份。
    set_pixel(1, 1, 15, 25, 35);
    set_pixel(2, 1, 45, 55, 65);
    set_pixel(1, 2, 75, 85, 95);
    set_pixel(2, 2, 105, 115, 125);

    set_pixel(5, 1, 15, 25, 35);
    set_pixel(6, 1, 45, 55, 65);
    set_pixel(5, 2, 75, 85, 95);
    set_pixel(6, 2, 105, 115, 125);

    // 模板 B: 只在 ROI 内放一份。
    set_pixel(2, 3, 10, 110, 30);
    set_pixel(3, 3, 50, 150, 70);
    set_pixel(2, 4, 90, 190, 120);
    set_pixel(3, 4, 130, 220, 160);

    std::vector<uchar> templ_a_bgra(2 * 2 * 4, 0);
    std::vector<uchar> templ_b_bgra(2 * 2 * 4, 0);

    auto copy_patch = [&](std::vector<uchar> &templ, int templ_width, int src_x, int src_y) {
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                const size_t src_index = static_cast<size_t>(((src_y + y) * source_width + (src_x + x)) * 4);
                const size_t dst_index = static_cast<size_t>(((y * templ_width) + x) * 4);
                for (int channel = 0; channel < 4; ++channel) {
                    templ[dst_index + channel] = source_bgra[src_index + channel];
                }
            }
        }
    };

    copy_patch(templ_a_bgra, 2, 1, 1);
    copy_patch(templ_b_bgra, 2, 2, 3);

    const std::wstring templ_a_path = test_support::GetTempBmpPath(L"opencv_template_region_a.bmp");
    const std::wstring templ_b_path = test_support::GetTempBmpPath(L"opencv_template_region_b.bmp");
    ASSERT_TRUE(WriteBytesToFile(templ_a_path, test_support::BuildBmp32TopDown(2, 2, templ_a_bgra)));
    ASSERT_TRUE(WriteBytesToFile(templ_b_path, test_support::BuildBmp32TopDown(2, 2, templ_b_bgra)));

    opcv::ImageHandle source_image;
    source_image.bytes = source_bgra;
    source_image.width = source_width;
    source_image.height = source_height;
    source_image.channels = 4;

    ASSERT_TRUE(opcv::LoadTemplate(L"templ_a", templ_a_path));
    ASSERT_TRUE(opcv::LoadTemplate(L"templ_b", templ_b_path));

    opcv::Region region;
    region.x = 0;
    region.y = 0;
    region.width = 4;
    region.height = 5;

    opcv::NamedMatchResult any_result;
    ASSERT_TRUE(
        opcv::MatchAnyTemplate(source_image, {L"templ_b", L"templ_a"}, region, 0.999, any_result,
                               opcv::SearchDirection::LeftToRight));
    EXPECT_TRUE(any_result.name == L"templ_a" || any_result.name == L"templ_b");
    EXPECT_LT(any_result.match.x, region.x + region.width);
    EXPECT_LT(any_result.match.y, region.y + region.height);

    std::vector<opcv::NamedMatchResult> all_results;
    ASSERT_TRUE(opcv::MatchAllTemplates(
        source_image,
        {L"templ_a", L"templ_b"},
        region,
        0.999,
        all_results));

    ASSERT_EQ(all_results.size(), 2u);
    std::sort(
        all_results.begin(),
        all_results.end(),
        [](const opcv::NamedMatchResult &lhs, const opcv::NamedMatchResult &rhs) {
            if (lhs.name != rhs.name) {
                return lhs.name < rhs.name;
            }
            if (lhs.match.y != rhs.match.y) {
                return lhs.match.y < rhs.match.y;
            }
            return lhs.match.x < rhs.match.x;
        });

    EXPECT_EQ(all_results[0].name, L"templ_a");
    EXPECT_EQ(all_results[0].match.x, 1);
    EXPECT_EQ(all_results[0].match.y, 1);
    EXPECT_EQ(all_results[1].name, L"templ_b");
    EXPECT_EQ(all_results[1].match.x, 2);
    EXPECT_EQ(all_results[1].match.y, 3);

    opcv::RemoveAllTemplates();
    ::DeleteFileW(templ_a_path.c_str());
    ::DeleteFileW(templ_b_path.c_str());
}

TEST(OpenCvTest, PreprocessApisWorkOnSimpleImage) {
    std::vector<uchar> source_bgra = {
        0, 0, 0, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 0, 0, 0, 255,
    };

    opcv::ImageHandle source;
    source.bytes = source_bgra;
    source.width = 2;
    source.height = 2;
    source.channels = 4;

    opcv::ImageHandle gray;
    ASSERT_TRUE(opcv::ToGray(source, gray));
    EXPECT_EQ(gray.channels, 1);
    EXPECT_EQ(gray.width, 2);
    EXPECT_EQ(gray.height, 2);

    opcv::ImageHandle binary;
    ASSERT_TRUE(opcv::ToBinary(source, binary));
    EXPECT_EQ(binary.channels, 1);

    opcv::ImageHandle thresholded;
    ASSERT_TRUE(opcv::Threshold(source, thresholded, 128.0, 255.0, opcv::ThresholdMode::Binary));
    EXPECT_EQ(thresholded.channels, 1);
    EXPECT_EQ(thresholded.bytes[0], 0);
    EXPECT_EQ(thresholded.bytes[1], 255);

    opcv::ImageHandle color_mask;
    ASSERT_TRUE(opcv::InRange(source, color_mask, opcv::InRangeColorSpace::Bgr, {250, 250, 250}, {255, 255, 255}));
    EXPECT_EQ(color_mask.channels, 1);
    EXPECT_EQ(color_mask.bytes[0], 0);
    EXPECT_EQ(color_mask.bytes[1], 255);

    opcv::ImageHandle hsv_mask;
    ASSERT_TRUE(opcv::InRange(source, hsv_mask, opcv::InRangeColorSpace::Hsv, {0, 0, 250}, {179, 10, 255}));
    EXPECT_EQ(hsv_mask.channels, 1);
    EXPECT_EQ(hsv_mask.bytes[1], 255);

    opcv::ImageHandle edge;
    ASSERT_TRUE(opcv::ToEdge(source, edge));
    EXPECT_EQ(edge.channels, 1);

    opcv::ImageHandle outline;
    ASSERT_TRUE(opcv::ToOutline(source, outline));
    EXPECT_EQ(outline.channels, 1);

    std::vector<uchar> filled_bgra(static_cast<size_t>(6 * 6 * 4), 0);
    for (int y = 1; y < 5; ++y) {
        for (int x = 1; x < 5; ++x) {
            const size_t index = static_cast<size_t>((y * 6 + x) * 4);
            filled_bgra[index + 0] = 255;
            filled_bgra[index + 1] = 255;
            filled_bgra[index + 2] = 255;
            filled_bgra[index + 3] = 255;
        }
    }

    opcv::ImageHandle filled;
    filled.bytes = filled_bgra;
    filled.width = 6;
    filled.height = 6;
    filled.channels = 4;

    opcv::ImageHandle filled_outline;
    ASSERT_TRUE(opcv::ToOutline(filled, filled_outline));
    EXPECT_EQ(filled_outline.channels, 1);
    EXPECT_GT(filled_outline.bytes[static_cast<size_t>(1 * 6 + 1)], 0);
    EXPECT_EQ(filled_outline.bytes[static_cast<size_t>(3 * 6 + 3)], 0);

    opcv::ImageHandle denoised;
    ASSERT_TRUE(opcv::Denoise(source, denoised));
    EXPECT_EQ(denoised.channels, 1);

    opcv::ImageHandle equalized;
    ASSERT_TRUE(opcv::Equalize(source, equalized));
    EXPECT_EQ(equalized.channels, 1);

    opcv::ImageHandle clahe;
    ASSERT_TRUE(opcv::CLAHE(source, clahe, 2.0, 2));
    EXPECT_EQ(clahe.channels, 1);

    opcv::ImageHandle blurred;
    ASSERT_TRUE(opcv::Blur(source, blurred, opcv::BlurMode::Gaussian, 3));
    EXPECT_EQ(blurred.channels, source.channels);

    opcv::ImageHandle sharpened;
    ASSERT_TRUE(opcv::Sharpen(source, sharpened, 1.0));
    EXPECT_EQ(sharpened.channels, source.channels);

    opcv::ImageHandle thinned;
    ASSERT_TRUE(opcv::Thin(source, thinned));
    EXPECT_EQ(thinned.channels, 1);

    opcv::ImageHandle zhang_suen_thin;
    ASSERT_TRUE(opcv::Thin(filled, zhang_suen_thin, opcv::ThinMode::ZhangSuen));
    EXPECT_EQ(zhang_suen_thin.channels, 1);
    EXPECT_GT(cv::countNonZero(cv::Mat(zhang_suen_thin.height, zhang_suen_thin.width, CV_8UC1,
                                       zhang_suen_thin.bytes.data())),
              0);

    opcv::ImageHandle guo_hall_thin;
    ASSERT_TRUE(opcv::Thin(filled, guo_hall_thin, opcv::ThinMode::GuoHall));
    EXPECT_EQ(guo_hall_thin.channels, 1);
    EXPECT_GT(cv::countNonZero(cv::Mat(guo_hall_thin.height, guo_hall_thin.width, CV_8UC1,
                                       guo_hall_thin.bytes.data())),
              0);

    opcv::ImageHandle morph_thin;
    ASSERT_TRUE(opcv::Thin(filled, morph_thin, opcv::ThinMode::Morph));
    EXPECT_EQ(morph_thin.channels, 1);

    std::vector<uchar> regions_bgra(static_cast<size_t>(8 * 8 * 4), 0);
    for (int y = 1; y < 4; ++y) {
        for (int x = 1; x < 4; ++x) {
            const size_t index = static_cast<size_t>((y * 8 + x) * 4);
            regions_bgra[index + 0] = 255;
            regions_bgra[index + 1] = 255;
            regions_bgra[index + 2] = 255;
            regions_bgra[index + 3] = 255;
        }
    }
    for (int y = 5; y < 7; ++y) {
        for (int x = 5; x < 7; ++x) {
            const size_t index = static_cast<size_t>((y * 8 + x) * 4);
            regions_bgra[index + 0] = 255;
            regions_bgra[index + 1] = 255;
            regions_bgra[index + 2] = 255;
            regions_bgra[index + 3] = 255;
        }
    }

    opcv::ImageHandle regions;
    regions.bytes = regions_bgra;
    regions.width = 8;
    regions.height = 8;
    regions.channels = 4;

    std::vector<opcv::RegionAnalysisResult> components;
    ASSERT_TRUE(opcv::ConnectedComponents(regions, 1.0, components));
    ASSERT_EQ(components.size(), 2u);
    EXPECT_EQ(components[0].x, 1);
    EXPECT_EQ(components[0].y, 1);
    EXPECT_EQ(components[0].width, 3);
    EXPECT_EQ(components[0].height, 3);
    EXPECT_EQ(components[0].area, 9.0);

    std::vector<opcv::ContourAnalysisResult> contours;
    ASSERT_TRUE(opcv::FindContours(regions, 0.0, contours));
    EXPECT_EQ(contours.size(), 2u);
    EXPECT_TRUE(std::any_of(contours.begin(), contours.end(), [](const opcv::ContourAnalysisResult &item) {
        return item.x == 1 && item.y == 1 && item.width == 3 && item.height == 3 && item.points > 0;
    }));

    opcv::ImageHandle morphed;
    ASSERT_TRUE(opcv::Morphology(binary, morphed, opcv::MorphologyMode::Dilate, 3, 1));
    EXPECT_EQ(morphed.channels, 1);
    EXPECT_EQ(morphed.width, 2);
    EXPECT_EQ(morphed.height, 2);

    opcv::Region crop_region{1, 0, 1, 2};
    opcv::ImageHandle cropped;
    ASSERT_TRUE(opcv::Crop(source, crop_region, cropped));
    EXPECT_EQ(cropped.width, 1);
    EXPECT_EQ(cropped.height, 2);

    opcv::ImageHandle resized;
    ASSERT_TRUE(opcv::Resize(source, 4, 4, resized));
    EXPECT_EQ(resized.width, 4);
    EXPECT_EQ(resized.height, 4);

    opcv::ImageHandle cropped_valid;
    ASSERT_TRUE(opcv::CropValid(source, cropped_valid));
    EXPECT_GT(cropped_valid.width, 0);
    EXPECT_GT(cropped_valid.height, 0);
}

TEST(OpenCvTest, FilePreprocessApisWork) {
    const cv::Mat source(8, 8, CV_8UC3, cv::Scalar(0, 0, 0));
    cv::Mat writable = source.clone();
    cv::rectangle(writable, cv::Rect(1, 1, 3, 3), cv::Scalar(0, 0, 255), cv::FILLED);
    cv::rectangle(writable, cv::Rect(5, 5, 2, 2), cv::Scalar(0, 0, 255), cv::FILLED);

    const std::wstring src_path = test_support::GetTempBmpPath(L"opencv_preprocess_src.png");
    const std::wstring threshold_path = test_support::GetTempBmpPath(L"opencv_preprocess_threshold.png");
    const std::wstring range_path = test_support::GetTempBmpPath(L"opencv_preprocess_range.png");
    const std::wstring morph_path = test_support::GetTempBmpPath(L"opencv_preprocess_morph.png");
    const std::wstring thin_path = test_support::GetTempBmpPath(L"opencv_preprocess_thin.png");
    const std::wstring binary_path = test_support::GetTempBmpPath(L"opencv_preprocess_binary.png");
    const std::wstring edge_path = test_support::GetTempBmpPath(L"opencv_preprocess_edge.png");
    const std::wstring outline_path = test_support::GetTempBmpPath(L"opencv_preprocess_outline.png");
    const std::wstring denoise_path = test_support::GetTempBmpPath(L"opencv_preprocess_denoise.png");
    const std::wstring crop_valid_path = test_support::GetTempBmpPath(L"opencv_preprocess_crop_valid.png");
    const std::wstring equalize_path = test_support::GetTempBmpPath(L"opencv_preprocess_equalize.png");
    const std::wstring clahe_path = test_support::GetTempBmpPath(L"opencv_preprocess_clahe.png");
    const std::wstring blur_path = test_support::GetTempBmpPath(L"opencv_preprocess_blur.png");
    const std::wstring sharpen_path = test_support::GetTempBmpPath(L"opencv_preprocess_sharpen.png");
    const std::wstring pipeline_path = test_support::GetTempBmpPath(L"opencv_preprocess_pipeline.png");
    ASSERT_TRUE(WritePngFile(src_path, writable));

    op::Op op;
    long ret = 0;
    op.CvToBinary(src_path.c_str(), binary_path.c_str(), &ret);
    EXPECT_EQ(ret, 1);
    EXPECT_TRUE(std::filesystem::exists(binary_path));

    op.CvToEdge(src_path.c_str(), edge_path.c_str(), &ret);
    EXPECT_EQ(ret, 1);
    EXPECT_TRUE(std::filesystem::exists(edge_path));

    op.CvToOutline(src_path.c_str(), outline_path.c_str(), &ret);
    EXPECT_EQ(ret, 1);
    EXPECT_TRUE(std::filesystem::exists(outline_path));

    op.CvDenoise(src_path.c_str(), denoise_path.c_str(), &ret);
    EXPECT_EQ(ret, 1);
    EXPECT_TRUE(std::filesystem::exists(denoise_path));

    op.CvEqualize(src_path.c_str(), equalize_path.c_str(), &ret);
    EXPECT_EQ(ret, 1);
    EXPECT_TRUE(std::filesystem::exists(equalize_path));

    op.CvCLAHE(src_path.c_str(), clahe_path.c_str(), 2.0, 2, &ret);
    EXPECT_EQ(ret, 1);
    EXPECT_TRUE(std::filesystem::exists(clahe_path));

    op.CvBlur(src_path.c_str(), blur_path.c_str(), L"gaussian", 3, &ret);
    EXPECT_EQ(ret, 1);
    EXPECT_TRUE(std::filesystem::exists(blur_path));

    op.CvSharpen(src_path.c_str(), sharpen_path.c_str(), 1.0, &ret);
    EXPECT_EQ(ret, 1);
    EXPECT_TRUE(std::filesystem::exists(sharpen_path));

    op.CvCropValid(src_path.c_str(), crop_valid_path.c_str(), &ret);
    EXPECT_EQ(ret, 1);
    EXPECT_TRUE(std::filesystem::exists(crop_valid_path));

    op.CvThreshold(src_path.c_str(), threshold_path.c_str(), 1.0, 255.0, L"binary", &ret);
    EXPECT_EQ(ret, 1);
    EXPECT_TRUE(std::filesystem::exists(threshold_path));

    op.CvInRange(src_path.c_str(), range_path.c_str(), L"bgr", L"0,0,250", L"5,5,255", &ret);
    EXPECT_EQ(ret, 1);
    EXPECT_TRUE(std::filesystem::exists(range_path));

    op.CvMorphology(range_path.c_str(), morph_path.c_str(), L"dilate", 3, 1, &ret);
    EXPECT_EQ(ret, 1);
    EXPECT_TRUE(std::filesystem::exists(morph_path));

    op.CvThin(morph_path.c_str(), thin_path.c_str(), L"zhang_suen", &ret);
    EXPECT_EQ(ret, 1);
    EXPECT_TRUE(std::filesystem::exists(thin_path));

    op.CvThin(morph_path.c_str(), thin_path.c_str(), L"guo_hall", &ret);
    EXPECT_EQ(ret, 1);

    op.CvThin(morph_path.c_str(), thin_path.c_str(), L"morph", &ret);
    EXPECT_EQ(ret, 1);

    std::wstring components_json;
    op.CvConnectedComponents(range_path.c_str(), 1.0, components_json, &ret);
    EXPECT_EQ(ret, 1);
    EXPECT_NE(components_json.find(L"\"ok\":1"), std::wstring::npos);
    EXPECT_NE(components_json.find(L"\"area\":9"), std::wstring::npos);

    std::wstring contours_json;
    op.CvFindContours(range_path.c_str(), 0.0, contours_json, &ret);
    EXPECT_EQ(ret, 1);
    EXPECT_NE(contours_json.find(L"\"perimeter\""), std::wstring::npos);
    EXPECT_NE(contours_json.find(L"\"points\""), std::wstring::npos);

    op.CvPreprocessPipeline(
        src_path.c_str(),
        pipeline_path.c_str(),
        L"inrange:bgr,0,0,250,5,5,255|morph:open,3,1|thin:zhang_suen",
        &ret);
    EXPECT_EQ(ret, 1);
    EXPECT_TRUE(std::filesystem::exists(pipeline_path));

    op.CvThin(morph_path.c_str(), thin_path.c_str(), L"bad_mode", &ret);
    EXPECT_EQ(ret, 0);

    op.CvThreshold(src_path.c_str(), threshold_path.c_str(), 1.0, 255.0, L"bad_mode", &ret);
    EXPECT_EQ(ret, 0);

    op.CvBlur(src_path.c_str(), blur_path.c_str(), L"bad_mode", 3, &ret);
    EXPECT_EQ(ret, 0);

    op.CvPreprocessPipeline(src_path.c_str(), pipeline_path.c_str(), L"bad_step", &ret);
    EXPECT_EQ(ret, 0);

    ::DeleteFileW(src_path.c_str());
    ::DeleteFileW(threshold_path.c_str());
    ::DeleteFileW(range_path.c_str());
    ::DeleteFileW(morph_path.c_str());
    ::DeleteFileW(thin_path.c_str());
    ::DeleteFileW(binary_path.c_str());
    ::DeleteFileW(edge_path.c_str());
    ::DeleteFileW(outline_path.c_str());
    ::DeleteFileW(denoise_path.c_str());
    ::DeleteFileW(crop_valid_path.c_str());
    ::DeleteFileW(equalize_path.c_str());
    ::DeleteFileW(clahe_path.c_str());
    ::DeleteFileW(blur_path.c_str());
    ::DeleteFileW(sharpen_path.c_str());
    ::DeleteFileW(pipeline_path.c_str());
}

TEST(OpenCvTest, LoadTemplateAutomaticallyUsesAlphaAsMaskAndFindsValidHit) {
    opcv::RemoveAllTemplates();

    cv::Mat templ(4, 4, CV_8UC4, cv::Scalar(0, 0, 0, 0));
    templ.at<cv::Vec4b>(1, 1) = cv::Vec4b(10, 100, 200, 255);
    templ.at<cv::Vec4b>(1, 2) = cv::Vec4b(10, 100, 200, 255);
    templ.at<cv::Vec4b>(2, 1) = cv::Vec4b(10, 100, 200, 255);
    templ.at<cv::Vec4b>(2, 2) = cv::Vec4b(10, 100, 200, 255);

    const std::wstring template_path = test_support::GetTempBmpPath(L"opencv_alpha_template.png");
    ASSERT_TRUE(WritePngFile(template_path, templ));

    std::vector<uchar> source_bgra(static_cast<size_t>(8 * 8 * 4), 0);
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            const size_t index = static_cast<size_t>((y * 8 + x) * 4);
            source_bgra[index + 0] = static_cast<uchar>((x * 41 + y * 13) % 255);
            source_bgra[index + 1] = static_cast<uchar>((x * 17 + y * 29) % 255);
            source_bgra[index + 2] = static_cast<uchar>((x * 11 + y * 23) % 255);
            source_bgra[index + 3] = 255;
        }
    }

    auto place_center = [&](int ox, int oy) {
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                const size_t index = static_cast<size_t>(((oy + 1 + y) * 8 + (ox + 1 + x)) * 4);
                source_bgra[index + 0] = 10;
                source_bgra[index + 1] = 100;
                source_bgra[index + 2] = 200;
            }
        }
    };
    place_center(3, 2);

    opcv::ImageHandle source;
    source.bytes = source_bgra;
    source.width = 8;
    source.height = 8;
    source.channels = 4;

    ASSERT_TRUE(opcv::LoadTemplate(L"alpha_icon", template_path));

    opcv::MatchResult result;
    ASSERT_TRUE(opcv::MatchTemplate(source, L"alpha_icon", {0, 0, 8, 8}, 0.95, result));
    EXPECT_GE(result.x, 0);
    EXPECT_GE(result.y, 0);
    EXPECT_LE(result.x, 4);
    EXPECT_LE(result.y, 4);

    opcv::RemoveAllTemplates();
    ::DeleteFileW(template_path.c_str());
}

TEST(OpenCvTest, FeatureMatchTemplateFindsPattern) {
    opcv::RemoveAllTemplates();

    std::vector<uchar> source_bgra;
    int source_width = 0;
    int source_height = 0;
    int template_x = 0;
    int template_y = 0;
    int template_width = 0;
    int template_height = 0;
    std::wstring template_path;
    BuildFeaturePatternData(
        source_bgra,
        source_width,
        source_height,
        template_x,
        template_y,
        template_width,
        template_height,
        template_path);

    opcv::ImageHandle source;
    source.bytes = source_bgra;
    source.width = source_width;
    source.height = source_height;
    source.channels = 4;

    ASSERT_TRUE(opcv::LoadTemplate(L"feature_target", template_path));

    opcv::MatchResult result;
    ASSERT_TRUE(opcv::FeatureMatchTemplate(
        source,
        L"feature_target",
        {0, 0, source_width, source_height},
        0.05,
        result));
    EXPECT_NEAR(result.x, template_x, 2);
    EXPECT_NEAR(result.y, template_y, 2);

    opcv::RemoveAllTemplates();
    ::DeleteFileW(template_path.c_str());
}

TEST(OpenCvTest, EdgeMatchTemplateFindsOutline) {
    opcv::RemoveAllTemplates();

    const int source_width = 20;
    const int source_height = 20;
    std::vector<uchar> source_bgra(static_cast<size_t>(source_width * source_height * 4), 0);

    auto set_white = [&](int x, int y) {
        const size_t index = static_cast<size_t>((y * source_width + x) * 4);
        source_bgra[index + 0] = 255;
        source_bgra[index + 1] = 255;
        source_bgra[index + 2] = 255;
        source_bgra[index + 3] = 255;
    };

    for (int x = 0; x < 6; ++x) {
        set_white(7 + x, 6);
        set_white(7 + x, 11);
    }
    for (int y = 0; y < 6; ++y) {
        set_white(7, 6 + y);
        set_white(12, 6 + y);
    }

    std::vector<uchar> templ_bgra(static_cast<size_t>(6 * 6 * 4), 0);
    for (int y = 0; y < 6; ++y) {
        for (int x = 0; x < 6; ++x) {
            const size_t src_index = static_cast<size_t>(((6 + y) * source_width + (7 + x)) * 4);
            const size_t dst_index = static_cast<size_t>((y * 6 + x) * 4);
            for (int channel = 0; channel < 4; ++channel) {
                templ_bgra[dst_index + channel] = source_bgra[src_index + channel];
            }
        }
    }

    const auto template_bmp = test_support::BuildBmp32TopDown(6, 6, templ_bgra);
    const std::wstring template_path = test_support::GetTempBmpPath(L"opencv_edge_template.bmp");
    ASSERT_TRUE(WriteBytesToFile(template_path, template_bmp));

    opcv::ImageHandle source;
    source.bytes = source_bgra;
    source.width = source_width;
    source.height = source_height;
    source.channels = 4;

    ASSERT_TRUE(opcv::LoadTemplate(L"edge_box", template_path));

    opcv::MatchResult result;
    ASSERT_TRUE(opcv::EdgeMatchTemplate(source, L"edge_box", {0, 0, source_width, source_height}, 0.5, result));
    EXPECT_NEAR(result.x, 7, 1);
    EXPECT_NEAR(result.y, 6, 1);

    opcv::RemoveAllTemplates();
    ::DeleteFileW(template_path.c_str());
}

TEST(OpenCvTest, ShapeMatchTemplateFindsRectangle) {
    opcv::RemoveAllTemplates();

    const int source_width = 30;
    const int source_height = 24;
    std::vector<uchar> source_bgra(static_cast<size_t>(source_width * source_height * 4), 0);

    auto fill_white = [&](int x1, int y1, int x2, int y2) {
        for (int y = y1; y < y2; ++y) {
            for (int x = x1; x < x2; ++x) {
                const size_t index = static_cast<size_t>((y * source_width + x) * 4);
                source_bgra[index + 0] = 255;
                source_bgra[index + 1] = 255;
                source_bgra[index + 2] = 255;
                source_bgra[index + 3] = 255;
            }
        }
    };

    fill_white(10, 7, 18, 15);

    std::vector<uchar> templ_bgra(static_cast<size_t>(8 * 8 * 4), 0);
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            const size_t src_index = static_cast<size_t>(((7 + y) * source_width + (10 + x)) * 4);
            const size_t dst_index = static_cast<size_t>((y * 8 + x) * 4);
            for (int channel = 0; channel < 4; ++channel) {
                templ_bgra[dst_index + channel] = source_bgra[src_index + channel];
            }
        }
    }

    const auto template_bmp = test_support::BuildBmp32TopDown(8, 8, templ_bgra);
    const std::wstring template_path = test_support::GetTempBmpPath(L"opencv_shape_template.bmp");
    ASSERT_TRUE(WriteBytesToFile(template_path, template_bmp));

    opcv::ImageHandle source;
    source.bytes = source_bgra;
    source.width = source_width;
    source.height = source_height;
    source.channels = 4;

    ASSERT_TRUE(opcv::LoadTemplate(L"shape_rect", template_path));

    opcv::MatchResult result;
    ASSERT_TRUE(opcv::ShapeMatchTemplate(
        source,
        L"shape_rect",
        {0, 0, source_width, source_height},
        0.8,
        result));
    EXPECT_EQ(result.x, 10);
    EXPECT_EQ(result.y, 7);
    EXPECT_EQ(result.width, 8);
    EXPECT_EQ(result.height, 8);

    opcv::RemoveAllTemplates();
    ::DeleteFileW(template_path.c_str());
}

TEST(OpenCvBenchmark, DISABLED_MatchingHotPaths) {
    opcv::RemoveAllTemplates();

    auto real_cases = RealPhotoTemplateCases();
    real_cases.erase(
        std::remove_if(
            real_cases.begin(),
            real_cases.end(),
            [](const RealImageCase &item) { return !HasDemoAssetPair(item); }),
        real_cases.end());
    if (real_cases.size() < 4) {
        GTEST_SKIP() << "Real image template assets are not available.";
    }

    const auto record_benchmark = [&](const std::string &name, const std::function<bool()> &operation) {
        bool benchmark_ok = false;
        const BenchmarkStats stats = MeasureBenchmark(operation, benchmark_ok);
        ASSERT_TRUE(benchmark_ok) << name;
        RecordBenchmarkStats(name, stats);
    };

    opcv::ImageHandle real_source;
    ASSERT_TRUE(opcv::LoadImageFromFile(FindDemoAsset(real_cases[0].source_file), real_source));
    opcv::ImageHandle real_template;
    ASSERT_TRUE(opcv::LoadImageFromFile(FindDemoAsset(real_cases[0].template_file), real_template));
    ASSERT_TRUE(opcv::LoadTemplate(L"bench_real_single", FindDemoAsset(real_cases[0].template_file)));

    opcv::MatchResult named_match;
    record_benchmark(
        "bench_real_named_match",
        [&]() {
            return opcv::MatchTemplate(
                real_source,
                L"bench_real_single",
                {0, 0, real_source.width, real_source.height},
                0.98,
                named_match,
                opcv::SearchDirection::LeftToRight,
                opcv::StripMode::None,
                1,
                opcv::MatchColorMode::Color);
        });
    ExpectNearRect(named_match, real_cases[0].template_rect, 1);

    std::vector<opcv::MatchCandidate> direct_candidates;
    record_benchmark(
        "bench_real_direct_candidates",
        [&]() {
            return opcv::MatchTemplate(
                real_source,
                real_template,
                {0, 0, real_source.width, real_source.height},
                direct_candidates,
                1,
                opcv::MatchColorMode::Color);
        });
    opcv::MatchResult direct_best;
    ASSERT_TRUE(opcv::GetBestMatch(direct_candidates, real_template.width, real_template.height, direct_best));
    ExpectNearRect(direct_best, real_cases[0].template_rect, 1);

    const std::wstring scale_source_path = FindDemoAsset(L"opencv_real_scale_source.png");
    const std::wstring scale_template_path = FindDemoAsset(L"opencv_real_scale_template.png");
    if (scale_source_path.empty() || scale_template_path.empty()) {
        GTEST_SKIP() << "Real scale matching assets are not available.";
    }
    opcv::ImageHandle scale_source;
    ASSERT_TRUE(opcv::LoadImageFromFile(scale_source_path, scale_source));
    ASSERT_TRUE(opcv::LoadTemplate(L"bench_real_scale", scale_template_path));

    opcv::MatchResult manual_scale_match;
    record_benchmark(
        "bench_real_scale_manual",
        [&]() {
            return opcv::MatchTemplateScale(
                scale_source,
                L"bench_real_scale",
                {0, 0, scale_source.width, scale_source.height},
                {1.0, 1.5},
                0.98,
                manual_scale_match,
                1,
                opcv::MatchColorMode::Color);
        });
    ExpectNearRect(manual_scale_match, cv::Rect(80, 420, 144, 144), 1);

    opcv::MatchResult auto_scale_match;
    record_benchmark(
        "bench_real_scale_auto",
        [&]() {
            return opcv::MatchTemplateScale(
                scale_source,
                L"bench_real_scale",
                {0, 0, scale_source.width, scale_source.height},
                {},
                0.98,
                auto_scale_match,
                1,
                opcv::MatchColorMode::Color);
        });
    ExpectNearRect(auto_scale_match, cv::Rect(80, 420, 144, 144), 1);

    const std::wstring game_source_path = FindDemoAsset(L"opencv_game_scene_source.png");
    const std::wstring coin_template_path = FindDemoAsset(L"opencv_game_coin_template.png");
    if (game_source_path.empty() || coin_template_path.empty()) {
        GTEST_SKIP() << "Game scene matching assets are not available.";
    }
    opcv::ImageHandle game_source;
    ASSERT_TRUE(opcv::LoadImageFromFile(game_source_path, game_source));
    ASSERT_TRUE(opcv::LoadTemplate(L"bench_game_coin", coin_template_path));

    std::vector<opcv::NamedMatchResult> game_coin_matches;
    record_benchmark(
        "bench_game_coin_all",
        [&]() {
            return opcv::MatchAllTemplates(
                game_source,
                {L"bench_game_coin"},
                {0, 0, game_source.width, game_source.height},
                0.98,
                game_coin_matches,
                opcv::SearchDirection::LeftToRight,
                opcv::StripMode::None,
                cv::TM_CCORR_NORMED,
                opcv::MatchColorMode::Color);
        });
    EXPECT_GE(game_coin_matches.size(), 4u);

    opcv::ImageHandle hard_scale_source;
    ASSERT_TRUE(opcv::LoadImageFromFile(FindDemoAsset(L"opencv_hard_scale_source.png"), hard_scale_source));
    ASSERT_TRUE(opcv::LoadTemplate(L"bench_hard_scale", FindDemoAsset(L"opencv_hard_scale_template.png")));
    opcv::MatchResult hard_auto_scale_match;
    record_benchmark(
        "bench_hard_scale_auto",
        [&]() {
            return opcv::MatchTemplateScale(
                hard_scale_source,
                L"bench_hard_scale",
                {0, 0, hard_scale_source.width, hard_scale_source.height},
                {},
                0.98,
                hard_auto_scale_match,
                1,
                opcv::MatchColorMode::Color);
        });
    ExpectNearRect(hard_auto_scale_match, cv::Rect(90, 450, 72, 72), 1);

    opcv::RemoveAllTemplates();
}
