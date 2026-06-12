#include "OpenCvHelpers.h"

#include <algorithm>
#include <cstring>
#include <filesystem>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/geometry.hpp>
#include <opencv2/imgproc.hpp>

namespace opcv::helpers {

size_t imageByteCount(int width, int height, int channels) {
    return static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(channels);
}

int normalizeOddKernelSize(int kernel_size) {
    int value = std::max(1, kernel_size);
    if (value % 2 == 0) {
        ++value;
    }
    return value;
}

cv::Mat createMatView(ImageHandle &image) {
    if (image.width <= 0 || image.height <= 0 || image.channels <= 0 || image.bytes.empty()) {
        return {};
    }

    int type = 0;
    if (image.channels == 1) {
        type = CV_8UC1;
    } else if (image.channels == 3) {
        type = CV_8UC3;
    } else if (image.channels == 4) {
        type = CV_8UC4;
    } else {
        return {};
    }

    return cv::Mat(image.height, image.width, type, image.bytes.data());
}

cv::Mat createMatView(const ImageHandle &image) {
    return createMatView(const_cast<ImageHandle &>(image));
}

ImageHandle createImageFromMat(const cv::Mat &mat) {
    ImageHandle image;
    if (mat.empty()) {
        return image;
    }

    image.width = mat.cols;
    image.height = mat.rows;
    image.channels = mat.channels();
    image.bytes.resize(imageByteCount(image.width, image.height, image.channels));
    std::memcpy(image.bytes.data(), mat.data, image.bytes.size());
    return image;
}

cv::Mat createMatFromRawMemory(const RawImageView &source) {
    if (source.data == nullptr || source.width <= 0 || source.height <= 0) {
        return {};
    }

    int channels = 0;
    int type = 0;
    if (source.format == PixelFormat::Gray) {
        channels = 1;
        type = CV_8UC1;
    } else if (source.format == PixelFormat::Bgr) {
        channels = 3;
        type = CV_8UC3;
    } else if (source.format == PixelFormat::Bgra) {
        channels = 4;
        type = CV_8UC4;
    } else {
        return {};
    }

    const int expected_stride = source.width * channels;
    const size_t step = static_cast<size_t>(source.stride > 0 ? source.stride : expected_stride);
    if (step < static_cast<size_t>(expected_stride)) {
        return {};
    }

    const cv::Mat wrapped(source.height, source.width, type, const_cast<unsigned char *>(source.data), step);
    return wrapped.clone();
}

cv::Rect clampRegion(const Region &region, const ImageHandle &source) {
    const int x = std::max(0, region.x);
    const int y = std::max(0, region.y);
    const int right = std::min(source.width, region.x + region.width);
    const int bottom = std::min(source.height, region.y + region.height);
    return cv::Rect(x, y, std::max(0, right - x), std::max(0, bottom - y));
}

bool prepareMatchInputs(
    const cv::Mat &source,
    const cv::Mat &templ,
    MatchColorMode color_mode,
    cv::Mat &norm_source,
    cv::Mat &norm_templ) {
    if (source.empty() || templ.empty()) {
        return false;
    }

    if (color_mode == MatchColorMode::Gray) {
        if (source.channels() == 1) {
            norm_source = source;
        } else if (source.channels() == 3) {
            cv::cvtColor(source, norm_source, cv::COLOR_BGR2GRAY);
        } else if (source.channels() == 4) {
            cv::cvtColor(source, norm_source, cv::COLOR_BGRA2GRAY);
        } else {
            return false;
        }

        if (templ.channels() == 1) {
            norm_templ = templ;
        } else if (templ.channels() == 3) {
            cv::cvtColor(templ, norm_templ, cv::COLOR_BGR2GRAY);
        } else if (templ.channels() == 4) {
            cv::cvtColor(templ, norm_templ, cv::COLOR_BGRA2GRAY);
        } else {
            return false;
        }
        return norm_source.type() == norm_templ.type();
    }

    if (source.type() == templ.type() && source.channels() != 4) {
        norm_source = source;
        norm_templ = templ;
        return true;
    }

    if (source.channels() == 3) {
        norm_source = source;
    } else if (source.channels() == 4) {
        cv::cvtColor(source, norm_source, cv::COLOR_BGRA2BGR);
    } else {
        return false;
    }

    if (templ.channels() == 3) {
        norm_templ = templ;
    } else if (templ.channels() == 4) {
        cv::cvtColor(templ, norm_templ, cv::COLOR_BGRA2BGR);
    } else {
        return false;
    }

    return norm_source.type() == norm_templ.type();
}

bool prepareMaskInput(const cv::Mat &mask, const cv::Mat &norm_templ, cv::Mat &norm_mask) {
    if (mask.empty() || norm_templ.empty()) {
        return false;
    }
    if (mask.cols != norm_templ.cols || mask.rows != norm_templ.rows) {
        return false;
    }

    if (mask.channels() == 1) {
        norm_mask = mask;
    } else if (mask.channels() == 3) {
        cv::cvtColor(mask, norm_mask, cv::COLOR_BGR2GRAY);
    } else if (mask.channels() == 4) {
        cv::cvtColor(mask, norm_mask, cv::COLOR_BGRA2GRAY);
    } else {
        return false;
    }

    return norm_mask.type() == CV_8UC1;
}

bool isSupportedMatchMethod(int method) {
    return method >= cv::TM_SQDIFF && method <= cv::TM_CCOEFF_NORMED;
}

double convertMatchScoreToSimilarity(int method, float raw_score) {
    if (method == cv::TM_SQDIFF || method == cv::TM_SQDIFF_NORMED) {
        return 1.0 - static_cast<double>(raw_score);
    }
    return static_cast<double>(raw_score);
}

bool collectTemplateMatches(
    const cv::Mat &source,
    const cv::Mat &templ,
    const cv::Mat *mask,
    int offset_x,
    int offset_y,
    std::vector<MatchCandidate> &matches,
    int method,
    MatchColorMode color_mode) {
    matches.clear();
    if (source.empty() || templ.empty()) {
        return false;
    }

    if (!isSupportedMatchMethod(method)) {
        return false;
    }

    cv::Mat norm_source;
    cv::Mat norm_templ;
    if (!prepareMatchInputs(source, templ, color_mode, norm_source, norm_templ)) {
        return false;
    }

    cv::Mat norm_mask;
    if (mask != nullptr && !prepareMaskInput(*mask, norm_templ, norm_mask)) {
        return false;
    }

    if (norm_templ.cols > norm_source.cols || norm_templ.rows > norm_source.rows) {
        return false;
    }

    cv::Mat result;
    if (!norm_mask.empty()) {
        cv::matchTemplate(norm_source, norm_templ, result, method, norm_mask);
    } else {
        cv::matchTemplate(norm_source, norm_templ, result, method);
    }
    if (result.empty()) {
        return false;
    }

    const int candidate_count = result.rows * result.cols;
    matches.reserve(static_cast<size_t>(candidate_count));
    for (int y = 0; y < result.rows; ++y) {
        for (int x = 0; x < result.cols; ++x) {
            MatchCandidate candidate;
            candidate.x = offset_x + x;
            candidate.y = offset_y + y;
            candidate.score = convertMatchScoreToSimilarity(method, result.at<float>(y, x));
            matches.push_back(candidate);
        }
    }
    return true;
}

bool findFirstTemplateMatch(
    const cv::Mat &source,
    const cv::Mat &templ,
    const cv::Mat *mask,
    int offset_x,
    int offset_y,
    int templ_width,
    int templ_height,
    double threshold,
    MatchResult &result,
    SearchDirection dir,
    int method,
    MatchColorMode color_mode) {
    result = {};
    if (source.empty() || templ.empty()) {
        return false;
    }
    if (!isSupportedMatchMethod(method)) {
        return false;
    }

    cv::Mat norm_source;
    cv::Mat norm_templ;
    if (!prepareMatchInputs(source, templ, color_mode, norm_source, norm_templ)) {
        return false;
    }

    cv::Mat norm_mask;
    if (mask != nullptr && !prepareMaskInput(*mask, norm_templ, norm_mask)) {
        return false;
    }

    if (norm_templ.cols > norm_source.cols || norm_templ.rows > norm_source.rows) {
        return false;
    }

    const double clamped_threshold = std::clamp(threshold, 0.0, 1.0);
    auto try_match = [&](const cv::Mat &match_result, int x, int y, int tile_offset_x, int tile_offset_y) {
        const double score = convertMatchScoreToSimilarity(method, match_result.at<float>(y, x));
        if (score < clamped_threshold) {
            return false;
        }

        result.x = offset_x + tile_offset_x + x;
        result.y = offset_y + tile_offset_y + y;
        result.width = templ_width;
        result.height = templ_height;
        result.score = score;
        return true;
    };

    auto scan_tile = [&](const cv::Mat &match_result, int tile_offset_x, int tile_offset_y) {
        if (match_result.empty()) {
            return false;
        }

        switch (dir) {
        case SearchDirection::RightToLeft:
            for (int x = match_result.cols - 1; x >= 0; --x) {
                for (int y = 0; y < match_result.rows; ++y) {
                    if (try_match(match_result, x, y, tile_offset_x, tile_offset_y)) {
                        return true;
                    }
                }
            }
            break;
        case SearchDirection::TopToBottom:
            for (int y = 0; y < match_result.rows; ++y) {
                for (int x = 0; x < match_result.cols; ++x) {
                    if (try_match(match_result, x, y, tile_offset_x, tile_offset_y)) {
                        return true;
                    }
                }
            }
            break;
        case SearchDirection::BottomToTop:
            for (int y = match_result.rows - 1; y >= 0; --y) {
                for (int x = 0; x < match_result.cols; ++x) {
                    if (try_match(match_result, x, y, tile_offset_x, tile_offset_y)) {
                        return true;
                    }
                }
            }
            break;
        case SearchDirection::LeftToRight:
        default:
            for (int x = 0; x < match_result.cols; ++x) {
                for (int y = 0; y < match_result.rows; ++y) {
                    if (try_match(match_result, x, y, tile_offset_x, tile_offset_y)) {
                        return true;
                    }
                }
            }
            break;
        }

        return false;
    };

    auto match_and_scan_tile = [&](const cv::Rect &source_tile, int tile_offset_x, int tile_offset_y) {
        const cv::Mat tile_source = norm_source(source_tile);
        cv::Mat match_result;
        if (!norm_mask.empty()) {
            cv::matchTemplate(tile_source, norm_templ, match_result, method, norm_mask);
        } else {
            cv::matchTemplate(tile_source, norm_templ, match_result, method);
        }
        return scan_tile(match_result, tile_offset_x, tile_offset_y);
    };

    constexpr int kCandidateTileSpan = 128;
    const int candidate_cols = norm_source.cols - norm_templ.cols + 1;
    const int candidate_rows = norm_source.rows - norm_templ.rows + 1;

    if (dir == SearchDirection::LeftToRight) {
        for (int tile_x = 0; tile_x < candidate_cols; tile_x += kCandidateTileSpan) {
            const int tile_cols = std::min(kCandidateTileSpan, candidate_cols - tile_x);
            const cv::Rect source_tile(tile_x, 0, tile_cols + norm_templ.cols - 1, norm_source.rows);
            if (match_and_scan_tile(source_tile, tile_x, 0)) {
                return true;
            }
        }
        return false;
    }

    if (dir == SearchDirection::RightToLeft) {
        for (int tile_x = ((candidate_cols - 1) / kCandidateTileSpan) * kCandidateTileSpan; tile_x >= 0;
             tile_x -= kCandidateTileSpan) {
            const int tile_cols = std::min(kCandidateTileSpan, candidate_cols - tile_x);
            const cv::Rect source_tile(tile_x, 0, tile_cols + norm_templ.cols - 1, norm_source.rows);
            if (match_and_scan_tile(source_tile, tile_x, 0)) {
                return true;
            }
            if (tile_x == 0) {
                break;
            }
        }
        return false;
    }

    if (dir == SearchDirection::BottomToTop) {
        for (int tile_y = ((candidate_rows - 1) / kCandidateTileSpan) * kCandidateTileSpan; tile_y >= 0;
             tile_y -= kCandidateTileSpan) {
            const int tile_rows = std::min(kCandidateTileSpan, candidate_rows - tile_y);
            const cv::Rect source_tile(0, tile_y, norm_source.cols, tile_rows + norm_templ.rows - 1);
            if (match_and_scan_tile(source_tile, 0, tile_y)) {
                return true;
            }
            if (tile_y == 0) {
                break;
            }
        }
        return false;
    }

    for (int tile_y = 0; tile_y < candidate_rows; tile_y += kCandidateTileSpan) {
        const int tile_rows = std::min(kCandidateTileSpan, candidate_rows - tile_y);
        const cv::Rect source_tile(0, tile_y, norm_source.cols, tile_rows + norm_templ.rows - 1);
        if (match_and_scan_tile(source_tile, 0, tile_y)) {
            return true;
        }
    }

    return false;
}

bool getBestMatch(const std::vector<MatchCandidate> &matches, int templ_width, int templ_height, MatchResult &result) {
    if (matches.empty()) {
        result = {};
        return false;
    }

    const auto best = std::max_element(
        matches.begin(),
        matches.end(),
        [](const MatchCandidate &lhs, const MatchCandidate &rhs) { return lhs.score < rhs.score; });

    result.x = best->x;
    result.y = best->y;
    result.width = templ_width;
    result.height = templ_height;
    result.score = best->score;
    return true;
}

bool isMatchAboveThreshold(const MatchResult &result, double threshold) {
    return result.score >= std::clamp(threshold, 0.0, 1.0);
}

void collectThresholdMatches(
    const std::vector<MatchCandidate> &matches,
    int templ_width,
    int templ_height,
    double threshold,
    std::vector<MatchResult> &results) {
    results.clear();
    const double clamped_threshold = std::clamp(threshold, 0.0, 1.0);
    for (const auto &candidate : matches) {
        if (candidate.score < clamped_threshold) {
            continue;
        }

        MatchResult result;
        result.x = candidate.x;
        result.y = candidate.y;
        result.width = templ_width;
        result.height = templ_height;
        result.score = candidate.score;
        results.push_back(result);
    }
}

bool loadImageFromFile(const std::wstring &file_path, ImageHandle &image) {
    const std::string path = std::filesystem::path(file_path).string();
    const cv::Mat mat = cv::imread(path, cv::IMREAD_UNCHANGED);
    if (mat.empty()) {
        image = {};
        return false;
    }

    image = createImageFromMat(mat);
    return true;
}

bool loadImageFromMemory(const RawImageView &source, ImageHandle &image) {
    const cv::Mat mat = createMatFromRawMemory(source);
    if (mat.empty()) {
        image = {};
        return false;
    }

    image = createImageFromMat(mat);
    return true;
}

bool toGray(const ImageHandle &source, ImageHandle &gray) {
    const cv::Mat source_mat = createMatView(source);
    if (source_mat.empty()) {
        gray = {};
        return false;
    }

    cv::Mat gray_mat;
    if (source.channels == 1) {
        gray_mat = source_mat.clone();
    } else if (source.channels == 3) {
        cv::cvtColor(source_mat, gray_mat, cv::COLOR_BGR2GRAY);
    } else if (source.channels == 4) {
        cv::cvtColor(source_mat, gray_mat, cv::COLOR_BGRA2GRAY);
    } else {
        gray = {};
        return false;
    }

    gray = createImageFromMat(gray_mat);
    return true;
}

bool toMask(const ImageHandle &source, ImageHandle &mask) {
    if (!toGray(source, mask)) {
        return false;
    }

    for (auto &value : mask.bytes) {
        value = value == 0 ? 0 : 255;
    }
    return true;
}

bool thresholdImage(
    const ImageHandle &source,
    ImageHandle &output,
    double threshold,
    double max_value,
    ThresholdMode mode) {
    ImageHandle gray;
    if (!toGray(source, gray)) {
        output = {};
        return false;
    }

    const cv::Mat gray_mat = createMatView(gray);
    if (gray_mat.empty()) {
        output = {};
        return false;
    }

    cv::Mat threshold_mat;
    const double safe_max_value = max_value > 0.0 ? max_value : 255.0;
    switch (mode) {
    case ThresholdMode::Binary:
        cv::threshold(gray_mat, threshold_mat, threshold, safe_max_value, cv::THRESH_BINARY);
        break;
    case ThresholdMode::BinaryInv:
        cv::threshold(gray_mat, threshold_mat, threshold, safe_max_value, cv::THRESH_BINARY_INV);
        break;
    case ThresholdMode::Otsu:
        cv::threshold(gray_mat, threshold_mat, 0.0, safe_max_value, cv::THRESH_BINARY | cv::THRESH_OTSU);
        break;
    case ThresholdMode::OtsuInv:
        cv::threshold(gray_mat, threshold_mat, 0.0, safe_max_value, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
        break;
    case ThresholdMode::Adaptive:
    case ThresholdMode::AdaptiveInv: {
        const int adaptive_type =
            mode == ThresholdMode::Adaptive ? cv::THRESH_BINARY : cv::THRESH_BINARY_INV;
        // 自适应阈值要求奇数窗口，固定 11 能覆盖小图标和文字边缘。
        cv::adaptiveThreshold(gray_mat, threshold_mat, safe_max_value, cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                              adaptive_type, 11, 2.0);
        break;
    }
    default:
        output = {};
        return false;
    }

    output = createImageFromMat(threshold_mat);
    return output.channels == 1 && !output.bytes.empty();
}

bool inRangeImage(
    const ImageHandle &source,
    ImageHandle &output,
    InRangeColorSpace color_space,
    const std::vector<double> &lower,
    const std::vector<double> &upper) {
    const cv::Mat source_mat = createMatView(source);
    if (source_mat.empty()) {
        output = {};
        return false;
    }

    cv::Mat range_source;
    int required_channels = 3;
    if (color_space == InRangeColorSpace::Gray) {
        ImageHandle gray;
        if (!toGray(source, gray)) {
            output = {};
            return false;
        }
        range_source = createMatView(gray);
        required_channels = 1;
    } else {
        if (source.channels == 3) {
            range_source = source_mat;
        } else if (source.channels == 4) {
            cv::cvtColor(source_mat, range_source, cv::COLOR_BGRA2BGR);
        } else {
            output = {};
            return false;
        }

        if (color_space == InRangeColorSpace::Hsv) {
            cv::cvtColor(range_source, range_source, cv::COLOR_BGR2HSV);
        } else if (color_space != InRangeColorSpace::Bgr) {
            output = {};
            return false;
        }
    }

    if (range_source.empty() || lower.size() < static_cast<size_t>(required_channels) ||
        upper.size() < static_cast<size_t>(required_channels)) {
        output = {};
        return false;
    }

    cv::Scalar lower_bound;
    cv::Scalar upper_bound;
    for (int i = 0; i < required_channels; ++i) {
        lower_bound[i] = lower[static_cast<size_t>(i)];
        upper_bound[i] = upper[static_cast<size_t>(i)];
    }

    cv::Mat mask;
    cv::inRange(range_source, lower_bound, upper_bound, mask);
    output = createImageFromMat(mask);
    return output.channels == 1 && !output.bytes.empty();
}

bool morphologyImage(
    const ImageHandle &source,
    ImageHandle &output,
    MorphologyMode mode,
    int kernel_size,
    int iterations) {
    ImageHandle mask;
    if (!toMask(source, mask)) {
        output = {};
        return false;
    }

    const cv::Mat mask_mat = createMatView(mask);
    if (mask_mat.empty()) {
        output = {};
        return false;
    }

    // OpenCV 的形态学窗口用奇数更自然，传入偶数时向上修正。
    const int safe_kernel_size = normalizeOddKernelSize(kernel_size);
    const int safe_iterations = std::max(1, iterations);
    const cv::Mat kernel =
        cv::getStructuringElement(cv::MORPH_RECT, cv::Size(safe_kernel_size, safe_kernel_size));

    cv::Mat morphed;
    switch (mode) {
    case MorphologyMode::Erode:
        cv::erode(mask_mat, morphed, kernel, cv::Point(-1, -1), safe_iterations);
        break;
    case MorphologyMode::Dilate:
        cv::dilate(mask_mat, morphed, kernel, cv::Point(-1, -1), safe_iterations);
        break;
    case MorphologyMode::Open:
        cv::morphologyEx(mask_mat, morphed, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), safe_iterations);
        break;
    case MorphologyMode::Close:
        cv::morphologyEx(mask_mat, morphed, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), safe_iterations);
        break;
    default:
        output = {};
        return false;
    }

