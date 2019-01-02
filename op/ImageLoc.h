#pragma once
#ifndef __IMAGELOC_H_
#define __IMAGELOC_H_
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
using images_t = std::vector<std::wstring>;
class ImageLoc
{
public:
	
	ImageLoc();
	~ImageLoc();
	long input_image(byte* image_data, int width, int height, int pixel);
	long imageloc(images_t& images, double sim, long&x, long&y);
private:
	cv::Mat _src;
	cv::Mat _src_gray;
	cv::Mat _target;
	cv::Mat _target_gray;
	cv::Mat _result;
};

#endif

