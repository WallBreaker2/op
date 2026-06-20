#pragma once
#ifndef KIERO_D3D9_HPP
#define KIERO_D3D9_HPP

#include <vector>

namespace kiero {

enum {
  Implementation_D3D9 = KIERO_IMPL_FREE_SLOT,
};

enum {
  Error_D3D9_Direct3DCreate9Failed = Error_BaseIndex,
  Error_D3D9_CreateDeviceFailed,
};

struct D3D9Output {
  // Corresponds to IDirect3DDevice9 indexes
  // For example: device_methods[0] is IDirect3DDevice9::QueryInterface
  // Find your index here: https://github.com/apitrace/dxsdk/blob/d964b66467aaa734edbc24326da8119f5f063dd3/Include/d3d9.h#L426
  std::vector<void*> device_methods;
};

template <>
kiero::Error kiero::locate<kiero::Implementation_D3D9>(void* in, void* out);

} // namespace kiero

#undef KIERO_IMPL_CURR_SLOT
#define KIERO_IMPL_CURR_SLOT kiero::Implementation_D3D9

#endif // KIERO_D3D9_HPP