#pragma once
#include "../core/optype.h"
#include <mutex>

class OcrWrapper {
  private:
    OcrWrapper();

  public:
    OcrWrapper(const OcrWrapper &) = delete;
    OcrWrapper operator=(const OcrWrapper &) = delete;
    static OcrWrapper *getInstance();
    ~OcrWrapper();
    int init(const std::wstring &enginePath, const std::wstring &dllName, const vector<string> &argv);
    int release();
    int ocr(byte *data, int w, int h, int bpp, vocr_rec_t &result);

  private:
    std::mutex m_mutex;
    std::string m_endpoint;
    int m_timeout_ms;
};
