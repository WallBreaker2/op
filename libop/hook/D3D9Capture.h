#pragma once

#include <d3d9.h>

namespace op::hook {

HRESULT __stdcall dx9_hkEndScene(IDirect3DDevice9 *thiz);

} // namespace op::hook
