#pragma once
#include "../core/optype.h"
namespace tesseract {
class TessBaseAPI;
};

class tess_ocr {
  public:
    tess_ocr();
    ~tess_ocr();
    int init();
    int release();
    int ocr(byte *data, int w, int h, int bpp, vocr_rec_t &result);

  private:
    tesseract::TessBaseAPI *m_api;
};