    output = createImageFromMat(morphed);
    return output.channels == 1 && !output.bytes.empty();
}

bool alphaToMask(const ImageHandle &source, ImageHandle &mask) {
    mask = {};
    if (source.channels != 4 || source.width <= 0 || source.height <= 0) {
        return false;
    }
    if (source.bytes.empty()) {
        return false;
    }

    mask.width = source.width;
    mask.height = source.height;
    mask.channels = 1;
    mask.bytes.resize(static_cast<size_t>(mask.width) * static_cast<size_t>(mask.height), 0);

    bool has_any_transparent = false;
    for (int y = 0; y < source.height; ++y) {
        for (int x = 0; x < source.width; ++x) {
            const size_t src_index = static_cast<size_t>((y * source.width + x) * 4);
            const size_t dst_index = static_cast<size_t>(y * source.width + x);
            const unsigned char alpha = source.bytes[src_index + 3];
            mask.bytes[dst_index] = alpha == 0 ? 0 : 255;
            if (alpha == 0) {
                has_any_transparent = true;
            }
        }
    }

    if (!has_any_transparent) {
        mask = {};
        return false;
    }

    return true;
}

bool shouldInvertShapeMask(const cv::Mat &binary_mat, const cv::Mat &gray_mat) {
    if (binary_mat.empty() || binary_mat.channels() != 1 || gray_mat.empty() || gray_mat.channels() != 1) {
        return false;
    }

    const int rows = binary_mat.rows;
    const int cols = binary_mat.cols;
    if (rows <= 0 || cols <= 0 || gray_mat.rows != rows || gray_mat.cols != cols) {
        return false;
    }

    double gray_min = 0.0;
    double gray_max = 0.0;
    cv::minMaxLoc(gray_mat, &gray_min, &gray_max);
    if (gray_max - gray_min < 16.0) {
        return false;
    }

    int border_pixels = 0;
    int border_foreground = 0;
    auto sample_pixel = [&](int y, int x) {
        ++border_pixels;
        if (binary_mat.at<uchar>(y, x) != 0) {
            ++border_foreground;
        }
    };

    for (int x = 0; x < cols; ++x) {
        sample_pixel(0, x);
        if (rows > 1) {
            sample_pixel(rows - 1, x);
        }
    }
    for (int y = 1; y + 1 < rows; ++y) {
        sample_pixel(y, 0);
        if (cols > 1) {
            sample_pixel(y, cols - 1);
        }
    }

    if (border_pixels == 0) {
        return false;
    }

    const double border_foreground_ratio =
        static_cast<double>(border_foreground) / static_cast<double>(border_pixels);
    const double foreground_ratio =
        static_cast<double>(cv::countNonZero(binary_mat)) / static_cast<double>(rows * cols);
    return border_foreground_ratio >= 0.60 && foreground_ratio >= 0.50;
}

