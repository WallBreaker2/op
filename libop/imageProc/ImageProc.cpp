//#include "stdafx.h"
#include "ImageProc.h"
#include "../core/helpfunc.h"
#include <fstream>
#include <bitset>
#include <algorithm>
#include <sstream>
ImageProc::ImageProc()
{
	_curr_idx = 0;
	_enable_cache = 1;
}

ImageProc::~ImageProc()
{
}

long ImageProc::Capture(const std::wstring &file)
{
	wstring fpath = file;
	if (fpath.find(L'\\') == -1)
		fpath = _curr_path + L"\\" + fpath;

	return _src.write(fpath.data());
}

long ImageProc::CmpColor(long x, long y, const std::wstring &scolor, double sim)
{
	std::vector<color_df_t> vcolor;
	str2colordfs(scolor, vcolor);
	return ImageBase::CmpColor(_src.at<color_t>(0, 0), vcolor, sim);
}

long ImageProc::FindColor(const wstring &color, double sim, long dir, long &x, long &y)
{
	std::vector<color_df_t> colors;
	str2colordfs(color, colors);
	//setlog("%s cr size=%d",colors[0].color.tostr().data(), colors.size());
	//setlog("sim:,dir:%d", dir);
	return ImageBase::FindColor(colors, dir, x, y);
}

long ImageProc::FindColoEx(const wstring &color, double sim, long dir, wstring &retstr)
{
	std::vector<color_df_t> colors;
	str2colordfs(color, colors);
	return ImageBase::FindColorEx(colors, retstr);
}

long ImageProc::FindMultiColor(const wstring &first_color, const wstring &offset_color, double sim, long dir, long &x, long &y)
{
	std::vector<color_df_t> vfirst_color;
	str2colordfs(first_color, vfirst_color);
	std::vector<wstring> vseconds;
	split(offset_color, vseconds, L",");
	std::vector<pt_cr_df_t> voffset_cr;
	pt_cr_df_t tp;
	for (auto &it : vseconds)
	{
		size_t id1, id2;
		id1 = it.find(L'|');
		id2 = (id1 == wstring::npos ? wstring::npos : it.find(L'|', id1));
		if (id2 != wstring::npos)
		{
			swscanf(it.c_str(), L"%d|%d", &tp.x, &tp.y);
			if (id2 + 1 != it.length())
				str2colordfs(it.substr(id2 + 1), tp.crdfs);
			else
				break;
			voffset_cr.push_back(tp);
		}
	}
	return ImageBase::FindMultiColor(vfirst_color, voffset_cr, sim, dir, x, y);
}

