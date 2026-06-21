#pragma once

#include <dxgi.h>

namespace op::hook {

HRESULT __stdcall dx10_hkPresent(IDXGISwapChain *thiz, UINT SyncInterval, UINT Flags);

} // namespace op::hook
