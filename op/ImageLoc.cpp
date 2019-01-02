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
		_src.create(height, width, CV_8UC4);
		uint *p, *p2;
		for (i = 0; i < height; ++i) {
			p = _src.ptr<uint>(i);
			p2 = (uint*)image_data + i * width;
			for (j = 0; j < width; ++j) {
				*p++ = *p2++;
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
	for (auto&it : images) {
		_target=cv::imread(_wsto_string(it));
		if (_target.empty()) {
			setlog(L"ImageLoc::imageloc,file:%s read false.", it.c_str());
			continue;
		}
		cv::cvtColor(_src, _src_gray, CV_BGR2GRAY);
		cv::cvtColor(_target, _target_gray, CV_BGR2GRAY);
		cv::matchTemplate(_src_gray, _target_gray, _result, CV_TM_SQDIFF_NORMED);
		cv::normalize(_result, _result, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());
		double minval, maxval;
		cv::Point pt1, pt2;
		cv::minMaxLoc(_result, &minval, &maxval, &pt1, &pt2);
		setlog(L"[ImageLoc::imageloc]file=%s, minval=%lf,[%d,%d]",it.c_str(), minval, pt1.x, pt1.y);
		if (minval <= sim) {
			x = pt1.x; y = pt1.y;
			return 1;
		}
		else {
			return 0;
		}
	}
	return 0;


}