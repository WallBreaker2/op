#pragma once

#include "OpContext.h"
#include "base/Utils.h"

#include <utility>

namespace op::internal {

template <typename Fn>
void capture_region(OpContext *context, long x, long y, long width, long height, Fn &&fn) {
    if (!context)
        return;

    if (!context->bkproc.requestCapture(x, y, width, height, context->image_proc._src)) {
        setlog("error requestCapture");
        return;
    }

    // 截图偏移必须和成功捕获后的坐标保持一致，后续找图、找色会用它还原窗口坐标。
    context->image_proc.set_offset(x, y);
    std::forward<Fn>(fn)();
}

template <typename Fn>
void capture_converted_region(OpContext *context, long x1, long y1, long x2, long y2, Fn &&fn) {
    // 这里的坐标已经过 RectConvert，直接作为截图偏移使用。
    capture_region(context, x1, y1, x2 - x1, y2 - y1, std::forward<Fn>(fn));
}

template <typename Fn>
void with_captured_region(OpContext *context, long &x1, long &y1, long &x2, long &y2, long width, long height,
                          Fn &&fn) {
    if (!context || !context->bkproc.check_bind() || !context->bkproc.RectConvert(x1, y1, x2, y2))
        return;

    capture_region(context, x1, y1, width, height, std::forward<Fn>(fn));
}

template <typename Fn>
void with_captured_region(OpContext *context, long &x1, long &y1, long &x2, long &y2, Fn &&fn) {
    if (!context || !context->bkproc.check_bind() || !context->bkproc.RectConvert(x1, y1, x2, y2))
        return;

    const long width = x2 - x1;
    const long height = y2 - y1;
    // 区域被裁剪后再设置偏移，调用方看到的仍是实际截图范围。
    capture_region(context, x1, y1, width, height, std::forward<Fn>(fn));
}

} // namespace op::internal
