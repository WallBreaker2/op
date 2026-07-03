#pragma once
#ifndef OP_OP_RESULT_H_
#define OP_OP_RESULT_H_

namespace op::internal {

template <typename Result, typename Value>
void set_result(Result *result, Value value) noexcept {
    if (result)
        *result = static_cast<Result>(value);
}

} // namespace op::internal

#endif // OP_OP_RESULT_H_
