#include <d3d9.h>
#include "kiero.hpp"
#include "kiero_d3d9.hpp"
#include "kiero_intern.hpp"

using Direct3DCreate9_t = IDirect3D9*(WINAPI*)(UINT SDKVersion);

template <>
kiero::Error kiero::locate<kiero::Implementation_D3D9>(void* in, void* out)
{
  KIERO_UNUSED(in);

  auto d3d9_dll = GetModuleHandleA("d3d9.dll");
  if (!d3d9_dll) {
    KIERO_DBG_MSG("d3d9.dll not loaded");
    return Error_ModuleNotFound;
  }

  auto Direct3DCreate9 = (
    (Direct3DCreate9_t)
    GetProcAddress(d3d9_dll, "Direct3DCreate9")
  );
  if (!Direct3DCreate9) {
    KIERO_DBG_MSG("Direct3DCreate9 not found");
    return Error_MethodNotFound;
  }

  auto d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
  if (!d3d9) {
    KIERO_DBG_MSG("Direct3DCreate9 failed");
    return Error_D3D9_Direct3DCreate9Failed;
  }
  KIERO_DEFER([&]() { d3d9->Release(); });

  kiero::_::DummyWin32Window window;
  kiero::_::create_dummy_win32_window(&window);
  KIERO_DEFER([&]() { kiero::_::destroy_dummy_win32_window(&window); });

  D3DDISPLAYMODE display_mode = {};
  auto hresult = d3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &display_mode);
  if (hresult != S_OK) {
    KIERO_DBG_MSG("GetAdapterDisplayMode failed (%d)", hresult);
    return Error_D3D9_CreateDeviceFailed;
  }

  D3DPRESENT_PARAMETERS present_parameters = {};
  present_parameters.BackBufferWidth = 0;
  present_parameters.BackBufferHeight = 0;
  present_parameters.BackBufferFormat = display_mode.Format;
  present_parameters.BackBufferCount = 0;
  present_parameters.MultiSampleType = D3DMULTISAMPLE_NONE;
  present_parameters.MultiSampleQuality = 0;
  present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
  present_parameters.hDeviceWindow = window.hwnd;
  present_parameters.Windowed = TRUE;
  present_parameters.EnableAutoDepthStencil = FALSE;
  present_parameters.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
  present_parameters.Flags = 0;
  present_parameters.FullScreen_RefreshRateInHz = 0;
  present_parameters.PresentationInterval = 0;

  IDirect3DDevice9* device;
  hresult = d3d9->CreateDevice(
    D3DADAPTER_DEFAULT,
    D3DDEVTYPE_HAL,
    window.hwnd,
    D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
    &present_parameters,
    &device
  );
  if (hresult != S_OK) {
    KIERO_DBG_MSG("CreateDevice failed (%d)", hresult);
    return Error_D3D9_CreateDeviceFailed;
  }
  KIERO_DEFER([&]() { device->Release(); });

  D3D9Output* output = (D3D9Output*)out;

  void** device_vtable = *(void***)device;

  // COM vtables are not null-terminated. Keep this count aligned with the
  // original kiero table size used before the kiero2 migration.
  output->device_methods.assign(device_vtable, device_vtable + 119);

  return Error_Nil;
}
