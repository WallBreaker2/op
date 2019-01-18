#pragma once
#ifndef __XIMAGE_H_
#define __XIMAGE_H_
#include <opencv2/core.hpp>
//积分二值化
void thresholdIntegral(const cv::Mat& inputMat, cv::Mat& outputMat);
//垂直方向投影
void picshadowx(const cv::Mat& binary, std::vector<cv::Mat>& out_put,std::vector<int>&ys);
//水平方向投影并行分割
void picshadowy(const cv::Mat& binary, std::vector<cv::Mat>&out_put,std::vector<int>&xs);

#endif

