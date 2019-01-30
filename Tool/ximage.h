#pragma once
#ifndef __XIMAGE_H_
#define __XIMAGE_H_
#include <opencv2/core.hpp>
#include <fstream>
#include "Dict.h"
#include<map>
struct point_t {
	int x, y;
	bool operator<(const point_t&rhs) const {
		if (std::abs(y - rhs.y) < 9)
			return x < rhs.x;
		else
			return y < rhs.y;
	}
	bool operator==(const point_t&rhs) const {
		return x == rhs.x&&y == rhs.y;
	}
};

//积分二值化
void graytobinary(const cv::Mat& inputMat, cv::Mat& outputMat);
//垂直方向投影(x)轴
void binshadowx(const cv::Mat& binary,const rect_t& rc, std::vector<rect_t>& out_put);
//水平方向投影(y)轴
void binshadowy(const cv::Mat& binary, const rect_t& rc, std::vector<rect_t>&out_put);
//图像裁剪
void bin_image_cut(const cv::Mat& binary, const rect_t&inrc, rect_t& outrc);
//ocr in sim=1.0
void _bin_ocr(const cv::Mat& binary, cv::Mat& record, const rect_t&rc,const Dict& dict, std::map<point_t,std::wstring>&outstr);
//ocr with sim<1.0
void _bin_ocr(const cv::Mat& binary, cv::Mat& record, const rect_t&rc, const Dict& dict, double sim, std::map<point_t, std::wstring>&outstr);
//ocr wrapper
void bin_ocr(const cv::Mat& binary, cv::Mat& record, const Dict& dict,double sim, std::wstring& outstr);


#endif

