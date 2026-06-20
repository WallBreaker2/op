#include "FrameInfo.h"

std::ostream &operator<<(std::ostream &o, op::capture::FrameInfo const &rhs) {
    o << "hwnd:" << rhs.hwnd << std::endl
      << "frameId:" << rhs.frameId << std::endl
      << "time:" << rhs.time << std::endl
      << "height" << rhs.height << std::endl
      << "width:" << rhs.width << std::endl;
    return o;
}

std::wostream &operator<<(std::wostream &o, op::capture::FrameInfo const &rhs) {
    o << L"hwnd:" << rhs.hwnd << std::endl
      << L"frameId:" << rhs.frameId << std::endl
      << L"time:" << rhs.time << std::endl
      << L"height" << rhs.height << std::endl
      << L"width:" << rhs.width << std::endl;
    return o;
}
