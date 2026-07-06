#include <dxgi.h>
#include <d3d10.h>
#include "kiero.hpp"
#include "kiero_d3d10.hpp"
#include "kiero_intern.hpp"

using CreateDXGIFactory_t =
  HRESULT(WINAPI*)(REFIID riid, void** ppFactory);

using D3D10CreateDeviceAndSwapChain_t = HRESULT(WINAPI*)(
  IDXGIAdapter*,
  D3D10_DRIVER_TYPE,
  HMODULE,
  UINT,
  UINT,
  DXGI_SWAP_CHAIN_DESC*,
  IDXGISwapChain**,
  ID3D10Device**
);

template <>
kiero::Error kiero::locate<kiero::Implementation_D3D10>(void* in, void* out)
{
  KIERO_UNUSED(in);

  auto dxgi_dll = GetModuleHandleA("dxgi.dll");
  if (!dxgi_dll) {
    KIERO_DBG_MSG("dxgi.dll not loaded");
    return Error_ModuleNotFound;
  }

  auto d3d10_dll = GetModuleHandleA("d3d10.dll");
  if (!d3d10_dll) {
    KIERO_DBG_MSG("d3d10.dll not loaded");
    return Error_ModuleNotFound;
  }

  auto CreateDXGIFactory = (
    (CreateDXGIFactory_t)
    GetProcAddress(dxgi_dll, "CreateDXGIFactory")
  );
  if (!CreateDXGIFactory) {
    KIERO_DBG_MSG("CreateDXGIFactory not found");
    return Error_MethodNotFound;
  }

  IDXGIFactory* factory = nullptr;
  auto hresult = CreateDXGIFactory(
    __uuidof(IDXGIFactory),
    (void**)&factory
  );
  if (hresult != S_OK) {
    KIERO_DBG_MSG("CreateDXGIFactory failed (%d)", hresult);
    return Error_D3D10_CreateDXGIFactoryFailed;
  }
  KIERO_DEFER([&]() { factory->Release(); });

  IDXGIAdapter* adapter;
  hresult = factory->EnumAdapters(0, &adapter);
  if (hresult != S_OK) {
    KIERO_DBG_MSG("EnumAdapters failed (%d)", hresult);
    return Error_D3D10_EnumAdaptersFailed;
  }
  KIERO_DEFER([&]() { adapter->Release(); });

  auto D3D10CreateDeviceAndSwapChain = (
    (D3D10CreateDeviceAndSwapChain_t)
    GetProcAddress(d3d10_dll, "D3D10CreateDeviceAndSwapChain")
  );
  if (!D3D10CreateDeviceAndSwapChain) {
    KIERO_DBG_MSG("D3D10CreateDeviceAndSwapChain not found");
    return Error_MethodNotFound;
  }

  kiero::_::DummyWin32Window window;
  kiero::_::create_dummy_win32_window(&window);
  KIERO_DEFER([&]() { kiero::_::destroy_dummy_win32_window(&window); });

  DXGI_SWAP_CHAIN_DESC sc_desc = {};
  sc_desc.BufferDesc.Width = 100;
  sc_desc.BufferDesc.Height = 100;
  sc_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sc_desc.BufferDesc.RefreshRate.Numerator = 60;
  sc_desc.BufferDesc.RefreshRate.Denominator = 1;
  sc_desc.SampleDesc.Count = 1;
  sc_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sc_desc.BufferCount = 1;
  sc_desc.OutputWindow = window.hwnd;
  sc_desc.Windowed = TRUE;
  sc_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  sc_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  IDXGISwapChain* swapchain;
  ID3D10Device* device;
  hresult = D3D10CreateDeviceAndSwapChain(
    adapter,
    D3D10_DRIVER_TYPE_HARDWARE,
    nullptr,
    0,
    D3D10_SDK_VERSION,
    &sc_desc,
    &swapchain,
    &device
  );
  if (hresult != S_OK) {
    KIERO_DBG_MSG(
      "D3D10CreateDeviceAndSwapChain failed (%d)",
      hresult
    );
    return Error_D3D10_DeviceCreateFailed;
  }
  KIERO_DEFER([&]() {
    swapchain->Release();
    device->Release();
  });

  D3D10Output* output = (D3D10Output*)out;

  void** swapchain_vtable = *(void***)swapchain;
  void** device_vtable = *(void***)device;

  // COM vtables are not null-terminated. Keep these counts aligned with the
  // original kiero table sizes used before the kiero2 migration.
  output->swapchain_methods.assign(swapchain_vtable, swapchain_vtable + 18);
  output->device_methods.assign(device_vtable, device_vtable + 98);

  return Error_Nil;
}
