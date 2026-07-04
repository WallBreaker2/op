#include "TemplateMatcher.h"
#include "OpenCvHelpers.h"
#include "../base/ThreadPool.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <filesystem>
#include <future>
#include <iomanip>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <thread>
#include <unordered_map>

#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/geometry.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

using namespace opcv::helpers;

namespace {

// 模板缓存条目，同时保存彩色版和灰度版。
struct TemplateEntry {
    struct ScaledTemplateEntry {
        opcv::ImageHandle color;
        opcv::ImageHandle gray;
        opcv::ImageHandle mask;
        bool has_mask = false;
    };

    opcv::ImageHandle color;
    opcv::ImageHandle gray;
    opcv::ImageHandle edge;
    opcv::ImageHandle mask;
    std::vector<cv::Point> shape_contour;
    bool has_edge_cache = false;
    bool has_mask = false;
    bool has_shape_cache = false;
    std::vector<cv::KeyPoint> feature_keypoints;
    cv::Mat feature_descriptors;
    bool has_feature_cache = false;
    std::unordered_map<std::wstring, ScaledTemplateEntry> scaled_templates;
};

// 模板缓存，按名字保存已加载模板。
std::unordered_map<std::wstring, TemplateEntry> gTemplateStore;
std::shared_mutex gTemplateStoreMutex;

ThreadPool &getOpenCvThreadPool() {
    static ThreadPool pool(std::max(1u, std::thread::hardware_concurrency()));
    return pool;
}

template <typename Callback>
bool withTemplateEntryExclusive(const std::wstring &template_name, Callback &&callback) {
    std::unique_lock<std::shared_mutex> lock(gTemplateStoreMutex);
    const auto it = gTemplateStore.find(template_name);
    if (it == gTemplateStore.end()) {
        return false;
    }

    return callback(it->second);
}

opcv::SearchDirection normalizeSearchDirection(opcv::SearchDirection dir) {
    switch (dir) {
    case opcv::SearchDirection::LeftToRight:
    case opcv::SearchDirection::RightToLeft:
    case opcv::SearchDirection::TopToBottom:
    case opcv::SearchDirection::BottomToTop:
        return dir;
    default:
        return opcv::SearchDirection::LeftToRight;
    }
}

opcv::StripMode normalizeStripMode(opcv::StripMode mode) {
    switch (mode) {
    case opcv::StripMode::None:
    case opcv::StripMode::Horizontal:
    case opcv::StripMode::Vertical:
        return mode;
    default:
        return opcv::StripMode::None;
    }
}

bool isDirectionCompatibleWithStrip(opcv::SearchDirection dir, opcv::StripMode mode) {
    if (mode == opcv::StripMode::None) {
        return true;
    }
    if (mode == opcv::StripMode::Horizontal) {
        return dir == opcv::SearchDirection::LeftToRight || dir == opcv::SearchDirection::RightToLeft;
    }
    return dir == opcv::SearchDirection::TopToBottom || dir == opcv::SearchDirection::BottomToTop;
}

std::vector<opcv::Region> buildStripRegions(const opcv::Region &region, int templ_width, int templ_height,
                                            opcv::StripMode strip_mode, opcv::SearchDirection dir) {
    std::vector<opcv::Region> strips;
    if (strip_mode == opcv::StripMode::None) {
        strips.push_back(region);
        return strips;
    }

    // 分条带并行只接受和条带轴一致的方向，避免无意义组合。
    if (!isDirectionCompatibleWithStrip(dir, strip_mode)) {
        return strips;
    }

    const int total_length = strip_mode == opcv::StripMode::Horizontal ? region.width : region.height;
    const int overlap = std::max(1, strip_mode == opcv::StripMode::Horizontal ? templ_width : templ_height);
    if (total_length <= overlap) {
        strips.push_back(region);
        return strips;
    }

    const size_t worker_count = std::max<size_t>(1, std::thread::hardware_concurrency());
    const int searchable_span = std::max(1, total_length - overlap);
    const size_t strip_count = std::min(worker_count, static_cast<size_t>(searchable_span));
    const int step = std::max(1, (searchable_span + static_cast<int>(strip_count) - 1) /
                                     static_cast<int>(strip_count));

    for (size_t index = 0; index < strip_count; ++index) {
        const int start = static_cast<int>(index) * step;
        if (start >= total_length) {
            break;
        }
        const int end = std::min(total_length, start + step + overlap);
        if (end <= start) {
            continue;
        }

        opcv::Region strip = region;
        if (strip_mode == opcv::StripMode::Horizontal) {
            strip.x = region.x + start;
            strip.width = end - start;
        } else {
            strip.y = region.y + start;
            strip.height = end - start;
        }
        strips.push_back(strip);
    }

    // 让条带分发顺序和搜索方向保持一致，配合命中即停语义优先搜索目标方向上的条带。
    if (dir == opcv::SearchDirection::RightToLeft || dir == opcv::SearchDirection::BottomToTop) {
        std::reverse(strips.begin(), strips.end());
    }

    return strips;
}

bool matchResultPrecedesInDirection(const opcv::MatchResult &lhs, const opcv::MatchResult &rhs, opcv::SearchDirection dir) {
    switch (dir) {
    case opcv::SearchDirection::RightToLeft:
        return lhs.x > rhs.x || (lhs.x == rhs.x && lhs.y < rhs.y);
    case opcv::SearchDirection::TopToBottom:
        return lhs.y < rhs.y || (lhs.y == rhs.y && lhs.x < rhs.x);
    case opcv::SearchDirection::BottomToTop:
        return lhs.y > rhs.y || (lhs.y == rhs.y && lhs.x < rhs.x);
    case opcv::SearchDirection::LeftToRight:
    default:
        return lhs.x < rhs.x || (lhs.x == rhs.x && lhs.y < rhs.y);
    }
}

void dedupeMatchResults(std::vector<opcv::MatchResult> &results) {
    std::sort(results.begin(), results.end(), [](const opcv::MatchResult &lhs, const opcv::MatchResult &rhs) {
        if (lhs.x != rhs.x) {
            return lhs.x < rhs.x;
        }
        if (lhs.y != rhs.y) {
            return lhs.y < rhs.y;
        }
        if (lhs.width != rhs.width) {
            return lhs.width < rhs.width;
        }
        if (lhs.height != rhs.height) {
            return lhs.height < rhs.height;
        }
        return lhs.score > rhs.score;
    });

    results.erase(std::unique(results.begin(), results.end(), [](const opcv::MatchResult &lhs, const opcv::MatchResult &rhs) {
                      return lhs.x == rhs.x && lhs.y == rhs.y && lhs.width == rhs.width && lhs.height == rhs.height;
                  }),
                  results.end());
}

bool matchResultCentersOverlap(const opcv::MatchResult &lhs, const opcv::MatchResult &rhs) {
    const double lhs_center_x = static_cast<double>(lhs.x) + static_cast<double>(lhs.width) / 2.0;
    const double lhs_center_y = static_cast<double>(lhs.y) + static_cast<double>(lhs.height) / 2.0;
    const double rhs_center_x = static_cast<double>(rhs.x) + static_cast<double>(rhs.width) / 2.0;
    const double rhs_center_y = static_cast<double>(rhs.y) + static_cast<double>(rhs.height) / 2.0;

    const bool lhs_contains_rhs_center =
        rhs_center_x >= lhs.x && rhs_center_x < lhs.x + lhs.width &&
        rhs_center_y >= lhs.y && rhs_center_y < lhs.y + lhs.height;
    if (lhs_contains_rhs_center) {
        return true;
    }

    const bool rhs_contains_lhs_center =
        lhs_center_x >= rhs.x && lhs_center_x < rhs.x + rhs.width &&
        lhs_center_y >= rhs.y && lhs_center_y < rhs.y + rhs.height;
    return rhs_contains_lhs_center;
}

void suppressOverlappingMatchResults(std::vector<opcv::MatchResult> &results, opcv::SearchDirection dir) {
    if (results.empty()) {
        return;
    }

    std::stable_sort(results.begin(), results.end(), [dir](const opcv::MatchResult &lhs, const opcv::MatchResult &rhs) {
        if (lhs.score != rhs.score) {
            return lhs.score > rhs.score;
        }
        return matchResultPrecedesInDirection(lhs, rhs, dir);
    });

    std::vector<opcv::MatchResult> filtered;
    filtered.reserve(results.size());
    for (const auto &candidate : results) {
        bool overlaps_existing = false;
        for (const auto &selected : filtered) {
            if (matchResultCentersOverlap(candidate, selected)) {
                overlaps_existing = true;
                break;
            }
        }
        if (!overlaps_existing) {
            filtered.push_back(candidate);
        }
    }

    std::stable_sort(filtered.begin(), filtered.end(), [dir](const opcv::MatchResult &lhs, const opcv::MatchResult &rhs) {
        return matchResultPrecedesInDirection(lhs, rhs, dir);
    });
    results.swap(filtered);
}

constexpr int kPyramidMinRegionWidth = 960;
constexpr int kPyramidMinRegionHeight = 540;
constexpr int kPyramidMinTemplateSize = 32;
constexpr int kPyramidMinCoarseTemplateSize = 16;
constexpr int kPyramidMaxLevels = 4;
constexpr int kPyramidCoarseCandidates = 12;
constexpr int kPyramidAllCoarseCandidates = 256;
constexpr double kPyramidCoarseThresholdRelax = 0.12;
constexpr double kMaskedPyramidCoarseThresholdRelax = 0.04;
constexpr double kPyramidDefinitiveMissThresholdRelax = 0.07;
constexpr double kScaleAutoEarlyStopSimilarity = 0.995;
constexpr size_t kScaleAutoFastPathCandidates = 2;
constexpr size_t kScaleAutoMaxProbeRefineCandidates = 4;
constexpr double kScaleAutoProbeThresholdRelax = 0.06;
constexpr double kScaleAutoStrongProbeSimilarity = 0.96;
constexpr double kShapeAreaRatioWeight = 0.45;
constexpr double kShapeAspectRatioWeight = 0.25;
constexpr double kShapeSizeRatioWeight = 0.20;
constexpr double kShapeMinAreaScore = 0.45;
constexpr double kShapeMinAspectScore = 0.65;
constexpr double kShapeMinSizeScore = 0.65;
constexpr double kFeatureMinInlierRatio = 0.35;
constexpr int kFeatureMinGeometryInliers = 6;
constexpr double kFeatureMaxBoundsScale = 3.0;
constexpr double kFeatureMinBoundsScale = 0.25;

struct TemplateSnapshot {
    opcv::ImageHandle templ;
    opcv::ImageHandle mask;
    bool has_mask = false;
};

bool collectRegionCandidates(
    const opcv::ImageHandle &source,
    const opcv::ImageHandle &templ,
    const opcv::ImageHandle *mask,
    const opcv::Region &region,
    int method,
    opcv::MatchColorMode color_mode,
    std::vector<opcv::MatchCandidate> &matches);

bool collectRegionThresholdMatches(
    const opcv::ImageHandle &source,
    const opcv::ImageHandle &templ,
    const opcv::ImageHandle *mask,
    const opcv::Region &region,
    double threshold,
    int method,
    opcv::MatchColorMode color_mode,
    std::vector<opcv::MatchResult> &results);

bool pyramidFindAllMatches(
    const opcv::ImageHandle &source,
    const opcv::ImageHandle &templ,
    const opcv::ImageHandle *mask,
    double threshold,
    opcv::SearchDirection dir,
    int method,
    opcv::MatchColorMode color_mode,
    size_t max_candidates,
    std::vector<opcv::MatchResult> &results,
    bool use_peak_candidates = false,
    double coarse_threshold_relax = kPyramidCoarseThresholdRelax,
    bool *saturated = nullptr,
    bool *definitive_miss = nullptr);

bool shouldTryPyramid(
    const opcv::Region &region,
    int templ_width,
    int templ_height,
    opcv::StripMode strip_mode) {
    return strip_mode == opcv::StripMode::None &&
           region.width >= kPyramidMinRegionWidth &&
           region.height >= kPyramidMinRegionHeight &&
           templ_width >= kPyramidMinTemplateSize &&
           templ_height >= kPyramidMinTemplateSize;
}

bool shouldProbeAutomaticScalesFirst(const cv::Rect &roi, int templ_width, int templ_height) {
    return roi.width >= kPyramidMinRegionWidth &&
           roi.height >= kPyramidMinRegionHeight &&
           templ_width >= kPyramidMinTemplateSize &&
           templ_height >= kPyramidMinTemplateSize;
}

std::vector<double> buildAutomaticScaleCandidates(
    int templ_width,
    int templ_height,
    const cv::Rect &roi) {
    std::vector<double> scales = {
        1.0, 0.75, 1.25, 0.90, 1.10, 0.60, 1.50, 0.50, 1.75, 2.0,
    };

    scales.erase(
        std::remove_if(
            scales.begin(),
            scales.end(),
            [&](double scale) {
                const int scaled_width = static_cast<int>(std::lround(templ_width * scale));
                const int scaled_height = static_cast<int>(std::lround(templ_height * scale));
                return scale <= 0.0 || scaled_width < 4 || scaled_height < 4 || scaled_width > roi.width ||
                       scaled_height > roi.height;
            }),
        scales.end());
    return scales;
}

double ratioPenalty(double ratio) {
    if (ratio <= 0.0) {
        return 0.0;
    }
    const double normalized = ratio < 1.0 ? ratio : 1.0 / ratio;
    return std::clamp(normalized, 0.0, 1.0);
}

bool getTemplateSnapshot(
    const std::wstring &template_name,
    opcv::MatchColorMode color_mode,
    TemplateSnapshot &snapshot) {
    snapshot = {};

    std::shared_lock<std::shared_mutex> lock(gTemplateStoreMutex);
    const auto it = gTemplateStore.find(template_name);
    if (it == gTemplateStore.end()) {
        return false;
    }

    snapshot.templ = color_mode == opcv::MatchColorMode::Gray ? it->second.gray : it->second.color;
    if (it->second.has_mask) {
        snapshot.mask = it->second.mask;
        snapshot.has_mask = true;
    }

    return snapshot.templ.width > 0 && snapshot.templ.height > 0 && !snapshot.templ.bytes.empty();
}

opcv::Region rectToRegion(const cv::Rect &rect) {
    opcv::Region region;
    region.x = rect.x;
    region.y = rect.y;
    region.width = rect.width;
    region.height = rect.height;
    return region;
}

bool resizeForPyramid(const opcv::ImageHandle &source, int width, int height, opcv::ImageHandle &output) {
    const cv::Mat source_mat = createMatView(source);
    if (source_mat.empty() || width <= 0 || height <= 0) {
        output = {};
        return false;
    }

    cv::Mat resized;
    const int interpolation = width < source.width || height < source.height ? cv::INTER_AREA : cv::INTER_LINEAR;
    cv::resize(source_mat, resized, cv::Size(width, height), 0.0, 0.0, interpolation);
    output = createImageFromMat(resized);
    return !output.bytes.empty();
}

bool resizeMaskForPyramid(const opcv::ImageHandle &source, int width, int height, opcv::ImageHandle &output) {
    const cv::Mat source_mat = createMatView(source);
    if (source_mat.empty() || width <= 0 || height <= 0) {
        output = {};
        return false;
    }

    cv::Mat resized;
    cv::resize(source_mat, resized, cv::Size(width, height), 0.0, 0.0, cv::INTER_NEAREST);
    opcv::ImageHandle resized_mask = createImageFromMat(resized);
    return toMask(resized_mask, output);
}

bool buildPyramidLevels(
    const opcv::ImageHandle &source,
    const opcv::ImageHandle &templ,
    const opcv::ImageHandle *mask,
    std::vector<opcv::ImageHandle> &source_levels,
    std::vector<opcv::ImageHandle> &templ_levels,
    std::vector<opcv::ImageHandle> &mask_levels) {
    source_levels.clear();
    templ_levels.clear();
    mask_levels.clear();

    source_levels.push_back(source);
    templ_levels.push_back(templ);
    if (mask != nullptr) {
        mask_levels.push_back(*mask);
    }

    for (int level = 1; level < kPyramidMaxLevels; ++level) {
        const auto &prev_source = source_levels.back();
        const auto &prev_templ = templ_levels.back();
        const int next_source_width = std::max(1, prev_source.width / 2);
        const int next_source_height = std::max(1, prev_source.height / 2);
        const int next_templ_width = std::max(1, prev_templ.width / 2);
        const int next_templ_height = std::max(1, prev_templ.height / 2);

        if (next_source_width < kPyramidMinRegionWidth / 2 ||
            next_source_height < kPyramidMinRegionHeight / 2 ||
            next_templ_width < kPyramidMinCoarseTemplateSize ||
            next_templ_height < kPyramidMinCoarseTemplateSize) {
            break;
        }
        if (next_templ_width > next_source_width || next_templ_height > next_source_height) {
            break;
        }

        opcv::ImageHandle next_source;
        opcv::ImageHandle next_templ;
        if (!resizeForPyramid(source, next_source_width, next_source_height, next_source) ||
            !resizeForPyramid(templ, next_templ_width, next_templ_height, next_templ)) {
            return false;
        }

        source_levels.push_back(std::move(next_source));
        templ_levels.push_back(std::move(next_templ));

        if (mask != nullptr) {
            opcv::ImageHandle next_mask;
            if (!resizeMaskForPyramid(*mask, next_templ_width, next_templ_height, next_mask)) {
                return false;
            }
            mask_levels.push_back(std::move(next_mask));
        }
    }

    return source_levels.size() > 1 && templ_levels.size() == source_levels.size() &&
           (mask == nullptr || mask_levels.size() == source_levels.size());
}

bool collectBestCandidates(
    const opcv::ImageHandle &source,
    const opcv::ImageHandle &templ,
    const opcv::ImageHandle *mask,
    const opcv::Region &region,
    double threshold,
    int method,
    opcv::MatchColorMode color_mode,
    size_t max_candidates,
    std::vector<opcv::MatchResult> &results,
    bool *saturated = nullptr) {
    results.clear();
    if (saturated != nullptr) {
        *saturated = false;
    }
    if (max_candidates == 0) {
        return false;
    }

    std::vector<opcv::MatchCandidate> candidates;
    if (!collectRegionCandidates(source, templ, mask, region, method, color_mode, candidates)) {
        return false;
    }

    const double clamped_threshold = std::clamp(threshold, 0.0, 1.0);
    std::vector<opcv::MatchCandidate> filtered;
    filtered.reserve(candidates.size());
    for (const auto &candidate : candidates) {
        if (candidate.score >= clamped_threshold) {
            filtered.push_back(candidate);
        }
    }

    if (filtered.empty()) {
        return false;
    }
    if (saturated != nullptr && filtered.size() > max_candidates) {
        *saturated = true;
    }

    std::stable_sort(filtered.begin(), filtered.end(), [](const opcv::MatchCandidate &lhs,
                                                          const opcv::MatchCandidate &rhs) {
        return lhs.score > rhs.score;
    });

    const size_t limit = std::min(max_candidates, filtered.size());
    results.reserve(limit);
    for (size_t index = 0; index < limit; ++index) {
        opcv::MatchResult match;
        match.x = filtered[index].x;
        match.y = filtered[index].y;
        match.width = templ.width;
        match.height = templ.height;
        match.score = filtered[index].score;
        results.push_back(match);
    }

    return !results.empty();
}

bool collectPeakCandidates(
    const opcv::ImageHandle &source,
    const opcv::ImageHandle &templ,
    const opcv::ImageHandle *mask,
    const opcv::Region &region,
    double threshold,
    int method,
    opcv::MatchColorMode color_mode,
    size_t max_candidates,
    std::vector<opcv::MatchResult> &results,
    bool *saturated = nullptr) {
    results.clear();
    if (saturated != nullptr) {
        *saturated = false;
    }
    if (max_candidates == 0) {
        return false;
    }

    const cv::Mat source_mat = createMatView(source);
    const cv::Mat templ_mat = createMatView(templ);
    if (source_mat.empty() || templ_mat.empty()) {
        return false;
    }

    const cv::Rect roi = clampRegion(region, source);
    if (roi.width <= 0 || roi.height <= 0 || templ.width > roi.width || templ.height > roi.height) {
        return false;
    }

    cv::Mat mask_mat;
    cv::Mat norm_mask;
    if (mask != nullptr) {
        mask_mat = createMatView(*mask);
        if (mask_mat.empty()) {
            return false;
        }
    }

    cv::Mat norm_source;
    cv::Mat norm_templ;
    if (!prepareMatchInputs(source_mat(roi), templ_mat, color_mode, norm_source, norm_templ)) {
        return false;
    }
    if (!mask_mat.empty() && !prepareMaskInput(mask_mat, norm_templ, norm_mask)) {
        return false;
    }

    cv::Mat match_result;
    if (!norm_mask.empty()) {
        cv::matchTemplate(norm_source, norm_templ, match_result, method, norm_mask);
    } else {
        cv::matchTemplate(norm_source, norm_templ, match_result, method);
    }
    if (match_result.empty()) {
        return false;
    }

    const double clamped_threshold = std::clamp(threshold, 0.0, 1.0);
    const bool lower_is_better = method == cv::TM_SQDIFF || method == cv::TM_SQDIFF_NORMED;
    const double raw_threshold = lower_is_better ? 1.0 - clamped_threshold : clamped_threshold;
    cv::Mat available(match_result.size(), CV_8UC1, cv::Scalar(255));
    const cv::Rect result_bounds(0, 0, match_result.cols, match_result.rows);
    const int suppress_half_width = std::max(1, templ.width / 2);
    const int suppress_half_height = std::max(1, templ.height / 2);

    results.reserve(max_candidates);
    while (cv::countNonZero(available) > 0) {
        double min_score = 0.0;
        double max_score = 0.0;
        cv::Point min_location;
        cv::Point max_location;
        cv::minMaxLoc(match_result, &min_score, &max_score, &min_location, &max_location, available);

        const cv::Point location = lower_is_better ? min_location : max_location;
        const double raw_score_value = lower_is_better ? min_score : max_score;
        if ((lower_is_better && raw_score_value > raw_threshold) ||
            (!lower_is_better && raw_score_value < raw_threshold)) {
            break;
        }

        if (results.size() >= max_candidates) {
            if (saturated != nullptr) {
                *saturated = true;
            }
            break;
        }

        opcv::MatchResult match;
        match.x = roi.x + location.x;
        match.y = roi.y + location.y;
        match.width = templ.width;
        match.height = templ.height;
        match.score = convertMatchScoreToSimilarity(method, static_cast<float>(raw_score_value));
        results.push_back(match);

        cv::Rect suppress_region(
            location.x - suppress_half_width,
            location.y - suppress_half_height,
            suppress_half_width * 2 + 1,
            suppress_half_height * 2 + 1);
        suppress_region &= result_bounds;
        if (suppress_region.empty()) {
            break;
        }
        available(suppress_region).setTo(0);
    }

    return !results.empty();
}

opcv::Region buildRefineRegion(
    const opcv::MatchResult &coarse_match,
    const opcv::ImageHandle &source,
    const opcv::ImageHandle &templ,
    int level) {
    const int scaled_x = coarse_match.x * 2;
    const int scaled_y = coarse_match.y * 2;
    const int radius = std::max(8, std::max(templ.width, templ.height) / 3 + level * 2);

    opcv::Region region;
    region.x = scaled_x - radius;
    region.y = scaled_y - radius;
    region.width = templ.width + radius * 2 + 2;
    region.height = templ.height + radius * 2 + 2;
    return rectToRegion(clampRegion(region, source));
}

bool pyramidFindFirstMatch(
    const opcv::ImageHandle &source,
    const opcv::ImageHandle &templ,
    const opcv::ImageHandle *mask,
    double threshold,
    opcv::SearchDirection dir,
    int method,
    opcv::MatchColorMode color_mode,
    opcv::MatchResult &result,
    bool *definitive_miss = nullptr) {
    std::vector<opcv::MatchResult> results;
    if (!pyramidFindAllMatches(source, templ, mask, threshold, dir, method, color_mode, kPyramidCoarseCandidates,
                               results, false, kPyramidCoarseThresholdRelax, nullptr, definitive_miss)) {
        return false;
    }

    result = results.front();
    return true;
}

bool pyramidFindAllMatches(
    const opcv::ImageHandle &source,
    const opcv::ImageHandle &templ,
    const opcv::ImageHandle *mask,
    double threshold,
    opcv::SearchDirection dir,
    int method,
    opcv::MatchColorMode color_mode,
    size_t max_candidates,
    std::vector<opcv::MatchResult> &results,
    bool use_peak_candidates,
    double coarse_threshold_relax,
    bool *saturated,
    bool *definitive_miss) {
    results.clear();
    if (saturated != nullptr) {
        *saturated = false;
    }
    if (definitive_miss != nullptr) {
        *definitive_miss = false;
    }

    std::vector<opcv::ImageHandle> source_levels;
    std::vector<opcv::ImageHandle> templ_levels;
    std::vector<opcv::ImageHandle> mask_levels;
    if (!buildPyramidLevels(source, templ, mask, source_levels, templ_levels, mask_levels)) {
        return false;
    }

    const int top_level = static_cast<int>(source_levels.size()) - 1;
    const opcv::ImageHandle *top_mask = mask != nullptr ? &mask_levels[static_cast<size_t>(top_level)] : nullptr;
    const double coarse_threshold = std::max(0.0, std::clamp(threshold, 0.0, 1.0) - coarse_threshold_relax);

    std::vector<opcv::MatchResult> candidates;
    bool top_saturated = false;
    const bool collected_candidates = use_peak_candidates
        ? collectPeakCandidates(
              source_levels[static_cast<size_t>(top_level)],
              templ_levels[static_cast<size_t>(top_level)],
              top_mask,
              {0, 0, source_levels[static_cast<size_t>(top_level)].width,
               source_levels[static_cast<size_t>(top_level)].height},
              coarse_threshold,
              method,
              color_mode,
              max_candidates,
              candidates,
              &top_saturated)
        : collectBestCandidates(
            source_levels[static_cast<size_t>(top_level)],
            templ_levels[static_cast<size_t>(top_level)],
            top_mask,
            {0, 0, source_levels[static_cast<size_t>(top_level)].width,
             source_levels[static_cast<size_t>(top_level)].height},
            coarse_threshold,
            method,
            color_mode,
            max_candidates,
            candidates,
            &top_saturated);
    if (!collected_candidates) {
        if (definitive_miss != nullptr) {
            std::vector<opcv::MatchResult> best_probe;
            if (collectBestCandidates(
                    source_levels[static_cast<size_t>(top_level)],
                    templ_levels[static_cast<size_t>(top_level)],
                    top_mask,
                    {0, 0, source_levels[static_cast<size_t>(top_level)].width,
                     source_levels[static_cast<size_t>(top_level)].height},
                    0.0,
                    method,
                    color_mode,
                    1,
                    best_probe) &&
                !best_probe.empty()) {
                const double miss_threshold =
                    std::max(0.0, std::clamp(threshold, 0.0, 1.0) - kPyramidDefinitiveMissThresholdRelax);
                *definitive_miss = best_probe.front().score < miss_threshold;
            }
        }
        return false;
    }
    if (saturated != nullptr && top_saturated) {
        *saturated = true;
    }
    if (definitive_miss != nullptr && !candidates.empty()) {
        const double miss_threshold =
            std::max(0.0, std::clamp(threshold, 0.0, 1.0) - kPyramidDefinitiveMissThresholdRelax);
        if (candidates.front().score < miss_threshold) {
            *definitive_miss = true;
            return false;
        }
    }

    for (int level = top_level - 1; level >= 0; --level) {
        std::vector<opcv::MatchResult> refined;
        for (const auto &candidate : candidates) {
            const opcv::ImageHandle *level_mask = mask != nullptr ? &mask_levels[static_cast<size_t>(level)] : nullptr;
            std::vector<opcv::MatchResult> local_matches;
            if (!collectRegionThresholdMatches(
                    source_levels[static_cast<size_t>(level)],
                    templ_levels[static_cast<size_t>(level)],
                    level_mask,
                    buildRefineRegion(candidate, source_levels[static_cast<size_t>(level)],
                                      templ_levels[static_cast<size_t>(level)], level),
                    level == 0 ? threshold : coarse_threshold,
                    method,
                    color_mode,
                    local_matches)) {
                continue;
            }
            refined.insert(refined.end(), local_matches.begin(), local_matches.end());
        }

        if (refined.empty()) {
            return false;
        }

        dedupeMatchResults(refined);
        suppressOverlappingMatchResults(refined, dir);
        if (refined.size() > max_candidates && saturated != nullptr) {
            *saturated = true;
        }
        if (refined.size() > max_candidates) {
            refined.resize(max_candidates);
        }
        candidates.swap(refined);
    }

    dedupeMatchResults(candidates);
    suppressOverlappingMatchResults(candidates, dir);
    results.swap(candidates);
    return !results.empty();
}

// 复用单个 ORB 实例，避免重复构造。
cv::Ptr<cv::ORB> getOrbDetector() {
    static cv::Ptr<cv::ORB> detector = cv::ORB::create(1000, 1.2f, 8, 5, 0, 2, cv::ORB::HARRIS_SCORE, 15, 10);
    return detector;
}

// 从灰度图提取 ORB 特征。
bool extractOrbFeatures(const opcv::ImageHandle &gray, std::vector<cv::KeyPoint> &keypoints, cv::Mat &descriptors) {
    keypoints.clear();
    descriptors.release();

    if (gray.channels != 1) {
        return false;
    }

    const cv::Mat gray_mat = createMatView(gray);
    if (gray_mat.empty()) {
        return false;
    }

    auto detector = getOrbDetector();
    detector->detectAndCompute(gray_mat, cv::noArray(), keypoints, descriptors);
    return !keypoints.empty() && !descriptors.empty();
}

bool ensureTemplateFeatures(TemplateEntry &entry) {
    if (entry.has_feature_cache) {
        return !entry.feature_keypoints.empty() && !entry.feature_descriptors.empty();
    }

    if (extractOrbFeatures(entry.gray, entry.feature_keypoints, entry.feature_descriptors)) {
        entry.has_feature_cache = true;
        return true;
    }

    entry.feature_keypoints.clear();
    entry.feature_descriptors.release();
    entry.has_feature_cache = false;
    return false;
}

bool ensureTemplateEdge(TemplateEntry &entry) {
    if (entry.has_edge_cache) {
        return entry.edge.channels == 1 && !entry.edge.bytes.empty();
    }

    if (toEdge(entry.color, entry.edge)) {
        entry.has_edge_cache = true;
        return true;
    }

    entry.edge = {};
    entry.has_edge_cache = false;
    return false;
}

bool ensureTemplateShape(TemplateEntry &entry) {
    if (entry.has_shape_cache) {
        return !entry.shape_contour.empty();
    }

    opcv::ImageHandle shape_mask;
    if (toShapeMask(entry.color, shape_mask) && extractLargestContour(shape_mask, entry.shape_contour)) {
        entry.has_shape_cache = true;
        return true;
    }

    entry.shape_contour.clear();
    entry.has_shape_cache = false;
    return false;
}

// 在 ROI 内执行 ORB 特征匹配并生成目标包围框。
bool featureMatchRegion(
    const opcv::ImageHandle &source,
    const std::vector<cv::KeyPoint> &templ_keypoints,
    const cv::Mat &templ_descriptors,
    int templ_width,
    int templ_height,
    const opcv::Region &region,
    double threshold,
    opcv::MatchResult &result) {
    result = {};

    opcv::ImageHandle source_gray;
    if (!toGray(source, source_gray)) {
        return false;
    }

    const cv::Mat source_mat = createMatView(source_gray);
    if (source_mat.empty()) {
        return false;
    }

    const cv::Rect roi = clampRegion(region, source_gray);
    if (roi.width <= 0 || roi.height <= 0) {
        return false;
    }

    const cv::Mat cropped = source_mat(roi);
    if (cropped.empty()) {
        return false;
    }

    std::vector<cv::KeyPoint> source_keypoints;
    cv::Mat source_descriptors;
    auto detector = getOrbDetector();
    detector->detectAndCompute(cropped, cv::noArray(), source_keypoints, source_descriptors);
    if (source_keypoints.empty() || source_descriptors.empty()) {
        return false;
    }

    cv::BFMatcher matcher(cv::NORM_HAMMING, false);
    std::vector<std::vector<cv::DMatch>> knn_matches;
    matcher.knnMatch(templ_descriptors, source_descriptors, knn_matches, 2);

    std::vector<cv::DMatch> good_matches;
    good_matches.reserve(knn_matches.size());
    for (const auto &pair : knn_matches) {
        if (pair.size() < 2) {
            continue;
        }
        if (pair[0].distance < 0.75f * pair[1].distance) {
            good_matches.push_back(pair[0]);
        }
    }

    if (good_matches.size() < 4) {
        cv::BFMatcher cross_matcher(cv::NORM_HAMMING, true);
        std::vector<cv::DMatch> cross_matches;
        cross_matcher.match(templ_descriptors, source_descriptors, cross_matches);
        if (!cross_matches.empty()) {
            std::sort(
                cross_matches.begin(),
                cross_matches.end(),
                [](const cv::DMatch &lhs, const cv::DMatch &rhs) { return lhs.distance < rhs.distance; });

            const float distance_limit = std::max(32.0f, cross_matches.front().distance * 2.5f);
            for (const auto &match : cross_matches) {
                if (match.distance > distance_limit) {
                    break;
                }
                good_matches.push_back(match);
                if (good_matches.size() >= 12) {
                    break;
                }
            }
        }
    }

    if (good_matches.size() < 3) {
        return false;
    }

    if (good_matches.size() >= 4 && templ_width >= 64 && templ_height >= 64) {
        std::vector<cv::Point2f> templ_points;
        std::vector<cv::Point2f> source_points;
        templ_points.reserve(good_matches.size());
        source_points.reserve(good_matches.size());
        for (const auto &match : good_matches) {
            templ_points.push_back(templ_keypoints[match.queryIdx].pt);
            source_points.push_back(source_keypoints[match.trainIdx].pt);
        }

        cv::Mat inlier_mask;
        int inlier_count = 0;
        const cv::Mat homography = cv::findHomography(templ_points, source_points, cv::RANSAC, 4.0, inlier_mask);
        if (!homography.empty() && inlier_mask.rows == static_cast<int>(good_matches.size())) {
            for (int index = 0; index < inlier_mask.rows; ++index) {
                if (inlier_mask.at<uchar>(index, 0) != 0) {
                    ++inlier_count;
                }
            }

            const double inlier_ratio =
                static_cast<double>(inlier_count) / static_cast<double>(std::max<size_t>(good_matches.size(), 1));
            if (inlier_count >= kFeatureMinGeometryInliers && inlier_ratio >= kFeatureMinInlierRatio) {
                std::vector<cv::Point2f> templ_corners = {
                    cv::Point2f(0.0f, 0.0f),
                    cv::Point2f(static_cast<float>(templ_width), 0.0f),
                    cv::Point2f(static_cast<float>(templ_width), static_cast<float>(templ_height)),
                    cv::Point2f(0.0f, static_cast<float>(templ_height)),
                };
                std::vector<cv::Point2f> projected_corners;
                cv::perspectiveTransform(templ_corners, projected_corners, homography);
                const cv::Rect projected_rect = cv::boundingRect(projected_corners);
                const double width_scale = static_cast<double>(projected_rect.width) / std::max(templ_width, 1);
                const double height_scale = static_cast<double>(projected_rect.height) / std::max(templ_height, 1);
                if (projected_rect.width > 0 && projected_rect.height > 0 &&
                    projected_rect.x + projected_rect.width >= 0 &&
                    projected_rect.y + projected_rect.height >= 0 &&
                    projected_rect.x < roi.width &&
                    projected_rect.y < roi.height &&
                    width_scale >= kFeatureMinBoundsScale &&
                    height_scale >= kFeatureMinBoundsScale &&
                    width_scale <= kFeatureMaxBoundsScale &&
                    height_scale <= kFeatureMaxBoundsScale) {
                    const double keypoint_score = static_cast<double>(good_matches.size()) /
                                                  static_cast<double>(std::max<size_t>(templ_keypoints.size(), 1));
                    const double geometry_score = std::clamp(0.5 * keypoint_score + 0.5 * inlier_ratio, 0.0, 1.0);
                    if (geometry_score >= std::clamp(threshold, 0.0, 1.0)) {
                        result.x = roi.x + projected_rect.x;
                        result.y = roi.y + projected_rect.y;
                        result.width = projected_rect.width;
                        result.height = projected_rect.height;
                        result.score = geometry_score;
                        return true;
                    }
                }
            }
        }
    }

    std::vector<float> offsets_x;
    std::vector<float> offsets_y;
    offsets_x.reserve(good_matches.size());
    offsets_y.reserve(good_matches.size());
    for (const auto &match : good_matches) {
        const cv::Point2f source_pt = source_keypoints[match.trainIdx].pt;
        const cv::Point2f templ_pt = templ_keypoints[match.queryIdx].pt;
        offsets_x.push_back(source_pt.x - templ_pt.x);
        offsets_y.push_back(source_pt.y - templ_pt.y);
    }

    if (offsets_x.empty() || offsets_y.empty()) {
        return false;
    }

    const auto middle_x = offsets_x.begin() + static_cast<std::ptrdiff_t>(offsets_x.size() / 2);
    const auto middle_y = offsets_y.begin() + static_cast<std::ptrdiff_t>(offsets_y.size() / 2);
    std::nth_element(offsets_x.begin(), middle_x, offsets_x.end());
    std::nth_element(offsets_y.begin(), middle_y, offsets_y.end());

    const float match_x = *middle_x;
    const float match_y = *middle_y;

    const double score =
        static_cast<double>(good_matches.size()) / static_cast<double>(std::max<size_t>(templ_keypoints.size(), 1));
    const double clamped_score = std::clamp(score, 0.0, 1.0);
    if (clamped_score < std::clamp(threshold, 0.0, 1.0)) {
        return false;
    }

    result.x = roi.x + static_cast<int>(std::round(match_x));
    result.y = roi.y + static_cast<int>(std::round(match_y));
    result.width = templ_width;
    result.height = templ_height;
    result.score = clamped_score;
    return true;
}

// 在 ROI 内执行轮廓/形状匹配并生成目标包围框。
bool shapeMatchRegion(
    const opcv::ImageHandle &source,
    const std::vector<cv::Point> &templ_contour,
    const opcv::Region &region,
    double threshold,
    opcv::MatchResult &result) {
    result = {};
    if (templ_contour.size() < 4) {
        return false;
    }

    const cv::Rect roi = clampRegion(region, source);
    if (roi.width <= 0 || roi.height <= 0) {
        return false;
    }

    const cv::Mat source_mat = createMatView(source);
    if (source_mat.empty()) {
        return false;
    }

    const cv::Mat cropped = source_mat(roi).clone();
    if (cropped.empty()) {
        return false;
    }

    opcv::ImageHandle roi_image = createImageFromMat(cropped);
    opcv::ImageHandle shape_mask;
    if (!toShapeMask(roi_image, shape_mask)) {
        return false;
    }

    std::vector<std::vector<cv::Point>> contours;
    if (!extractCandidateContours(shape_mask, contours)) {
        return false;
    }

    const cv::Rect templ_rect = cv::boundingRect(templ_contour);
    const double templ_area = std::max(cv::contourArea(templ_contour), 1.0);
    const double templ_aspect =
        templ_rect.height > 0 ? static_cast<double>(templ_rect.width) / static_cast<double>(templ_rect.height) : 1.0;
    const double templ_diag = std::max(
        std::sqrt(static_cast<double>(templ_rect.width * templ_rect.width + templ_rect.height * templ_rect.height)),
        1.0);

    double best_similarity = -1.0;
    cv::Rect best_rect;
    for (const auto &candidate : contours) {
        const double raw_score = cv::matchShapes(templ_contour, candidate, cv::CONTOURS_MATCH_I1, 0.0);
        const double shape_similarity = 1.0 / (1.0 + std::max(raw_score, 0.0));
        const cv::Rect candidate_rect = cv::boundingRect(candidate);
        if (candidate_rect.width <= 0 || candidate_rect.height <= 0) {
            continue;
        }

        const double candidate_area = std::max(cv::contourArea(candidate), 1.0);
        const double candidate_aspect =
            static_cast<double>(candidate_rect.width) / static_cast<double>(candidate_rect.height);
        const double candidate_diag = std::sqrt(
            static_cast<double>(candidate_rect.width * candidate_rect.width + candidate_rect.height * candidate_rect.height));
        const double area_score = ratioPenalty(candidate_area / templ_area);
        const double aspect_score = ratioPenalty(candidate_aspect / templ_aspect);
        const double size_score = ratioPenalty(candidate_diag / templ_diag);
        if (area_score < kShapeMinAreaScore || aspect_score < kShapeMinAspectScore ||
            size_score < kShapeMinSizeScore) {
            continue;
        }
        const double penalty = (1.0 - kShapeAreaRatioWeight - kShapeAspectRatioWeight - kShapeSizeRatioWeight) +
                               kShapeAreaRatioWeight * area_score +
                               kShapeAspectRatioWeight * aspect_score +
                               kShapeSizeRatioWeight * size_score;
        const double similarity = shape_similarity * std::clamp(penalty, 0.0, 1.0);
        if (similarity <= best_similarity) {
            continue;
        }

        best_similarity = similarity;
        best_rect = candidate_rect;
    }

    const double clamped_threshold = std::clamp(threshold, 0.0, 1.0);
    if (best_similarity < clamped_threshold || best_rect.width <= 0 || best_rect.height <= 0) {
        return false;
    }

    result.x = roi.x + best_rect.x;
    result.y = roi.y + best_rect.y;
    result.width = best_rect.width;
    result.height = best_rect.height;
    result.score = best_similarity;
    return true;
}

// 在指定区域内对源图与模板图执行匹配，返回所有候选结果。
bool collectRegionCandidates(
    const opcv::ImageHandle &source,
    const opcv::ImageHandle &templ,
    const opcv::ImageHandle *mask,
    const opcv::Region &region,
    int method,
    opcv::MatchColorMode color_mode,
    std::vector<opcv::MatchCandidate> &matches) {
    const cv::Mat source_mat = createMatView(source);
    const cv::Mat templ_mat = createMatView(templ);
    if (source_mat.empty() || templ_mat.empty()) {
        return false;
    }

    const cv::Rect roi = clampRegion(region, source);
    if (roi.width <= 0 || roi.height <= 0) {
        return false;
    }
    if (templ.width > roi.width || templ.height > roi.height) {
        return false;
    }

    const cv::Mat cropped = source_mat(roi);
    cv::Mat mask_mat;
    const cv::Mat *mask_ptr = nullptr;
    if (mask != nullptr) {
        mask_mat = createMatView(*mask);
        if (mask_mat.empty()) {
            return false;
        }
        mask_ptr = &mask_mat;
    }

    return collectTemplateMatches(cropped, templ_mat, mask_ptr, roi.x, roi.y, matches, method, color_mode);
}

bool collectRegionThresholdMatches(
    const opcv::ImageHandle &source,
    const opcv::ImageHandle &templ,
    const opcv::ImageHandle *mask,
    const opcv::Region &region,
    double threshold,
    int method,
    opcv::MatchColorMode color_mode,
    std::vector<opcv::MatchResult> &results) {
    results.clear();

    const cv::Mat source_mat = createMatView(source);
    const cv::Mat templ_mat = createMatView(templ);
    if (source_mat.empty() || templ_mat.empty()) {
        return false;
    }

    const cv::Rect roi = clampRegion(region, source);
    if (roi.width <= 0 || roi.height <= 0 || templ.width > roi.width || templ.height > roi.height) {
        return false;
    }

    cv::Mat mask_mat;
    cv::Mat norm_mask;
    if (mask != nullptr) {
        mask_mat = createMatView(*mask);
        if (mask_mat.empty()) {
            return false;
        }
    }

    cv::Mat norm_source;
    cv::Mat norm_templ;
    if (!prepareMatchInputs(source_mat(roi), templ_mat, color_mode, norm_source, norm_templ)) {
        return false;
    }
    if (!mask_mat.empty() && !prepareMaskInput(mask_mat, norm_templ, norm_mask)) {
        return false;
    }

    cv::Mat match_result;
    if (!norm_mask.empty()) {
        cv::matchTemplate(norm_source, norm_templ, match_result, method, norm_mask);
    } else {
        cv::matchTemplate(norm_source, norm_templ, match_result, method);
    }
    if (match_result.empty()) {
        return false;
    }

    const double clamped_threshold = std::clamp(threshold, 0.0, 1.0);
    for (int y = 0; y < match_result.rows; ++y) {
        for (int x = 0; x < match_result.cols; ++x) {
            const double score = convertMatchScoreToSimilarity(method, match_result.at<float>(y, x));
            if (score < clamped_threshold) {
                continue;
            }

            opcv::MatchResult match;
            match.x = roi.x + x;
            match.y = roi.y + y;
            match.width = templ.width;
            match.height = templ.height;
            match.score = score;
            results.push_back(match);
        }
    }

    return !results.empty();
}

bool findBestPreparedMatch(
    const opcv::ImageHandle &source,
    const opcv::ImageHandle &templ,
    const opcv::ImageHandle *mask,
    const opcv::Region &region,
    double threshold,
    opcv::MatchResult &result,
    int method = 1,
    opcv::MatchColorMode color_mode = opcv::MatchColorMode::Gray) {
    result = {};

    const cv::Mat source_mat = createMatView(source);
    const cv::Mat templ_mat = createMatView(templ);
    if (source_mat.empty() || templ_mat.empty()) {
        return false;
    }

    const cv::Rect roi = clampRegion(region, source);
    if (roi.width <= 0 || roi.height <= 0 || templ.width > roi.width || templ.height > roi.height) {
        return false;
    }

    cv::Mat mask_mat;
    cv::Mat norm_mask;
    if (mask != nullptr) {
        mask_mat = createMatView(*mask);
        if (mask_mat.empty()) {
            return false;
        }
    }

    cv::Mat norm_source;
    cv::Mat norm_templ;
    if (!prepareMatchInputs(source_mat(roi), templ_mat, color_mode, norm_source, norm_templ)) {
        return false;
    }
    if (!mask_mat.empty() && !prepareMaskInput(mask_mat, norm_templ, norm_mask)) {
        return false;
    }

    cv::Mat match_result;
    if (!norm_mask.empty()) {
        cv::matchTemplate(norm_source, norm_templ, match_result, method, norm_mask);
    } else {
        cv::matchTemplate(norm_source, norm_templ, match_result, method);
    }
    if (match_result.empty()) {
        return false;
    }

    double min_score = 0.0;
    double max_score = 0.0;
    cv::Point min_location;
    cv::Point max_location;
    cv::minMaxLoc(match_result, &min_score, &max_score, &min_location, &max_location);

    const bool lower_is_better = method == cv::TM_SQDIFF || method == cv::TM_SQDIFF_NORMED;
    const cv::Point best_location = lower_is_better ? min_location : max_location;
    const float raw_score = static_cast<float>(lower_is_better ? min_score : max_score);

    result.x = roi.x + best_location.x;
    result.y = roi.y + best_location.y;
    result.width = templ.width;
    result.height = templ.height;
    result.score = convertMatchScoreToSimilarity(method, raw_score);
    return isMatchAboveThreshold(result, threshold);
}

bool findFirstNamedMatch(
    const opcv::ImageHandle &source,
    const std::wstring &template_name,
    const opcv::Region &region,
    double threshold,
    opcv::SearchDirection dir,
    int method,
    opcv::MatchColorMode color_mode,
    opcv::MatchResult &result) {
    result = {};

    const cv::Mat source_mat = createMatView(source);
    if (source_mat.empty()) {
        return false;
    }

    const cv::Rect roi = clampRegion(region, source);
    if (roi.width <= 0 || roi.height <= 0) {
        return false;
    }

    TemplateSnapshot snapshot;
    if (!getTemplateSnapshot(template_name, color_mode, snapshot)) {
        return false;
    }

    if (snapshot.templ.width > roi.width || snapshot.templ.height > roi.height) {
        return false;
    }

    const cv::Mat cropped = source_mat(roi);
    if (cropped.empty()) {
        return false;
    }

    cv::Mat mask_mat;
    const cv::Mat *mask_ptr = nullptr;
    if (snapshot.has_mask) {
        mask_mat = createMatView(snapshot.mask);
        if (mask_mat.empty()) {
            return false;
        }
        mask_ptr = &mask_mat;
    }

    return findFirstTemplateMatch(
        cropped,
        createMatView(snapshot.templ),
        mask_ptr,
        roi.x,
        roi.y,
        snapshot.templ.width,
        snapshot.templ.height,
        threshold,
        result,
        dir,
        method,
        color_mode);
}

// 只读取模板尺寸，避免条带模式为拿宽高先做一次整区域匹配。
bool getNamedTemplateSize(
    const std::wstring &template_name,
    opcv::MatchColorMode color_mode,
    int &templ_width,
    int &templ_height) {
    templ_width = 0;
    templ_height = 0;

    std::shared_lock<std::shared_mutex> lock(gTemplateStoreMutex);
    const auto it = gTemplateStore.find(template_name);
    if (it == gTemplateStore.end()) {
        return false;
    }

    const opcv::ImageHandle &templ =
        color_mode == opcv::MatchColorMode::Gray ? it->second.gray : it->second.color;
    if (templ.width <= 0 || templ.height <= 0) {
        return false;
    }

    templ_width = templ.width;
    templ_height = templ.height;
    return true;
}

// 按区域收集单个模板所有达到阈值的命中结果。
bool collectNamedThresholdMatches(
    const opcv::ImageHandle &source,
    const std::wstring &template_name,
    const opcv::Region &region,
    double threshold,
    opcv::SearchDirection dir,
    int method,
    opcv::MatchColorMode color_mode,
    std::vector<opcv::MatchResult> &results) {
    results.clear();

    TemplateSnapshot snapshot;
    if (!getTemplateSnapshot(template_name, color_mode, snapshot)) {
        return false;
    }

    const opcv::ImageHandle *mask_ptr = snapshot.has_mask ? &snapshot.mask : nullptr;
    if (!collectRegionThresholdMatches(source, snapshot.templ, mask_ptr, region, threshold, method, color_mode,
                                       results)) {
        return false;
    }

    suppressOverlappingMatchResults(results, dir);
    return !results.empty();
}

std::wstring normalizeScaleKey(double scale) {
    std::wostringstream oss;
    oss << std::fixed << std::setprecision(6) << scale;
    return oss.str();
}

bool buildScaledTemplateEntry(
    const TemplateEntry &base_entry,
    double scale,
    TemplateEntry::ScaledTemplateEntry &scaled_entry) {
    if (scale <= 0.0) {
        return false;
    }

    const int width = std::max(1, static_cast<int>(std::lround(base_entry.color.width * scale)));
    const int height = std::max(1, static_cast<int>(std::lround(base_entry.color.height * scale)));

    if (!resizeImage(base_entry.color, width, height, scaled_entry.color)) {
        return false;
    }
    if (!toGray(scaled_entry.color, scaled_entry.gray)) {
        return false;
    }

    scaled_entry.mask = {};
    scaled_entry.has_mask = false;
    if (base_entry.has_mask) {
        opcv::ImageHandle resized_mask;
        if (!resizeImage(base_entry.mask, width, height, resized_mask)) {
            return false;
        }
        if (!toMask(resized_mask, scaled_entry.mask)) {
            return false;
        }
        scaled_entry.has_mask = true;
    }

    return true;
}

struct ScaleProbeCandidate {
    double scale = 1.0;
    double score = 0.0;
    size_t original_index = 0;
};

struct ScaleProbeSourceCache {
    std::vector<cv::Mat> levels;
};

bool getScaledTemplateSnapshot(
    const std::wstring &template_name,
    double scale,
    opcv::MatchColorMode color_mode,
    opcv::ImageHandle &templ,
    opcv::ImageHandle &mask,
    bool &has_mask) {
    templ = {};
    mask = {};
    has_mask = false;
    if (scale <= 0.0) {
        return false;
    }

    std::unique_lock<std::shared_mutex> lock(gTemplateStoreMutex);
    const auto it = gTemplateStore.find(template_name);
    if (it == gTemplateStore.end()) {
        return false;
    }

    TemplateEntry &entry = it->second;
    if (std::abs(scale - 1.0) < 1e-6) {
        templ = color_mode == opcv::MatchColorMode::Gray ? entry.gray : entry.color;
        if (entry.has_mask) {
            mask = entry.mask;
            has_mask = true;
        }
        return templ.width > 0 && templ.height > 0;
    }

    const std::wstring scale_key = normalizeScaleKey(scale);
    auto scaled_it = entry.scaled_templates.find(scale_key);
    if (scaled_it == entry.scaled_templates.end()) {
        TemplateEntry::ScaledTemplateEntry scaled_entry;
        if (!buildScaledTemplateEntry(entry, scale, scaled_entry)) {
            return false;
        }
        scaled_it = entry.scaled_templates.emplace(scale_key, std::move(scaled_entry)).first;
    }

    templ = color_mode == opcv::MatchColorMode::Gray ? scaled_it->second.gray : scaled_it->second.color;
    if (scaled_it->second.has_mask) {
        mask = scaled_it->second.mask;
        has_mask = true;
    }
    return templ.width > 0 && templ.height > 0;
}

bool probeScaleCandidate(
    const opcv::ImageHandle &source,
    const opcv::ImageHandle &templ,
    const opcv::ImageHandle *mask,
    const opcv::Region &region,
    int method,
    opcv::MatchColorMode color_mode,
    double &score) {
    score = 0.0;

    const cv::Mat source_mat = createMatView(source);
    const cv::Mat templ_mat = createMatView(templ);
    if (source_mat.empty() || templ_mat.empty() || !isSupportedMatchMethod(method)) {
        return false;
    }

    const cv::Rect roi = clampRegion(region, source);
    if (roi.width <= 0 || roi.height <= 0 || templ.width > roi.width || templ.height > roi.height) {
        return false;
    }

    cv::Rect probe_roi = roi;
    cv::Mat probe_source = source_mat(probe_roi);
    cv::Mat probe_templ = templ_mat;
    cv::Mat probe_mask;
    if (mask != nullptr) {
        probe_mask = createMatView(*mask);
        if (probe_mask.empty()) {
            return false;
        }
    }

    while (probe_templ.cols >= kPyramidMinCoarseTemplateSize * 2 &&
           probe_templ.rows >= kPyramidMinCoarseTemplateSize * 2 &&
           probe_source.cols / 2 >= std::max(1, probe_templ.cols / 2) &&
           probe_source.rows / 2 >= std::max(1, probe_templ.rows / 2)) {
        cv::Mat next_source;
        cv::Mat next_templ;
        cv::resize(probe_source, next_source, cv::Size(probe_source.cols / 2, probe_source.rows / 2), 0.0, 0.0,
                   cv::INTER_AREA);
        cv::resize(probe_templ, next_templ,
                   cv::Size(std::max(1, static_cast<int>(std::lround(static_cast<double>(probe_templ.cols) / 2.0))),
                            std::max(1, static_cast<int>(std::lround(static_cast<double>(probe_templ.rows) / 2.0)))),
                   0.0, 0.0, cv::INTER_AREA);
        probe_source = std::move(next_source);
        probe_templ = std::move(next_templ);
        if (!probe_mask.empty()) {
            cv::Mat next_mask;
            cv::resize(probe_mask, next_mask, probe_templ.size(), 0.0, 0.0, cv::INTER_NEAREST);
            probe_mask = std::move(next_mask);
        }
    }

    cv::Mat norm_source;
    cv::Mat norm_templ;
    if (!prepareMatchInputs(probe_source, probe_templ, color_mode, norm_source, norm_templ)) {
        return false;
    }

    cv::Mat norm_mask;
    if (!probe_mask.empty() && !prepareMaskInput(probe_mask, norm_templ, norm_mask)) {
        return false;
    }

    cv::Mat match_result;
    if (!norm_mask.empty()) {
        cv::matchTemplate(norm_source, norm_templ, match_result, method, norm_mask);
    } else {
        cv::matchTemplate(norm_source, norm_templ, match_result, method);
    }
    if (match_result.empty()) {
        return false;
    }

    double min_score = 0.0;
    double max_score = 0.0;
    cv::Point min_location;
    cv::Point max_location;
    cv::minMaxLoc(match_result, &min_score, &max_score, &min_location, &max_location);
    const bool lower_is_better = method == cv::TM_SQDIFF || method == cv::TM_SQDIFF_NORMED;
    score = convertMatchScoreToSimilarity(method, static_cast<float>(lower_is_better ? min_score : max_score));
    return true;
}

bool prepareScaleProbeSourceCache(
    const opcv::ImageHandle &source,
    const opcv::Region &region,
    ScaleProbeSourceCache &cache) {
    cache = {};

    const cv::Mat source_mat = createMatView(source);
    if (source_mat.empty()) {
        return false;
    }

    const cv::Rect roi = clampRegion(region, source);
    if (roi.width <= 0 || roi.height <= 0) {
        return false;
    }

    cache.levels.push_back(source_mat(roi));
    while (static_cast<int>(cache.levels.size()) <= kPyramidMaxLevels) {
        const cv::Mat &previous = cache.levels.back();
        if (previous.cols <= 1 || previous.rows <= 1) {
            break;
        }

        cv::Mat next_source;
        cv::resize(previous, next_source, cv::Size(previous.cols / 2, previous.rows / 2), 0.0, 0.0,
                   cv::INTER_AREA);
        cache.levels.push_back(std::move(next_source));
    }

    return !cache.levels.empty();
}

int scaleProbeDownsampleSteps(const cv::Mat &source, int templ_width, int templ_height) {
    int steps = 0;
    int source_width = source.cols;
    int source_height = source.rows;
    while (templ_width >= kPyramidMinCoarseTemplateSize * 2 &&
           templ_height >= kPyramidMinCoarseTemplateSize * 2 &&
           source_width / 2 >= std::max(1, templ_width / 2) &&
           source_height / 2 >= std::max(1, templ_height / 2)) {
        source_width /= 2;
        source_height /= 2;
        templ_width = std::max(1, static_cast<int>(std::lround(static_cast<double>(templ_width) / 2.0)));
        templ_height = std::max(1, static_cast<int>(std::lround(static_cast<double>(templ_height) / 2.0)));
        ++steps;
    }
    return steps;
}

bool probeScaleCandidate(
    const ScaleProbeSourceCache &source_cache,
    const opcv::ImageHandle &templ,
    const opcv::ImageHandle *mask,
    int method,
    opcv::MatchColorMode color_mode,
    double &score) {
    score = 0.0;

    const cv::Mat templ_mat = createMatView(templ);
    if (source_cache.levels.empty() || templ_mat.empty() || !isSupportedMatchMethod(method)) {
        return false;
    }

    int downsample_steps = scaleProbeDownsampleSteps(source_cache.levels.front(), templ.width, templ.height);
    downsample_steps = std::min(downsample_steps, static_cast<int>(source_cache.levels.size()) - 1);

    cv::Mat probe_templ = templ_mat;
    cv::Mat probe_mask;
    if (mask != nullptr) {
        probe_mask = createMatView(*mask);
        if (probe_mask.empty()) {
            return false;
        }
    }

    for (int step = 0; step < downsample_steps; ++step) {
        cv::Mat next_templ;
        cv::resize(probe_templ, next_templ,
                   cv::Size(std::max(1, static_cast<int>(std::lround(static_cast<double>(probe_templ.cols) / 2.0))),
                            std::max(1, static_cast<int>(std::lround(static_cast<double>(probe_templ.rows) / 2.0)))),
                   0.0, 0.0, cv::INTER_AREA);
        probe_templ = std::move(next_templ);
        if (!probe_mask.empty()) {
            cv::Mat next_mask;
            cv::resize(probe_mask, next_mask, probe_templ.size(), 0.0, 0.0, cv::INTER_NEAREST);
            probe_mask = std::move(next_mask);
        }
    }

    const cv::Mat &probe_source = source_cache.levels[static_cast<size_t>(downsample_steps)];
    if (probe_templ.cols > probe_source.cols || probe_templ.rows > probe_source.rows) {
        return false;
    }

    cv::Mat norm_source;
    cv::Mat norm_templ;
    if (!prepareMatchInputs(probe_source, probe_templ, color_mode, norm_source, norm_templ)) {
        return false;
    }

    cv::Mat norm_mask;
    if (!probe_mask.empty() && !prepareMaskInput(probe_mask, norm_templ, norm_mask)) {
        return false;
    }

    cv::Mat match_result;
    if (!norm_mask.empty()) {
        cv::matchTemplate(norm_source, norm_templ, match_result, method, norm_mask);
    } else {
        cv::matchTemplate(norm_source, norm_templ, match_result, method);
    }
    if (match_result.empty()) {
        return false;
    }

    double min_score = 0.0;
    double max_score = 0.0;
    cv::Point min_location;
    cv::Point max_location;
    cv::minMaxLoc(match_result, &min_score, &max_score, &min_location, &max_location);
    const bool lower_is_better = method == cv::TM_SQDIFF || method == cv::TM_SQDIFF_NORMED;
    score = convertMatchScoreToSimilarity(method, static_cast<float>(lower_is_better ? min_score : max_score));
    return true;
}

std::vector<ScaleProbeCandidate> probeAutomaticScaleCandidates(
    const opcv::ImageHandle &source,
    const std::wstring &template_name,
    const opcv::Region &region,
    const std::vector<double> &scale_values,
    size_t first_index,
    size_t last_index,
    int method,
    opcv::MatchColorMode color_mode) {
    const size_t start_index = std::min(first_index, scale_values.size());
    const size_t end_index = std::min(last_index, scale_values.size());

    std::vector<ScaleProbeCandidate> probed;
    ScaleProbeSourceCache source_cache;
    const bool has_source_cache = prepareScaleProbeSourceCache(source, region, source_cache);

    for (size_t index = start_index; index < end_index; ++index) {
        opcv::ImageHandle templ;
        opcv::ImageHandle mask;
        bool has_mask = false;
        if (!getScaledTemplateSnapshot(template_name, scale_values[index], color_mode, templ, mask, has_mask)) {
            continue;
        }

        double score = 0.0;
        const opcv::ImageHandle *mask_ptr = has_mask ? &mask : nullptr;
        const bool probed_ok =
            has_source_cache
                ? probeScaleCandidate(source_cache, templ, mask_ptr, method, color_mode, score)
                : probeScaleCandidate(source, templ, mask_ptr, region, method, color_mode, score);
        if (!probed_ok) {
            continue;
        }

        ScaleProbeCandidate candidate;
        candidate.scale = scale_values[index];
        candidate.score = score;
        candidate.original_index = index;
        probed.push_back(candidate);
    }

    std::stable_sort(probed.begin(), probed.end(), [](const ScaleProbeCandidate &lhs, const ScaleProbeCandidate &rhs) {
        if (lhs.score != rhs.score) {
            return lhs.score > rhs.score;
        }
        return lhs.original_index < rhs.original_index;
    });
    return probed;
}

std::vector<double> selectProbeScaleValues(
    const std::vector<ScaleProbeCandidate> &probed,
    size_t max_candidates) {
    std::vector<double> ordered;
    ordered.reserve(std::min(max_candidates, probed.size()));
    size_t appended = 0;
    for (const auto &candidate : probed) {
        ordered.push_back(candidate.scale);
        ++appended;
        if (appended >= max_candidates) {
            break;
        }
    }
    return ordered;
}

std::vector<double> buildAutomaticScaleProbeOrder(
    const opcv::ImageHandle &source,
    const std::wstring &template_name,
    const opcv::Region &region,
    const std::vector<double> &scale_values,
    size_t first_index,
    int method,
    opcv::MatchColorMode color_mode) {
    const auto probed = probeAutomaticScaleCandidates(
        source,
        template_name,
        region,
        scale_values,
        first_index,
        scale_values.size(),
        method,
        color_mode);
    return selectProbeScaleValues(probed, kScaleAutoMaxProbeRefineCandidates);
}

bool refineScaleCandidate(
    const opcv::ImageHandle &source,
    const std::wstring &template_name,
    const opcv::Region &region,
    const cv::Rect &roi,
    double scale,
    double threshold,
    int method,
    opcv::MatchColorMode color_mode,
    opcv::MatchResult &best_result,
    bool &found) {
    if (scale <= 0.0) {
        return true;
    }

    opcv::ImageHandle templ;
    opcv::ImageHandle mask;
    bool has_mask = false;
    if (!getScaledTemplateSnapshot(template_name, scale, color_mode, templ, mask, has_mask)) {
        return false;
    }

    if (templ.width > roi.width || templ.height > roi.height) {
        return true;
    }

    opcv::MatchResult scaled_result;
    const opcv::ImageHandle *mask_ptr = has_mask ? &mask : nullptr;
    if (!findBestPreparedMatch(source, templ, mask_ptr, region, threshold, scaled_result, method, color_mode)) {
        return true;
    }

    if (!found || scaled_result.score > best_result.score) {
        best_result = scaled_result;
        found = true;
    }
    return true;
}

bool refineScaleCandidates(
    const opcv::ImageHandle &source,
    const std::wstring &template_name,
    const opcv::Region &region,
    const cv::Rect &roi,
    const std::vector<double> &scale_values,
    double threshold,
    int method,
    opcv::MatchColorMode color_mode,
    bool stop_on_high_score,
    opcv::MatchResult &best_result,
    bool &found) {
    for (double scale : scale_values) {
        if (!refineScaleCandidate(source, template_name, region, roi, scale, threshold, method, color_mode, best_result,
                                  found)) {
            return false;
        }
        if (stop_on_high_score && found && best_result.score >= kScaleAutoEarlyStopSimilarity) {
            break;
        }
    }
    return true;
}

// 在非条带模式下并行查找任意模板；一旦任意模板命中，则返回该结果。
bool findAnyNamedMatchParallel(
    const opcv::ImageHandle &source,
    const std::vector<std::wstring> &template_names,
    const opcv::Region &region,
    double threshold,
    opcv::SearchDirection dir,
    int method,
    opcv::MatchColorMode color_mode,
    opcv::NamedMatchResult &result) {
    result = {};
    if (template_names.empty()) {
        return false;
    }

    const size_t worker_count =
        std::min(template_names.size(), static_cast<size_t>(std::max(1u, std::thread::hardware_concurrency())));
    std::atomic<size_t> next_index(0);
    std::atomic<bool> found(false);
    std::mutex result_mutex;
    opcv::NamedMatchResult found_result;

    ThreadPool &pool = getOpenCvThreadPool();
    std::vector<std::future<void>> tasks;
    tasks.reserve(worker_count);
    for (size_t worker = 0; worker < worker_count; ++worker) {
        tasks.push_back(pool.enqueue([&, worker]() {
            while (!found.load(std::memory_order_acquire)) {
                const size_t index = next_index.fetch_add(1, std::memory_order_relaxed);
                if (index >= template_names.size()) {
                    break;
                }

                opcv::MatchResult local_match;
                if (!findFirstNamedMatch(source, template_names[index], region, threshold, dir, method, color_mode,
                                         local_match)) {
                    continue;
                }

                bool expected = false;
                if (found.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
                    std::lock_guard<std::mutex> lock(result_mutex);
                    found_result.name = template_names[index];
                    found_result.match = local_match;
                }
                break;
            }
        }));
    }

    for (auto &task : tasks) {
        task.get();
    }

    if (!found.load(std::memory_order_acquire)) {
        return false;
    }

    result = found_result;
    return true;
}

// 在条带模式下并行查找任意一个命中；一旦命中，则停止分发后续条带。
// 说明：这里不保证返回方向上的首个命中，只保证返回一个满足阈值的命中结果。
bool findAnyStripMatch(
    const opcv::ImageHandle &source,
    const std::wstring &template_name,
    const std::vector<opcv::Region> &strips,
    double threshold,
    opcv::SearchDirection dir,
    int method,
    opcv::MatchColorMode color_mode,
    opcv::MatchResult &result) {
    result = {};
    if (strips.empty()) {
        return false;
    }

    const size_t worker_count =
        std::min(strips.size(), static_cast<size_t>(std::max(1u, std::thread::hardware_concurrency())));
    std::atomic<size_t> next_index(0);
    std::atomic<bool> found(false);
    std::mutex result_mutex;
    opcv::MatchResult found_result;

    ThreadPool &pool = getOpenCvThreadPool();
    std::vector<std::future<void>> tasks;
    tasks.reserve(worker_count);
    for (size_t worker = 0; worker < worker_count; ++worker) {
        tasks.push_back(pool.enqueue([&, worker]() {
            while (!found.load(std::memory_order_acquire)) {
                const size_t index = next_index.fetch_add(1, std::memory_order_relaxed);
                if (index >= strips.size()) {
                    break;
                }

                opcv::MatchResult local_match;
                if (!findFirstNamedMatch(source, template_name, strips[index], threshold, dir, method, color_mode,
                                         local_match)) {
                    continue;
                }

                bool expected = false;
                if (found.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
                    std::lock_guard<std::mutex> lock(result_mutex);
                    found_result = local_match;
                }
                break;
            }
        }));
    }

    for (auto &task : tasks) {
        task.get();
    }

    if (!found.load(std::memory_order_acquire)) {
        return false;
    }

    result = found_result;
    return true;
}

// 在条带模式下把“模板 + 条带”展开成同一层任务并发搜索；谁先命中谁返回。
bool findAnyNamedStripMatchParallel(
    const opcv::ImageHandle &source,
    const std::vector<std::wstring> &template_names,
    const opcv::Region &region,
    double threshold,
    opcv::SearchDirection dir,
    opcv::StripMode strip_mode,
    int method,
    opcv::MatchColorMode color_mode,
    opcv::NamedMatchResult &result) {
    result = {};
    if (template_names.empty()) {
        return false;
    }

    struct SearchTask {
        const std::wstring *template_name = nullptr;
        opcv::Region strip_region;
    };

    std::vector<SearchTask> search_tasks;
    search_tasks.reserve(template_names.size());
    for (const auto &template_name : template_names) {
        int templ_width = 0;
        int templ_height = 0;
        if (!getNamedTemplateSize(template_name, color_mode, templ_width, templ_height)) {
            continue;
        }

        const auto strips = buildStripRegions(region, templ_width, templ_height, strip_mode, dir);
        for (const auto &strip_region : strips) {
            SearchTask task;
            task.template_name = &template_name;
            task.strip_region = strip_region;
            search_tasks.push_back(task);
        }
    }

    if (search_tasks.empty()) {
        return false;
    }

    const size_t worker_count =
        std::min(search_tasks.size(), static_cast<size_t>(std::max(1u, std::thread::hardware_concurrency())));
    std::atomic<size_t> next_index(0);
    std::atomic<bool> found(false);
    std::mutex result_mutex;
    opcv::NamedMatchResult found_result;

    ThreadPool &pool = getOpenCvThreadPool();
    std::vector<std::future<void>> tasks;
    tasks.reserve(worker_count);
    for (size_t worker = 0; worker < worker_count; ++worker) {
        tasks.push_back(pool.enqueue([&, worker]() {
            while (!found.load(std::memory_order_acquire)) {
                const size_t index = next_index.fetch_add(1, std::memory_order_relaxed);
                if (index >= search_tasks.size()) {
                    break;
                }

                const auto &search_task = search_tasks[index];
                opcv::MatchResult local_match;
                if (!findFirstNamedMatch(
                        source, *search_task.template_name, search_task.strip_region, threshold, dir, method,
                        color_mode, local_match)) {
                    continue;
                }

                bool expected = false;
                if (found.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
                    std::lock_guard<std::mutex> lock(result_mutex);
                    found_result.name = *search_task.template_name;
                    found_result.match = local_match;
                }
                break;
            }
        }));
    }

    for (auto &task : tasks) {
        task.get();
    }

    if (!found.load(std::memory_order_acquire)) {
        return false;
    }

    result = found_result;
    return true;
}

// 并行收集多个模板在指定区域内所有达到阈值的命中结果。
void collectAllThresholdNamedMatchesParallel(
    const opcv::ImageHandle &source,
    const std::vector<std::wstring> &template_names,
    const opcv::Region &region,
    double threshold,
    opcv::SearchDirection dir,
    int method,
    opcv::MatchColorMode color_mode,
    std::vector<opcv::NamedMatchResult> &results) {
    results.clear();
    if (template_names.empty()) {
        return;
    }

    const unsigned int hw = std::max(1u, std::thread::hardware_concurrency());
    const size_t worker_count = std::min(template_names.size(), static_cast<size_t>(hw));
    const size_t chunk_size = (template_names.size() + worker_count - 1) / worker_count;

    ThreadPool &pool = getOpenCvThreadPool();
    std::vector<std::future<std::vector<opcv::NamedMatchResult>>> tasks;
    tasks.reserve(worker_count);
    for (size_t begin = 0; begin < template_names.size(); begin += chunk_size) {
        const size_t end = std::min(template_names.size(), begin + chunk_size);
        tasks.push_back(pool.enqueue([&, begin, end]() {
            std::vector<opcv::NamedMatchResult> partial;
            for (size_t index = begin; index < end; ++index) {
                std::vector<opcv::MatchResult> threshold_matches;
                if (!collectNamedThresholdMatches(
                        source,
                        template_names[index],
                        region,
                        threshold,
                        dir,
                        method,
                        color_mode,
                        threshold_matches)) {
                    continue;
                }

                for (const auto &match : threshold_matches) {
                    opcv::NamedMatchResult named;
                    named.name = template_names[index];
                    named.match = match;
                    partial.push_back(std::move(named));
                }
            }
            return partial;
        }));
    }

    for (auto &task : tasks) {
        auto partial = task.get();
        results.insert(results.end(), partial.begin(), partial.end());
    }
}

} // namespace

