#pragma once
#include "../core/optype.h"
#include <mutex>

class YoloWrapper {
  private:
    YoloWrapper();

  public:
    YoloWrapper(const YoloWrapper &) = delete;
    YoloWrapper operator=(const YoloWrapper &) = delete;
    static YoloWrapper *getInstance();
    ~YoloWrapper();
    int init(const std::wstring &enginePath, const std::wstring &dllName, const vector<string> &argv);
    int release();
    int detect(byte *data, int w, int h, int bpp, double conf, double iou, vyolo_rec_t &result);

  private:
    std::mutex m_mutex;
    std::string m_endpoint;
    int m_timeout_ms;
};
