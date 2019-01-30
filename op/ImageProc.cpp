#include "stdafx.h"
#include "ImageProc.h"
#include "Tool.h"
#include <fstream>
#include <bitset>
#include <algorithm>
ImageProc::ImageProc()
{
	_curr_idx = 0;
}


ImageProc::~ImageProc()
{
}

long ImageProc::Capture(const std::wstring& file) {
	return cv::imwrite(_wsto_string(file),_src);
}
long ImageProc::CmpColor(long x, long y, const std::wstring& scolor, double sim) {
	std::vector<color_df_t> vcolor;
	str2colordfs(scolor, vcolor);
	return ImageBase::CmpColor(x, y, vcolor, sim);

	return 0;
	
}

long ImageProc::FindColor(const wstring& color, double sim, long dir, long&x, long&y) {
	std::vector<color_df_t>colors;
	str2colordfs(color, colors);
	return ImageBase::FindColor(colors, x, y);
}

long ImageProc::FindColoEx(const wstring& color, double sim, long dir, wstring& retstr) {
	std::vector<color_df_t>colors;
	str2colordfs(color, colors);
	return ImageBase::FindColorEx(colors, retstr);
}

long ImageProc::FindMultiColor(const wstring& first_color, const wstring& offset_color, double sim, long dir, long&x, long&y) {
	std::vector<color_df_t>vfirst_color;
	str2colordfs(first_color, vfirst_color);
	std::vector<wstring> vseconds;
	Tool::split(offset_color, vseconds, L",");
	std::vector<pt_cr_df_t>voffset_cr;
	pt_cr_df_t tp;
	for (auto&it : vseconds) {
		size_t id1, id2;
		id1 = it.find(L'|');
		id2 = (id1 == wstring::npos ? wstring::npos : it.find(L'|', id1));
		if (id2 != wstring::npos) {
			swscanf(it.c_str(), L"%d|%d", &tp.x, &tp.y);
			if (id2 + 1 != it.length())
				str2colordfs(it.substr(id2 + 1), tp.crdfs);
			else
				break;
			voffset_cr.push_back(tp);
		}
	}
	return ImageBase::FindMultiColor(vfirst_color,voffset_cr,sim,dir,x,y);
}

long ImageProc::FindMultiColorEx(const wstring& first_color, const wstring& offset_color, double sim, long dir, wstring& retstr) {
	std::vector<color_df_t>vfirst_color;
	str2colordfs(first_color, vfirst_color);
	std::vector<wstring> vseconds;
	Tool::split(offset_color, vseconds, L",");
	std::vector<pt_cr_df_t>voffset_cr;
	pt_cr_df_t tp;
	for (auto&it : vseconds) {
		size_t id1, id2;
		id1 = it.find(L'|');
		id2 = (id1 == wstring::npos ? wstring::npos : it.find(L'|', id1));
		if (id2 != wstring::npos) {
			swscanf(it.c_str(), L"%d|%d", &tp.x, &tp.y);
			if (id2 + 1 != it.length())
				str2colordfs(it.substr(id2 + 1), tp.crdfs);
			else
				break;
			voffset_cr.push_back(tp);
		}
	}
	return ImageBase::FindMultiColorEx(vfirst_color, voffset_cr, sim, dir, retstr);
}
//Í¼ÐÎ¶¨Î»
long ImageProc::FindPic(long x1, long y1, long x2, long y2, const std::wstring& files, const wstring& delta_colors, double sim, long dir, long& x, long &y) {
	return 0;
}
//
long ImageProc::FindPicEx(long x1, long y1, long x2, long y2, const std::wstring& files, const wstring& delta_colors, double sim, long dir, wstring& retstr) {
	return 0;
}


long ImageProc::SetDict(int idx, const wstring& file_name) {
	if (idx < 0 || idx >= _max_dict)
		return 0;
	_dicts[idx].clear();
	_dicts[idx].read_dict(file_name);
	if (_dicts->info._word_count)
		return 1;
	else
		return 0;

}

long ImageProc::UseDict(int idx) {
	if (idx < 0 || idx >= _max_dict)
		return 0;
	_curr_idx = idx;
	return 1;
}

long ImageProc::OCR(const wstring& color, double sim, std::wstring& out_str) {
	out_str.clear();
	vector<color_df_t> colors;
	str2colordfs(color, colors);
	if (sim<0. || sim>1.)
		sim = 1.;

	long s;
	ImageBase::bgr2binary(colors);

	s = ImageBase::Ocr(_dicts[_curr_idx], sim, out_str);
	return s;

}


wstring ImageProc::GetColor(long x, long y) {
	color_t cr;
	if (ImageBase::GetPixel(x, y, cr)) {
		return cr.tostr();
	}
	else {
		return L"";
	}
}


void ImageProc::str2colordfs(const wstring& color_str, std::vector<color_df_t>& colors) {
	std::vector<wstring>vstr, vstr2;
	color_df_t cr;
	colors.clear();
	Tool::split(color_str, vstr, L"|");
	for (auto&it : vstr) {
		Tool::split(it, vstr2, L"-");
		cr.color.str2color(vstr2[0]);
		cr.df.str2color(vstr2.size() == 2 ? vstr2[1] : L"000000");
		colors.push_back(cr);
	}
}

long ImageProc::OcrEx(const wstring& color, double sim, std::wstring& out_str) {
	out_str.clear();
	vector<color_df_t> colors;
	str2colordfs(color, colors);
	if (sim<0. || sim>1.)
		sim = 1.;
	ImageBase::bgr2binary(colors);
	return ImageBase::OcrEx(_dicts[_curr_idx], sim, out_str);
}

long ImageProc::FindStr(const wstring& str, const wstring& color, double sim, long& retx, long& rety) {
	vector<wstring> vstr;
	vector<color_df_t> colors;
	Tool::split(str, vstr, L"|");
	str2colordfs(color, colors);
	if (sim<0. || sim>1.)
		sim = 1.;
	ImageBase::bgr2binary(colors);
	return ImageBase::FindStr(_dicts[_curr_idx], vstr, sim, retx, rety);
}

long ImageProc::FindStrEx(const wstring& str, const wstring& color, double sim, std::wstring& out_str) {
	out_str.clear();
	vector<wstring> vstr;
	vector<color_df_t> colors;
	Tool::split(str, vstr, L"|");
	str2colordfs(color, colors);
	if (sim<0. || sim>1.)
		sim = 1.;
	ImageBase::bgr2binary(colors);
	return ImageBase::FindStrEx(_dicts[_curr_idx], vstr, sim, out_str);
}

long ImageProc::OcrAuto(double sim, std::wstring& retstr) {
	retstr.clear();
	
	if (sim<0. || sim>1.)
		sim = 1.;
	ImageBase::graytobinary();
	return ImageBase::Ocr(_dicts[_curr_idx], sim,retstr);
}
