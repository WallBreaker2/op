#include "client/ClientContext.h"

#include <libop.h>

std::atomic<int> op::Client::s_id(0);

op::Client::Client() : m_context(std::make_unique<internal::ClientContext>(s_id++)) {
}

op::Client::~Client() {
}
