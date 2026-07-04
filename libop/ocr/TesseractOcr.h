#pragma once
#include "../base/Types.h"
namespace tesseract {
class TessBaseAPI;
};

namespace op::ocr {

class TesseractOcr {
  public:
    TesseractOcr();
    ~TesseractOcr();
    int init();
    int release();
    int ocr(byte *data, int w, int h, int bpp, vocr_rec_t &result);

  private:
    tesseract::TessBaseAPI *m_api;
};

} // namespace op::ocr