bool toEdge(const ImageHandle &source, ImageHandle &edge) {
    ImageHandle gray;
    if (!toGray(source, gray)) {
        edge = {};
        return false;
    }

    const cv::Mat gray_mat = createMatView(gray);
    if (gray_mat.empty()) {
        edge = {};
        return false;
    }

    cv::Mat edge_mat;
    cv::Canny(gray_mat, edge_mat, 60.0, 180.0);
    if (edge_mat.empty()) {
        edge = {};
        return false;
    }

    edge = createImageFromMat(edge_mat);
    return edge.channels == 1;
}

bool toShapeMask(const ImageHandle &source, ImageHandle &mask) {
    if (alphaToMask(source, mask)) {
        return true;
    }

    ImageHandle gray;
    if (!toGray(source, gray)) {
        mask = {};
        return false;
    }

    const cv::Mat gray_mat = createMatView(gray);
    if (gray_mat.empty()) {
        mask = {};
        return false;
    }

    cv::Mat binary_mat;
    cv::threshold(gray_mat, binary_mat, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    if (binary_mat.empty()) {
        mask = {};
        return false;
    }
    if (shouldInvertShapeMask(binary_mat, gray_mat)) {
        cv::bitwise_not(binary_mat, binary_mat);
    }

    mask = createImageFromMat(binary_mat);
    return mask.channels == 1;
}

bool denoise(const ImageHandle &source, ImageHandle &output) {
    ImageHandle gray;
    if (!toGray(source, gray)) {
        output = {};
        return false;
    }

    const cv::Mat gray_mat = createMatView(gray);
    if (gray_mat.empty()) {
        output = {};
        return false;
    }

    cv::Mat denoise_mat;
    cv::medianBlur(gray_mat, denoise_mat, 3);
    if (denoise_mat.empty()) {
        output = {};
        return false;
    }

    output = createImageFromMat(denoise_mat);
    return output.channels == 1;
}

bool equalizeImage(const ImageHandle &source, ImageHandle &output) {
    ImageHandle gray;
    if (!toGray(source, gray)) {
        output = {};
        return false;
    }

    const cv::Mat gray_mat = createMatView(gray);
    if (gray_mat.empty()) {
        output = {};
        return false;
    }

    cv::Mat equalized;
    cv::equalizeHist(gray_mat, equalized);
    output = createImageFromMat(equalized);
    return output.channels == 1 && !output.bytes.empty();
}

bool claheImage(const ImageHandle &source, ImageHandle &output, double clip_limit, int tile_grid_size) {
    ImageHandle gray;
    if (!toGray(source, gray)) {
        output = {};
        return false;
    }

    const cv::Mat gray_mat = createMatView(gray);
    if (gray_mat.empty()) {
        output = {};
        return false;
    }

    // clip_limit 控制增强强度，tile_grid_size 控制局部网格大小。
    const double safe_clip_limit = clip_limit > 0.0 ? clip_limit : 2.0;
    const int safe_tile_size = std::max(1, tile_grid_size);
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(safe_clip_limit, cv::Size(safe_tile_size, safe_tile_size));

    cv::Mat clahe_mat;
    clahe->apply(gray_mat, clahe_mat);
    output = createImageFromMat(clahe_mat);
    return output.channels == 1 && !output.bytes.empty();
}

bool blurImage(const ImageHandle &source, ImageHandle &output, BlurMode mode, int kernel_size) {
    const cv::Mat source_mat = createMatView(source);
    if (source_mat.empty()) {
        output = {};
        return false;
    }

    const int safe_kernel_size = normalizeOddKernelSize(kernel_size);
    cv::Mat blurred;
    switch (mode) {
    case BlurMode::Gaussian:
        cv::GaussianBlur(source_mat, blurred, cv::Size(safe_kernel_size, safe_kernel_size), 0.0);
        break;
    case BlurMode::Median:
        cv::medianBlur(source_mat, blurred, safe_kernel_size);
        break;
    case BlurMode::Bilateral:
        cv::bilateralFilter(source_mat, blurred, safe_kernel_size, 75.0, 75.0);
        break;
    case BlurMode::Box:
        cv::blur(source_mat, blurred, cv::Size(safe_kernel_size, safe_kernel_size));
        break;
    default:
        output = {};
        return false;
    }

    output = createImageFromMat(blurred);
    return output.channels == source.channels && !output.bytes.empty();
}

bool sharpenImage(const ImageHandle &source, ImageHandle &output, double strength) {
    const cv::Mat source_mat = createMatView(source);
    if (source_mat.empty()) {
        output = {};
        return false;
    }

    const double safe_strength = std::max(0.0, strength);
    cv::Mat blurred;
    cv::GaussianBlur(source_mat, blurred, cv::Size(0, 0), 1.0);

    cv::Mat sharpened;
    // 反遮罩锐化：原图增强，模糊图抵消，strength 越大边缘越锐。
    cv::addWeighted(source_mat, 1.0 + safe_strength, blurred, -safe_strength, 0.0, sharpened);
    output = createImageFromMat(sharpened);
    return output.channels == source.channels && !output.bytes.empty();
}

bool thinMorphImage(const ImageHandle &source, ImageHandle &output) {
    ImageHandle binary;
    if (!toShapeMask(source, binary)) {
        output = {};
        return false;
    }

    const cv::Mat src = createMatView(binary);
    if (src.empty()) {
        output = {};
        return false;
    }

    cv::Mat current = src.clone();
    cv::Mat skeleton = cv::Mat::zeros(current.size(), CV_8UC1);
    cv::Mat eroded;
    cv::Mat temp;
    cv::Mat element = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));

    while (true) {
        const int previous_skeleton_count = cv::countNonZero(skeleton);
        cv::erode(current, eroded, element);
        cv::dilate(eroded, temp, element);
        cv::subtract(current, temp, temp);
        cv::bitwise_or(skeleton, temp, skeleton);
        eroded.copyTo(current);
        // 某些小图块会进入“当前图像未归零，但骨架不再变化”的状态。
        if (cv::countNonZero(current) == 0 || cv::countNonZero(skeleton) == previous_skeleton_count) {
            break;
        }
    }

    output = createImageFromMat(skeleton);
    return output.channels == 1;
}

