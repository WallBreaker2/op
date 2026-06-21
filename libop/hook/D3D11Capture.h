#pragma once

#include <dxgi.h>

namespace op::hook {

HRESULT __stdcall dx11_hkPresent(IDXGISwapChain *thiz, UINT SyncInterval, UINT Flags);

} // namespace op::hook
