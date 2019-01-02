#include "stdafx.h"
#include "ImageLoc.h"
#include "Common.h"

ImageLoc::ImageLoc()
{
}


ImageLoc::~ImageLoc()
{
}


long ImageLoc::input_image(byte* image_data, int width, int height, int pixel) {
	int i, j, k; 
	
	if (pixel == 4) {
		_src.create(height, width, CV_8UC3);
		uchar *p, *p2;
		for (i = 0; i < height; ++i) {
			p = _src.ptr<uchar>(i);
			p2 = image_data + (height - i - 1) * width * 4;
			for (j = 0; j < width; ++j) {
				*p++ = *p2++; *p++ = *p2++;
				*p++ = *p2++; ++p2;
			}
		}
	}
	else
		return 0;
	return 1;
}

long ImageLoc::imageloc(images_t& images,double sim, long&x, long&y) {
	x = y = -1;
	if (_src.empty())return 0;
	cv::imwrite("input.png", _src);
	for (auto&it : images) {
		_target=cv::imread(_wsto_string(it));
		if (_target.empty()) {
			setlog(L"ImageLoc::imageloc,file:%s read false.", it.c_str());
			continue;
		}
		//cv::cvtColor(_src, _src_gray, CV_BGR2GRAY);
		//cv::cvtColor(_target, _target_gray, CV_BGR2GRAY);
		int result_cols = _src.cols - _target.cols + 1;
		int result_rows = _src.rows - _target.rows + 1;
		_result.create(result_rows, result_cols, CV_32FC1);
		cv::matchTemplate(_src, _target, _result, CV_TM_SQDIFF);
		//cv::normalize(_result, _result, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());
		
		
		double minval, maxval;
		cv::Point pt1, pt2;
		cv::minMaxLoc(_result, &minval, &maxval, &pt1, &pt2);
		minval /= 255.*255.*_target.rows*_target.cols;
		minval = 1. - minval;
		setlog(L"[ImageLoc::imageloc]file=%s, minval=%lf,[%d,%d]",it.c_str(), minval, pt1.x, pt1.y);
		
		if (minval>=sim) {
			x = pt1.x; y = pt1.y;
			return 1;
		}
		else {
			return 0;
		}
	}
	return 0;


}