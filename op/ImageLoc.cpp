#include "stdafx.h"
#include "ImageLoc.h"
#include "helpfunc.h"

using std::to_wstring;
//检查是否为透明图，返回透明像素个数
int check_transparent(Image* img) {
	if (img->width < 2 || img->height < 2)
		return 0;
	uint c0 = *img->begin();
	bool x = c0 == img->at<uint>(0, img->width - 1) &&
		c0 == img->at<uint>(img->height - 1, 0) &&
		c0 == img->at<uint>(img->height - 1, img->width - 1);
	if (!x)
		return 0;

	int ct = 0;
	for (auto it : *img)
		if (it == c0)
			++ct;

	return ct < img->height*img->width ? ct : 0;
}

void get_match_points(const Image& img, vector<uint>&points) {
	points.clear();
	uint cbk = *img.begin();
	for (int i = 0; i < img.height; ++i) {
		for (int j = 0; j < img.width; ++j)
			if (cbk != img.at<uint>(i, j))
				points.push_back((i << 16) | j);
	}
}


ImageBase::ImageBase()
{
	_x1 = _y1 = 0;
	_dx = _dy = 0;
}


ImageBase::~ImageBase()
{
}


long ImageBase::input_image(byte* psrc, int width, int height, long x1, long y1, long x2, long y2, int type) {
	int i, j;
	_x1 = x1; _y1 = y1;
	int cw = x2 - x1, ch = y2 - y1;
	_src.create(cw, ch);
	if (type == -1) {//倒过来读
		uchar *p, *p2;
		for (i = 0; i < ch; ++i) {
			p = _src.ptr<uchar>(i);
			p2 = psrc + (height - i - 1 - y1) * width * 4 + x1 * 4;//偏移
			memcpy(p, p2, 4 * cw);
		}
	}
	else {
		uchar *p, *p2;
		for (i = 0; i < ch; ++i) {
			p = _src.ptr<uchar>(i);
			p2 = psrc + (i + y1) * width * 4 + x1 * 4;
			memcpy(p, p2, 4 * cw);
		}
	}
	return 1;
}

