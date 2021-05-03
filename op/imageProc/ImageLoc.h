#pragma once
#ifndef __IMAGELOC_H_
#define __IMAGELOC_H_
/*
������ͼ���㷨������ͼ����ң���ɫ����ƥ�䣨�����ɫ��
����ocr��ͼ��������ƣ���Ҳ����ImageLoc��ʵ��
*/
#include <vector>
#include "./core/optype.h"
#include <string>
#include "include/Dict.h"
#include "include/color.h"

inline int HEX2INT(wchar_t c) {
	if (L'0' <= c && c <= L'9')
		return c - L'0';
	if (L'A' <= c && c <= L'Z')
		return c - L'A' + 10;
	if (L'a' <= c && c <= L'z')
		return c - L'a' + 10;
	return 0;
}


#define SET_BIT(x, idx) (x |= 1u << (idx))

#define GET_BIT(x, idx) ((x >> (idx)) & 1u)

using img_names = std::vector<std::wstring>;
//����Ƿ�Ϊ͸��ͼ
int check_transparent(Image* img);
//��ȡƥ���
void get_match_points(const Image& img, vector<int>&points);
//generate next index for kmp
void gen_next(const Image& img, vector<int>& next);
//sum of all pixels
int inline sum(uchar* begin, uchar* end) {
	int s = 0;
	while (begin != end)
		s += *begin++;
	return s;
}

void extractConnectivity(const ImageBin& src, int threshold, std::vector<ImageBin>& out);


/*
��������ʵ��һЩͼ���ܣ���ͼ��λ����ocr��
*/
class ImageBase
{
public:
	
	const static int _max_return_obj_ct = 1800;

	ImageBase();

	~ImageBase();

	//brief:����ͼ�񣬽���ͼ�ξ���,��ͼ�����ǰ����
	//image_data:	4�ӽڶ��������ָ��
	//widht:		ͼ�����
	//hegith:		h
	//x1,y1,x2,y2 ��������
	//type:			��������,type=0��ʾ�������룬Ϊ-1ʱ��ʾ��������
	//long input_image(byte* psrc, int cols, int height,long x1,long y1,long x2,long y2, int type = 0);

	void set_offset(int x1, int y1);
	
	long is_valid(long x, long y) {
		return x >= 0 && y >= 0 && x < _src.width && y < _src.height;
	}

	long GetPixel(long x, long y, color_t&cr);

	long CmpColor(color_t color, std::vector<color_df_t>&colors, double sim);

	long FindColor(std::vector<color_df_t>&colors,int dir, long&x, long&y);

	long FindColorEx(std::vector<color_df_t>&colors, std::wstring& retstr);

	long FindMultiColor(std::vector<color_df_t>&first_color, std::vector<pt_cr_df_t>& offset_color, double sim, long dir, long&x, long&y);

	long FindMultiColorEx(std::vector<color_df_t>&first_color, std::vector<pt_cr_df_t>& offset_color, double sim, long dir, std::wstring& retstr);

	long FindPic(std::vector<Image*>&pics,color_t dfcolor,double sim, long&x, long&y);

	long FindPicEx(std::vector<Image*>& pics, color_t dfcolor, double sim, vpoint_desc_t& vpd);

	long Ocr(Dict& dict, double sim, std::wstring& ret_str);

	long OcrEx(Dict& dict, double sim, std::wstring& out_str);

	long FindStr(Dict& dict, const vector<wstring>& vstr,  double sim, long& retx, long& rety);

	long FindStrEx(Dict& dict, const vector<wstring>& vstr, double sim, std::wstring& out_str);
	//����������Ŀ��ͼ���е�ֱ��
	//���룺����
	//�����outStr:ֱ������[���߽Ƕȣ�ֱ�ߵ�ԭ��ľ���];ret:��ֱ���ϵĵ������
	long FindLine(double sim, std::wstring& outStr);
private:
	//rgb����ƥ��
	template<bool nodfcolor>
	long simple_match(long x, long y, Image* timg, color_t dfcolor,int tnrom, double sim);
	//͸��ͼƥ��
	template<bool nodfcolor>
	long trans_match(long x, long y, Image* timg, color_t dfcolor, vector<uint>points, int max_error);
	//�Ҷ�ƥ��
	long real_match(long x, long y, ImageBin* timg, int tnorm, double sim);
	//��¼��
	void record_sum(const ImageBin & gray);
	//[x1,x2),[y1,y2)
	int region_sum(int x1, int y1, int x2, int y2);

	

	int get_bk_color(inputbin bin);

	
	
	//��ֱ����ͶӰ,Ͷ��x��
	void binshadowx(const rect_t& rc, std::vector<rect_t>& out_put);
	//ˮƽ����ͶӰ��Ͷ��(y)��
	void binshadowy(const rect_t& rc, std::vector<rect_t>&out_put);


	
	//ocr ��ȫƥ��ģʽ
	void _bin_ocr(const Dict& dict, std::map<point_t, std::wstring>&ps);
	//ocr ģ��ƥ��ģʽ
	void _bin_ocr(const Dict& dict,double sim, std::map<point_t, std::wstring>&ps);
	//ocr wrapper
	//template<int _type>
	//void bin_ocr(const Image& binary, Image& record, const Dict& dict, int* max_error, std::wstring& outstr);
public:
	/*
	if(abs(cr-src)<=df) pixel=1;
	else pixel=0;
	*/
	void bgr2binary(vector<color_df_t>& colors);
	//��ֵ�� auto
	void bgr2binarybk(const vector<color_df_t>& bk_colors);
	//ͼ��ü�
	void bin_image_cut(int min_word_h, const rect_t& inrc, rect_t& outrc);
	void get_rois(int min_word_h, std::vector<rect_t>& vroi);
	//ocrʶ�𣬷���ʶ�𵽵��ּ���Ӧ����

	void bin_ocr(const Dict& dict, double sim, std::map<point_t, std::wstring>&ps);


public:
	Image _src;
	ImageBin _gray;
	ImageBin _record;
	ImageBin _binary;
	Image _sum;
private:
	//��ʼ��
	int _x1, _y1;
	//ƫ��
	int _dx, _dy;
	
};

#endif