bool thinZhangSuenMat(cv::Mat &image) {
    if (image.empty() || image.type() != CV_8UC1) {
        return false;
    }

    image /= 255;
    cv::Mat marker = cv::Mat::zeros(image.size(), CV_8UC1);
    bool changed = false;

    do {
        changed = false;
        for (int step = 0; step < 2; ++step) {
            marker.setTo(0);
            for (int y = 1; y < image.rows - 1; ++y) {
                for (int x = 1; x < image.cols - 1; ++x) {
                    const uchar p1 = image.at<uchar>(y, x);
                    if (p1 == 0) {
                        continue;
                    }

                    const uchar p2 = image.at<uchar>(y - 1, x);
                    const uchar p3 = image.at<uchar>(y - 1, x + 1);
                    const uchar p4 = image.at<uchar>(y, x + 1);
                    const uchar p5 = image.at<uchar>(y + 1, x + 1);
                    const uchar p6 = image.at<uchar>(y + 1, x);
                    const uchar p7 = image.at<uchar>(y + 1, x - 1);
                    const uchar p8 = image.at<uchar>(y, x - 1);
                    const uchar p9 = image.at<uchar>(y - 1, x - 1);

                    // A 是 0->1 的跳变次数，B 是八邻域前景点数量。
                    const int transitions = (p2 == 0 && p3 == 1) + (p3 == 0 && p4 == 1) +
                                            (p4 == 0 && p5 == 1) + (p5 == 0 && p6 == 1) +
                                            (p6 == 0 && p7 == 1) + (p7 == 0 && p8 == 1) +
                                            (p8 == 0 && p9 == 1) + (p9 == 0 && p2 == 1);
                    const int neighbors = p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9;
                    const bool keep_connectivity =
                        step == 0 ? (p2 * p4 * p6 == 0 && p4 * p6 * p8 == 0)
                                  : (p2 * p4 * p8 == 0 && p2 * p6 * p8 == 0);

                    if (transitions == 1 && neighbors >= 2 && neighbors <= 6 && keep_connectivity) {
                        marker.at<uchar>(y, x) = 1;
                    }
                }
            }

            if (cv::countNonZero(marker) > 0) {
                image &= ~marker;
                changed = true;
            }
        }
    } while (changed);

    image *= 255;
    return true;
}

