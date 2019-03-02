#include "stdafx.h"
#include "ImageLoc.h"
#include "Common.h"
#include "ocr.h"
#include "Tool.h"




ImageBase::ImageBase()
{
	_x1 = _y1 = 0;
	_dx = _dy = 0;
}


ImageBase::~ImageBase()
{
}


long ImageBase::input_image(byte* image_data, int width, int height, long x1, long y1, long x2, long y2, int type) {
	int i, j;
	_x1 = x1; _y1 = y1;
	int cw = x2 - x1 - 1, ch = y2 - y1 - 1;
	if (type == -1) {//倒过来读
		_src.create(ch, cw, CV_8UC3);
		uchar *p, *p2;
		for (i = 0; i < ch; ++i) {
			p = _src.ptr<uchar>(i);
			p2 = image_data + (height - i - 1 - y1) * width * 4 + x1 * 4;//偏移
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

void ImageBase::set_offset(int dx, int dy) {
	_dx = -dx;
	_dy = -dy;
}

void ImageBase::bgr2binary(vector<color_df_t>& colors) {
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

//二值化
void ImageBase::graytobinary()
{
	cv::cvtColor(_src, _target, CV_BGR2GRAY);
	auto mval = cv::mean(_target);
	int c1 = WORD_COLOR, c2 = WORD_BKCOLOR;
	// black bk
	if (mval[0] < 128) std::swap(c1, c2);

	_binary.create(_target.size(), CV_8UC1);
	for (int i = 0; i < _target.rows; ++i) {
		auto p1 = _target.ptr<uchar>(i);
		auto p2 = _binary.ptr<uchar>(i);
		for (int j = 0; j < _target.cols; ++j) {
			p2[j] = (p1[j] < 128 ? c1 : c2);
		}
	}
}

//long ImageBase::imageloc(images_t& images, double sim, long&x, long&y) {
//	x = y = -1;
//	if (_src.empty())return 0;
//	//cv::imwrite("input.png", _src);
//	for (auto&it : images) {
//		_target = cv::imread(_wsto_string(it));
//		if (_target.empty()) {
//			//setlog(L"ImageExtend::imageloc,file:%s read false.", it.c_str());
//			continue;
//		}
//		//cv::cvtColor(_src, _src_gray, CV_BGR2GRAY);
//		//cv::cvtColor(_target, _target_gray, CV_BGR2GRAY);
//		int result_cols = _src.cols - _target.cols + 1;
//		int result_rows = _src.rows - _target.rows + 1;
//		_result.create(result_rows, result_cols, CV_32FC1);
//		cv::matchTemplate(_src, _target, _result, CV_TM_SQDIFF);
//		//cv::normalize(_result, _result, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());
//
//
//		double minval, maxval, bestval;
//		cv::Point pt1, pt2;
//		cv::minMaxLoc(_result, &minval, &maxval, &pt1, &pt2);
//		minval /= 255.*255.*_target.rows*_target.cols;
//		bestval = 1. - minval;
//		//setlog(L"ImageExtend::imageloc(),file=%s, bestval=%lf,pos=[%d,%d]",it.c_str(), bestval, pt1.x, pt1.y);
//
//		if (bestval >= sim) {
//			x = pt1.x; y = pt1.y;
//			return 1;
//		}
//		else {
//			return 0;
//		}
//	}
//	return 0;
//
//
//}

long ImageBase::simple_match(long x, long y, cv::Mat* timg, color_t dfcolor, int max_error) {
	int err_ct = 0, k;
	for (int i = 0; i < timg->rows; ++i) {
		auto p1 = _src.ptr<uchar>(i + y) + x * 3;
		auto p2 = timg->ptr<uchar>(i);
		for (int j = 0; j < timg->cols; ++j) {
			if (*(color_t*)p1 - *(color_t*)p2 > dfcolor)
				++err_ct;
			if (err_ct > max_error)
				return 0;
			p1 += 3; p2 += 3;
		}
	}
	return 1;
}

long ImageBase::GetPixel(long x, long y, color_t&cr) {
	if (!is_valid(x, y))
		return 0;
	auto p = _src.ptr<uchar>(y) + 3 * x;
	static_assert(sizeof(color_t) == 4);
	cr.b = p[0]; cr.g = p[1]; cr.r = p[2];
	return 1;
}

long ImageBase::CmpColor(long x, long y, std::vector<color_df_t>&colors, double sim) {
	color_t cr;
	if (GetPixel(x, y, cr)) {
		for (auto&it : colors) {
			if (it.color - cr <= it.df)
				return 1;
		}
	}
	return 0;
}

long ImageBase::FindColor(vector<color_df_t>& colors, long&x, long&y) {
	for (int i = 0; i < _src.rows; ++i) {
		uchar* p = _src.ptr<uchar>(i);
		for (int j = 0; j < _src.cols; ++j) {
			for (auto&it : colors) {//对每个颜色描述
				if ((*(color_t*)p - it.color) <= it.df) {
					x = j+_x1+_dx; y = i+_y1+_dy;
					return 1;
				}
			}
			p += 3;
		}
	}
	x = y = -1;
	return 0;
}

long ImageBase::FindColorEx(vector<color_df_t>& colors, std::wstring& retstr) {
	retstr.clear();
	int find_ct = 0;
	for (int i = 0; i < _src.rows; ++i) {
		uchar* p = _src.ptr<uchar>(i);
		for (int j = 0; j < _src.cols; ++j) {
			for (auto&it : colors) {//对每个颜色描述
				if ((*(color_t*)p - it.color) <= it.df) {
					retstr += std::to_wstring(j+_x1+_dx) + L"," + std::to_wstring(i+_y1+_dy);
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

long ImageBase::FindMultiColor(std::vector<color_df_t>&first_color, std::vector<pt_cr_df_t>& offset_color, double sim, long dir, long&x, long&y) {
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
					x = j+_x1+_dx, y = i + _y1 + _dy;
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

long ImageBase::FindMultiColorEx(std::vector<color_df_t>&first_color, std::vector<pt_cr_df_t>& offset_color, double sim, long dir, std::wstring& retstr) {
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
					retstr += std::to_wstring(j + _x1 + _dx) + L"," + std::to_wstring(i + _y1 + _dy);
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

long ImageBase::FindPic(std::vector<cv::Mat*>&pics, color_t dfcolor, double sim, long&x, long&y) {

	for (int i = 0; i < _src.rows; ++i) {
		uchar* p = _src.ptr<uchar>(i);
		for (int j = 0; j < _src.cols; ++j) {
			for (auto pic : pics) {
				//step 1. 边界检查
				if (i + pic->rows > _src.rows || j + pic->cols > _src.cols)
					continue;
				//step 2. 计算最大误差
				int max_err_ct = pic->rows*pic->cols*(1.0 - sim);
				//step 3. 开始匹配
				if (simple_match(j, i, pic, dfcolor, max_err_ct)) {
					x = j + _x1 + _dx; y = i + _y1 + _dy;
					return 1;
				}
			}//end for pics

		}//end for j
	}//end for i
	x = y = -1;
	return 0;
}

long ImageBase::FindPicEx(std::vector<cv::Mat*>&pics, color_t dfcolor, double sim, wstring& retstr) {
	int obj_ct = 0;
	retstr.clear();
	for (int i = 0; i < _src.rows; ++i) {
		uchar* p = _src.ptr<uchar>(i);
		for (int j = 0; j < _src.cols; ++j) {
			for (auto pic : pics) {
				//step 1. 边界检查
				if (i + pic->rows > _src.rows || j + pic->cols > _src.cols)
					continue;
				//step 2. 计算最大误差
				int max_err_ct = pic->rows*pic->cols*(1.0 - sim);
				//step 3. 开始匹配
				if (simple_match(j, i, pic, dfcolor, max_err_ct)) {
					retstr += std::to_wstring(j + _x1 + _dx) + L"," + std::to_wstring(i + _y1 + _dy);
					retstr += L"|";
					++obj_ct;
					if (obj_ct > _max_return_obj_ct)
						goto _quick_return;
					else
						break;
				}
			}//end for pics

		}//end for j
	}//end for i
_quick_return:
	return obj_ct;
}


long ImageBase::Ocr(Dict& dict, double sim, wstring& retstr) {
	retstr.clear();
	std::map<point_t, wstring> ps;
	bin_ocr(_binary, _target, dict, sim, ps);
	for (auto&it : ps) {
		retstr += it.second;
	}
	return 1;
}

long ImageBase::OcrEx(Dict& dict, double sim, std::wstring& retstr) {
	retstr.clear();
	std::map<point_t, wstring> ps;
	bin_ocr(_binary, _target, dict, sim, ps);
	//x1,y1,str....|x2,y2,str2...|...
	int find_ct = 0;
	for (auto&it : ps) {
		retstr += std::to_wstring(it.first.x + _x1 + _dx);
		retstr += L",";
		retstr += std::to_wstring(it.first.y + _y1 + _dy);
		retstr += L",";
		retstr += it.second;
		retstr += L"|";
		++find_ct;
		if (find_ct > _max_return_obj_ct)
			break;
	}
	if (!retstr.empty() && retstr.back() == L'|')
		retstr.pop_back();
	return find_ct;
}

long ImageBase::FindStr(Dict& dict, const vector<wstring>& vstr, double sim, long& retx, long& rety) {
	retx = rety = -1;
	std::map<point_t, wstring> ps;
	bin_ocr(_binary, _target, dict, sim, ps);
	for (auto&it : ps) {
		for (auto&s : vstr) {
			if (it.second == s) {
				retx = it.first.x + _x1 + _dx;
				rety = it.first.y + _y1 + _dy;
				return 1;
			}
		}
	}
	return 0;
}

long ImageBase::FindStrEx(Dict& dict, const vector<wstring>& vstr, double sim, std::wstring& retstr) {
	retstr.clear();
	std::map<point_t, wstring> ps;
	bin_ocr(_binary, _target, dict, sim, ps);
	int find_ct = 0;
	for (auto&it : ps) {
		for (auto&s : vstr) {
			if (it.second == s) {
				retstr += std::to_wstring(it.first.x + _x1 + _dx);
				retstr += L",";
				retstr += std::to_wstring(it.first.y + _y1 + _dy);
				retstr += L"|";
				++find_ct;
				if (find_ct > _max_return_obj_ct)
					goto _quick_return;
				else
					break;
			}
		}
	}
_quick_return:
	if (!retstr.empty() && retstr.back() == L'|')
		retstr.pop_back();
	return find_ct;
}
