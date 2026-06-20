#pragma once
#include "../runtime/Types.h"
#include <mutex>

namespace op::yolo {

class YoloDetector {
  private:
    YoloDetector();

  public:
    YoloDetector(const YoloDetector &) = delete;
    YoloDetector &operator=(const YoloDetector &) = delete;
    static YoloDetector *getInstance();
    ~YoloDetector();
    int init(const std::wstring &enginePath, const std::wstring &dllName, const vector<string> &argv);
    int release();
    int detect(byte *data, int w, int h, int bpp, double conf, double iou, vyolo_rec_t &result);

  private:
    std::mutex m_mutex;
    std::string m_endpoint;
    int m_timeout_ms;
};

} // namespace op::yolo
