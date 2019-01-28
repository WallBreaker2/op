#include "stdafx.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <list>
#include "ocr.h"
using cv::Mat;
//积分二值化
void thresholdIntegral(const Mat& inputMat, Mat& outputMat)
{

	outputMat.create(inputMat.size(), CV_8UC1);
	for (int i = 0; i < inputMat.rows; ++i) {
		auto p1 = inputMat.ptr<uchar>(i);
		auto p2 = outputMat.ptr<uchar>(i);
		for (int j = 0; j < inputMat.cols; ++j) {
			p2[j] = (p1[j] < 128 ? 0 : 255);
		}
	}
}
//垂直方向投影
void binshadowx(const Mat& binary, const rect_t& rc, std::vector<rect_t>& out_put)
{
	//qDebug("in x rc:%d,%d,%d,%d", rc.x1, rc.y1, rc.x2, rc.y2);
	out_put.clear();
	//ys.clear();
	//Mat paintx(binary.size(), CV_8UC1, cv::Scalar(255)); //创建一个全白图片，用作显示

	//int* blackcout = new int[binary.cols];
	std::vector<int> ys;
	ys.resize(binary.cols);
	memset(&ys[0], 0, binary.cols * 4);

	for (int i = rc.y1; i < rc.y2; i++)
	{
		for (int j = rc.x1; j < rc.x2; j++)
		{
			if (binary.at<uchar>(i, j) == 0)
			{
				ys[j]++; //垂直投影按列在x轴进行投影
			}
		}
	}

	int startindex = 0;
	int endindex = 0;
	bool inblock = false; //是否遍历到字符位置

	for (int j = rc.x1; j < rc.x2; j++)
	{

		if (!inblock&&ys[j] != 0) //进入有字符区域
		{
			inblock = true;
			startindex = j;
			//std::cout << "startindex:" << startindex << std::endl;
		}
		if (inblock&&ys[j] == 0) //进入空白区
		{
			endindex = j;
			inblock = false;
			//Mat roi = binary.colRange(startindex, endindex + 1);
			rect_t roi;
			roi.x1 = startindex; roi.y1 = rc.y1;
			roi.x2 = endindex; roi.y2 = rc.y2;
			//qDebug("out xrc:%d,%d,%d,%d", roi.x1, roi.y1, roi.x2, roi.y2);
			out_put.push_back(roi);
		}
	}
	//special case
	if (inblock) {
		out_put.push_back(rc);
	}

}
//水平方向投影并行分割
void binshadowy(const Mat& binary, const rect_t& rc, std::vector<rect_t>&out_put)
{
	//qDebug("in y rc:%d,%d,%d,%d", rc.x1, rc.y1, rc.x2, rc.y2);
	out_put.clear();
	//是否为白色或者黑色根据二值图像的处理得来
	//Mat painty(binary.size(), CV_8UC1, cv::Scalar(255)); //初始化为全白
	//水平投影
	//int* pointcount = new int[binary.rows]; //在二值图片中记录行中特征点的个数
	std::vector<int> xs;
	xs.resize(binary.rows);
	memset(&xs[0], 0, binary.rows * 4);//注意这里需要进行初始化

	for (int i = rc.y1; i < rc.y2; i++)
	{
		for (int j = rc.x1; j < rc.x2; j++)
		{
			if (binary.at<uchar>(i, j) == WORD_COLOR)
			{
				xs[i]++; //记录每行中黑色点的个数 //水平投影按行在y轴上的投影
			}
		}
	}


	//std::vector<Mat> result;
	int startindex = 0;
	int endindex = 0;
	bool inblock = false; //是否遍历到字符位置

	for (int i = rc.y1; i < rc.y2; i++)
	{

		if (!inblock&&xs[i] != 0) //进入有字符区域
		{
			inblock = true;
			startindex = i;
			//std::cout << "startindex:" << startindex << std::endl;
		}
		if (inblock&&xs[i] == 0) //进入空白区
		{
			endindex = i;
			inblock = false;
			//Mat roi = binary.rowRange(startindex, endindex + 1); //从而记录从开始到结束行的位置，即可进行行切分
			rect_t roi;
			roi.x1 = rc.x1; roi.y1 = startindex;
			roi.x2 = rc.x2; roi.y2 = endindex;
			//qDebug("out yrc:%d,%d,%d,%d", roi.x1, roi.y1, roi.x2, roi.y2);
			out_put.push_back(roi);
		}
	}

	if (inblock) {
		out_put.push_back(rc);
	}

}

