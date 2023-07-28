#pragma once
#include "../core/optype.h"
#define ocr_engine_ok 0
#define ocr_engine_err 1
struct ocr_engine;
struct ocr_engine_ocr_result {
    int x1, y1, x2, y2;
    char* text;
    float confidence;
};

typedef int(_stdcall* ocr_engine_init_t)(ocr_engine** obj, char* argv[], int argc);

typedef  int(_stdcall* ocr_engine_ocr_t)(ocr_engine* pocr, void* image, int w, int h, int bpp, ocr_engine_ocr_result** ppresult, int* num_of_result);

typedef  int(_stdcall* ocr_engine_release_t)(ocr_engine* obj);

class OcrWrapper {
public:
	OcrWrapper();
	~OcrWrapper();
	int init(const std::wstring& enginePath, const std::wstring& dllName, const vector<string>& argv);
	int release();
	int ocr(byte* data, int w, int h, int bpp, vocr_rec_t& result);
private:
	static ocr_engine_init_t ocr_engine_init;
	static ocr_engine_ocr_t ocr_engine_ocr;
	static ocr_engine_release_t ocr_engine_release;
	ocr_engine* m_engine;
};