namespace opcv {

// 返回 OpenCV 运行时版本字符串。
std::wstring GetOpenCvVersion() {
    const std::string version = cv::getVersionString();
    return std::wstring(version.begin(), version.end());
}

// 从本地文件加载图像到内存包装对象。
bool LoadImageFromFile(const std::wstring &file_path, ImageHandle &image) {
    return loadImageFromFile(file_path, image);
}

// 把图像对象保存到本地文件。
bool SaveImageToFile(const ImageHandle &image, const std::wstring &file_path) {
    const cv::Mat image_mat = createMatView(image);
    if (image_mat.empty()) {
        return false;
    }

    try {
        return cv::imwrite(std::filesystem::path(file_path).string(), image_mat);
    } catch (...) {
        return false;
    }
}

// 从原始像素内存加载图像到内存包装对象。
bool LoadImageFromMemory(const RawImageView &source, ImageHandle &image) {
    return loadImageFromMemory(source, image);
}

// 把输入图像转换成单通道灰度图。
bool ToGray(const ImageHandle &source, ImageHandle &gray) {
    return toGray(source, gray);
}

// 把图像转换为二值图。
bool ToBinary(const ImageHandle &source, ImageHandle &output) {
    return toShapeMask(source, output);
}

// 按指定模式执行阈值化，输出单通道二值图。
bool Threshold(
    const ImageHandle &source,
    ImageHandle &output,
    double threshold,
    double max_value,
    ThresholdMode mode) {
    return thresholdImage(source, output, threshold, max_value, mode);
}

// 按颜色范围生成 mask，输出单通道二值图。
bool InRange(
    const ImageHandle &source,
    ImageHandle &output,
    InRangeColorSpace color_space,
    const std::vector<double> &lower,
    const std::vector<double> &upper) {
    return inRangeImage(source, output, color_space, lower, upper);
}

// 对二值图/mask 执行形态学处理。
bool Morphology(
    const ImageHandle &source,
    ImageHandle &output,
    MorphologyMode mode,
    int kernel_size,
    int iterations) {
    return morphologyImage(source, output, mode, kernel_size, iterations);
}

// 对图像做边缘提取。
bool ToEdge(const ImageHandle &source, ImageHandle &output) {
    return toEdge(source, output);
}

bool ToOutline(const ImageHandle &source, ImageHandle &output) {
    ImageHandle shape_mask;
    if (!toShapeMask(source, shape_mask)) {
        output = {};
        return false;
    }

    const cv::Mat mask_mat = createMatView(shape_mask);
    if (mask_mat.empty()) {
        output = {};
        return false;
    }

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask_mat.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    if (contours.empty()) {
        output = {};
        return false;
    }

    cv::Mat outline = cv::Mat::zeros(mask_mat.size(), CV_8UC1);
    cv::drawContours(outline, contours, -1, cv::Scalar(255), 1);

    output = createImageFromMat(outline);
    return output.channels == 1 && !output.bytes.empty();
}

// 对图像进行去噪处理。
bool Denoise(const ImageHandle &source, ImageHandle &output) {
    return denoise(source, output);
}

// 对灰度图执行全局直方图均衡化。
bool Equalize(const ImageHandle &source, ImageHandle &output) {
    return equalizeImage(source, output);
}

// 对灰度图执行局部自适应直方图均衡化。
bool CLAHE(const ImageHandle &source, ImageHandle &output, double clip_limit, int tile_grid_size) {
    return claheImage(source, output, clip_limit, tile_grid_size);
}

// 对图像执行模糊/平滑处理。
bool Blur(const ImageHandle &source, ImageHandle &output, BlurMode mode, int kernel_size) {
    return blurImage(source, output, mode, kernel_size);
}

// 对图像执行锐化处理。
bool Sharpen(const ImageHandle &source, ImageHandle &output, double strength) {
    return sharpenImage(source, output, strength);
}

// 对图像进行细化/抽骨处理。
bool Thin(const ImageHandle &source, ImageHandle &output, ThinMode mode) {
    return thinImage(source, output, mode);
}

// 裁剪指定区域图像。
bool Crop(const ImageHandle &source, const Region &region, ImageHandle &output) {
    return crop(source, region, output);
}

// 把图像缩放到指定大小。
bool Resize(const ImageHandle &source, int width, int height, ImageHandle &output) {
    return resizeImage(source, width, height, output);
}

// 裁剪图像中的有效区域。
bool CropValid(const ImageHandle &source, ImageHandle &output) {
    return cropValid(source, output);
}

// 获取二值图/mask 中的连通区域。
bool ConnectedComponents(const ImageHandle &source, double min_area, std::vector<RegionAnalysisResult> &results) {
    return connectedComponentsImage(source, min_area, results);
}

// 获取二值图/mask 中的轮廓区域。
bool FindContours(const ImageHandle &source, double min_area, std::vector<ContourAnalysisResult> &results) {
    return findContoursImage(source, min_area, results);
}

// 对指定区域执行模板匹配并返回所有候选分数。
bool MatchTemplate(
    const ImageHandle &source,
    const ImageHandle &templ,
    const Region &region,
    std::vector<MatchCandidate> &matches,
    int method,
    MatchColorMode color_mode) {
    return collectRegionCandidates(source, templ, nullptr, region, method, color_mode, matches);
}

// 从候选结果中选出分数最高的匹配项。
bool GetBestMatch(const std::vector<MatchCandidate> &matches, int templ_width, int templ_height, MatchResult &result) {
    return getBestMatch(matches, templ_width, templ_height, result);
}

// 判断最佳结果是否达到给定阈值。
bool IsMatchAboveThreshold(const MatchResult &result, double threshold) {
    return isMatchAboveThreshold(result, threshold);
}

// 把模板加载进内部缓存，供后续按名字匹配。
bool LoadTemplate(const std::wstring &name, const std::wstring &file_path) {
    if (name.empty()) {
        return false;
    }

    ImageHandle color;
    if (!loadImageFromFile(file_path, color)) {
        return false;
    }

    ImageHandle gray;
    if (!toGray(color, gray)) {
        return false;
    }

    TemplateEntry entry;
    entry.color = std::move(color);
    entry.gray = std::move(gray);
    alphaToMask(entry.color, entry.mask);
    entry.has_mask = !entry.mask.bytes.empty();
    std::unique_lock<std::shared_mutex> lock(gTemplateStoreMutex);
    gTemplateStore[name] = std::move(entry);
    return true;
}

// 把带 mask 的模板加载进内部缓存，供后续按名字匹配。
bool LoadMaskedTemplate(
    const std::wstring &name,
    const std::wstring &template_path,
    const std::wstring &mask_path) {
    if (name.empty()) {
        return false;
    }

    ImageHandle color;
    if (!loadImageFromFile(template_path, color)) {
        return false;
    }

    ImageHandle gray;
    if (!toGray(color, gray)) {
        return false;
    }

    ImageHandle raw_mask;
    if (!loadImageFromFile(mask_path, raw_mask)) {
        return false;
    }

    ImageHandle mask;
    if (!toMask(raw_mask, mask)) {
        return false;
    }

    if (mask.width != color.width || mask.height != color.height) {
        return false;
    }

    TemplateEntry entry;
    entry.color = std::move(color);
    entry.gray = std::move(gray);
    entry.mask = std::move(mask);
    entry.has_mask = true;

    std::unique_lock<std::shared_mutex> lock(gTemplateStoreMutex);
    gTemplateStore[name] = std::move(entry);
    return true;
}

// 批量加载模板列表，会尽量加载所有项，任意一项失败则返回 false。
bool LoadTemplateList(const std::vector<std::pair<std::wstring, std::wstring>> &templates) {
    bool all_loaded = true;
    for (const auto &item : templates) {
        if (!LoadTemplate(item.first, item.second)) {
            all_loaded = false;
        }
    }
    return all_loaded;
}

// 判断模板缓存中是否存在指定名字。
bool HasTemplate(const std::wstring &name) {
    std::shared_lock<std::shared_mutex> lock(gTemplateStoreMutex);
    return gTemplateStore.find(name) != gTemplateStore.end();
}

// 删除指定名字模板。
bool RemoveTemplate(const std::wstring &name) {
    std::unique_lock<std::shared_mutex> lock(gTemplateStoreMutex);
    return gTemplateStore.erase(name) > 0;
}

// 删除所有已加载模板。
void RemoveAllTemplates() {
    std::unique_lock<std::shared_mutex> lock(gTemplateStoreMutex);
    gTemplateStore.clear();
}

// 返回所有模板名，便于外部查看当前缓存状态。
std::vector<std::wstring> GetAllTemplateNames() {
    std::vector<std::wstring> names;
    std::shared_lock<std::shared_mutex> lock(gTemplateStoreMutex);
    names.reserve(gTemplateStore.size());
    for (const auto &item : gTemplateStore) {
        names.push_back(item.first);
    }
    std::sort(names.begin(), names.end());
    return names;
}

// 返回当前模板缓存数量。
int GetTemplateCount() {
    std::shared_lock<std::shared_mutex> lock(gTemplateStoreMutex);
    return static_cast<int>(gTemplateStore.size());
}

// 使用指定模板名在给定区域内执行匹配，并返回一个达到阈值的命中。
bool MatchTemplate(
    const ImageHandle &source,
    const std::wstring &template_name,
    const Region &region,
    double threshold,
    MatchResult &result,
    SearchDirection dir,
    StripMode strip_mode,
    int method,
    MatchColorMode color_mode) {
    const SearchDirection normalized_dir = normalizeSearchDirection(dir);
    const StripMode normalized_strip_mode = normalizeStripMode(strip_mode);

    const opcv::ImageHandle *search_source = &source;
    opcv::ImageHandle gray_source;
    if (color_mode == MatchColorMode::Gray) {
        if (!toGray(source, gray_source)) {
            result = {};
            return false;
        }
        search_source = &gray_source;
    }

    if (normalized_strip_mode == StripMode::None) {
        TemplateSnapshot snapshot;
        if (!getTemplateSnapshot(template_name, color_mode, snapshot)) {
            result = {};
            return false;
        }

        const cv::Rect roi = clampRegion(region, *search_source);
        if (roi.width <= 0 || roi.height <= 0 || snapshot.templ.width > roi.width ||
            snapshot.templ.height > roi.height) {
            result = {};
            return false;
        }

        if (shouldTryPyramid(rectToRegion(roi), snapshot.templ.width, snapshot.templ.height, normalized_strip_mode)) {
            const cv::Mat source_mat = createMatView(*search_source);
            if (!source_mat.empty()) {
                opcv::ImageHandle roi_source = createImageFromMat(source_mat(roi).clone());
                opcv::MatchResult pyramid_match;
                const opcv::ImageHandle *mask_ptr = snapshot.has_mask ? &snapshot.mask : nullptr;
                bool pyramid_definitive_miss = false;
                if (pyramidFindFirstMatch(
                        roi_source,
                        snapshot.templ,
                        mask_ptr,
                        threshold,
                        normalized_dir,
                        method,
                        color_mode,
                        pyramid_match,
                        &pyramid_definitive_miss)) {
                    pyramid_match.x += roi.x;
                    pyramid_match.y += roi.y;
                    result = pyramid_match;
                    return true;
                }
                if (pyramid_definitive_miss) {
                    result = {};
                    return false;
                }
            }
        }

        return findFirstNamedMatch(*search_source, template_name, region, threshold, normalized_dir, method, color_mode,
                                   result);
    }

    if (!isDirectionCompatibleWithStrip(normalized_dir, normalized_strip_mode)) {
        result = {};
        return false;
    }

    int templ_width = 0;
    int templ_height = 0;
    if (!getNamedTemplateSize(template_name, color_mode, templ_width, templ_height)) {
        result = {};
        return false;
    }

    const auto strips = buildStripRegions(region, templ_width, templ_height, normalized_strip_mode, normalized_dir);
    if (strips.empty()) {
        result = {};
        return false;
    }
    return findAnyStripMatch(*search_source, template_name, strips, threshold, normalized_dir, method, color_mode,
                             result);
}

// 使用指定模板名在多个缩放比例下执行匹配，scales 为空时自动尝试内部候选。
bool MatchTemplateScale(
    const ImageHandle &source,
    const std::wstring &template_name,
    const Region &region,
    const std::vector<double> &scales,
    double threshold,
    MatchResult &result,
    int method,
    MatchColorMode color_mode) {
    result = {};

    const cv::Rect roi = clampRegion(region, source);
    if (roi.width <= 0 || roi.height <= 0) {
        return false;
    }

    int base_template_width = 0;
    int base_template_height = 0;
    if (!getNamedTemplateSize(template_name, color_mode, base_template_width, base_template_height)) {
        return false;
    }

    const bool automatic_scales = scales.empty();
    const std::vector<double> scale_values =
        automatic_scales ? buildAutomaticScaleCandidates(base_template_width, base_template_height, roi) : scales;
    if (scale_values.empty()) {
        return false;
    }

    MatchResult best_result;
    bool found = false;

    if (automatic_scales) {
        const bool probe_first = shouldProbeAutomaticScalesFirst(roi, base_template_width, base_template_height);
        size_t fast_count = std::min(kScaleAutoFastPathCandidates, scale_values.size());
        bool skip_fast_refine = false;
        if (probe_first && fast_count > 0) {
            const auto fast_probed = probeAutomaticScaleCandidates(
                source,
                template_name,
                region,
                scale_values,
                0,
                fast_count,
                method,
                color_mode);
            skip_fast_refine = fast_probed.empty() || fast_probed.front().score < kScaleAutoStrongProbeSimilarity;
            if (!skip_fast_refine) {
                const std::vector<double> fast_scale_values = selectProbeScaleValues(fast_probed, fast_count);
                if (!refineScaleCandidates(source, template_name, region, roi, fast_scale_values, threshold, method,
                                           color_mode, true, best_result, found)) {
                    return false;
                }
            }
        } else if (fast_count > 0) {
            const std::vector<double> fast_scale_values(scale_values.begin(), scale_values.begin() + fast_count);
            if (!refineScaleCandidates(source, template_name, region, roi, fast_scale_values, threshold, method,
                                       color_mode, true, best_result, found)) {
                return false;
            }
        }
        if ((!found || best_result.score < kScaleAutoEarlyStopSimilarity) && scale_values.size() > fast_count) {
            const std::vector<double> probed_scale_values =
                buildAutomaticScaleProbeOrder(source, template_name, region, scale_values, fast_count, method,
                                              color_mode);
            if (!refineScaleCandidates(source, template_name, region, roi, probed_scale_values, threshold, method,
                                       color_mode, true, best_result, found)) {
                return false;
            }
        }
    } else if (!refineScaleCandidates(source, template_name, region, roi, scale_values, threshold, method, color_mode,
                                      false, best_result, found)) {
        return false;
    }

    if (!found) {
        return false;
    }

    result = best_result;
    return true;
}

// 在指定区域内并行搜索多个模板，并返回任意一个率先命中的结果。
bool MatchAnyTemplate(
    const ImageHandle &source,
    const std::vector<std::wstring> &template_names,
    const Region &region,
    double threshold,
    NamedMatchResult &result,
    SearchDirection dir,
    StripMode strip_mode,
    int method,
    MatchColorMode color_mode) {
    result = {};
    const SearchDirection normalized_dir = normalizeSearchDirection(dir);
    const StripMode normalized_strip_mode = normalizeStripMode(strip_mode);
    if (template_names.empty()) {
        return false;
    }

    const opcv::ImageHandle *search_source = &source;
    opcv::ImageHandle gray_source;
    if (color_mode == MatchColorMode::Gray) {
        if (!toGray(source, gray_source)) {
            result = {};
            return false;
        }
        search_source = &gray_source;
    }

    if (normalized_strip_mode == StripMode::None) {
        const cv::Rect roi = clampRegion(region, *search_source);
        if (roi.width <= 0 || roi.height <= 0) {
            return false;
        }

        const cv::Mat source_mat = createMatView(*search_source);
        if (!source_mat.empty()) {
            opcv::ImageHandle roi_source = createImageFromMat(source_mat(roi).clone());
            bool saw_pyramid_candidate = false;
            bool needs_direct_fallback = false;
            for (const auto &template_name : template_names) {
                TemplateSnapshot snapshot;
                if (!getTemplateSnapshot(template_name, color_mode, snapshot)) {
                    needs_direct_fallback = true;
                    continue;
                }
                if (snapshot.templ.width > roi.width || snapshot.templ.height > roi.height ||
                    !shouldTryPyramid(rectToRegion(roi), snapshot.templ.width, snapshot.templ.height,
                                      normalized_strip_mode)) {
                    needs_direct_fallback = true;
                    continue;
                }
                saw_pyramid_candidate = true;

                opcv::MatchResult pyramid_match;
                const opcv::ImageHandle *mask_ptr = snapshot.has_mask ? &snapshot.mask : nullptr;
                bool pyramid_definitive_miss = false;
                if (pyramidFindFirstMatch(
                        roi_source,
                        snapshot.templ,
                        mask_ptr,
                        threshold,
                        normalized_dir,
                        method,
                        color_mode,
                        pyramid_match,
                        &pyramid_definitive_miss)) {
                    pyramid_match.x += roi.x;
                    pyramid_match.y += roi.y;
                    result.name = template_name;
                    result.match = pyramid_match;
                    return true;
                }
                if (pyramid_definitive_miss) {
                    continue;
                }
                needs_direct_fallback = true;
            }
            if (saw_pyramid_candidate && !needs_direct_fallback) {
                return false;
            }
        }

        return findAnyNamedMatchParallel(
            *search_source, template_names, region, threshold, normalized_dir, method, color_mode, result);
    }
    if (!isDirectionCompatibleWithStrip(normalized_dir, normalized_strip_mode)) {
        return false;
    }

    return findAnyNamedStripMatchParallel(
        *search_source, template_names, region, threshold, normalized_dir, normalized_strip_mode, method, color_mode,
        result);
}

// 在指定区域内返回所有达到阈值的模板匹配结果。
bool MatchAllTemplates(
    const ImageHandle &source,
    const std::vector<std::wstring> &template_names,
    const Region &region,
    double threshold,
    std::vector<NamedMatchResult> &results,
    SearchDirection dir,
    StripMode strip_mode,
    int method,
    MatchColorMode color_mode) {
    const SearchDirection normalized_dir = normalizeSearchDirection(dir);
    const StripMode normalized_strip_mode = normalizeStripMode(strip_mode);
    const opcv::ImageHandle *search_source = &source;
    opcv::ImageHandle gray_source;
    if (color_mode == MatchColorMode::Gray) {
        if (!toGray(source, gray_source)) {
            results.clear();
            return false;
        }
        search_source = &gray_source;
    }
    if (normalized_strip_mode == StripMode::None) {
        results.clear();

        std::vector<std::wstring> direct_template_names;
        direct_template_names.reserve(template_names.size());

        const cv::Rect roi = clampRegion(region, *search_source);
        const cv::Mat source_mat = createMatView(*search_source);
        opcv::ImageHandle roi_source;
        const bool can_prepare_pyramid_roi = roi.width > 0 && roi.height > 0 && !source_mat.empty();
        if (can_prepare_pyramid_roi) {
            roi_source = createImageFromMat(source_mat(roi).clone());
        }

        if (can_prepare_pyramid_roi && !roi_source.bytes.empty()) {
            for (const auto &template_name : template_names) {
                bool needs_direct_search = true;
                TemplateSnapshot snapshot;
                if (!getTemplateSnapshot(template_name, color_mode, snapshot)) {
                    direct_template_names.push_back(template_name);
                    continue;
                }
                if (snapshot.templ.width <= roi.width && snapshot.templ.height <= roi.height &&
                    shouldTryPyramid(rectToRegion(roi), snapshot.templ.width, snapshot.templ.height,
                                     normalized_strip_mode)) {
                    std::vector<MatchResult> template_matches;
                    bool pyramid_saturated = false;
                    const opcv::ImageHandle *mask_ptr = snapshot.has_mask ? &snapshot.mask : nullptr;
                    const double coarse_relax =
                        snapshot.has_mask ? kMaskedPyramidCoarseThresholdRelax : kPyramidCoarseThresholdRelax;
                    if (pyramidFindAllMatches(
                            roi_source,
                            snapshot.templ,
                            mask_ptr,
                            threshold,
                            normalized_dir,
                            method,
                            color_mode,
                            kPyramidAllCoarseCandidates,
                            template_matches,
                            true,
                            coarse_relax,
                            &pyramid_saturated) &&
                        !pyramid_saturated) {
                        for (auto &match : template_matches) {
                            match.x += roi.x;
                            match.y += roi.y;
                            NamedMatchResult named;
                            named.name = template_name;
                            named.match = match;
                            results.push_back(std::move(named));
                        }
                        needs_direct_search = false;
                    }
                }

                if (needs_direct_search) {
                    direct_template_names.push_back(template_name);
                }
            }
        } else {
            direct_template_names = template_names;
        }

        if (!direct_template_names.empty()) {
            std::vector<NamedMatchResult> direct_results;
            collectAllThresholdNamedMatchesParallel(*search_source, direct_template_names, region, threshold, normalized_dir,
                                                    method, color_mode, direct_results);
            results.insert(results.end(), direct_results.begin(), direct_results.end());
        }

        return !results.empty();
    }
    if (!isDirectionCompatibleWithStrip(normalized_dir, normalized_strip_mode)) {
        results.clear();
        return false;
    }

    results.clear();
    for (const auto &template_name : template_names) {
        int templ_width = 0;
        int templ_height = 0;
        if (!getNamedTemplateSize(template_name, color_mode, templ_width, templ_height)) {
            continue;
        }

        const auto strips = buildStripRegions(region, templ_width, templ_height, normalized_strip_mode, normalized_dir);
        if (strips.empty()) {
            continue;
        }

        ThreadPool &pool = getOpenCvThreadPool();
        std::vector<std::future<std::vector<MatchResult>>> tasks;
        tasks.reserve(strips.size());
        for (const auto &strip_region : strips) {
            tasks.push_back(pool.enqueue([&, strip_region]() {
                std::vector<MatchResult> partial;
                collectNamedThresholdMatches(*search_source, template_name, strip_region, threshold, normalized_dir, method,
                                             color_mode, partial);
                return partial;
            }));
        }

        std::vector<MatchResult> merged;
        for (auto &task : tasks) {
            auto partial = task.get();
            merged.insert(merged.end(), partial.begin(), partial.end());
        }
        dedupeMatchResults(merged);

        for (const auto &match : merged) {
            NamedMatchResult named;
            named.name = template_name;
            named.match = match;
            results.push_back(std::move(named));
        }
    }
    return !results.empty();
}

// 使用 ORB 特征匹配在给定区域内查找指定模板。
bool FeatureMatchTemplate(
    const ImageHandle &source,
    const std::wstring &template_name,
    const Region &region,
    double threshold,
    MatchResult &result) {
    result = {};

    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
    int templ_width = 0;
    int templ_height = 0;
    if (!withTemplateEntryExclusive(template_name, [&](TemplateEntry &entry) {
            if (!ensureTemplateFeatures(entry)) {
                return false;
            }

            keypoints = entry.feature_keypoints;
            descriptors = entry.feature_descriptors.clone();
            templ_width = entry.gray.width;
            templ_height = entry.gray.height;
            return true;
        })) {
        return false;
    }

    return featureMatchRegion(source, keypoints, descriptors, templ_width, templ_height, region, threshold, result);
}

// 使用边缘匹配在给定区域内查找指定模板。
bool EdgeMatchTemplate(
    const ImageHandle &source,
    const std::wstring &template_name,
    const Region &region,
    double threshold,
    MatchResult &result) {
    result = {};

    ImageHandle source_edge;
    if (!toEdge(source, source_edge)) {
        return false;
    }

    ImageHandle templ_edge;
    if (!withTemplateEntryExclusive(template_name, [&](TemplateEntry &entry) {
            if (!ensureTemplateEdge(entry)) {
                return false;
            }

            templ_edge = entry.edge;
            return true;
        })) {
        return false;
    }

    return findBestPreparedMatch(source_edge, templ_edge, nullptr, region, threshold, result, cv::TM_CCOEFF_NORMED);
}

// 使用轮廓/形状匹配在给定区域内查找指定模板。
bool ShapeMatchTemplate(
    const ImageHandle &source,
    const std::wstring &template_name,
    const Region &region,
    double threshold,
    MatchResult &result) {
    result = {};

    std::vector<cv::Point> templ_contour;
    if (!withTemplateEntryExclusive(template_name, [&](TemplateEntry &entry) {
            if (!ensureTemplateShape(entry)) {
                return false;
            }

            templ_contour = entry.shape_contour;
            return true;
        })) {
        return false;
    }

    return shapeMatchRegion(source, templ_contour, region, threshold, result);
}

} // namespace opcv

