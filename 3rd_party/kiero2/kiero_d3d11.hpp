#pragma once
#ifndef KIERO_D3D11_HPP
#define KIERO_D3D11_HPP

#include <vector>

namespace kiero {

enum {
  Implementation_D3D11 = KIERO_IMPL_FREE_SLOT,
};

enum {
  Error_D3D11_CreateDXGIFactoryFailed = Error_BaseIndex,
  Error_D3D11_EnumAdaptersFailed,
  Error_D3D11_CreateDeviceAndSwapChainFailed,
};

struct D3D11Output {
  // Corresponds to IDXGISwapChain indexes
  // For example: swapchain_methods[0] is IDXGISwapChain::QueryInterface
  // Find your index here: https://github.com/apitrace/dxsdk/blob/d964b66467aaa734edbc24326da8119f5f063dd3/Include/dxgi.h#L1767
  std::vector<void*> swapchain_methods;

  // Corresponds to ID3D11Device indexes
  // For example: device_methods[0] is ID3D11Device::QueryInterface
  // Find your index here: https://github.com/apitrace/dxsdk/blob/d964b66467aaa734edbc24326da8119f5f063dd3/Include/d3d11.h#L1421
  std::vector<void*> device_methods;

  // Corresponds to ID3D11DeviceContext indexes
  // For example: context_methods[0] is ID3D11DeviceContext::QueryInterface
  // Find your index here: https://github.com/apitrace/dxsdk/blob/d964b66467aaa734edbc24326da8119f5f063dd3/Include/d3d11.h#L8308
  std::vector<void*> context_methods;
};

template <>
kiero::Error kiero::locate<kiero::Implementation_D3D11>(void* in, void* out);

} // namespace kiero

#undef KIERO_IMPL_CURR_SLOT
#define KIERO_IMPL_CURR_SLOT kiero::Implementation_D3D11

#endif // KIERO_D3D11_HPP
