#pragma once
#ifndef KIERO_D3D12_HPP
#define KIERO_D3D12_HPP

#include <vector>

namespace kiero {

enum {
  Implementation_D3D12 = KIERO_IMPL_FREE_SLOT,
};

enum {
  Error_D3D12_CreateDXGIFactoryFailed = Error_BaseIndex,
  Error_D3D12_EnumAdaptersFailed,
  Error_D3D12_CreateDeviceFailed,
  Error_D3D12_CreateCommandQueueFailed,
  Error_D3D12_CreateCommandAllocatorFailed,
  Error_D3D12_CreateCommandListFailed,
  Error_D3D12_CreateSwapChainFailed,
};

struct D3D12Output {
  // Corresponds to ID3D12Device indexes
  // For example: device_methods[0] is ID3D12Device::QueryInterface
  // Find your index here: https://github.com/apitrace/dxsdk/blob/d964b66467aaa734edbc24326da8119f5f063dd3/Include/d3d12.h#L8028
  std::vector<void*> device_methods;

  // Corresponds to ID3D12CommandQueue indexes
  // For example: command_queue_methods[0] is ID3D12CommandQueue::QueryInterface
  // Find your index here: https://github.com/apitrace/dxsdk/blob/d964b66467aaa734edbc24326da8119f5f063dd3/Include/d3d12.h#L7591
  std::vector<void*> command_queue_methods;

  // Corresponds to ID3D12CommandAllocator indexes
  // For example: command_allocator_methods[0] is ID3D12CommandAllocator::QueryInterface
  // Find your index here: https://github.com/apitrace/dxsdk/blob/d964b66467aaa734edbc24326da8119f5f063dd3/Include/d3d12.h#L4344
  std::vector<void*> command_allocator_methods;

  // Corresponds to ID3D12GraphicsCommandList indexes
  // For example: command_list_methods[0] is ID3D12GraphicsCommandList::QueryInterface
  // Find your index here: https://github.com/apitrace/dxsdk/blob/d964b66467aaa734edbc24326da8119f5f063dd3/Include/d3d12.h#L5615
  std::vector<void*> command_list_methods;

  // Corresponds to IDXGISwapChain indexes
  // For example: swapchain_methods[0] is IDXGISwapChain::QueryInterface
  // Find your index here: https://github.com/apitrace/dxsdk/blob/d964b66467aaa734edbc24326da8119f5f063dd3/Include/dxgi.h#L1767
  std::vector<void*> swapchain_methods;
};

template <>
kiero::Error kiero::locate<kiero::Implementation_D3D12>(void* in, void* out);

} // namespace kiero

#undef KIERO_IMPL_CURR_SLOT
#define KIERO_IMPL_CURR_SLOT kiero::Implementation_D3D12

#endif // KIERO_D3D12_HPP
