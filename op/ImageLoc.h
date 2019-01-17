#pragma once
#ifndef __IMAGELOC_H_
#define __IMAGELOC_H_
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <vector>
#include "Common.h"



inline int HEX2INT(wchar_t c) {
	if (L'0' <= c && c <= L'9')
		return c - L'0';
	if (L'A' <= c && c <= L'Z')
		return c - L'A' + 10;
	if (L'a' <= c && c <= L'z')
		return c - L'a' + 10;
	return 0;
}

#define SET_BIT(x, idx) x |= 1u << (idx)

#define GET_BIT(x, idx) (x >> (idx)) & 1u
using images_t = std::vector<std::wstring>;
//颜色结构
struct color_t {
	uchar b, g, r, alpha;
	color_t():b(0),g(0),r(0),alpha(0){}
	//absolute val
	color_t operator-(const color_t& rhs) {
		color_t c;
		c.b = b - rhs.b;
		c.g = g - rhs.g;
		c.r = r - rhs.r;
		return c;
	}
	bool operator<=(const color_t& rhs) {
		return b <= rhs.b&&g <= rhs.g&&r <= rhs.r;
	}
	color_t& str2color(std::wstring&s) {
		int r, g, b;
		std::transform(s.begin(), s.end(), s.begin(), ::towupper);
		swscanf(s.c_str(), L"%02X%02X%02X", &r, &g, &b);
		this->b = b; this->r = r; this->g = g;
		return *this;
	}
	std::wstring tostr() {
		wchar_t buff[10];
		wsprintf(buff, L"%02X%02X%02X", r, g, b);
		return buff;
	}
};

//颜色-偏色结构
struct color_df_t {
	//颜色
	color_t color;
	//偏色
	color_t df;
};

//ocr 列,占16位，即最大的字高为16
using ocrline_t = unsigned __int16;


struct one_words_t {
	//字名
	std::wstring word;
	//字形
	std::vector<ocrline_t> binlines;
	//1数量
	int bit_ct;
	//字高
	int height;
};
using dict_t = std::vector<one_words_t>;
/*
此类用于实现一些图像功能，如图像定位，简单ocr等
*/
class ImageExtend
{
public:
	template<typename T>
	static int get_bit_count(T x);


	ImageExtend();

	~ImageExtend();

	//brief:输入图像，建立图形矩阵,在图像操作前调用
	//image_data:	4子节对齐的像素指针
	//widht:		图像宽度
	//hegith:		h
	//x1,y1,x2,y2 拷贝区域
	//type:			输入类型,type=0表示正常输入，为-1时表示倒置输入
	long input_image(byte* image_data, int width, int height,long x1,long y1,long x2,long y2, int type = 0);
	//brief:图像定位
	//images:图像文件名，可以为多个
	//sim:精度5-599.
	//x,y:目标坐标
	long imageloc(images_t& images, double sim, long&x, long&y);

	long is_valid(long x, long y) {
		return x >= 0 && y >= 0 && x < _src.cols && y < _src.rows;
	}

	long GetPixel(long x, long y, color_t&cr);

	long FindColor(std::vector<color_df_t>&colors, long&x, long&y);
	long Ocr(dict_t& dict, double sim, std::wstring& ret_str);
	/*
	if(abs(cr-src)<=df) pixel=1;
	else pixel=0;
	*/
	void bgr2binary(vector<color_df_t>& colors);
	
private:
	cv::Mat _src;
	cv::Mat _src_gray;
	cv::Mat _target;
	cv::Mat _target_gray;
	cv::Mat _result;
private:
	
	/*
	match like this:
	(x1,y1)--------
	|				|
	|				|
	---------------(x2,y2)
	*/
	long full_match(int width, uchar* ps,ocrline_t*lines, int line_ct,int n) {
		ocrline_t line;
		long s = 0;
		int tp;
		for (int j = 0; j < line_ct; ++j) {
			auto p = ps + j;
			line = 0;
			for (int i = 0; i < n; ++i) {
				tp = *(p + i * width) & 1;
				tp <<= (15 - i);
				line |= tp;
			}
			if (line != lines[j])
				return 0;
		}
		return 1;
	}
};

#endif