long ImageProc::FindMultiColorEx(const wstring &first_color, const wstring &offset_color, double sim, long dir, wstring &retstr)
{
	std::vector<color_df_t> vfirst_color;
	str2colordfs(first_color, vfirst_color);
	std::vector<wstring> vseconds;
	split(offset_color, vseconds, L",");
	std::vector<pt_cr_df_t> voffset_cr;
	pt_cr_df_t tp;
	for (auto &it : vseconds)
	{
		size_t id1, id2;
		id1 = it.find(L'|');
		id2 = (id1 == wstring::npos ? wstring::npos : it.find(L'|', id1));
		if (id2 != wstring::npos)
		{
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
//图形定位
long ImageProc::FindPic(const std::wstring &files, const wstring &delta_colors, double sim, long dir, long &x, long &y)
{
	vector<Image *> vpic;
	//vector<color_t> vcolor;
	color_t dfcolor;
	vector<std::wstring> vpic_name;
	files2mats(files, vpic, vpic_name);
	dfcolor.str2color(delta_colors);
	//str2colors(delta_colors, vcolor);
	sim = 0.5 + sim / 2;
	//long ret = ImageBase::FindPic(vpic, dfcolor, sim, x, y);
	long ret = ImageBase::FindPicTh(vpic, dfcolor, sim, x, y);
	//清理缓存
	if (!_enable_cache)
		_pic_cache.clear();
	return ret;
}
//
long ImageProc::FindPicEx(const std::wstring &files, const wstring &delta_colors, double sim, long dir, wstring &retstr, bool returnID)
{
	vector<Image *> vpic;
	vpoint_desc_t vpd;
	//vector<color_t> vcolor;
	color_t dfcolor;
	vector<std::wstring> vpic_name;
	files2mats(files, vpic, vpic_name);
	dfcolor.str2color(delta_colors);
	sim = 0.5 + sim / 2;
	long ret = ImageBase::FindPicExTh(vpic, dfcolor, sim, vpd);
	std::wstringstream ss(std::wstringstream::in|std::wstringstream::out);
	if (returnID)
	{
		for (auto &it : vpd)
		{
			ss << it.id << L"," << it.pos << L"|";
		}
	}
	else
	{
		for (auto &it : vpd)
		{
			ss << vpic_name[it.id] << L"," << it.pos << L"|";
		}
	}
	//清理缓存
	if (!_enable_cache)
		_pic_cache.clear();
	retstr = ss.str();
	if (vpd.size())
		retstr.pop_back();
	return ret;
}

long ImageProc::FindColorBlock(const wstring &color, double sim, long count, long height, long width, long &x, long &y)
{

	vector<color_df_t> colors;
	if (str2colordfs(color, colors) == 0)
	{
		bgr2binary(colors);
	}
	else
	{
		bgr2binarybk(colors);
	}
	return ImageBase::FindColorBlock(sim, count, height, width, x, y);
}

long ImageProc::FindColorBlockEx(const wstring &color, double sim, long count, long height, long width, wstring &retstr)
{
	vector<color_df_t> colors;
	if (str2colordfs(color, colors) == 0)
	{
		bgr2binary(colors);
	}
	else
	{
		bgr2binarybk(colors);
	}
	return ImageBase::FindColorBlockEx(sim, count, height, width, retstr);
}

long ImageProc::SetDict(int idx, const wstring &file_name)
{
	if (idx < 0 || idx >= _max_dict)
		return 0;
	_dicts[idx].clear();
	wstring fullpath;
	if (Path2GlobalPath(file_name, _curr_path, fullpath))
	{
		if (fullpath.substr(fullpath.length() - 4) == L".txt")
			_dicts[idx].read_dict_dm(_ws2string(fullpath));
		else
			_dicts[idx].read_dict(_ws2string(fullpath));
	}
	else
	{
		setlog(L"file '%s' does not exist", file_name.c_str());
	}

	return _dicts[idx].empty() ? 0 : 1;
}

long ImageProc::SetMemDict(int idx, void *data, long size)
{
	if (idx < 0 || idx >= _max_dict)
		return 0;
	_dicts[idx].clear();
	_dicts[idx].read_memory_dict_dm((const char *)data, size);
	return _dicts[idx].empty() ? 0 : 1;
}

long ImageProc::UseDict(int idx)
{
	if (idx < 0 || idx >= _max_dict)
		return 0;
	_curr_idx = idx;
	return 1;
}

long ImageProc::OCR(const wstring &color, double sim, std::wstring &out_str)
{
	out_str.clear();
	vector<color_df_t> colors;
	if (str2colordfs(color, colors) == 0)
	{
		bgr2binary(colors);
	}
	else
	{
		bgr2binarybk(colors);
	}
	if (sim < 0. || sim > 1.)
		sim = 1.;
	long s;
	s = ImageBase::Ocr(_dicts[_curr_idx], sim, out_str);
	return s;
}

wstring ImageProc::GetColor(long x, long y)
{
	color_t cr;
	if (ImageBase::GetPixel(x, y, cr))
	{
		return _s2wstring(cr.tostr());
	}
	else
	{
		return L"";
	}
}

int ImageProc::str2colordfs(const wstring &color_str, std::vector<color_df_t> &colors)
{
	std::vector<wstring> vstr, vstr2;
	color_df_t cr;
	colors.clear();
	int ret = 0;
	if (color_str.empty())
	{ //default
		return 1;
	}
	if (color_str[0] == L'@')
	{ //bk color info
		ret = 1;
	}
	split(ret ? color_str.substr(1) : color_str, vstr, L"|");
	for (auto &it : vstr)
	{
		split(it, vstr2, L"-");
		cr.color.str2color(vstr2[0]);
		cr.df.str2color(vstr2.size() == 2 ? vstr2[1] : L"000000");
		colors.push_back(cr);
	}
	return ret;
}

void ImageProc::str2colors(const wstring &color, std::vector<color_t> &vcolor)
{
	std::vector<wstring> vstr, vstr2;
	color_t cr;
	vcolor.clear();
	split(color, vstr, L"|");
	for (auto &it : vstr)
	{
		cr.str2color(it);
		vcolor.push_back(cr);
	}
}

long ImageProc::LoadPic(const wstring &files)
{
	//std::vector<wstring>vstr, vstr2;
	std::vector<wstring> vstr;
	int loaded = 0;
	split(files, vstr, L"|");
	wstring tp;
	for (auto &it : vstr)
	{
		//路径转化
		if (!Path2GlobalPath(it, _curr_path, tp))
			continue;
		//先在缓存中查找
		if (!_pic_cache.count(tp))
		{
			_pic_cache[tp].read(tp.data());
		}
		//已存在于缓存中的文件也算加载成功
		loaded++;
	}
	return loaded;
}

long ImageProc::FreePic(const wstring &files)
{
	std::vector<wstring> vstr;
	int loaded = 0;
	split(files, vstr, L"|");
	wstring tp;
	for (auto &it : vstr)
	{
		//看当前目录
		auto cache_it = _pic_cache.find(it);
		//没查到再看一下资源目录
		if (cache_it == _pic_cache.end())
		{
			cache_it = _pic_cache.find(_curr_path + L"\\" + it);
		}
		//查到了就释放
		if (cache_it != _pic_cache.end())
		{
			cache_it->second.release();
			_pic_cache.erase(cache_it);
			loaded++;
		}
	}
	return loaded;
}

long ImageProc::LoadMemPic(const wstring &file_name, void *data, long size)
{
	try
	{
		if (!_pic_cache.count(file_name))
		{
			_pic_cache[file_name].read(data, size);
		}
	}
	catch (...)
	{
		return 0;
	}
	return 1;
}

void ImageProc::files2mats(const wstring &files, std::vector<Image *> &vpic, std::vector<wstring> &vstr)
{
	//std::vector<wstring>vstr, vstr2;
	Image *pm;
	vpic.clear();
	split(files, vstr, L"|");
	wstring tp;
	for (auto &it : vstr)
	{
		//先在缓存中查找是否已加载，包括从内存中加载的文件
		if (_pic_cache.count(it))
		{
			pm = &_pic_cache[it];
		}
		else
		{
			//路径转化
			if (!Path2GlobalPath(it, _curr_path, tp))
				continue;
			//再检测一次，包括绝对路径的文件
			if (_pic_cache.count(tp))
			{
				pm = &_pic_cache[tp];
			}
			else
			{
				_pic_cache[tp].read(tp.data());
				pm = &_pic_cache[tp];
			}
		}
		vpic.push_back(pm);
	}
}

long ImageProc::OcrEx(const wstring &color, double sim, std::wstring &out_str)
{
	out_str.clear();
	vector<color_df_t> colors;
	if (str2colordfs(color, colors) == 0)
	{
		bgr2binary(colors);
	}
	else
	{
		bgr2binarybk(colors);
	}
	if (sim < 0. || sim > 1.)
		sim = 1.;

	return ImageBase::OcrEx(_dicts[_curr_idx], sim, out_str);
}

long ImageProc::FindStr(const wstring &str, const wstring &color, double sim, long &retx, long &rety)
{
	vector<wstring> vstr;
	vector<color_df_t> colors;
	split(str, vstr, L"|");
	if (str2colordfs(color, colors) == 0)
	{
		bgr2binary(colors);
	}
	else
	{
		bgr2binarybk(colors);
	}
	if (sim < 0. || sim > 1.)
		sim = 1.;
	return ImageBase::FindStr(_dicts[_curr_idx], vstr, sim, retx, rety);
}

long ImageProc::FindStrEx(const wstring &str, const wstring &color, double sim, std::wstring &out_str)
{
	out_str.clear();
	vector<wstring> vstr;
	vector<color_df_t> colors;
	split(str, vstr, L"|");
	if (str2colordfs(color, colors) == 0)
	{
		bgr2binary(colors);
	}
	else
	{
		bgr2binarybk(colors);
	}
	if (sim < 0. || sim > 1.)
		sim = 1.;
	return ImageBase::FindStrEx(_dicts[_curr_idx], vstr, sim, out_str);
}

long ImageProc::OcrAuto(double sim, std::wstring &retstr)
{
	retstr.clear();

	if (sim < 0. || sim > 1.)
		sim = 1.;
	vector<color_df_t> colors;
	bgr2binarybk(colors);
	return ImageBase::Ocr(_dicts[_curr_idx], sim, retstr);
	//_tes.SetImage(_src.pdata, _src.width, _src.height, 4, _src.width * 4);
	//_tes.gette
	return 0;
}

long ImageProc::OcrFromFile(const wstring &files, const wstring &color, double sim, std::wstring &retstr)
{
	retstr.clear();
	if (sim < 0. || sim > 1.)
		sim = 1.;
	wstring fullpath;
	vector<color_df_t> colors;
	str2colordfs(color, colors);
	if (Path2GlobalPath(files, _curr_path, fullpath))
	{
		_src.read(fullpath.data());
		if (str2colordfs(color, colors) == 0)
		{
			bgr2binary(colors);
		}
		else
		{
			bgr2binarybk(colors);
		}
		if (sim < 0. || sim > 1.)
			sim = 1.;

		return ImageBase::Ocr(_dicts[_curr_idx], sim, retstr);
	}
	return 0;
}

long ImageProc::OcrAutoFromFile(const wstring &files, double sim, std::wstring &retstr)
{
	retstr.clear();
	if (sim < 0. || sim > 1.)
		sim = 1.;
	wstring fullpath;

	if (Path2GlobalPath(files, _curr_path, fullpath))
	{
		_src.read(fullpath.data());
		vector<color_df_t> colors;
		bgr2binarybk(colors);

		return ImageBase::Ocr(_dicts[_curr_idx], sim, retstr);
	}
	return 0;
}

long ImageProc::FindLine(const wstring &color, double sim, wstring &retStr)
{
	retStr.clear();
	vector<color_df_t> colors;
	if (str2colordfs(color, colors) == 0)
	{
		bgr2binary(colors);
	}
	else
	{
		bgr2binarybk(colors);
	}
	if (sim < 0. || sim > 1.)
		sim = 1.;

	_src.write(L"_src.bmp");
	_gray.write(L"gray.bmp");
	_binary.write(L"_binary.bmp");
	return ImageBase::FindLine(sim, retStr);
}