void ImageBase::set_offset(int dx, int dy) {
	_dx = -dx;
	_dy = -dy;
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
//		//cv::normalize(_result, _result, 0, 1, cv::NORM_MINMAX, -1, Image());
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
template<bool nodfcolor>
long ImageBase::simple_match(long x, long y, Image* timg, color_t dfcolor, int max_error) {
	int err_ct = 0;


	uint* pscreen_top, *pscreen_bottom, *pimg_top, *pimg_bottom;
	pscreen_top = _src.ptr<uint>(y) + x;
	pscreen_bottom = _src.ptr<uint>(y + timg->height - 1) + x;
	pimg_top = timg->ptr<uint>(0);
	pimg_bottom = timg->ptr<uint>(timg->height - 1);
	while (pscreen_top <= pscreen_bottom) {

		auto ps1 = pscreen_top, ps2 = pscreen_top + timg->width - 1;
		auto ps3 = pscreen_bottom, ps4 = pscreen_bottom + timg->width - 1;
		auto pt1 = pimg_top, pt2 = pimg_top + timg->width - 1;
		auto pt3 = pimg_bottom, pt4 = pimg_bottom + timg->width - 1;
		while (ps1 <= ps2) {
			if (nodfcolor) {
				if (*ps1++ != *pt1++)++err_ct;//top left
				if (*ps2-- != *pt2--)++err_ct;//top right
				if (*ps3++ != *pt3++)++err_ct;//bottom left
				if (*ps4-- != *pt4--)++err_ct;//bottom right
			}
			else {
				if (!IN_RANGE(*(color_t*)ps1++, *(color_t*)pt1++, dfcolor))
					++err_ct;
				if (!IN_RANGE(*(color_t*)ps2--, *(color_t*)pt2--, dfcolor))
					++err_ct;
				if (!IN_RANGE(*(color_t*)ps3++, *(color_t*)pt3++, dfcolor))
					++err_ct;
				if (!IN_RANGE(*(color_t*)ps4--, *(color_t*)pt4--, dfcolor))
					++err_ct;
			}

			if (err_ct > max_error)
				return 0;
		}
		pscreen_top += _src.width;
		pscreen_bottom -= _src.width;
	}

	return 1;
}
template<bool nodfcolor>
long ImageBase::trans_match(long x, long y, Image* timg, color_t dfcolor, vector<uint>pts, int max_error) {
	int err_ct = 0, k, dx, dy;
	int left, right;
	left = 0; right = pts.size() - 1;
	while(left<=right){
		auto it = pts[left];
		if (nodfcolor) {
			if (_src.at<uint>(y + PTY(pts[left]), x + PTX(pts[left])) != timg->at<uint>(PTY(pts[left]), PTX(pts[left])))
				++err_ct;
			if (_src.at<uint>(y + PTY(pts[right]), x + PTX(pts[right])) != timg->at<uint>(PTY(pts[right]), PTX(pts[right])))
				++err_ct;
		}
		else {
			color_t cr1, cr2;
			cr1 = _src.at<color_t>(y + PTY(pts[left]), x + PTX(pts[left]));
			cr2 = timg->at<color_t>(PTY(pts[left]), PTX(pts[left]));
			if (!IN_RANGE(cr1, cr2, dfcolor))
				++err_ct;
			cr1 = _src.at<color_t>(y + PTY(pts[right]), x + PTX(pts[right]));
			cr2 = timg->at<color_t>(PTY(pts[right]), PTX(pts[right]));
			if (!IN_RANGE(cr1, cr2, dfcolor))
				++err_ct;
		}
		
		++left; --right;
		if (err_ct > max_error)
			return 0;
	}
	return 1;
}

//long ImageBase::ndiff_match(long x, long y, Image* timg, int max_error) {
//	int err_ct = 0, k;
//	//background color
//	uint c0 = timg->at<uint>(0, 0);
//	for (int i = 0; i < timg->rows; ++i) {
//		auto p1 = _src.ptr<uchar>(i + y) + x * 4;
//		auto p2 = timg->ptr<uchar>(i);
//		for (int j = 0; j < timg->cols; ++j) {
//			if (*(uint*)p2 != c0) {//ignore background color
//				if (*(uint*)p1 - *(uint*)p2)
//					++err_ct;
//				if (err_ct > max_error)
//					return 0;
//			}
//
//			p1 += 4; p2 += 3;
//		}
//	}
//	return 1;
//}

long ImageBase::GetPixel(long x, long y, color_t&cr) {
	if (!is_valid(x, y)) {
		setlog("Invalid pos:%d %d", x, y);
		return 0;
	}

	auto p = _src.ptr<uchar>(y) + 4 * x;
	static_assert(sizeof(color_t) == 4);
	cr.b = p[0]; cr.g = p[1]; cr.r = p[2];
	return 1;
}

long ImageBase::CmpColor(long x, long y, std::vector<color_df_t>&colors, double sim) {
	color_t cr;
	if (GetPixel(x, y, cr)) {
		for (auto&it : colors) {
			if (IN_RANGE(cr, it.color, it.df))
				return 1;
		}
	}
	return 0;
}

long ImageBase::FindColor(vector<color_df_t>& colors, long&x, long&y) {
	for (int i = 0; i < _src.height; ++i) {
		auto p = _src.ptr<color_t>(i);
		for (int j = 0; j < _src.width; ++j) {
			for (auto&it : colors) {//对每个颜色描述
				if (IN_RANGE(*p, it.color, it.df)) {
					x = j + _x1 + _dx; y = i + _y1 + _dy;
					return 1;
				}
			}
			p++;
		}
	}
	x = y = -1;
	return 0;
}

long ImageBase::FindColorEx(vector<color_df_t>& colors, std::wstring& retstr) {
	retstr.clear();
	int find_ct = 0;
	for (int i = 0; i < _src.height; ++i) {
		auto p = _src.ptr<color_t>(i);
		for (int j = 0; j < _src.width; ++j) {
			for (auto&it : colors) {//对每个颜色描述
				if (IN_RANGE(*p, it.color, it.df)) {
					retstr += std::to_wstring(j + _x1 + _dx) + L"," + std::to_wstring(i + _y1 + _dy);
					retstr += L"|";
					++find_ct;
					//return 1;
					if (find_ct > _max_return_obj_ct)
						goto _quick_break;
					break;
				}
			}
			p++;
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
	for (int i = 0; i < _src.height; ++i) {
		auto p = _src.ptr<color_t>(i);
		for (int j = 0; j < _src.width; ++j) {
			//step 1. find first color
			for (auto&it : first_color) {//对每个颜色描述
				if (IN_RANGE(*p, it.color, it.df)) {
					//匹配其他坐标
					err_ct = 0;
					for (auto&off_cr : offset_color) {
						if (!CmpColor(j + off_cr.x, i + off_cr.y, off_cr.crdfs, sim))
							++err_ct;
						if (err_ct > max_err_ct)
							goto _quick_break;
					}
					//ok
					x = j + _x1 + _dx, y = i + _y1 + _dy;
					return 1;
				}
			}
		_quick_break:
			p++;
		}
	}
	x = y = -1;
	return 0;
}

long ImageBase::FindMultiColorEx(std::vector<color_df_t>&first_color, std::vector<pt_cr_df_t>& offset_color, double sim, long dir, std::wstring& retstr) {
	int max_err_ct = offset_color.size()*(1. - sim);
	int err_ct;
	int find_ct = 0;
	for (int i = 0; i < _src.height; ++i) {
		auto p = _src.ptr<color_t>(i);
		for (int j = 0; j < _src.width; ++j) {
			//step 1. find first color
			for (auto&it : first_color) {//对每个颜色描述
				if (IN_RANGE(*p, it.color, it.df)) {
					//匹配其他坐标
					err_ct = 0;
					for (auto&off_cr : offset_color) {
						if (!CmpColor(j + off_cr.x, i + off_cr.y, off_cr.crdfs, sim))
							++err_ct;
						if (err_ct > max_err_ct)
							goto _quick_break;
					}
					//ok
					retstr += to_wstring(j + _x1 + _dx) + L"," + to_wstring(i + _y1 + _dy);
					retstr += L"|";
					++find_ct;
					if (find_ct > _max_return_obj_ct)
						goto _quick_return;
					else
						goto _quick_break;
				}
			}
		_quick_break:
			p++;
		}
	}
_quick_return:
	if (!retstr.empty() && retstr.back() == L'|')
		retstr.pop_back();
	return find_ct;
	//x = y = -1;
}

long ImageBase::FindPic(std::vector<Image*>&pics, color_t dfcolor, double sim, long&x, long&y) {
	x = y = -1;
	vector<uint> points;
	bool nodfcolor = color2uint(dfcolor) == 0;
	int match_ret = 0;
	//将小循环放在最外面，提高处理速度
	for (int pic_id = 0; pic_id < pics.size(); ++pic_id) {
		auto pic = pics[pic_id];
		int use_ts_match = check_transparent(pic);

		if (use_ts_match)
			get_match_points(*pic, points);

		for (int i = 0; i < _src.height; ++i) {
			for (int j = 0; j < _src.width; ++j) {
				//step 1. 边界检查
				if (i + pic->height > _src.height || j + pic->width > _src.width)
					continue;
				//step 2. 计算最大误差
				int max_err_ct = (pic->height*pic->width - use_ts_match)*(1.0 - sim);
				//step 3. 开始匹配
				if (nodfcolor)
					match_ret = (use_ts_match ? trans_match<true>(j, i, pic, dfcolor, points, max_err_ct) :
						simple_match<true>(j, i, pic, dfcolor, max_err_ct));
				else
					match_ret = (use_ts_match ? trans_match<false>(j, i, pic, dfcolor, points, max_err_ct) :
						simple_match<false>(j, i, pic, dfcolor, max_err_ct));
				if (match_ret) {
					x = j + _x1 + _dx; y = i + _y1 + _dy;
					return pic_id;
				}

			}//end for j
		}//end for i
	}//end for pics


	return -1;
}

long ImageBase::FindPicEx(std::vector<Image*>&pics, color_t dfcolor, double sim, wstring& retstr) {
	int obj_ct = 0;
	retstr.clear();
	vector<uint> points;
	bool nodfcolor = color2uint(dfcolor) == 0;
	int match_ret = 0;

	for (int pic_id = 0; pic_id < pics.size(); ++pic_id) {
		auto pic = pics[pic_id];
		int use_ts_match = check_transparent(pic);

		if (use_ts_match)
			get_match_points(*pic, points);

		for (int i = 0; i < _src.height; ++i) {
			for (int j = 0; j < _src.width; ++j) {

				//step 1. 边界检查
				if (i + pic->height > _src.height || j + pic->width > _src.width)
					continue;
				//step 2. 计算最大误差
				int max_err_ct = (pic->height*pic->width - use_ts_match)*(1.0 - sim);
				//step 3. 开始匹配
				if (nodfcolor)
					match_ret = (use_ts_match ? trans_match<true>(j, i, pic, dfcolor, points, max_err_ct) :
						simple_match<true>(j, i, pic, dfcolor, max_err_ct));
				else
					match_ret = (use_ts_match ? trans_match<false>(j, i, pic, dfcolor, points, max_err_ct) :
						simple_match<false>(j, i, pic, dfcolor, max_err_ct));
				if (match_ret) {
					retstr += std::to_wstring(j + _x1 + _dx) + L"," + std::to_wstring(i + _y1 + _dy);
					retstr += L"|";
					++obj_ct;
					if (obj_ct > _max_return_obj_ct)
						goto _quick_return;
					else
						break;
				}


			}//end for j
		}//end for i
	}//end for pics
_quick_return:
	return obj_ct;
}


long ImageBase::Ocr(Dict& dict, double sim, wstring& retstr) {
	retstr.clear();
	std::map<point_t, wstring> ps;
	bin_ocr(_binary, _record, dict, sim, ps);
	for (auto&it : ps) {
		retstr += it.second;
	}
	return 1;
}

long ImageBase::OcrEx(Dict& dict, double sim, std::wstring& retstr) {
	retstr.clear();
	std::map<point_t, wstring> ps;
	bin_ocr(_binary, _record, dict, sim, ps);
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
	bin_ocr(_binary, _record, dict, sim, ps);
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
	bin_ocr(_binary, _record, dict, sim, ps);
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
