#pragma once

#include "TemplateMatcher.h"

#include <opencv2/core.hpp>

namespace opcv::helpers {

size_t imageByteCount(int width, int height, int channels);

cv::Mat createMatView(ImageHandle &image);
cv::Mat createMatView(const ImageHandle &image);
ImageHandle createImageFromMat(const cv::Mat &mat);
cv::Mat createMatFromRawMemory(const RawImageView &source);

cv::Rect clampRegion(const Region &region, const ImageHandle &source);

bool prepareMatchInputs(
    const cv::Mat &source,
    const cv::Mat &templ,
    MatchColorMode color_mode,
    cv::Mat &norm_source,
    cv::Mat &norm_templ);

bool prepareMaskInput(
    const cv::Mat &mask,
    const cv::Mat &norm_templ,
    cv::Mat &norm_mask);

bool isSupportedMatchMethod(int method);
double convertMatchScoreToSimilarity(int method, float raw_score);

bool collectTemplateMatches(
    const cv::Mat &source,
    const cv::Mat &templ,
    const cv::Mat *mask,
    int offset_x,
    int offset_y,
    std::vector<MatchCandidate> &matches,
    int method,
    MatchColorMode color_mode);

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
    MatchColorMode color_mode);

bool getBestMatch(
    const std::vector<MatchCandidate> &matches,
    int templ_width,
    int templ_height,
    MatchResult &result);

bool isMatchAboveThreshold(const MatchResult &result, double threshold);

void collectThresholdMatches(
    const std::vector<MatchCandidate> &matches,
    int templ_width,
    int templ_height,
    double threshold,
    std::vector<MatchResult> &results);

bool loadImageFromFile(const std::wstring &file_path, ImageHandle &image);
bool loadImageFromMemory(const RawImageView &source, ImageHandle &image);

bool toGray(const ImageHandle &source, ImageHandle &gray);
bool toMask(const ImageHandle &source, ImageHandle &mask);
bool alphaToMask(const ImageHandle &source, ImageHandle &mask);
bool thresholdImage(const ImageHandle &source, ImageHandle &output, double threshold, double max_value,
                    ThresholdMode mode);
bool inRangeImage(const ImageHandle &source, ImageHandle &output, InRangeColorSpace color_space,
                  const std::vector<double> &lower, const std::vector<double> &upper);
bool morphologyImage(const ImageHandle &source, ImageHandle &output, MorphologyMode mode, int kernel_size,
                     int iterations);
bool toEdge(const ImageHandle &source, ImageHandle &edge);
bool toShapeMask(const ImageHandle &source, ImageHandle &mask);

bool denoise(const ImageHandle &source, ImageHandle &output);
bool equalizeImage(const ImageHandle &source, ImageHandle &output);
bool claheImage(const ImageHandle &source, ImageHandle &output, double clip_limit, int tile_grid_size);
bool blurImage(const ImageHandle &source, ImageHandle &output, BlurMode mode, int kernel_size);
bool sharpenImage(const ImageHandle &source, ImageHandle &output, double strength);
bool thinImage(const ImageHandle &source, ImageHandle &output, ThinMode mode);
bool crop(const ImageHandle &source, const Region &region, ImageHandle &output);
bool resizeImage(const ImageHandle &source, int width, int height, ImageHandle &output);
bool cropValid(const ImageHandle &source, ImageHandle &output);
bool connectedComponentsImage(const ImageHandle &source, double min_area, std::vector<RegionAnalysisResult> &results);
bool findContoursImage(const ImageHandle &source, double min_area, std::vector<ContourAnalysisResult> &results);

bool extractLargestContour(const ImageHandle &mask, std::vector<cv::Point> &contour);
bool extractCandidateContours(const ImageHandle &mask, std::vector<std::vector<cv::Point>> &contours);

} // namespace opcv::helpers

