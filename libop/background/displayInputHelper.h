#pragma once

#include "include/Image.hpp"
#include <string>

namespace display_input_helper {

bool parse_mem_display_input(const std::wstring &method, Image &output, std::wstring &normalized_method);

}
