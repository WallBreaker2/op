#include <dxgi.h>
#include <d3d12.h>
#include "kiero.hpp"
#include "kiero_d3d12.hpp"
#include "kiero_intern.hpp"

using CreateDXGIFactory_t =
  HRESULT(WINAPI*)(REFIID riid, void** ppFactory);

using D3D12CreateDevice_t = HRESULT(WINAPI*)(
  IUnknown*,
  D3D_FEATURE_LEVEL,
  REFIID,
  void**
);

template <>
kiero::Error kiero::locate<kiero::Implementation_D3D12>(void* in, void* out)
{
  KIERO_UNUSED(in);

  auto dxgi_dll = GetModuleHandleA("dxgi.dll");
  if (!dxgi_dll) {
    KIERO_DBG_MSG("dxgi.dll not loaded");
    return Error_ModuleNotFound;
  }

  auto d3d12_dll = GetModuleHandleA("d3d12.dll");
  if (!d3d12_dll) {
    KIERO_DBG_MSG("d3d12.dll not loaded");
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
    KIERO_DBG_MSG("CreateDXGIFactory failed");
    return Error_D3D12_CreateDXGIFactoryFailed;
  }
  KIERO_DEFER([&]() { factory->Release(); });

  IDXGIAdapter* adapter;
  hresult = factory->EnumAdapters(0, &adapter);
  if (hresult != S_OK) {
    KIERO_DBG_MSG("EnumAdapters failed (%d)", hresult);
    return Error_D3D12_EnumAdaptersFailed;
  }
  KIERO_DEFER([&]() { adapter->Release(); });

  auto D3D12CreateDevice = (
    (D3D12CreateDevice_t)
    GetProcAddress(d3d12_dll, "D3D12CreateDevice")
  );
  if (!D3D12CreateDevice) {
    KIERO_DBG_MSG("D3D12CreateDevice not found");
    return Error_MethodNotFound;
  }

  ID3D12Device* device;
  hresult = D3D12CreateDevice(
    adapter,
    D3D_FEATURE_LEVEL_11_0,
    __uuidof(ID3D12Device),
    (void**)&device
  );
  if (hresult != S_OK) {
    KIERO_DBG_MSG("D3D12CreateDevice failed (%d)", hresult);
    return Error_D3D12_CreateDeviceFailed;
  }
  KIERO_DEFER([&]() { device->Release(); });

  D3D12_COMMAND_QUEUE_DESC cq_desc = {};
  cq_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  cq_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

  ID3D12CommandQueue* command_queue;
  hresult = device->CreateCommandQueue(
    &cq_desc,
    __uuidof(ID3D12CommandQueue),
    (void**)&command_queue
  );
  if (hresult != S_OK) {
    KIERO_DBG_MSG("CreateCommandQueue failed (%d)", hresult);
    return Error_D3D12_CreateCommandQueueFailed;
  }
  KIERO_DEFER([&]() { command_queue->Release(); });

  ID3D12CommandAllocator* command_allocator;
  hresult = device->CreateCommandAllocator(
    D3D12_COMMAND_LIST_TYPE_DIRECT,
    __uuidof(ID3D12CommandAllocator),
    (void**)&command_allocator
  );
  if (hresult != S_OK) {
    KIERO_DBG_MSG("CreateCommandAllocator failed (%d)", hresult);
    return Error_D3D12_CreateCommandAllocatorFailed;
  }
  KIERO_DEFER([&]() { command_allocator->Release(); });

  ID3D12GraphicsCommandList* command_list;
  hresult = device->CreateCommandList(
    0,
    D3D12_COMMAND_LIST_TYPE_DIRECT,
    command_allocator,
    nullptr,
    __uuidof(ID3D12GraphicsCommandList),
    (void**)&command_list
  );
  if (hresult != S_OK) {
    KIERO_DBG_MSG("CreateCommandList failed (%d)", hresult);
    return Error_D3D12_CreateCommandListFailed;
  }
  KIERO_DEFER([&]() { command_list->Release(); });

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
  sc_desc.BufferCount = 2;
  sc_desc.OutputWindow = window.hwnd;
  sc_desc.Windowed = TRUE;
  sc_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  sc_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  IDXGISwapChain* swapchain;
  hresult = factory->CreateSwapChain(
    command_queue,
    &sc_desc,
    &swapchain
  );
  if (hresult != S_OK) {
    KIERO_DBG_MSG("CreateSwapChain failed (%d)", hresult);
    return Error_D3D12_CreateSwapChainFailed;
  }
  KIERO_DEFER([&]() { swapchain->Release(); });

  D3D12Output* output = (D3D12Output*)out;

  for (auto vtable = *(void***)device; vtable; vtable++) {
    auto ptr = *vtable;
    if (!ptr) break;
    output->device_methods.push_back(ptr);
  }

  for (auto vtable = *(void***)command_queue; vtable; vtable++) {
    auto ptr = *vtable;
    if (!ptr) break;
    output->command_queue_methods.push_back(ptr);
  }

  for (auto vtable = *(void***)command_allocator; vtable; vtable++) {
    auto ptr = *vtable;
    if (!ptr) break;
    output->command_allocator_methods.push_back(ptr);
  }

  for (auto vtable = *(void***)command_list; vtable; vtable++) {
    auto ptr = *vtable;
    if (!ptr) break;
    output->command_list_methods.push_back(ptr);
  }

  for (auto vtable = *(void***)swapchain; vtable; vtable++) {
    auto ptr = *vtable;
    if (!ptr) break;
    output->swapchain_methods.push_back(ptr);
  }

  return Error_Nil;
}