void bin_image_cut(const cv::Mat& binary, const rect_t&inrc, rect_t& outrc) {
	//水平裁剪
	std::vector<int>v;

	int i, j;
	v.resize(binary.rows);
	for (auto&it : v)it = 0;
	for (i = inrc.y1; i < inrc.y2; ++i) {
		for (j = inrc.x1; j < inrc.x2; ++j)
			v[i] += (binary.at<uchar>(i, j) == 0 ? 1 : 0);
	}
	for (i = inrc.y1 + 1; i < inrc.y2; ++i)
		if (v[i - 1] == 0 && v[i] != 0) {
			outrc.y1 = i;
			break;
		}
	for (i = inrc.y2 - 2; i >= inrc.y1; --i)
		if (v[i + 1] == 0 && v[i] != 0) {
			outrc.y2 = i + 1;
			break;
		}
	//垂直裁剪
	v.resize(binary.cols);
	for (auto&it : v)it = 0;
	for (i = inrc.y1; i < inrc.y2; ++i) {
		for (j = inrc.x1; j < inrc.x2; ++j)
			v[j] += binary.at<uchar>(i, j) == 0 ? 1 : 0;
	}
	for (j = inrc.x1 + 1; i < inrc.x2; ++i)
		if (v[j - 1] == 0 && v[j] != 0) {
			outrc.x1 = j;
			break;
		}
	for (j = inrc.x2 - 2; j >= inrc.x1; --j)
		if (v[j + 1] == 0 && v[j] != 0) {
			outrc.x2 = j + 1;
			break;
		}
}

inline int full_match(const cv::Mat& binary, rect_t& rc, const word_t::cline_t* lines) {
	//匹配
	unsigned __int32 val;
	for (int x = rc.x1; x < rc.x2; ++x) {
		val = 0;
		for (int y = rc.y1, id = 31; y < rc.y2; ++y, --id) {
			if (binary.at<uchar>(y, x) == 0)
				SET_BIT(val, id);
		}
		if (lines[x - rc.x1] != val)
			return 0;
		//t = QString::asprintf("%08X", x);
		//tp += t;

	}
	return 1;
}

inline int part_match(const cv::Mat& binary, rect_t& rc, int max_error, const word_t::cline_t* lines) {
	//匹配
	unsigned __int32 sval, wval;
	int error_ct = 0;
	for (int x = rc.x1; x < rc.x2; ++x) {
		wval = lines[x - rc.x1];
		for (int y = rc.y1, id = 31; y < rc.y2; ++y, --id) {
			sval = binary.at<uchar>(y, x);
			if ((sval & 1) == GET_BIT(wval, id))
				++error_ct;
			if (error_ct > max_error)
				return 0;
		}
	}
	return 1;
}

inline void fill_rect(cv::Mat& record, const rect_t& rc) {
	//匹配
	unsigned __int32 val;
	int w = rc.width();
	for (int y = rc.y1; y < rc.y2; ++y) {
		uchar* p = record.ptr<uchar>(y) + rc.x1;
		memset(p, 1, sizeof(uchar)*w);
	}

}

