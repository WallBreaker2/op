#pragma once
#include "../core/optype.h"
namespace tesseract {
	class TessBaseAPI;
};
struct tess_rec_info {
	point_t left_top;
	point_t right_bottom;
	wstring text;
	float confidenc;
};

class tess_ocr {
public:
	tess_ocr();
	~tess_ocr();
	int init();
	int release();
	int ocr(byte* data, int w, int h, int bpp, vector<tess_rec_info>& result);
private:
	tesseract::TessBaseAPI* m_api;
};