bool thinGuoHallMat(cv::Mat &image) {
    if (image.empty() || image.type() != CV_8UC1) {
        return false;
    }

    image /= 255;
    cv::Mat marker = cv::Mat::zeros(image.size(), CV_8UC1);
    bool changed = false;

    do {
        changed = false;
        for (int step = 0; step < 2; ++step) {
            marker.setTo(0);
            for (int y = 1; y < image.rows - 1; ++y) {
                for (int x = 1; x < image.cols - 1; ++x) {
                    if (image.at<uchar>(y, x) == 0) {
                        continue;
                    }

                    const uchar p2 = image.at<uchar>(y - 1, x);
                    const uchar p3 = image.at<uchar>(y - 1, x + 1);
                    const uchar p4 = image.at<uchar>(y, x + 1);
                    const uchar p5 = image.at<uchar>(y + 1, x + 1);
                    const uchar p6 = image.at<uchar>(y + 1, x);
                    const uchar p7 = image.at<uchar>(y + 1, x - 1);
                    const uchar p8 = image.at<uchar>(y, x - 1);
                    const uchar p9 = image.at<uchar>(y - 1, x - 1);

                    // Guo-Hall 使用成对邻域保持连通性，对文字笔画通常更稳。
                    const int transitions = (p2 == 0 && (p3 || p4)) + (p4 == 0 && (p5 || p6)) +
                                            (p6 == 0 && (p7 || p8)) + (p8 == 0 && (p9 || p2));
                    const int n1 = (p9 || p2) + (p3 || p4) + (p5 || p6) + (p7 || p8);
                    const int n2 = (p2 || p3) + (p4 || p5) + (p6 || p7) + (p8 || p9);
                    const int min_neighbors = std::min(n1, n2);
                    const int m = step == 0 ? ((p6 || p7 || !p9) && p8) : ((p2 || p3 || !p5) && p4);

                    if (transitions == 1 && min_neighbors >= 2 && min_neighbors <= 3 && m == 0) {
                        marker.at<uchar>(y, x) = 1;
                    }
                }
            }

            if (cv::countNonZero(marker) > 0) {
                image &= ~marker;
                changed = true;
            }
        }
    } while (changed);

    image *= 255;
    return true;
}