void _bin_ocr(const cv::Mat& binary, cv::Mat& record, const rect_t&rc, const Dict& dict, std::map<point_t, std::wstring>&outstr) {
	int i, j, x, y, id;
	//outstr.clear();
	//给定下一个区
	//qDebug("in y rc:%d,%d,%d,%d", rc.x1, rc.y1, rc.x2, rc.y2);
	if (rc.width() <= 0 || rc.height() <= 0)
		return;
	//遍历行
	for (i = rc.y1; i < rc.y2; ++i) {
		//遍历列
		for (j = rc.x1; j < rc.x2; ++j) {
			if (record.at<uchar>(i, j))
				continue;
			point_t pt;
			pt.x = j; pt.y = i;
			//遍历字库
			//assert(i != 4 || j != 3);
			for (auto&it : dict.words) {
				if (it.info._char[0] == L'\0')
					continue;
				rect_t crc;
				crc.x1 = j; crc.y1 = i;
				crc.x2 = j + it.info.width; crc.y2 = i + it.info.height;
				//边界检查
				if (crc.y2 > rc.y2 || crc.x2 > rc.x2)
					continue;
				//match

				int matched = full_match(binary, crc, it.clines);
				if (matched) {
					if (crc.x2 < rc.x2)//还有剩余部分，检查右边是否空白
					{
						for (y = crc.y1; y < crc.y2; ++y)
							if (binary.at<uchar>(y, crc.x2) == 0)
								break;
						if (y == crc.y2) {
							//outstr.append(it.info._char);
							outstr[pt] = it.info._char;
							//设置下一个查找区域 分别为右边和下方
							//右边最先查找，下方最后
							//右
							fill_rect(record, crc);
							//goto __next_y;
							break;//words
						}

					}
					else {
						outstr[pt] = it.info._char;
						//

						fill_rect(record, crc);
						//设置下一个查找区域 只有上方
						//i = i - it.info.height;
						break;
					}

				}
			}//end for words
		}//end for j
	}//end for i


}

void _bin_ocr(const cv::Mat& binary, cv::Mat& record, const rect_t&rc, const Dict& dict, double sim, std::map<point_t, std::wstring>&outstr) {
	int i, j, x, y, id;
	if (rc.width() <= 0 || rc.height() <= 0)
		return;
	//遍历行
	for (i = rc.y1; i < rc.y2; ++i) {
		//遍历列
		for (j = rc.x1; j < rc.x2; ++j) {
			if (record.at<uchar>(i, j))
				continue;
			point_t pt;
			pt.x = j; pt.y = i;
			//遍历字库
			//assert(i != 4 || j != 3);
			for (auto&it : dict.words) {
				if (it.info._char[0] == L'\0')
					continue;
				rect_t crc;
				crc.x1 = j; crc.y1 = i;
				crc.x2 = j + it.info.width; crc.y2 = i + it.info.height;
				//边界检查
				if (crc.y2 > rc.y2 || crc.x2 > rc.x2)
					continue;
				//match
				int max_error = it.info.width*it.info.height*sim;
				int matched = part_match(binary, crc, max_error, it.clines);
				if (matched) {
					if (crc.x2 < rc.x2)//还有剩余部分，检查右边是否空白
					{
						for (y = crc.y1; y < crc.y2; ++y)
							if (binary.at<uchar>(y, crc.x2) == 0)
								break;
						if (y == crc.y2) {
							//outstr.append(it.info._char);
							outstr[pt] = it.info._char;
							//设置下一个查找区域 分别为右边和下方
							//右边最先查找，下方最后
							//右
							fill_rect(record, crc);
							//goto __next_y;
							break;//words
						}

					}
					else {
						outstr[pt] = it.info._char;
						//

						fill_rect(record, crc);
						//设置下一个查找区域 只有上方
						//i = i - it.info.height;
						break;
					}

				}
			}//end for words
		}//end for j
	}//end for i


}

void bin_ocr(const cv::Mat& binary, cv::Mat& record, const Dict& dict, double sim, std::wstring& outstr) {
	std::vector<rect_t> out_y, out_x;
	outstr.clear();
	if (dict.words.empty())return;
	if (binary.cols == 0 || binary.rows == 0)
		return;
	record.create(binary.size(), CV_8UC1);
	memset(record.data, 0, sizeof(uchar)*record.rows*record.cols);
	rect_t rc;
	rc.x1 = rc.y1 = 0;
	rc.x2 = binary.cols; rc.y2 = binary.rows;
	std::vector<rect_t> vrcx, vrcy;
	std::map<point_t, std::wstring> ms;
	binshadowy(binary, rc, vrcy);
	sim = 0.5 + sim / 2;
	for (auto&ity : vrcy) {
		binshadowx(binary, ity, vrcx);
		for (auto&itx : vrcx) {
			bin_image_cut(binary, itx, itx);
			ms.clear();
			if (sim > 1.0 - 1e-5) {
				_bin_ocr(binary, record, itx, dict, ms);
			}
			else {
				_bin_ocr(binary, record, itx, dict, sim, ms);
			}

			for (auto&it : ms) {
				outstr.append(it.second);
			}
		}

	}



}
