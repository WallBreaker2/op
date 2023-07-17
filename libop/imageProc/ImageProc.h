#pragma once
#include <string>
#include "ImageLoc.h"
#include <map>
//#include <tesseract/baseapi.h>
using std::wstring;
/*
此类为图像处理，包含以下工作
1.像素比较，查找
2.颜色转化
3.图像定位
4.简单OCR
5....
*/
class ImageProc:public ImageBase
{
public:
	const static int _max_dict = 10;
	
	ImageProc();
	~ImageProc();
	//
	long Capture(const std::wstring& file);

	long CmpColor(long x, long y, const std::wstring& scolor, double sim);

	long FindColor(const wstring& color,double sim,long dir, long&x, long&y);

	long FindColoEx(const wstring& color, double sim, long dir, wstring& retstr);

	long FindMultiColor(const wstring& first_color,const wstring& offset_color, double sim, long dir, long&x, long&y);

	long FindMultiColorEx(const wstring& first_color, const wstring& offset_color, double sim, long dir, wstring& retstr);
	//图形定位
	long FindPic(const std::wstring& files,const wstring& delta_colors, double sim,long dir, long& x, long &y);
	//
	long FindPicEx(const std::wstring& files, const wstring& delta_colors, double sim, long dir, wstring& retstr, bool returnID = true);

	long FindColorBlock(const wstring&  color, double sim, long count, long height, long width, long& x, long& y);

	long FindColorBlockEx(const wstring&  color, double sim, long count, long height, long width, wstring& retstr);

	std::wstring GetColor(long x, long y);

	long SetMemDict(int idx, void* data,long size);

	long SetDict(int idx,const wstring& file);

	long UseDict(int idx);

	long OCR(const wstring& color, double sim, std::wstring& out_str);

	long OcrEx(const wstring& color, double sim, std::wstring& out_str);

	long FindStr(const wstring& str, const wstring& color, double sim, long& retx,long& rety);

	long FindStrEx(const wstring& str, const wstring& color, double sim, std::wstring& out_str);

	long OcrAuto(double sim, std::wstring& retstr);

	long OcrFromFile(const wstring& files,const wstring& color, double sim, std::wstring& retstr);

	long OcrAutoFromFile(const wstring& files, double sim, std::wstring& retstr);

	long FindLine(const wstring& color, double sim, wstring& retStr);
	
	long LoadPic(const wstring& files);

	long FreePic(const wstring& files);

	long LoadMemPic(const wstring& file_name, void* data, long size);
private:
	//字库
	Dict _dicts[_max_dict];
	//当前字库索引
	int _curr_idx;
	
public:
	//当前目录
	wstring _curr_path;
	//图片缓存
	std::map<wstring, Image> _pic_cache;
	//是否使用图片缓存，默认开启
	int _enable_cache;

	//tesseract::TessBaseAPI _tes;
	
	
private:
	//RETURN TYPE 0:word colors info; 1:bk color info
	int str2colordfs(const wstring& color_str, std::vector<color_df_t>& colors);
	void str2colors(const wstring& color, std::vector<color_t>& vcolor);
	void files2mats(const wstring& files, std::vector<Image*>& vpic, std::vector<wstring>& vstr);
};