bool thinImage(const ImageHandle &source, ImageHandle &output, ThinMode mode) {
    if (mode == ThinMode::Morph) {
        return thinMorphImage(source, output);
    }

    ImageHandle binary;
    if (!toShapeMask(source, binary)) {
        output = {};
        return false;
    }

    cv::Mat image = createMatView(binary).clone();
    if (image.empty()) {
        output = {};
        return false;
    }

    const bool ok = mode == ThinMode::GuoHall ? thinGuoHallMat(image) : thinZhangSuenMat(image);
    if (!ok) {
        output = {};
        return false;
    }

    output = createImageFromMat(image);
    return output.channels == 1 && !output.bytes.empty();
}

bool crop(const ImageHandle &source, const Region &region, ImageHandle &output) {
    const cv::Mat source_mat = createMatView(source);
    if (source_mat.empty()) {
        output = {};
        return false;
    }

    const cv::Rect roi = clampRegion(region, source);
    if (roi.width <= 0 || roi.height <= 0) {
        output = {};
        return false;
    }

    output = createImageFromMat(source_mat(roi).clone());
    return !output.bytes.empty();
}

bool resizeImage(const ImageHandle &source, int width, int height, ImageHandle &output) {
    const cv::Mat source_mat = createMatView(source);
    if (source_mat.empty() || width <= 0 || height <= 0) {
        output = {};
        return false;
    }

    cv::Mat resized;
    cv::resize(source_mat, resized, cv::Size(width, height), 0.0, 0.0, cv::INTER_LINEAR);
    if (resized.empty()) {
        output = {};
        return false;
    }

    output = createImageFromMat(resized);
    return !output.bytes.empty();
}

