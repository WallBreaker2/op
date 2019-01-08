#pragma once
#ifndef __IMAGELOC_H_
#define __IMAGELOC_H_
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
using images_t = std::vector<std::wstring>;
class ImageLoc
{
public:
	
	ImageLoc();
	~ImageLoc();
	//
	
	//brief:输入图像，建立图形矩阵,在图像操作前调用
	//image_data:	4子节对齐的像素指针
	//widht:		图像宽度
	//height:		图像高度
	//type:			输入类型,type=0表示正常输入，为-1时表示倒置输入
	long input_image(byte* image_data, int width, int height,int type=0);
	//brief:图像定位
	//images:图像文件名，可以为多个
	//sim:精度
	//x,y:目标坐标
	long imageloc(images_t& images, double sim, long&x, long&y);
private:
	cv::Mat _src;
	cv::Mat _src_gray;
	cv::Mat _target;
	cv::Mat _target_gray;
	cv::Mat _result;
};

#endif

