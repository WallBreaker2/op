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


long ImageExtend::input_image(byte* image_data, int width,int height, long x1, long y1, long x2, long y2, int type) {
	int i, j, k; 
	int cw = x2 - x1 + 1, ch = y2 - y1 + 1;
	if (type==-1) {//倒过来读
		_src.create(ch, cw, CV_8UC3);
		uchar *p, *p2;
		for (i = 0; i < ch; ++i) {
			p = _src.ptr<uchar>(i);
			p2 = image_data + (height- i - 1-y1) * width*4+x1*4;//偏移
			for (j = 0; j < cw; ++j) {
				*p++ = *p2++; *p++ = *p2++;
				*p++ = *p2++; ++p2;
			}
		}
	}
	else {
		_src.create(ch, cw, CV_8UC3);
		uchar *p, *p2;
		for (i = 0; i < ch; ++i) {
			p = _src.ptr<uchar>(i);
			p2 = image_data + (i + y1) * width * 4 + x1 * 4;
			for (j = 0; j < cw; ++j) {
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
	//cv::imwrite("input.png", _src);
	for (auto&it : images) {
		_target=cv::imread(_wsto_string(it));
		if (_target.empty()) {
			//setlog(L"ImageExtend::imageloc,file:%s read false.", it.c_str());
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
		//setlog(L"ImageExtend::imageloc(),file=%s, bestval=%lf,pos=[%d,%d]",it.c_str(), bestval, pt1.x, pt1.y);
		
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

long ImageExtend::FindColor(vector<color_df_t>& colors, long&x, long&y) {
	for (int i = 0; i < _src.rows; ++i) {
		uchar* p = _src.ptr<uchar>(i);
		for (int j = 0; j < _src.cols; ++j) {
			for (auto&it : colors) {//对每个颜色描述
				if ((*(color_t*)p - it.color) <= it.df) {
					x = j; y = i;
					return 1;
				}
			}
			p += 3;
		}
	}
	x = y = -1;
	return 0;
}

void ImageExtend::bgr2binary(vector<color_df_t>& colors) {
	if (_src.empty())
		return;
	int ncols = _src.cols, nrows = _src.rows;
	_src_gray.create(nrows, ncols, CV_8UC1);
	for (int i = 0; i < nrows; ++i) {
		uchar* p = _src.ptr<uchar>(i);
		uchar* p2 = _src_gray.ptr<uchar>(i);
		for (int j = 0; j < ncols; ++j) {
			*p2 = 0;
			for (auto&it : colors) {//对每个颜色描述
				if ((*(color_t*)p - it.color) <= it.df) {
					*p2 = 0xff;
					break;
				}	
			}
			++p2;
			p += 3;
		}
	}
	//test
	cv::imwrite("src.bmp", _src);
	cv::imwrite("binary.bmp", _src_gray);
}

long ImageExtend::Ocr(dict_t& dict, double sim,wstring& ret_str) {
	ret_str.clear();
	long ret_val = 0;
	int nrows = _src_gray.rows, ncols = _src_gray.cols;
	//step 2.匹配
	for (int i = 0; i <nrows; ++i) {//
		auto ps = _src_gray.ptr<uchar>(i);
		for (int j = 0; j < ncols;) {
			//
			ret_val = 0;
			int k;
			for (k = 0; k < dict.size();++k) {
				//step 1. 边界检测
				if (i + dict[k].height <= nrows && j + dict[k].binlines.size() <= ncols) {
					
					if (full_match(ncols, ps + j, &dict[k].binlines[0], dict[k].binlines.size(), dict[k].height)) {
						ret_str += dict[k].word;
						ret_val = 1;
						break;
					}
				}
			}
			if (ret_val) {
				j += dict[k].binlines.size();
			}
			else {
				j += 1;
			}
				

			
		}
	}
	return 1;
}