bool cropValid(const ImageHandle &source, ImageHandle &output) {
    ImageHandle binary;
    if (!toShapeMask(source, binary)) {
        output = {};
        return false;
    }

    const cv::Mat binary_mat = createMatView(binary);
    if (binary_mat.empty()) {
        output = {};
        return false;
    }

    std::vector<cv::Point> non_zero_points;
    cv::findNonZero(binary_mat, non_zero_points);
    if (non_zero_points.empty()) {
        output = {};
        return false;
    }

    const cv::Rect rect = cv::boundingRect(non_zero_points);
    output = createImageFromMat(createMatView(source)(rect).clone());
    return !output.bytes.empty();
}

bool connectedComponentsImage(
    const ImageHandle &source,
    double min_area,
    std::vector<RegionAnalysisResult> &results) {
    results.clear();

    ImageHandle mask;
    if (!toMask(source, mask)) {
        return false;
    }

    const cv::Mat mask_mat = createMatView(mask);
    if (mask_mat.empty()) {
        return false;
    }

    // 先统一转成二值掩码，再用 OpenCV 统计每个前景连通域的外接框和面积。
    cv::Mat labels;
    cv::Mat stats;
    cv::Mat centroids;
    const int count = cv::connectedComponentsWithStats(mask_mat, labels, stats, centroids, 8, CV_32S);
    const double safe_min_area = std::max(0.0, min_area);
    for (int label = 1; label < count; ++label) {
        const int area = stats.at<int>(label, cv::CC_STAT_AREA);
        if (area < safe_min_area) {
            continue;
        }

        RegionAnalysisResult item;
        item.x = stats.at<int>(label, cv::CC_STAT_LEFT);
        item.y = stats.at<int>(label, cv::CC_STAT_TOP);
        item.width = stats.at<int>(label, cv::CC_STAT_WIDTH);
        item.height = stats.at<int>(label, cv::CC_STAT_HEIGHT);
        item.area = static_cast<double>(area);
        results.push_back(item);
    }

    return !results.empty();
}

