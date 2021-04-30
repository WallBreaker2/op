
#include "../include/kiero.h"
#include <Windows.h>

#ifdef KIERO_USE_MINHOOK
#include "../include/MinHook.h"
#endif
//#include <MinHook.h>
// Uncomment a needed graphical library (you can include all)
#include <d3d9.h>          // D3D9
#include <dxgi.h>          // D3D10/D3D11/D3D12 (must be included for d3d12 hook)
#include <d3d10_1.h>       // D3D10
#include <d3d10.h>         // D3D10
#define KIERO_D3D10_USAGE  // This need because d3d11.h includes d3d10.h
#include <d3d11.h>         // D3D11
//#include <d3d12.h>         // D3D12
//#include <gl/GL.h>         // OpenGL
//#include <vulkan/vulkan.h> // Vulkan
#include <string>
#if defined(KIERO_D3D10_USAGE) && !defined(__d3d10_h__)
# error KIERO_D3D10_USAGE defined, but d3d10.h not included
#endif

#if defined(__d3d12_h__) && !defined(__dxgi_h__)
# error d3d12.h included, but dxgi.h not included
#endif

static kiero::RenderType::Enum g_renderType = kiero::RenderType::None;

// See METHODSTABLE.txt for more information
#if KIERO_ARCH_X64
static uint64_t* g_methodsTable = NULL;
#else
static uint32_t* g_methodsTable = NULL;
#endif


HMODULE GetSystemModule(const char* module) {
	static std::string systemPath;
	if (systemPath.empty()) {
		systemPath.resize(2048);
		uint32_t res = GetSystemDirectoryA(&systemPath[0], systemPath.size());
		systemPath.resize(res);
	}

	std::string basePath = systemPath + "\\" + module;
	//MessageBoxA(NULL, basePath.data(), "", 0);
	return GetModuleHandleA(basePath.c_str());
}

