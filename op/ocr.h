#pragma once
#ifndef __OCR_H_
#define __OCR_H_
#include <fstream>
#include<map>
#include <vector>
#include "include/Dict.h"
#include "include/color.h"
struct point_t {
	int x, y;
	bool operator<(const point_t&rhs) const {
		if (std::abs(y - rhs.y) < 9)
			return x < rhs.x;
		else
			return y < rhs.y;
	}
	bool operator==(const point_t&rhs) const {
		return x == rhs.x&&y == rhs.y;
	}
};

using std::vector;

int get_bk_color(inputbin bin);


/*
if(abs(cr-src)<=df) pixel=1;
else pixel=0;
*/
void bgr2binary(inputimg src,outputbin bin, vector<color_df_t>& colors);
//二值化 auto
void auto2binary(inputimg src, outputbin bin);
//垂直方向投影,投到x轴
void binshadowx(const ImageBin& binary, const rect_t& rc, std::vector<rect_t>& out_put);
//水平方向投影，投到(y)轴
void binshadowy(const ImageBin& binary, const rect_t& rc, std::vector<rect_t>&out_put);
//图像裁剪
void bin_image_cut(const ImageBin& binary,int min_word_h, const rect_t&inrc, rect_t& outrc);

void get_rois(const ImageBin& bin,int min_word_h, std::vector<rect_t>& vroi);
//ocr in sim=1.0
void _bin_ocr(const ImageBin& binary, ImageBin& record, const rect_t&rc, const Dict& dict, std::map<point_t, std::wstring>&ps);
//ocr with sim<1.0
void _bin_ocr(const ImageBin& binary, ImageBin& record, const rect_t&rc, const Dict& dict, int *max_error, std::map<point_t, std::wstring>&ps);
//ocr wrapper
//template<int _type>
//void bin_ocr(const Image& binary, Image& record, const Dict& dict, int* max_error, std::wstring& outstr);

void bin_ocr(const ImageBin& binary, ImageBin& record, const Dict& dict, double sim, std::map<point_t, std::wstring>&ps);


#endif


