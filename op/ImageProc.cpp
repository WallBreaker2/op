#include "stdafx.h"
#include "ImageProc.h"
#include "Tool.h"
#include <fstream>
#include <bitset>
#include <algorithm>
ImageProc::ImageProc()
{
	_curr_dict = _dicts;
}


ImageProc::~ImageProc()
{
}

long ImageProc::FindPic(long x1, long y1, long x2, long y2, const std::wstring& files, double sim, long& x, long &y) {
	return 0;
}

long ImageProc::SetDict(int idx, const wstring& file_name) {
	if (idx < 0 || idx >= _max_dicts)
		return 0;
	_dicts[idx].clear();
	_dicts[idx].read_dict(file_name);
	if (_dicts->info._word_count)
		return 1;
	else
		return 0;

}

long ImageProc::UseDict(int idx) {
	if (idx < 0 || idx >= _max_dicts)
		return 0;
	_curr_dict = &_dicts[idx];
	return 1;
}

long ImageProc::OCR(const wstring& color, double sim, std::wstring& out_str) {
	out_str.clear();
	vector<color_df_t> colors;
	str2colordfs(color, colors);
	if (sim<0. || sim>1.)
		sim = 1.;

	long s;
	ImageExtend::bgr2binary(colors);

	s = ImageExtend::Ocr(*_curr_dict, sim, out_str);
	return s;

}

long ImageProc::FindColor(const wstring& color, long&x, long&y) {
	std::vector<color_df_t>colors;
	str2colordfs(color, colors);
	return ImageExtend::FindColor(colors, x, y);
}

wstring ImageProc::GetColor(long x, long y) {
	color_t cr;
	if (ImageExtend::GetPixel(x, y, cr)) {
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
