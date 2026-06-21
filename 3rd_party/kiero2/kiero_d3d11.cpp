#include <dxgi.h>
#include <d3d11.h>
#include "kiero.hpp"
#include "kiero_d3d11.hpp"
#include "kiero_intern.hpp"

using CreateDXGIFactory_t =
  HRESULT(WINAPI*)(REFIID riid, void** ppFactory);

using D3D11CreateDeviceAndSwapChain_t = HRESULT(WINAPI*)(
  IDXGIAdapter*,
  D3D_DRIVER_TYPE,
  HMODULE,
  UINT,
  const D3D_FEATURE_LEVEL*,
  UINT,
  UINT,
  const DXGI_SWAP_CHAIN_DESC*,
  IDXGISwapChain**,
  ID3D11Device**,
  D3D_FEATURE_LEVEL*,
  ID3D11DeviceContext**
);

template <>
kiero::Error kiero::locate<kiero::Implementation_D3D11>(void* in, void* out)
{
  KIERO_UNUSED(in);

  auto dxgi_dll = GetModuleHandleA("dxgi.dll");
  if (!dxgi_dll) {
    KIERO_DBG_MSG("dxgi.dll not loaded");
    return Error_ModuleNotFound;
  }

  auto d3d11_dll = GetModuleHandleA("d3d11.dll");
  if (!d3d11_dll) {
    KIERO_DBG_MSG("d3d11.dll not loaded");
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

  IDXGIFactory* factory;
  auto hresult = CreateDXGIFactory(
    __uuidof(IDXGIFactory),
    (void**)&factory
  );
  if (hresult != S_OK) {
    KIERO_DBG_MSG("CreateDXGIFactory failed (%d)", hresult);
    return Error_D3D11_CreateDXGIFactoryFailed;
  }
  KIERO_DEFER([&]() { factory->Release(); });

  IDXGIAdapter* adapter;
  hresult = factory->EnumAdapters(0, &adapter);
  if (hresult != S_OK) {
    KIERO_DBG_MSG("EnumAdapters failed (%d)", hresult);
    return Error_D3D11_EnumAdaptersFailed;
  }
  KIERO_DEFER([&]() { adapter->Release(); });

  auto D3D11CreateDeviceAndSwapChain =(
    (D3D11CreateDeviceAndSwapChain_t)
    GetProcAddress(d3d11_dll, "D3D11CreateDeviceAndSwapChain")
  );
  if (!D3D11CreateDeviceAndSwapChain) {
    KIERO_DBG_MSG("D3D11CreateDeviceAndSwapChain not found");
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

  const D3D_FEATURE_LEVEL feature_levels[] = {
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
  };

  IDXGISwapChain* swapchain;
  ID3D11Device* device;
  ID3D11DeviceContext* context;
  D3D_FEATURE_LEVEL feature_level;

  hresult = D3D11CreateDeviceAndSwapChain(
    adapter,
    D3D_DRIVER_TYPE_UNKNOWN,
    nullptr,
    0,
    feature_levels,
    ARRAYSIZE(feature_levels),
    D3D11_SDK_VERSION,
    &sc_desc,
    &swapchain,
    &device,
    &feature_level,
    &context
  );
  if (hresult != S_OK) {
    KIERO_DBG_MSG("D3D11CreateDeviceAndSwapChain failed (%d)", hresult);
    return Error_D3D11_CreateDeviceAndSwapChainFailed;
  }
  KIERO_DEFER([&]() {
    swapchain->Release();
    device->Release();
    context->Release();
  });

  D3D11Output* output = (D3D11Output*)out;

  for (auto vtable = *(void***)swapchain; vtable; vtable++) {
    auto ptr = *vtable;
    if (!ptr) break;
    output->swapchain_methods.push_back(ptr);
  }

  for (auto vtable = *(void***)device; vtable; vtable++) {
    auto ptr = *vtable;
    if (!ptr) break;
    output->device_methods.push_back(ptr);
  }

  for (auto vtable = *(void***)context; vtable; vtable++) {
    auto ptr = *vtable;
    if (!ptr) break;
    output->context_methods.push_back(ptr);
  }

  return Error_Nil;
}
