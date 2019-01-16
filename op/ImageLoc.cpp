#include "stdafx.h"
#include "ImageLoc.h"
#include "Common.h"
template<typename T>
int ImageExtend::get_bit_count(T x) {
	int s = 0;
	while (x) {
		s += x & 1;
		s >>= 1;
	}
	return s;
}



ImageExtend::ImageExtend()
{
}


ImageExtend::~ImageExtend()
{
}


long ImageExtend::input_image(byte* image_data, int width, int height,int type) {
	int i, j, k; 
	
	if (type==-1) {//µ¹¹ýÀ´¶Á
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
	else {
		_src.create(height, width, CV_8UC3);
		uchar *p, *p2;
		for (i = 0; i < height; ++i) {
			p = _src.ptr<uchar>(i);
			p2 = image_data + i * width * 4;
			for (j = 0; j < width; ++j) {
				*p++ = *p2++; *p++ = *p2++;
				*p++ = *p2++; ++p2;
			}
		}
	}
	return 1;
}

long ImageExtend::imageloc(images_t& images,double sim, long&x, long&y) {
	x = y = -1;
	if (_src.empty())return 0;
	cv::imwrite("input.png", _src);
	for (auto&it : images) {
		_target=cv::imread(_wsto_string(it));
		if (_target.empty()) {
			setlog(L"ImageExtend::imageloc,file:%s read false.", it.c_str());
			continue;
		}
		//cv::cvtColor(_src, _src_gray, CV_BGR2GRAY);
		//cv::cvtColor(_target, _target_gray, CV_BGR2GRAY);
		int result_cols = _src.cols - _target.cols + 1;
		int result_rows = _src.rows - _target.rows + 1;
		_result.create(result_rows, result_cols, CV_32FC1);
		cv::matchTemplate(_src, _target, _result, CV_TM_SQDIFF);
		//cv::normalize(_result, _result, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());
		
		
		double minval, maxval,bestval;
		cv::Point pt1, pt2;
		cv::minMaxLoc(_result, &minval, &maxval, &pt1, &pt2);
		minval /= 255.*255.*_target.rows*_target.cols;
		bestval = 1. - minval;
		setlog(L"ImageExtend::imageloc(),file=%s, bestval=%lf,pos=[%d,%d]",it.c_str(), bestval, pt1.x, pt1.y);
		
		if (bestval>=sim) {
			x = pt1.x; y = pt1.y;
			return 1;
		}
		else {
			return 0;
		}
	}
	return 0;


}

long ImageExtend::GetPixel(long x, long y,color_t&cr) {
	if (!is_valid(x, y))
		return 0;
	auto p = _src.ptr<uchar>(y) + 3 * x;
	static_assert(sizeof(color_t) == 4);
	cr.b = p[0]; cr.g = p[1]; cr.r = p[2];
	return 1;
}

long ImageExtend::FindColor(color_t cr,color_t df, long&x, long&y) {
	for (int i = 0; i < _src.rows; ++i) {
		uchar* p = _src.ptr<uchar>(i);
		for (int j = 0; j < _src.cols; ++j) {
			if ((*(color_t*)(p+j*3)-cr)<=df) {
				x = j; y = i;
				return 1;
			}
		}
	}
	x = y = -1;
	return 0;
}

void ImageExtend::bgr2binary(color_t cr, color_t df) {
	if (_src.empty())
		return;
	int ncols = _src.cols, nrows = _src.rows;
	_src_gray.create(nrows, ncols, CV_8UC1);
	for (int i = 0; i < nrows; ++i) {
		uchar* p = _src.ptr<uchar>(i);
		uchar* p2 = _src_gray.ptr<uchar>(i);
		for (int j = 0; j < ncols; ++j) {
			if ((*(color_t*)p - cr) <= df)
				*p2 = 1;
			else
				*p2 = 0;
			++p2;
			p += 3;
		}
	}
}

long ImageExtend::Ocr(one_words_t& words, double sim, long&x, long&y) {
	x = y = -1;
	long ret_val = 0;
	int nrows = _src_gray.rows, ncols = _src_gray.cols;
	//step 1. ±ß½ç¼ì²â
	if (words.binlines.size() > ncols || 11 > nrows)
		return ret_val;
	//step 2.Æ¥Åä
	for (int i = 0; i < nrows - 11 + 1; ++i) {
		auto ps = _src_gray.ptr<uchar>(i);
		for (int j = 0; j < ncols - words.binlines.size() + 1; ++j) {
			//
			auto s=full_match(ncols, ps + j, words.binlines.data(), words.binlines.size(), 11);
			if (s >= sim * words.bit_ct) {
				x = j; y = i;
				return 1;
			}
		}
	}
	return ret_val;
}
