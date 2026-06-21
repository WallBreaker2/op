#pragma once

#include "../../image/Image.h"
#include <string>

namespace op::capture {

bool ParseMemoryImageSource(const std::wstring &method, Image &output, std::wstring &normalized_method);

} // namespace op::capture
