#pragma once

#include <dxgiformat.h>

namespace op::hook {

DXGI_FORMAT NormalizeDxgiFormat(DXGI_FORMAT format);
int GetImageBufferFormat(DXGI_FORMAT format);

} // namespace op::hook
