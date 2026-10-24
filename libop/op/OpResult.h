#pragma once
#ifndef OP_OP_RESULT_H_
#define OP_OP_RESULT_H_

#include <string>

namespace op::internal {

template <typename Result, typename Value>
void set_result(Result *result, Value value) noexcept {
    if (result)
        *result = static_cast<Result>(value);
}

// const wchar_t* → wstring 的安全转换：nullptr 当作空串。
// 直接写 std::wstring(ptr) 时 nullptr 会走 _Traits::length(nullptr) 直接 AV，
// 而 COM(BSTR 为 NULL)、C API、裸 C++ 调用方都可能把 nullptr 传进门面。
inline std::wstring wstr(const wchar_t *s) {
    return s ? std::wstring(s) : std::wstring();
}

} // namespace op::internal

#endif // OP_OP_RESULT_H_
