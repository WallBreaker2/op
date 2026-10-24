#pragma once
#ifndef OP_YOLO_YOLO_RESPONSE_PARSER_H_
#define OP_YOLO_YOLO_RESPONSE_PARSER_H_

#include "../base/Types.h"

#include <string>

namespace op::yolo {

// 解析 YOLO 服务端响应。
//
// 返回：>0 结果条数；0 明确的空结果数组；-7 缺 code；-8 服务端报错（码回填到 server_code）；
//       -9 格式不认识。逐条记录字段缺失（bbox / confidence）时跳过该条，不算解析失败。
int parse_yolo_response(const std::string &body, vyolo_rec_t &out, int *server_code = nullptr);

} // namespace op::yolo

#endif // OP_YOLO_YOLO_RESPONSE_PARSER_H_
