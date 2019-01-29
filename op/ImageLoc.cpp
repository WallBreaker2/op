#include "stdafx.h"
#include "ImageLoc.h"
#include "Common.h"
#include "ocr.h"
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

long ImageExtend::CmpColor(long x, long y, std::vector<color_df_t>&colors, double sim) {
	color_t cr;
	if (GetPixel(x, y, cr)) {
		for (auto&it : colors) {
			if (it.color - cr <= it.df)
				return 1;
		}
	}
	return 0;
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

long ImageExtend::FindColorEx(vector<color_df_t>& colors,std::wstring& retstr) {
	retstr.clear();
	int find_ct = 0;
	for (int i = 0; i < _src.rows; ++i) {
		uchar* p = _src.ptr<uchar>(i);
		for (int j = 0; j < _src.cols; ++j) {
			for (auto&it : colors) {//对每个颜色描述
				if ((*(color_t*)p - it.color) <= it.df) {
					retstr += std::to_wstring(j) +L","+std::to_wstring(i);
					retstr += L"|";
					++find_ct;
					//return 1;
					if (find_ct > _max_return_obj_ct)
						goto _quick_break;
					break;
				}
			}
			p += 3;
		}
	}
	_quick_break:
	if (!retstr.empty() && retstr.back() == L'|')
		retstr.pop_back();
	return find_ct;
}

long ImageExtend::FindMultiColor(std::vector<color_df_t>&first_color, std::vector<pt_cr_df_t>& offset_color, double sim, long dir, long&x, long&y) {
	int max_err_ct = offset_color.size()*(1. - sim);
	int err_ct;
	for (int i = 0; i < _src.rows; ++i) {
		uchar* p = _src.ptr<uchar>(i);
		for (int j = 0; j < _src.cols; ++j) {
			//step 1. find first color
			for (auto&it : first_color) {//对每个颜色描述
				if ((*(color_t*)p - it.color) <= it.df) {
					//匹配其他坐标
					err_ct = 0;
					for (auto&off_cr : offset_color) {
						if (!CmpColor(j + off_cr.x, i + off_cr.y, off_cr.crdfs, sim))
							++err_ct;
						if (err_ct > max_err_ct)
							goto _quick_break;
					}
					//ok
					x = j, y = i;
					return 1;
				}
			}
			_quick_break:
			p += 3;
		}
	}
	x = y = -1;
	return 0;
}

long ImageExtend::FindMultiColorEx(std::vector<color_df_t>&first_color, std::vector<pt_cr_df_t>& offset_color, double sim, long dir, std::wstring& retstr) {
	int max_err_ct = offset_color.size()*(1. - sim);
	int err_ct;
	int find_ct = 0;
	for (int i = 0; i < _src.rows; ++i) {
		uchar* p = _src.ptr<uchar>(i);
		for (int j = 0; j < _src.cols; ++j) {
			//step 1. find first color
			for (auto&it : first_color) {//对每个颜色描述
				if ((*(color_t*)p - it.color) <= it.df) {
					//匹配其他坐标
					err_ct = 0;
					for (auto&off_cr : offset_color) {
						if (!CmpColor(j + off_cr.x, i + off_cr.y, off_cr.crdfs, sim))
							++err_ct;
						if (err_ct > max_err_ct)
							goto _quick_break;
					}
					//ok
					retstr += std::to_wstring(j) + L"," + std::to_wstring(i);
					retstr += L"|";
					++find_ct;
					if (find_ct > _max_return_obj_ct)
						goto _quick_return;
					else
						goto _quick_break;
				}
			}
		_quick_break:
			p += 3;
		}
	}
_quick_return:
	if (!retstr.empty() && retstr.back() == L'|')
		retstr.pop_back();
	return find_ct;
	//x = y = -1;
}

void ImageExtend::bgr2binary(vector<color_df_t>& colors) {
	if (_src.empty())
		return;
	int ncols = _src.cols, nrows = _src.rows;
	_binary.create(nrows, ncols, CV_8UC1);
	for (int i = 0; i < nrows; ++i) {
		uchar* p = _src.ptr<uchar>(i);
		uchar* p2 = _binary.ptr<uchar>(i);
		for (int j = 0; j < ncols; ++j) {
			*p2 = WORD_BKCOLOR;
			for (auto&it : colors) {//对每个颜色描述
				if ((*(color_t*)p - it.color) <= it.df) {
					*p2 = WORD_COLOR;
					break;
				}	
			}
			++p2;
			p += 3;
		}
	}
	//test
	//cv::imwrite("src.png", _src);
	//cv::imwrite("binary.png", _binary);
}

long ImageExtend::Ocr(Dict& dict, double sim,wstring& ret_str) {
	ret_str.clear();
	long ret_val = 0;
	bin_ocr(_binary, _target, dict,sim, ret_str);
	return 1;
}
