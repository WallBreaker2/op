#pragma once
#include "../base/Types.h"

#include <string>

namespace op::ocr {

// 解析 OCR HTTP 服务的响应体（`{"code":0,"results":[{"text":..,"bbox":[l,t,r,b],"confidence":..}]}`）。
//
// 返回值与 `HttpOcrService::ocr()` 的错误约定保持一致：
//   >= 0  解析到的结果条数（结果写进 out）
//   -7    找不到 `"code"` 字段
//   -8    服务端返回的 code 非 0
//   -9    响应里既没有可解析的结果，也没有明确的空结果标记 `"results":[]`
//
// 为什么不用 `std::regex`：`std::regex` 每匹配一次都要构造 `smatch`、每个对象还要再拷一份子串，
// 50 条结果就要上百次正则搜索，实测比手写扫描慢两个数量级（见 defect_fixes.md §72.33 的 A/B）。
// server_code（可选）：把响应里的 code 原样带出来，方便调用方打日志。
int parse_ocr_response(const std::string &body, vocr_rec_t &out, int *server_code = nullptr);

} // namespace op::ocr
