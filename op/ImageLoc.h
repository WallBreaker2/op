#pragma once
#ifndef __IMAGELOC_H_
#define __IMAGELOC_H_
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <vector>


inline int HEX2INT(wchar_t c) {
	if (L'0' <= c && c <= L'9')
		return c - L'0';
	if (L'A' <= c && c <= L'Z')
		return c - L'A' + 10;
	if (L'a' <= c && c <= L'z')
		return c - L'a' + 10;
	return 0;
}
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
//ocr 列,占16位，即最大的字高为16
using ocrline_t = unsigned __int16;
struct one_words_t {
	std::wstring word;
	std::vector<ocrline_t> binlines;
	int bit_ct;
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
	//height:		图像高度
	//type:			输入类型,type=0表示正常输入，为-1时表示倒置输入
	long input_image(byte* image_data, int width, int height,int type=0);
	//brief:图像定位
	//images:图像文件名，可以为多个
	//sim:精度5-599.
	//x,y:目标坐标
	long imageloc(images_t& images, double sim, long&x, long&y);

	long is_valid(long x, long y) {
		return x >= 0 && y >= 0 && x < _src.cols && y < _src.rows;
	}

	long GetPixel(long x, long y, color_t&cr);

	long FindColor(color_t cr,color_t df, long&x, long&y);
	long Ocr(one_words_t& words, double sim, long&x, long&y);
	/*
	if(abs(cr-src)<=df) pixel=1;
	else pixel=0;
	*/
	void bgr2binary(color_t cr, color_t df);
	
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
		ocrline_t line = 0;
		long s = 0;
		for (int j = 0; j < line_ct; ++j) {
			auto p = ps + j;
			for (int i = 0; i < 11; ++i) {
				line |= *(p + i * width);
				line <<= (15 - i);
			}
			if (line != lines[j])
				s += get_bit_count(line&lines[j]);
			else
				s += get_bit_count(line);
		}
		return s;
	}
};

#endif

