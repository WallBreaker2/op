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

  auto d3d11_dll = GetModuleHandleA("d3d11.dll");
  if (!d3d11_dll) {
    KIERO_DBG_MSG("d3d11.dll not loaded");
    return Error_ModuleNotFound;
  }

  auto D3D11CreateDeviceAndSwapChain =(
    (D3D11CreateDeviceAndSwapChain_t)
    GetProcAddress(d3d11_dll, "D3D11CreateDeviceAndSwapChain")
  );
  if (!D3D11CreateDeviceAndSwapChain) {
    KIERO_DBG_MSG("D3D11CreateDeviceAndSwapChain not found");
    return Error_MethodNotFound;
  }

  auto dxgi_dll = GetModuleHandleA("dxgi.dll");
  IDXGIFactory* factory = nullptr;
  IDXGIAdapter* adapter = nullptr;
  if (dxgi_dll) {
    auto CreateDXGIFactory = (
      (CreateDXGIFactory_t)
      GetProcAddress(dxgi_dll, "CreateDXGIFactory")
    );
    if (CreateDXGIFactory) {
      HRESULT factory_result = CreateDXGIFactory(
        __uuidof(IDXGIFactory),
        (void**)&factory
      );
      if (factory_result == S_OK && factory) {
        auto adapter_result = factory->EnumAdapters(0, &adapter);
        if (adapter_result != S_OK) {
          KIERO_DBG_MSG("EnumAdapters failed (%d), will try default hardware adapter", adapter_result);
        }
      } else {
        KIERO_DBG_MSG("CreateDXGIFactory failed (%d), will try default hardware adapter", factory_result);
      }
    } else {
      KIERO_DBG_MSG("CreateDXGIFactory not found, will try default hardware adapter");
    }
  } else {
    KIERO_DBG_MSG("dxgi.dll not loaded, will try default hardware adapter");
  }
  KIERO_DEFER([&]() {
    if (adapter) adapter->Release();
    if (factory) factory->Release();
  });

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

  IDXGISwapChain* swapchain = nullptr;
  ID3D11Device* device = nullptr;
  ID3D11DeviceContext* context = nullptr;
  D3D_FEATURE_LEVEL feature_level;

  auto release_created_objects = [&]() {
    if (swapchain) {
      swapchain->Release();
      swapchain = nullptr;
    }
    if (device) {
      device->Release();
      device = nullptr;
    }
    if (context) {
      context->Release();
      context = nullptr;
    }
  };

  auto create_dummy_swapchain = [&](IDXGIAdapter* selected_adapter, D3D_DRIVER_TYPE driver_type) {
    release_created_objects();
    return D3D11CreateDeviceAndSwapChain(
      selected_adapter,
      driver_type,
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
  };

  HRESULT hresult = adapter ? create_dummy_swapchain(adapter, D3D_DRIVER_TYPE_UNKNOWN) : E_FAIL;
  if (FAILED(hresult)) {
    KIERO_DBG_MSG("D3D11 adapter swapchain failed (%d), trying default hardware", hresult);
    hresult = create_dummy_swapchain(nullptr, D3D_DRIVER_TYPE_HARDWARE);
  }
  if (FAILED(hresult)) {
    KIERO_DBG_MSG("D3D11 hardware swapchain failed (%d), trying WARP", hresult);
    hresult = create_dummy_swapchain(nullptr, D3D_DRIVER_TYPE_WARP);
  }
  if (hresult != S_OK) {
    KIERO_DBG_MSG("D3D11CreateDeviceAndSwapChain failed (%d)", hresult);
    release_created_objects();
    return Error_D3D11_CreateDeviceAndSwapChainFailed;
  }
  KIERO_DEFER([&]() {
    swapchain->Release();
    device->Release();
    context->Release();
  });

  D3D11Output* output = (D3D11Output*)out;

  void** swapchain_vtable = *(void***)swapchain;
  void** device_vtable = *(void***)device;
  void** context_vtable = *(void***)context;

  // COM vtables are not null-terminated. Keep these counts aligned with the
  // original kiero table sizes used before the kiero2 migration.
  output->swapchain_methods.assign(swapchain_vtable, swapchain_vtable + 18);
  output->device_methods.assign(device_vtable, device_vtable + 43);
  output->context_methods.assign(context_vtable, context_vtable + 144);

  return Error_Nil;
}