kiero::Status::Enum kiero::init(int _renderType)
{
	if (_renderType != RenderType::None)
	{
		if (_renderType >= RenderType::D3D9 && _renderType <= RenderType::D3D12)
		{
			WNDCLASSEX windowClass;
			windowClass.cbSize = sizeof(WNDCLASSEX);
			windowClass.style = CS_HREDRAW | CS_VREDRAW;
			windowClass.lpfnWndProc = DefWindowProc;
			windowClass.cbClsExtra = 0;
			windowClass.cbWndExtra = 0;
			windowClass.hInstance = GetModuleHandle(NULL);
			windowClass.hIcon = NULL;
			windowClass.hCursor = NULL;
			windowClass.hbrBackground = NULL;
			windowClass.lpszMenuName = NULL;
			windowClass.lpszClassName = KIERO_TEXT("Kiero");
			windowClass.hIconSm = NULL;

			::RegisterClassEx(&windowClass);

			HWND window = ::CreateWindow(windowClass.lpszClassName, KIERO_TEXT("Kiero DirectX Window"), WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, windowClass.hInstance, NULL);

			if (_renderType == RenderType::D3D9)
			{
#ifdef _D3D9_H_
				HMODULE libD3D9;
				if ((libD3D9 = ::GetModuleHandle(KIERO_TEXT("d3d9.dll"))) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::ModuleNotFoundError;
				}

				void* Direct3DCreate9;
				if ((Direct3DCreate9 = ::GetProcAddress(libD3D9, "Direct3DCreate9")) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				LPDIRECT3D9 direct3D9;
				if ((direct3D9 = ((LPDIRECT3D9(__stdcall*)(uint32_t))(Direct3DCreate9))(D3D_SDK_VERSION)) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				D3DDISPLAYMODE displayMode;
				if (direct3D9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &displayMode) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				D3DPRESENT_PARAMETERS params;
				params.BackBufferWidth = 0;
				params.BackBufferHeight = 0;
				params.BackBufferFormat = displayMode.Format;
				params.BackBufferCount = 0;
				params.MultiSampleType = D3DMULTISAMPLE_NONE;
				params.MultiSampleQuality = NULL;
				params.SwapEffect = D3DSWAPEFFECT_DISCARD;
				params.hDeviceWindow = window;
				params.Windowed = 1;
				params.EnableAutoDepthStencil = 0;
				params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
				params.Flags = NULL;
				params.FullScreen_RefreshRateInHz = 0;
				params.PresentationInterval = 0;

				LPDIRECT3DDEVICE9 device;
				if (direct3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT, &params, &device) < 0)
				{
					direct3D9->Release();
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

#if KIERO_ARCH_X64
				g_methodsTable = (uint64_t*)::calloc(119, sizeof(uint64_t));
				::memcpy(g_methodsTable, *(uint64_t**)device, 119 * sizeof(uint64_t));
#else
				g_methodsTable = (uint32_t*)::calloc(119, sizeof(uint32_t));
				::memcpy(g_methodsTable, *(uint32_t**)device, 119 * sizeof(uint32_t));
#endif

#ifdef KIERO_USE_MINHOOK
				MH_Initialize();
#endif

				direct3D9->Release();
				direct3D9 = NULL;

				device->Release();
				device = NULL;

				g_renderType = RenderType::D3D9;

				::DestroyWindow(window);
				::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

				return Status::Success;
#endif // _D3D9_H_
			}
			else if (_renderType == RenderType::D3D10)
			{
#if defined(__d3d10_h__) && defined(KIERO_D3D10_USAGE)
				HMODULE libDXGI;
				HMODULE libD3D10;
				if ((libDXGI = ::GetModuleHandle(KIERO_TEXT("dxgi.dll"))) == NULL || (libD3D10 = ::GetModuleHandle(KIERO_TEXT("d3d10.dll"))) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::ModuleNotFoundError;
				}

				void* CreateDXGIFactory;
				if ((CreateDXGIFactory = ::GetProcAddress(libDXGI, "CreateDXGIFactory")) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				IDXGIFactory* factory;
				if (((long(__stdcall*)(const IID&, void**))(CreateDXGIFactory))(__uuidof(IDXGIFactory), (void**)&factory) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				IDXGIAdapter* adapter;
				if (factory->EnumAdapters(0, &adapter) == DXGI_ERROR_NOT_FOUND)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				void* D3D10CreateDeviceAndSwapChain;
				if ((D3D10CreateDeviceAndSwapChain = ::GetProcAddress(libD3D10, "D3D10CreateDeviceAndSwapChain")) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				DXGI_RATIONAL refreshRate;
				refreshRate.Numerator = 60;
				refreshRate.Denominator = 1;

				DXGI_MODE_DESC bufferDesc;
				bufferDesc.Width = 100;
				bufferDesc.Height = 100;
				bufferDesc.RefreshRate = refreshRate;
				bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
				bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

				DXGI_SAMPLE_DESC sampleDesc;
				sampleDesc.Count = 1;
				sampleDesc.Quality = 0;

				DXGI_SWAP_CHAIN_DESC swapChainDesc;
				swapChainDesc.BufferDesc = bufferDesc;
				swapChainDesc.SampleDesc = sampleDesc;
				swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				swapChainDesc.BufferCount = 1;
				swapChainDesc.OutputWindow = window;
				swapChainDesc.Windowed = 1;
				swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
				swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

				IDXGISwapChain* swapChain;
				ID3D10Device* device;

				if (((long(__stdcall*)(
					IDXGIAdapter*,
					D3D10_DRIVER_TYPE,
					HMODULE,
					UINT,
					UINT,
					DXGI_SWAP_CHAIN_DESC*,
					IDXGISwapChain**,
					ID3D10Device**))(D3D10CreateDeviceAndSwapChain))(adapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &swapChainDesc, &swapChain, &device) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

#if KIERO_ARCH_X64
				g_methodsTable = (uint64_t*)::calloc(116, sizeof(uint64_t));
				::memcpy(g_methodsTable, *(uint64_t**)swapChain, 18 * sizeof(uint64_t));
				::memcpy(g_methodsTable + 18, *(uint64_t**)device, 98 * sizeof(uint64_t));
#else

				g_methodsTable = (uint32_t*)::calloc(116, sizeof(uint32_t));
				::memcpy(g_methodsTable, *(uint32_t**)swapChain, 18 * sizeof(uint32_t));
				::memcpy(g_methodsTable + 18, *(uint32_t**)device, 98 * sizeof(uint32_t));
#endif

#ifdef KIERO_USE_MINHOOK
				MH_Initialize();
#endif

				swapChain->Release();
				swapChain = NULL;

				device->Release();
				device = NULL;

				::DestroyWindow(window);
				::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

				g_renderType = RenderType::D3D10;

				return Status::Success;
#endif // __d3d10_h__
			}
			else if (_renderType == RenderType::D3D11)
			{
#ifdef __d3d11_h__
				HMODULE libD3D11;
				if ((libD3D11 = ::GetModuleHandle(KIERO_TEXT("d3d11.dll"))) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::ModuleNotFoundError;
				}

				void* D3D11CreateDeviceAndSwapChain;
				if ((D3D11CreateDeviceAndSwapChain = ::GetProcAddress(libD3D11, "D3D11CreateDeviceAndSwapChain")) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				D3D_FEATURE_LEVEL featureLevel;
				const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_11_0 };

				DXGI_RATIONAL refreshRate;
				refreshRate.Numerator = 60;
				refreshRate.Denominator = 1;

				DXGI_MODE_DESC bufferDesc;
				bufferDesc.Width = 100;
				bufferDesc.Height = 100;
				bufferDesc.RefreshRate = refreshRate;
				bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
				bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

				DXGI_SAMPLE_DESC sampleDesc;
				sampleDesc.Count = 1;
				sampleDesc.Quality = 0;

				DXGI_SWAP_CHAIN_DESC swapChainDesc;
				swapChainDesc.BufferDesc = bufferDesc;
				swapChainDesc.SampleDesc = sampleDesc;
				swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				swapChainDesc.BufferCount = 1;
				swapChainDesc.OutputWindow = window;
				swapChainDesc.Windowed = 1;
				swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
				swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

				IDXGISwapChain* swapChain;
				ID3D11Device* device;
				ID3D11DeviceContext* context;

				if (((long(__stdcall*)(
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
					ID3D11DeviceContext**))(D3D11CreateDeviceAndSwapChain))(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevels, 1, D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device, &featureLevel, &context) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

#if KIERO_ARCH_X64
				g_methodsTable = (uint64_t*)::calloc(205, sizeof(uint64_t));
				::memcpy(g_methodsTable, *(uint64_t**)swapChain, 18 * sizeof(uint64_t));
				::memcpy(g_methodsTable + 18, *(uint64_t**)device, 43 * sizeof(uint64_t));
				::memcpy(g_methodsTable + 18 + 43, *(uint64_t**)context, 144 * sizeof(uint64_t));
#else
				g_methodsTable = (uint32_t*)::calloc(205, sizeof(uint32_t));
				::memcpy(g_methodsTable, *(uint32_t**)swapChain, 18 * sizeof(uint32_t));
				::memcpy(g_methodsTable + 18, *(uint32_t**)device, 43 * sizeof(uint32_t));
				::memcpy(g_methodsTable + 18 + 43, *(uint32_t**)context, 144 * sizeof(uint32_t));
#endif

#ifdef KIERO_USE_MINHOOK
				MH_Initialize();
#endif

				swapChain->Release();
				swapChain = NULL;

				device->Release();
				device = NULL;

				context->Release();
				context = NULL;

				::DestroyWindow(window);
				::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

				g_renderType = RenderType::D3D11;

				return Status::Success;
#endif // __d3d11_h__
			}
			else if (_renderType == RenderType::D3D12)
			{
#if defined(__d3d12_h__) && defined(__dxgi_h__)
				HMODULE libDXGI;
				HMODULE libD3D12;
				if ((libDXGI = ::GetModuleHandle(KIERO_TEXT("dxgi.dll"))) == NULL || (libD3D12 = ::GetModuleHandle(KIERO_TEXT("d3d12.dll"))) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::ModuleNotFoundError;
				}

				void* CreateDXGIFactory;
				if ((CreateDXGIFactory = ::GetProcAddress(libDXGI, "CreateDXGIFactory")) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				IDXGIFactory* factory;
				if (((long(__stdcall*)(const IID&, void**))(CreateDXGIFactory))(__uuidof(IDXGIFactory), (void**)&factory) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				IDXGIAdapter* adapter;
				if (factory->EnumAdapters(0, &adapter) == DXGI_ERROR_NOT_FOUND)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				void* D3D12CreateDevice;
				if ((D3D12CreateDevice = ::GetProcAddress(libD3D12, "D3D12CreateDevice")) == NULL)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				ID3D12Device* device;
				if (((long(__stdcall*)(IUnknown*, D3D_FEATURE_LEVEL, const IID&, void**))(D3D12CreateDevice))(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&device) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				D3D12_COMMAND_QUEUE_DESC queueDesc;
				queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
				queueDesc.Priority = 0;
				queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
				queueDesc.NodeMask = 0;

				ID3D12CommandQueue* commandQueue;
				if (device->CreateCommandQueue(&queueDesc, __uuidof(ID3D12CommandQueue), (void**)&commandQueue) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				ID3D12CommandAllocator* commandAllocator;
				if (device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&commandAllocator) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				ID3D12GraphicsCommandList* commandList;
				if (device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&commandList) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

				DXGI_RATIONAL refreshRate;
				refreshRate.Numerator = 60;
				refreshRate.Denominator = 1;

				DXGI_MODE_DESC bufferDesc;
				bufferDesc.Width = 100;
				bufferDesc.Height = 100;
				bufferDesc.RefreshRate = refreshRate;
				bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
				bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

				DXGI_SAMPLE_DESC sampleDesc;
				sampleDesc.Count = 1;
				sampleDesc.Quality = 0;

				DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
				swapChainDesc.BufferDesc = bufferDesc;
				swapChainDesc.SampleDesc = sampleDesc;
				swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				swapChainDesc.BufferCount = 2;
				swapChainDesc.OutputWindow = window;
				swapChainDesc.Windowed = 1;
				swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
				swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

				IDXGISwapChain* swapChain;
				if (factory->CreateSwapChain(commandQueue, &swapChainDesc, &swapChain) < 0)
				{
					::DestroyWindow(window);
					::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
					return Status::UnknownError;
				}

#if KIERO_ARCH_X64
				g_methodsTable = (uint64_t*)::calloc(150, sizeof(uint64_t));
				memcpy(g_methodsTable, *(uint64_t**)device, 44 * sizeof(uint64_t));
				memcpy(g_methodsTable + 44, *(uint64_t**)commandQueue, 19 * sizeof(uint64_t));
				memcpy(g_methodsTable + 44 + 19, *(uint64_t**)commandAllocator, 9 * sizeof(uint64_t));
				memcpy(g_methodsTable + 44 + 19 + 9, *(uint64_t**)commandList, 60 * sizeof(uint64_t));
				memcpy(g_methodsTable + 44 + 19 + 9 + 60, *(uint64_t**)swapChain, 18 * sizeof(uint64_t));
#else
				g_methodsTable = (uint32_t*)::calloc(150, sizeof(uint32_t));
				memcpy(g_methodsTable, *(uint32_t**)device, 44 * sizeof(uint32_t));
				memcpy(g_methodsTable + 44, *(uint32_t**)commandQueue, 19 * sizeof(uint32_t));
				memcpy(g_methodsTable + 44 + 19, *(uint32_t**)commandAllocator, 9 * sizeof(uint32_t));
				memcpy(g_methodsTable + 44 + 19 + 9, *(uint32_t**)commandList, 60 * sizeof(uint32_t));
				memcpy(g_methodsTable + 44 + 19 + 9 + 60, *(uint32_t**)swapChain, 18 * sizeof(uint32_t));
#endif

#ifdef KIERO_USE_MINHOOK
				MH_Initialize();
#endif

				device->Release();
				device = NULL;

				commandQueue->Release();
				commandQueue = NULL;

				commandAllocator->Release();
				commandAllocator = NULL;

				commandList->Release();
				commandList = NULL;

				swapChain->Release();
				swapChain = NULL;

				::DestroyWindow(window);
				::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

				g_renderType = RenderType::D3D12;

				return Status::Success;
#endif // __d3d12_h__
			}

			return Status::NotSupportedError;
		}
		else if (_renderType == RenderType::OpenGL)
		{

			HMODULE libOpenGL32 = GetSystemModule("opengl32.dll");
			if (libOpenGL32 == NULL)
			{
				return Status::ModuleNotFoundError;
			}

			const char* const methodsNames[] = {
				"glBegin","glEnd","wglSwapBuffers"
			};

			const size_t size = KIERO_ARRAY_SIZE(methodsNames);

#if KIERO_ARCH_X64
			g_methodsTable = (uint64_t*)::calloc(size, sizeof(uint64_t));

			for (int i = 0; i < size; i++)
			{
				g_methodsTable[i] = (uint64_t)::GetProcAddress(libOpenGL32, methodsNames[i]);
			}
#else
			g_methodsTable = (uint32_t*)::calloc(size, sizeof(uint32_t));

			for (int i = 0; i < size; i++)
			{
				g_methodsTable[i] = (uint32_t)::GetProcAddress(libOpenGL32, methodsNames[i]);
			}
#endif

#ifdef KIERO_USE_MINHOOK
			MH_Initialize();
#endif

			g_renderType = RenderType::OpenGL;

			return Status::Success;
			//#endif // __gl_h_
		}
		else if (_renderType == RenderType::OpenglES) {
			HMODULE libegl = GetModuleHandleW(L"libEGL.dll");
			if (libegl == NULL)
			{
				return Status::ModuleNotFoundError;
			}

			const char* const methodsNames[] = {
				"eglSwapBuffers"
			};

			const size_t size = KIERO_ARRAY_SIZE(methodsNames);

#if KIERO_ARCH_X64
			g_methodsTable = (uint64_t*)::calloc(size, sizeof(uint64_t));

			for (int i = 0; i < size; i++)
			{
				g_methodsTable[i] = (uint64_t)::GetProcAddress(libegl, methodsNames[i]);
			}
#else
			g_methodsTable = (uint32_t*)::calloc(size, sizeof(uint32_t));

			for (int i = 0; i < size; i++)
			{
				g_methodsTable[i] = (uint32_t)::GetProcAddress(libegl, methodsNames[i]);
			}
#endif

#ifdef KIERO_USE_MINHOOK
			MH_Initialize();
#endif

			g_renderType = RenderType::OpenglES;

			return Status::Success;
		}
		else if (_renderType == RenderType::Vulkan)
		{
#ifdef VULKAN_H_
			HMODULE libVulkan;
			if ((libVulkan = GetModuleHandle(KIERO_TEXT("vulcan-1.dll"))) == NULL)
			{
				return Status::ModuleNotFoundError;
			}

			const char* const methodsNames[] = {
				"vkCreateInstance", "vkDestroyInstance", "vkEnumeratePhysicalDevices", "vkGetPhysicalDeviceFeatures", "vkGetPhysicalDeviceFormatProperties", "vkGetPhysicalDeviceImageFormatProperties",
				"vkGetPhysicalDeviceProperties", "vkGetPhysicalDeviceQueueFamilyProperties", "vkGetPhysicalDeviceMemoryProperties", "vkGetInstanceProcAddr", "vkGetDeviceProcAddr", "vkCreateDevice",
				"vkDestroyDevice", "vkEnumerateInstanceExtensionProperties", "vkEnumerateDeviceExtensionProperties", "vkEnumerateDeviceLayerProperties", "vkGetDeviceQueue", "vkQueueSubmit", "vkQueueWaitIdle",
				"vkDeviceWaitIdle", "vkAllocateMemory", "vkFreeMemory", "vkMapMemory", "vkUnmapMemory", "vkFlushMappedMemoryRanges", "vkInvalidateMappedMemoryRanges", "vkGetDeviceMemoryCommitment",
				"vkBindBufferMemory", "vkBindImageMemory", "vkGetBufferMemoryRequirements", "vkGetImageMemoryRequirements", "vkGetImageSparseMemoryRequirements", "vkGetPhysicalDeviceSparseImageFormatProperties",
				"vkQueueBindSparse", "vkCreateFence", "vkDestroyFence", "vkResetFences", "vkGetFenceStatus", "vkWaitForFences", "vkCreateSemaphore", "vkDestroySemaphore", "vkCreateEvent", "vkDestroyEvent",
				"vkGetEventStatus", "vkSetEvent", "vkResetEvent", "vkCreateQueryPool", "vkDestroyQueryPool", "vkGetQueryPoolResults", "vkCreateBuffer", "vkDestroyBuffer", "vkCreateBufferView", "vkDestroyBufferView",
				"vkCreateImage", "vkDestroyImage", "vkGetImageSubresourceLayout", "vkCreateImageView", "vkDestroyImageView", "vkCreateShaderModule", "vkDestroyShaderModule", "vkCreatePipelineCache",
				"vkDestroyPipelineCache", "vkGetPipelineCacheData", "vkMergePipelineCaches", "vkCreateGraphicsPipelines", "vkCreateComputePipelines", "vkDestroyPipeline", "vkCreatePipelineLayout",
				"vkDestroyPipelineLayout", "vkCreateSampler", "vkDestroySampler", "vkCreateDescriptorSetLayout", "vkDestroyDescriptorSetLayout", "vkCreateDescriptorPool", "vkDestroyDescriptorPool",
				"vkResetDescriptorPool", "vkAllocateDescriptorSets", "vkFreeDescriptorSets", "vkUpdateDescriptorSets", "vkCreateFramebuffer", "vkDestroyFramebuffer", "vkCreateRenderPass", "vkDestroyRenderPass",
				"vkGetRenderAreaGranularity", "vkCreateCommandPool", "vkDestroyCommandPool", "vkResetCommandPool", "vkAllocateCommandBuffers", "vkFreeCommandBuffers", "vkBeginCommandBuffer", "vkEndCommandBuffer",
				"vkResetCommandBuffer", "vkCmdBindPipeline", "vkCmdSetViewport", "vkCmdSetScissor", "vkCmdSetLineWidth", "vkCmdSetDepthBias", "vkCmdSetBlendConstants", "vkCmdSetDepthBounds",
				"vkCmdSetStencilCompareMask", "vkCmdSetStencilWriteMask", "vkCmdSetStencilReference", "vkCmdBindDescriptorSets", "vkCmdBindIndexBuffer", "vkCmdBindVertexBuffers", "vkCmdDraw", "vkCmdDrawIndexed",
				"vkCmdDrawIndirect", "vkCmdDrawIndexedIndirect", "vkCmdDispatch", "vkCmdDispatchIndirect", "vkCmdCopyBuffer", "vkCmdCopyImage", "vkCmdBlitImage", "vkCmdCopyBufferToImage", "vkCmdCopyImageToBuffer",
				"vkCmdUpdateBuffer", "vkCmdFillBuffer", "vkCmdClearColorImage", "vkCmdClearDepthStencilImage", "vkCmdClearAttachments", "vkCmdResolveImage", "vkCmdSetEvent", "vkCmdResetEvent", "vkCmdWaitEvents",
				"vkCmdPipelineBarrier", "vkCmdBeginQuery", "vkCmdEndQuery", "vkCmdResetQueryPool", "vkCmdWriteTimestamp", "vkCmdCopyQueryPoolResults", "vkCmdPushConstants", "vkCmdBeginRenderPass", "vkCmdNextSubpass",
				"vkCmdEndRenderPass", "vkCmdExecuteCommands"
			};

			size_t size = KIERO_ARRAY_SIZE(methodsNames);

#if KIERO_ARCH_X64
			g_methodsTable = (uint64_t*)::calloc(size, sizeof(uint64_t));

			for (int i = 0; i < size; i++)
			{
				g_methodsTable[i] = (uint64_t)::GetProcAddress(libVulkan, methodsNames[i]);
			}
#else
			g_methodsTable = (uint32_t*)::calloc(size, sizeof(uint32_t));

			for (int i = 0; i < size; i++)
			{
				g_methodsTable[i] = (uint32_t)::GetProcAddress(libVulkan, methodsNames[i]);
			}
#endif

#ifdef KIERO_USE_MINHOOK
			MH_Initialize();
#endif

			g_renderType = RenderType::Vulkan;

			return Status::Success;
#endif // VULKAN_H_
		}

		return Status::NotSupportedError;
	}

	return Status::Success;
}

void kiero::shutdown()
{
	if (g_renderType > 0)
	{
#ifdef KIERO_USE_MINHOOK
		MH_Uninitialize();
#endif

		::free(g_methodsTable);
		g_methodsTable = NULL;
		g_renderType = RenderType::None;
	}
}

kiero::RenderType::Enum kiero::getRenderType()
{
	return g_renderType;
}

#if KIERO_ARCH_X64
uint64_t* kiero::getMethodsTable()
{
	return g_methodsTable;
}
#else
uint32_t* kiero::getMethodsTable()
{
	return g_methodsTable;
}
#endif


//void kiero::bind(uint16_t _index, void* _original, void* _function)
int kiero::bind(uint16_t _index, void** _original, void* _function)
{
	// TODO: Need own detour function

#ifdef KIERO_USE_MINHOOK
	if (g_renderType > 0)
	{
		//MH_CreateHook((void*)g_methodsTable[_index], _function, &_original);
		int r1 = MH_CreateHook((void*)g_methodsTable[_index], _function, _original);
		int r2 = MH_EnableHook((void*)g_methodsTable[_index]);


		return r1 == MH_OK && r2 == MH_OK ? 1 : 0;
	}
	return 0;
#endif
}

int kiero::unbind() {
	int ret = -1;
	if (g_renderType > 0)
	{
		MH_DisableHook(MH_ALL_HOOKS);
		ret = MH_RemoveHook(MH_ALL_HOOKS);
		
		kiero::shutdown();
		//MessageBoxA(NULL, MH_StatusToString((MH_STATUS)ret), "", 0);
		//MH_DisableHook((void*)g_methodsTable[_index]);
		//MH_RemoveHook((void*)g_methodsTable[_index]);
		//MH_CreateHook((void*)g_methodsTable[_index], _function, &_original);

	}
	return ret;
}