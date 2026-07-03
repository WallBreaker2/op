#include "op/OpContext.h"

#include <libop.h>

std::atomic<int> op::Op::s_id(0);

op::Op::Op() : m_context(std::make_unique<internal::OpContext>(s_id++)) {
}

op::Op::~Op() {
}
