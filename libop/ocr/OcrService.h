#pragma once
#include "../runtime/Types.h"
#include <mutex>

namespace op::ocr {

class HttpOcrService {
  private:
    HttpOcrService();

  public:
    HttpOcrService(const HttpOcrService &) = delete;
    HttpOcrService &operator=(const HttpOcrService &) = delete;
    static HttpOcrService *getInstance();
    ~HttpOcrService();
    int init(const std::wstring &enginePath, const std::wstring &dllName, const vector<string> &argv);
    int release();
    int ocr(byte *data, int w, int h, int bpp, vocr_rec_t &result);

  private:
    std::mutex m_mutex;
    std::string m_endpoint;
    int m_timeout_ms;
};

} // namespace op::ocr