bool findContoursImage(
    const ImageHandle &source,
    double min_area,
    std::vector<ContourAnalysisResult> &results) {
    results.clear();

    ImageHandle mask;
    if (!toMask(source, mask)) {
        return false;
    }

    const cv::Mat mask_mat = createMatView(mask);
    if (mask_mat.empty()) {
        return false;
    }

    // 轮廓会修改输入 Mat，使用 clone 保持上游图像只读。
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask_mat.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    const double safe_min_area = std::max(0.0, min_area);
    for (const auto &contour : contours) {
        const double area = std::abs(cv::contourArea(contour));
        if (area < safe_min_area) {
            continue;
        }

        const cv::Rect rect = cv::boundingRect(contour);
        ContourAnalysisResult item;
        item.x = rect.x;
        item.y = rect.y;
        item.width = rect.width;
        item.height = rect.height;
        item.area = area;
        item.perimeter = cv::arcLength(contour, true);
        item.points = static_cast<int>(contour.size());
        results.push_back(item);
    }

    return !results.empty();
}

bool extractLargestContour(const ImageHandle &mask, std::vector<cv::Point> &contour) {
    contour.clear();
    if (mask.channels != 1) {
        return false;
    }

    const cv::Mat mask_mat = createMatView(mask);
    if (mask_mat.empty()) {
        return false;
    }

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask_mat.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    if (contours.empty()) {
        return false;
    }

    double best_area = 0.0;
    for (const auto &candidate : contours) {
        const double area = std::abs(cv::contourArea(candidate));
        if (area <= best_area || candidate.size() < 4) {
            continue;
        }
        contour = candidate;
        best_area = area;
    }

    return !contour.empty();
}

bool extractCandidateContours(const ImageHandle &mask, std::vector<std::vector<cv::Point>> &contours) {
    contours.clear();
    if (mask.channels != 1) {
        return false;
    }

    const cv::Mat mask_mat = createMatView(mask);
    if (mask_mat.empty()) {
        return false;
    }

    std::vector<std::vector<cv::Point>> raw_contours;
    cv::findContours(mask_mat.clone(), raw_contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    for (const auto &candidate : raw_contours) {
        if (candidate.size() < 4) {
            continue;
        }
        const double area = std::abs(cv::contourArea(candidate));
        if (area < 9.0) {
            continue;
        }
        contours.push_back(candidate);
    }

    return !contours.empty();
}

} // namespace opcv::helpers

