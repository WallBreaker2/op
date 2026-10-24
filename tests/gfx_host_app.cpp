// 端到端验证用的「真实图形目标进程」，一个程序覆盖 d3d9 / d3d10 / d3d12 / opengl 四个后端。
//
// 为什么需要它（与 d3d11_host_app.cpp 同一个理由，只是后端从 1 个扩到 4 个）：
//   dx.d3d9 / dx.d3d10 / dx.d3d12 / opengl 这些后台截图把 op_x64.dll 注入到**别的进程**，
//   在目标进程的 EndScene / Present / wglSwapBuffers 里干活，把整帧写进命名共享段，
//   再由本进程的 HookCapture 读出来。这条链路只有在"两个进程 + 真实交换链"下才成立：
//     - 只有跨进程才能验证共享段是两边各自 open 的（映射缓存后才可能读到陈旧数据）；
//     - 只有真交换链才会真正走钩子（d3d9 挂 EndScene、dx10/11/12 挂 Present、GL 挂 SwapBuffers）。
//   所以这里做个最小的四后端窗口程序，按固定节奏换清屏色，让测试能判断"读到的是新帧"。
//
// 与 d3d11_host_app.exe 的分工：d3d11 那份保持不变（已有两条用例在跑），
// 这里只补 d3d9 / d3d10 / d3d12 / opengl，避免为了加后端去动已在回归里的东西。
//
// 用法：gfx_host_app.exe --api=d3d9|d3d10|d3d12|opengl [--seconds=N] [--static]
//                        [--width=W] [--height=H] [--gl-swap=wgl|gdi] [--gl-extra-swap] [--no-marker]
//                        [--d3d12-extra-swap]
//   --api       必填，决定走哪套初始化（同时决定窗口类名，见下）
//   --seconds   自动退出时间（默认 60 秒），测试结束会直接 TerminateProcess
//   --static    不换色（用于自证"必须读到不止一种颜色"这条断言有牙）
//   --gl-swap   opengl 的交换方式：gdi=SwapBuffers(hdc)（真实 GL 程序/GLFW/SDL 走的路），
//               wgl=opengl32!wglSwapBuffers（钩子直接挂的入口），
//               begin=交换前再走一次 glBegin/glEnd（覆盖 opengl.std），
//               finish=交换前再走一次 glFinish（覆盖 opengl.fi）。默认 gdi。
//   --no-marker 不画顶部标记带（默认四个后端都画）
//   --d3d12-extra-swap  只对 d3d12 有效：再建一条交换链（第二个窗口），
//               录制/提交/Present 全在独立线程里跑，用来验"两条链并发进 Present 钩子"。
//
// 窗口类名固定为 OpGfxHostWindow_<api>，测试按类名找窗口。
// 顶部标记带：d3d9 用 Clear 的矩形参数、opengl 用 glScissor、d3d10/d3d12 用一条
// width×height/4 的白色纹理 CopySubresourceRegion —— 都能在不写着色器的前提下画出
// 「上白下彩」的非对称帧，用来证明逐行翻转路径（传统 GL）没有把画面上下颠倒，
// 同时给"只搬请求区域"的用例提供一条稳定的颜色分界线。

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <d3d10.h>
#include <d3d12.h>
#include <d3d9.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <GL/gl.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

// wglSwapBuffers 没有出现在 Windows SDK 的头文件里（公开入口是 gdi32 的 SwapBuffers），
// 但 opengl32.dll 确实导出它，产品的 GL 钩子挂的就是这个符号。按导出原型自己声明。
extern "C" BOOL WINAPI wglSwapBuffers(HDC);

namespace {

using Microsoft::WRL::ComPtr;

// 三种纯色轮换。测试读到其中任意一种都算对，读到两种以上说明帧在更新。
//
// 只取 0.0f / 1.0f：这样 float -> UNORM 的换算对任何驱动都是精确的，测试可以按
// "RRGGBB" 精确比对，不需要容忍度。
//
// 颜色特意选成「红 / 青 / 品红」而不是「红 / 绿 / 蓝」：红绿蓝三色在 R/B 或 R/G
// 互换后仍然落在这三个颜色里，测试就永远发现不了通道颠倒（D3D12 抓帧把格式写死成
// IBF_R8G8B8A8 时就出过这个错）。红这一档在 R/B 互换后变成蓝、在 R/G 互换后变成绿，
// 蓝和绿都不在期望集合里，所以只要采样到红就能把通道颠倒抓出来；500ms 换一次色、
// 采 6 次跨越 3.6s，一定会采到红。
const float kColors[3][4] = {
    {1.0f, 0.0f, 0.0f, 1.0f}, // 红 FF0000
    {0.0f, 1.0f, 1.0f, 1.0f}, // 青 00FFFF
    {1.0f, 0.0f, 1.0f, 1.0f}, // 品红 FF00FF
};
const ULONGLONG kColorPeriodMs = 500;

// 顶部标记带高度：取 1/4 高度，保证测试采样点 (x, 20) 落在带内、(x, 中心) 落在带外。
int g_marker_rows = 90;

enum Api { API_D3D9 = 0, API_D3D10, API_D3D12, API_OPENGL };

Api g_api = API_D3D9;
std::wstring g_class_name;
const wchar_t *g_title = L"OP GFX HOST";

int g_width = 640;
int g_height = 360;
bool g_static = false;
bool g_marker = true;
bool g_gl_swap_wgl = false;
// --gl-swap=begin|finish：额外走一次 glBegin/glEnd 或 glFinish，用来触发
// opengl.std / opengl.fi 这两条入口（产品侧把钩子挂在 glBegin / glFinish 上）。
int g_gl_entry = 0; // 0=只 SwapBuffers，1=glBegin，2=glFinish
int g_color_index = 0;
int g_color_switches = 0;

D3DCOLOR ToD3DColor(const float *rgba) {
    const int r = static_cast<int>(rgba[0] * 255.0f + 0.5f);
    const int g = static_cast<int>(rgba[1] * 255.0f + 0.5f);
    const int b = static_cast<int>(rgba[2] * 255.0f + 0.5f);
    return D3DCOLOR_XRGB(r, g, b);
}

// ---------------------------------------------------------------------------
// D3D9：钩子挂在 IDirect3DDevice9::EndScene（索引 42），所以必须 Begin/EndScene 一对，
// 抓帧发生在真正的 EndScene 返回之后（不在场景内，GetRenderTargetData 才合法）。
// ---------------------------------------------------------------------------

IDirect3D9 *g_d3d9 = nullptr;
IDirect3DDevice9 *g_dev9 = nullptr;

bool InitD3D9(HWND hwnd) {
    g_d3d9 = ::Direct3DCreate9(D3D_SDK_VERSION);
    if (!g_d3d9) {
        std::printf("HOST_FAIL Direct3DCreate9 returned null\n");
        return false;
    }

    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = static_cast<UINT>(g_width);
    pp.BackBufferHeight = static_cast<UINT>(g_height);
    // X8R8G8B8 的内存序就是 B,G,R,X，正好等于共享段约定的 BGRA，不用做通道交换。
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.MultiSampleType = D3DMULTISAMPLE_NONE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = hwnd;
    pp.Windowed = TRUE;
    pp.EnableAutoDepthStencil = FALSE;
    pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    const DWORD flags = D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE;
    const HRESULT hr = g_d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, flags, &pp, &g_dev9);
    if (FAILED(hr) || !g_dev9) {
        std::printf("HOST_FAIL CreateDevice(d3d9) hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }
    return true;
}

void DrawD3D9(int index) {
    g_dev9->Clear(0, nullptr, D3DCLEAR_TARGET, ToD3DColor(kColors[index]), 1.0f, 0);
    if (g_marker) {
        D3DRECT band = {0, 0, static_cast<LONG>(g_width), static_cast<LONG>(g_marker_rows)};
        g_dev9->Clear(1, &band, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255, 255, 255), 1.0f, 0);
    }
    g_dev9->BeginScene();
    // EndScene 里就是注入的 dx9 钩子在干活。
    g_dev9->EndScene();
    g_dev9->Present(nullptr, nullptr, nullptr, nullptr);
}

void ReleaseD3D9() {
    if (g_dev9) {
        g_dev9->Release();
        g_dev9 = nullptr;
    }
    if (g_d3d9) {
        g_d3d9->Release();
        g_d3d9 = nullptr;
    }
}

// ---------------------------------------------------------------------------
// D3D10：钩子挂在 IDXGISwapChain::Present（索引 8）。
// 交换链用 DXGI_SWAP_EFFECT_DISCARD（老式 blt 模型），和 d3d11_host_app 保持一致。
// ---------------------------------------------------------------------------

ID3D10Device *g_dev10 = nullptr;
IDXGISwapChain *g_swap10 = nullptr;
ID3D10RenderTargetView *g_rtv10 = nullptr;
// 顶部标记带：d3d10 的 ClearRenderTargetView 没有矩形参数（那是 d3d9 独有的），
// 所以另建一条 width×marker_rows 的白色纹理，每帧 CopySubresourceRegion 到后备缓冲的 (0,0)。
// 这样不用引着色器，也能让"白色带必须落在图像顶部"这条方向断言覆盖到 d3d10。
ID3D10Texture2D *g_band10 = nullptr;

bool InitD3D10(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = static_cast<UINT>(g_width);
    sd.BufferDesc.Height = static_cast<UINT>(g_height);
    // 这里用 R8G8B8A8_UNORM 而不是 d3d11_host_app 的 B8G8R8A8_UNORM：
    //   1. 实测本机 D3D10 建 B8G8R8A8 交换链一律 E_INVALIDARG（两种驱动、两种 buffer 数、
    //      blt/flip 两种模型都试过，见 doc/defect_fixes.md §72），R8G8B8A8 才成功；
    //   2. 顺带把产品的「R8G8B8A8 后备缓冲 → BGRA 共享帧」通道交换分支也纳入端到端覆盖，
    //      d3d9/d3d11/d3d12 那边覆盖的是 B8G8R8A8 直拷分支，两边正好互补。
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    HRESULT hr = ::D3D10CreateDeviceAndSwapChain(nullptr, D3D10_DRIVER_TYPE_HARDWARE, nullptr, 0, D3D10_SDK_VERSION,
                                                 &sd, &g_swap10, &g_dev10);
    if (FAILED(hr) || !g_swap10 || !g_dev10) {
        // 实测：本机 D3D10CreateDeviceAndSwapChain 直接返回 E_INVALIDARG(0x80070057)，
        // 同样的 desc 走"先建设备、再用 DXGI 工厂建交换链"就能成。两者语义等价
        // （前者内部就是这么做的），所以这里保留回退，免得换机器又踩一次。
        std::printf("HOST_NOTE D3D10CreateDeviceAndSwapChain hr=0x%08X, retry with factory path\n",
                    static_cast<unsigned>(hr));
        g_swap10 = nullptr;
        g_dev10 = nullptr;
        hr = ::D3D10CreateDevice(nullptr, D3D10_DRIVER_TYPE_HARDWARE, nullptr, 0, D3D10_SDK_VERSION, &g_dev10);
        if (FAILED(hr) || !g_dev10) {
            std::printf("HOST_FAIL D3D10CreateDevice hr=0x%08X\n", static_cast<unsigned>(hr));
            return false;
        }
        ComPtr<IDXGIDevice> dxgi_device;
        ComPtr<IDXGIAdapter> adapter;
        ComPtr<IDXGIFactory> factory;
        hr = g_dev10->QueryInterface(IID_PPV_ARGS(&dxgi_device));
        if (SUCCEEDED(hr) && dxgi_device) {
            hr = dxgi_device->GetAdapter(&adapter);
        }
        if (SUCCEEDED(hr) && adapter) {
            hr = adapter->GetParent(IID_PPV_ARGS(&factory));
        }
        if (FAILED(hr) || !factory) {
            std::printf("HOST_FAIL DXGI factory hr=0x%08X\n", static_cast<unsigned>(hr));
            return false;
        }
        hr = factory->CreateSwapChain(g_dev10, &sd, &g_swap10);
        if (FAILED(hr) || !g_swap10) {
            std::printf("HOST_FAIL IDXGIFactory::CreateSwapChain hr=0x%08X\n", static_cast<unsigned>(hr));
            return false;
        }
    }

    ID3D10Texture2D *backbuffer = nullptr;
    hr = g_swap10->GetBuffer(0, __uuidof(ID3D10Texture2D), reinterpret_cast<void **>(&backbuffer));
    if (FAILED(hr) || !backbuffer) {
        std::printf("HOST_FAIL GetBuffer(d3d10) hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }
    hr = g_dev10->CreateRenderTargetView(backbuffer, nullptr, &g_rtv10);
    backbuffer->Release();
    if (FAILED(hr)) {
        std::printf("HOST_FAIL CreateRenderTargetView(d3d10) hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }

    if (g_marker) {
        D3D10_TEXTURE2D_DESC band_desc = {};
        band_desc.Width = static_cast<UINT>(g_width);
        band_desc.Height = static_cast<UINT>(g_marker_rows);
        band_desc.MipLevels = 1;
        band_desc.ArraySize = 1;
        band_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        band_desc.SampleDesc.Count = 1;
        band_desc.Usage = D3D10_USAGE_DEFAULT;
        const std::string white(static_cast<size_t>(g_width) * static_cast<size_t>(g_marker_rows) * 4u, '\xFF');
        D3D10_SUBRESOURCE_DATA band_init = {};
        band_init.pSysMem = white.data();
        band_init.SysMemPitch = static_cast<UINT>(g_width) * 4u;
        hr = g_dev10->CreateTexture2D(&band_desc, &band_init, &g_band10);
        if (FAILED(hr) || !g_band10) {
            std::printf("HOST_FAIL CreateTexture2D(band,d3d10) hr=0x%08X\n", static_cast<unsigned>(hr));
            return false;
        }
    }
    return true;
}

void DrawD3D10(int index) {
    g_dev10->ClearRenderTargetView(g_rtv10, kColors[index]);
    // 顶部标记带：拷进后备缓冲的 (0,0)。这条路径不绑定渲染目标，所以不需要先解绑；
    // CopySubresourceRegion 的源是 DEFAULT 用途的普通纹理，两边格式都是 R8G8B8A8。
    if (g_marker && g_band10) {
        ID3D10Texture2D *backbuffer = nullptr;
        if (SUCCEEDED(g_swap10->GetBuffer(0, __uuidof(ID3D10Texture2D),
                                         reinterpret_cast<void **>(&backbuffer))) &&
            backbuffer) {
            g_dev10->CopySubresourceRegion(backbuffer, 0, 0, 0, 0, g_band10, 0, nullptr);
            backbuffer->Release();
        }
    }
    // Present 里就是注入的 dx10 钩子在干活。
    g_swap10->Present(1, 0);
}

void ReleaseD3D10() {
    if (g_band10) {
        g_band10->Release();
        g_band10 = nullptr;
    }
    if (g_rtv10) {
        g_rtv10->Release();
        g_rtv10 = nullptr;
    }
    if (g_swap10) {
        g_swap10->Release();
        g_swap10 = nullptr;
    }
    if (g_dev10) {
        g_dev10->Release();
        g_dev10 = nullptr;
    }
}

// ---------------------------------------------------------------------------
// D3D12：钩子挂在 IDXGISwapChain::Present（索引 8）。
// 必须用 flip 模型（FLIP_DISCARD / FLIP_SEQUENTIAL）：blt 模型下 GetBuffer 拿到的
// 后台缓冲在 Present 时已经被换走，钩子抓不到当前帧。
//
// 这里刻意按"游戏的样子"搭，而不是最简三条命令：3 个后备缓冲 + 2 套
// （分配器 + 命令列表）轮流复用。真游戏基本都这么写，而我们之前只在"1 套分配器 +
// 2 个缓冲"上验过，多缓冲轮转下钩子拿到的 GetCurrentBackBufferIndex / 资源状态
// 组合是另一批（见 doc/next_steps.md §4.3）。
// ---------------------------------------------------------------------------

const UINT kD3D12BufferCount = 3;
// 两套分配器轮转：上一帧的分配器在 GPU 上还没跑完时，这一帧用另一套，不必等。
const UINT kD3D12AllocatorCount = 2;

ComPtr<ID3D12Device> g_dev12;
ComPtr<ID3D12CommandQueue> g_queue12;
ComPtr<ID3D12CommandAllocator> g_allocs12[kD3D12AllocatorCount];
ComPtr<ID3D12GraphicsCommandList> g_lists12[kD3D12AllocatorCount];
UINT g_alloc_slot12 = 0;
ComPtr<ID3D12Fence> g_fence12;
ComPtr<IDXGISwapChain3> g_swap12;
ComPtr<ID3D12DescriptorHeap> g_rtv_heap12;
// 顶部标记带（width×marker_rows 的白色纹理，常驻 COPY_SOURCE）。
ComPtr<ID3D12Resource> g_band12;
ComPtr<ID3D12Resource> g_back_buffers12[kD3D12BufferCount];
D3D12_CPU_DESCRIPTOR_HANDLE g_rtv_handles12[kD3D12BufferCount] = {};

// --d3d12-extra-swap：再起一条交换链，在独立线程里 Present。
// 实现块在 ReleaseD3D12 之后（那里才用到 OpenGL 之前的公共设施），所以先声明。
bool g_d3d12_extra_swap = false;
HANDLE g_thread12_b = nullptr;
bool InitD3D12ExtraSwap();
DWORD WINAPI ExtraSwapThread(LPVOID);
void ReleaseD3D12ExtraSwap();
UINT g_rtv_stride12 = 0;
HANDLE g_fence_event12 = nullptr;
UINT64 g_fence_value12 = 0;

D3D12_RESOURCE_BARRIER MakeTransition(ID3D12Resource *resource, D3D12_RESOURCE_STATES before,
                                      D3D12_RESOURCE_STATES after) {
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    return barrier;
}

bool InitD3D12(HWND hwnd) {
    ComPtr<IDXGIFactory4> factory;
    HRESULT hr = ::CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    if (FAILED(hr) || !factory) {
        std::printf("HOST_FAIL CreateDXGIFactory1 hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }

    // 显式挑第一个适配器：D3D12CreateDevice(nullptr, ...) 在混合显卡机器上可能选到
    // 不支持 D3D12 的那块，测试机不该踩这个坑。
    ComPtr<IDXGIAdapter1> adapter;
    factory->EnumAdapters1(0, adapter.GetAddressOf());
    hr = ::D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_dev12));
    if (FAILED(hr) || !g_dev12) {
        std::printf("HOST_FAIL D3D12CreateDevice hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }

    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    hr = g_dev12->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&g_queue12));
    if (FAILED(hr)) {
        std::printf("HOST_FAIL CreateCommandQueue hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }
    for (UINT i = 0; i < kD3D12AllocatorCount; ++i) {
        hr = g_dev12->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_allocs12[i]));
        if (FAILED(hr)) {
            std::printf("HOST_FAIL CreateCommandAllocator(%u) hr=0x%08X\n", i, static_cast<unsigned>(hr));
            return false;
        }
        hr = g_dev12->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_allocs12[i].Get(), nullptr,
                                        IID_PPV_ARGS(&g_lists12[i]));
        if (FAILED(hr)) {
            std::printf("HOST_FAIL CreateCommandList(%u) hr=0x%08X\n", i, static_cast<unsigned>(hr));
            return false;
        }
        // CreateCommandList 出来是打开状态，先关掉，后面每帧重新 Reset。
        g_lists12[i]->Close();
    }
    hr = g_dev12->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence12));
    if (FAILED(hr)) {
        std::printf("HOST_FAIL CreateFence hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }
    g_fence_event12 = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!g_fence_event12) {
        std::printf("HOST_FAIL CreateEventW err=%lu\n", ::GetLastError());
        return false;
    }

    DXGI_SWAP_CHAIN_DESC1 sd = {};
    sd.Width = static_cast<UINT>(g_width);
    sd.Height = static_cast<UINT>(g_height);
    sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = kD3D12BufferCount;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    sd.Scaling = DXGI_SCALING_STRETCH;

    ComPtr<IDXGISwapChain1> swap1;
    hr = factory->CreateSwapChainForHwnd(g_queue12.Get(), hwnd, &sd, nullptr, nullptr, &swap1);
    if (FAILED(hr) || !swap1) {
        std::printf("HOST_FAIL CreateSwapChainForHwnd hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }
    hr = swap1.As(&g_swap12);
    if (FAILED(hr) || !g_swap12) {
        std::printf("HOST_FAIL QueryInterface(IDXGISwapChain3) hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heap_desc.NumDescriptors = kD3D12BufferCount;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = g_dev12->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&g_rtv_heap12));
    if (FAILED(hr)) {
        std::printf("HOST_FAIL CreateDescriptorHeap hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }
    g_rtv_stride12 = g_dev12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE handle = g_rtv_heap12->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < kD3D12BufferCount; ++i) {
        hr = g_swap12->GetBuffer(i, IID_PPV_ARGS(&g_back_buffers12[i]));
        if (FAILED(hr)) {
            std::printf("HOST_FAIL GetBuffer(d3d12,%u) hr=0x%08X\n", i, static_cast<unsigned>(hr));
            return false;
        }
        g_rtv_handles12[i] = handle;
        g_dev12->CreateRenderTargetView(g_back_buffers12[i].Get(), nullptr, handle);
        handle.ptr += g_rtv_stride12;
    }

    if (g_marker) {
        // 白色标记带：先建一条 width×marker_rows 的 DEFAULT 纹理（初始 COPY_DEST），
        // 用 upload 堆里的全 0xFF 数据拷进去，最后转到 COPY_SOURCE 常驻。
        // d3d12 的 ClearRenderTargetView 同样没有矩形参数，只能这么画带。
        const UINT row_bytes = static_cast<UINT>(g_width) * 4u;
        const UINT64 band_bytes = static_cast<UINT64>(row_bytes) * static_cast<UINT64>(g_marker_rows);

        D3D12_HEAP_PROPERTIES upload_heap = {};
        upload_heap.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC upload_desc = {};
        upload_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        upload_desc.Width = band_bytes;
        upload_desc.Height = 1;
        upload_desc.DepthOrArraySize = 1;
        upload_desc.MipLevels = 1;
        upload_desc.Format = DXGI_FORMAT_UNKNOWN;
        upload_desc.SampleDesc.Count = 1;
        upload_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ComPtr<ID3D12Resource> upload;
        hr = g_dev12->CreateCommittedResource(&upload_heap, D3D12_HEAP_FLAG_NONE, &upload_desc,
                                             D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload));
        if (FAILED(hr) || !upload) {
            std::printf("HOST_FAIL CreateCommittedResource(upload,band) hr=0x%08X\n", static_cast<unsigned>(hr));
            return false;
        }
        void *mapped = nullptr;
        hr = upload->Map(0, nullptr, &mapped);
        if (FAILED(hr) || !mapped) {
            std::printf("HOST_FAIL Map(upload,band) hr=0x%08X\n", static_cast<unsigned>(hr));
            return false;
        }
        std::memset(mapped, 0xFF, static_cast<size_t>(band_bytes));
        upload->Unmap(0, nullptr);

        D3D12_HEAP_PROPERTIES band_heap = {};
        band_heap.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC band_desc = {};
        band_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        band_desc.Width = static_cast<UINT64>(g_width);
        band_desc.Height = static_cast<UINT>(g_marker_rows);
        band_desc.DepthOrArraySize = 1;
        band_desc.MipLevels = 1;
        band_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        band_desc.SampleDesc.Count = 1;
        band_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        hr = g_dev12->CreateCommittedResource(&band_heap, D3D12_HEAP_FLAG_NONE, &band_desc,
                                             D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&g_band12));
        if (FAILED(hr) || !g_band12) {
            std::printf("HOST_FAIL CreateCommittedResource(band,d3d12) hr=0x%08X\n", static_cast<unsigned>(hr));
            return false;
        }

        g_allocs12[0]->Reset();
        g_lists12[0]->Reset(g_allocs12[0].Get(), nullptr);
        D3D12_TEXTURE_COPY_LOCATION band_dst = {};
        band_dst.pResource = g_band12.Get();
        band_dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        band_dst.SubresourceIndex = 0;
        D3D12_TEXTURE_COPY_LOCATION band_src = {};
        band_src.pResource = upload.Get();
        band_src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        g_dev12->GetCopyableFootprints(&band_desc, 0, 1, 0, &band_src.PlacedFootprint, nullptr, nullptr, nullptr);
        g_lists12[0]->CopyTextureRegion(&band_dst, 0, 0, 0, &band_src, nullptr);
        D3D12_RESOURCE_BARRIER band_barrier =
            MakeTransition(g_band12.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);
        g_lists12[0]->ResourceBarrier(1, &band_barrier);
        g_lists12[0]->Close();
        ID3D12CommandList *band_lists[] = {g_lists12[0].Get()};
        g_queue12->ExecuteCommandLists(1, band_lists);
        g_queue12->Signal(g_fence12.Get(), ++g_fence_value12);
        g_fence12->SetEventOnCompletion(g_fence_value12, g_fence_event12);
        ::WaitForSingleObject(g_fence_event12, 5000);
        // upload 堆里的数据已经被拷进 g_band12，等到栅栏就能安全释放。
    }

    if (g_d3d12_extra_swap) {
        if (!InitD3D12ExtraSwap()) {
            return false;
        }
        g_thread12_b = ::CreateThread(nullptr, 0, ExtraSwapThread, nullptr, 0, nullptr);
        if (!g_thread12_b) {
            std::printf("HOST_FAIL CreateThread(extra swap) err=%lu\n", ::GetLastError());
            return false;
        }
        std::printf("HOST_NOTE extra swap chain started on its own thread\n");
        std::fflush(stdout);
    }
    return true;
}

void DrawD3D12(int index) {
    const UINT slot = g_swap12->GetCurrentBackBufferIndex();
    if (slot >= kD3D12BufferCount) {
        return;
    }
    // 两套（分配器 + 命令列表）轮转：上一帧的分配器即便还没在 GPU 上跑完，
    // 这一帧也能立刻开始录制，和真游戏的双缓冲提交是一个形状。
    const UINT alloc_slot = g_alloc_slot12;
    g_alloc_slot12 = (g_alloc_slot12 + 1) % kD3D12AllocatorCount;
    g_allocs12[alloc_slot]->Reset();
    g_lists12[alloc_slot]->Reset(g_allocs12[alloc_slot].Get(), nullptr);
    ID3D12GraphicsCommandList *list = g_lists12[alloc_slot].Get();

    D3D12_RESOURCE_BARRIER barrier = MakeTransition(g_back_buffers12[slot].Get(), D3D12_RESOURCE_STATE_PRESENT,
                                                    D3D12_RESOURCE_STATE_RENDER_TARGET);
    list->ResourceBarrier(1, &barrier);
    list->OMSetRenderTargets(1, &g_rtv_handles12[slot], FALSE, nullptr);
    list->ClearRenderTargetView(g_rtv_handles12[slot], kColors[index], 0, nullptr);

    if (g_marker && g_band12) {
        // 画顶部标记带：把白色带拷到后备缓冲的 (0,0)。拷贝前必须把渲染目标解绑，
        // 否则调试层会报"资源仍绑在 OM 上却被转成 COPY_DEST"。
        list->OMSetRenderTargets(0, nullptr, FALSE, nullptr);
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        list->ResourceBarrier(1, &barrier);
        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource = g_back_buffers12[slot].Get();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = 0;
        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = g_band12.Get();
        src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        src.SubresourceIndex = 0;
        list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    } else {
        // 后台缓冲交还给 Present 前必须回到 PRESENT —— 钩子也是在这个状态上做
        // PRESENT -> COPY_SOURCE -> PRESENT 的，状态对不上会让调试层报栅栏错配。
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    }
    list->ResourceBarrier(1, &barrier);

    list->Close();
    ID3D12CommandList *lists[] = {list};
    g_queue12->ExecuteCommandLists(1, lists);
    g_queue12->Signal(g_fence12.Get(), ++g_fence_value12);
    g_fence12->SetEventOnCompletion(g_fence_value12, g_fence_event12);
    // 先等这一帧画完再 Present：这样下一帧 Reset 分配器时上一帧一定已经执行完，
    // 也保证钩子在 Present 里抓到的后台缓冲内容是稳定的。
    ::WaitForSingleObject(g_fence_event12, 5000);
    // Present 里就是注入的 dx12 钩子在干活。
    g_swap12->Present(1, 0);
}

void ReleaseD3D12() {
    ReleaseD3D12ExtraSwap();
    for (UINT i = 0; i < kD3D12BufferCount; ++i) {
        g_back_buffers12[i].Reset();
    }
    g_band12.Reset();
    g_rtv_heap12.Reset();
    g_swap12.Reset();
    g_fence12.Reset();
    for (UINT i = 0; i < kD3D12AllocatorCount; ++i) {
        g_lists12[i].Reset();
        g_allocs12[i].Reset();
    }
    g_queue12.Reset();
    g_dev12.Reset();
    if (g_fence_event12) {
        ::CloseHandle(g_fence_event12);
        g_fence_event12 = nullptr;
    }
}

// ---------------------------------------------------------------------------
// 第二条交换链（--d3d12-extra-swap）
//
// 真游戏/编辑器常见一个进程里有多条交换链，而且各自在不同线程上 Present。
// 注入侧 `D3D12Capture` 是单例（自己一条拷贝队列 + 一套命令列表/回读缓冲），
// 两条链并发进 Present 钩子时那套状态是共享的 —— 这里就是把这种形状造出来：
// 第二个窗口 + 第二条交换链，录制、提交、Present 全在独立线程里做。
// 两个窗口用同一套颜色和同一条白色标记带，所以无论共享段里最后落的是哪条链的帧，
// 测试看到的内容都必须是"顶部白 + 底部轮换色"。
// ---------------------------------------------------------------------------

HWND g_hwnd12_b = nullptr;
ComPtr<IDXGISwapChain3> g_swap12_b;
ComPtr<ID3D12DescriptorHeap> g_rtv_heap12_b;
ComPtr<ID3D12Resource> g_back_buffers12_b[kD3D12BufferCount];
D3D12_CPU_DESCRIPTOR_HANDLE g_rtv_handles12_b[kD3D12BufferCount] = {};
ComPtr<ID3D12CommandAllocator> g_alloc12_b;
ComPtr<ID3D12GraphicsCommandList> g_list12_b;
ComPtr<ID3D12Fence> g_fence12_b;
HANDLE g_fence_event12_b = nullptr;
UINT64 g_fence_value12_b = 0;
volatile LONG g_stop12_b = 0;
int g_color_index_b = 0;

const wchar_t *kExtraSwapClass = L"OpGfxHostWindow_d3d12_b";

LRESULT CALLBACK ExtraSwapWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    return ::DefWindowProcW(hwnd, msg, wp, lp);
}

bool InitD3D12ExtraSwap() {
    HINSTANCE instance = ::GetModuleHandleW(nullptr);
    WNDCLASSEXW wc = {sizeof(wc)};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ExtraSwapWndProc;
    wc.hInstance = instance;
    wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kExtraSwapClass;
    if (!::RegisterClassExW(&wc) && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        std::printf("HOST_FAIL RegisterClassExW(extra) err=%lu\n", ::GetLastError());
        return false;
    }
    RECT rc = {0, 0, g_width, g_height};
    ::AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hwnd12_b = ::CreateWindowExW(0, kExtraSwapClass, L"OP GFX HOST d3d12 (extra swap)", WS_OVERLAPPEDWINDOW,
                                   CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr,
                                   nullptr, instance, nullptr);
    if (!g_hwnd12_b) {
        std::printf("HOST_FAIL CreateWindowExW(extra) err=%lu\n", ::GetLastError());
        return false;
    }
    ::ShowWindow(g_hwnd12_b, SW_SHOWNORMAL);
    ::UpdateWindow(g_hwnd12_b);

    DXGI_SWAP_CHAIN_DESC1 sd = {};
    sd.Width = static_cast<UINT>(g_width);
    sd.Height = static_cast<UINT>(g_height);
    sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = kD3D12BufferCount;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    sd.Scaling = DXGI_SCALING_STRETCH;

    Microsoft::WRL::ComPtr<IDXGIFactory2> factory;
    HRESULT hr = ::CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        std::printf("HOST_FAIL CreateDXGIFactory1(extra) hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }
    Microsoft::WRL::ComPtr<IDXGISwapChain1> swap1;
    hr = factory->CreateSwapChainForHwnd(g_queue12.Get(), g_hwnd12_b, &sd, nullptr, nullptr, &swap1);
    if (FAILED(hr) || !swap1) {
        std::printf("HOST_FAIL CreateSwapChainForHwnd(extra) hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }
    hr = swap1.As(&g_swap12_b);
    if (FAILED(hr) || !g_swap12_b) {
        std::printf("HOST_FAIL QueryInterface(IDXGISwapChain3,extra) hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heap_desc.NumDescriptors = kD3D12BufferCount;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = g_dev12->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&g_rtv_heap12_b));
    if (FAILED(hr)) {
        std::printf("HOST_FAIL CreateDescriptorHeap(extra) hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE handle = g_rtv_heap12_b->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < kD3D12BufferCount; ++i) {
        hr = g_swap12_b->GetBuffer(i, IID_PPV_ARGS(&g_back_buffers12_b[i]));
        if (FAILED(hr)) {
            std::printf("HOST_FAIL GetBuffer(d3d12,extra,%u) hr=0x%08X\n", i, static_cast<unsigned>(hr));
            return false;
        }
        g_rtv_handles12_b[i] = handle;
        g_dev12->CreateRenderTargetView(g_back_buffers12_b[i].Get(), nullptr, handle);
        handle.ptr += g_rtv_stride12;
    }

    hr = g_dev12->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_alloc12_b));
    if (FAILED(hr)) {
        std::printf("HOST_FAIL CreateCommandAllocator(extra) hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }
    hr = g_dev12->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_alloc12_b.Get(), nullptr,
                                    IID_PPV_ARGS(&g_list12_b));
    if (FAILED(hr)) {
        std::printf("HOST_FAIL CreateCommandList(extra) hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }
    g_list12_b->Close();
    hr = g_dev12->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence12_b));
    if (FAILED(hr)) {
        std::printf("HOST_FAIL CreateFence(extra) hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }
    g_fence_event12_b = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!g_fence_event12_b) {
        std::printf("HOST_FAIL CreateEventW(extra) err=%lu\n", ::GetLastError());
        return false;
    }
    return true;
}

// 只录制 + 提交 + 等栅栏，不 Present（Present 由线程循环调，方便对齐"钩子在 Present 里"）。
void DrawD3D12Extra(int index) {
    if (!g_swap12_b || !g_list12_b) {
        return;
    }
    const UINT slot = g_swap12_b->GetCurrentBackBufferIndex();
    if (slot >= kD3D12BufferCount) {
        return;
    }
    g_alloc12_b->Reset();
    g_list12_b->Reset(g_alloc12_b.Get(), nullptr);
    ID3D12GraphicsCommandList *list = g_list12_b.Get();

    D3D12_RESOURCE_BARRIER barrier = MakeTransition(g_back_buffers12_b[slot].Get(), D3D12_RESOURCE_STATE_PRESENT,
                                                    D3D12_RESOURCE_STATE_RENDER_TARGET);
    list->ResourceBarrier(1, &barrier);
    list->OMSetRenderTargets(1, &g_rtv_handles12_b[slot], FALSE, nullptr);
    list->ClearRenderTargetView(g_rtv_handles12_b[slot], kColors[index], 0, nullptr);

    if (g_marker && g_band12) {
        list->OMSetRenderTargets(0, nullptr, FALSE, nullptr);
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        list->ResourceBarrier(1, &barrier);
        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource = g_back_buffers12_b[slot].Get();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = 0;
        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = g_band12.Get();
        src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        src.SubresourceIndex = 0;
        list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    } else {
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    }
    list->ResourceBarrier(1, &barrier);
    list->Close();
    ID3D12CommandList *lists[] = {list};
    g_queue12->ExecuteCommandLists(1, lists);
    g_queue12->Signal(g_fence12_b.Get(), ++g_fence_value12_b);
    g_fence12_b->SetEventOnCompletion(g_fence_value12_b, g_fence_event12_b);
    ::WaitForSingleObject(g_fence_event12_b, 5000);
}

DWORD WINAPI ExtraSwapThread(LPVOID) {
    while (::InterlockedCompareExchange(&g_stop12_b, 0, 0) == 0) {
        DrawD3D12Extra(g_color_index_b);
        if (g_swap12_b) {
            g_swap12_b->Present(1, 0);
        }
        g_color_index_b = (g_color_index_b + 1) % 3;
        ::Sleep(16);
    }
    return 0;
}

void ReleaseD3D12ExtraSwap() {
    ::InterlockedExchange(&g_stop12_b, 1);
    if (g_thread12_b) {
        ::WaitForSingleObject(g_thread12_b, 5000);
        ::CloseHandle(g_thread12_b);
        g_thread12_b = nullptr;
    }
    for (UINT i = 0; i < kD3D12BufferCount; ++i) {
        g_back_buffers12_b[i].Reset();
    }
    g_rtv_heap12_b.Reset();
    g_swap12_b.Reset();
    g_fence12_b.Reset();
    g_list12_b.Reset();
    g_alloc12_b.Reset();
    if (g_fence_event12_b) {
        ::CloseHandle(g_fence_event12_b);
        g_fence_event12_b = nullptr;
    }
    if (g_hwnd12_b) {
        ::DestroyWindow(g_hwnd12_b);
        g_hwnd12_b = nullptr;
    }
}

// ---------------------------------------------------------------------------
// OpenGL：钩子挂在 opengl32!wglSwapBuffers。
//
// 这里刻意把交换方式做成开关，因为 Windows 上「调用 SwapBuffers」和「调用
// wglSwapBuffers」是两条入口：SwapBuffers 在 gdi32、wglSwapBuffers 在 opengl32。
// 真实 GL 程序（GLFW/SDL/游戏引擎）走的是 gdi32!SwapBuffers，所以默认走 gdi；
// 如果实测发现钩子只能截到直接调 wglSwapBuffers 的程序，那就是产品侧的缺口，
// 测试要用能证明钩子生效的那条路，并在 doc 里记下来。
//
// 画面：整屏清成轮换色，再用 glScissor 把**屏幕顶部**一条带清成白色。
// GL 的窗口坐标原点在左下，所以顶部带对应 y ∈ [height-band, height)；
// 而 glReadPixels 读出来的第一行是屏幕底部。产品侧传统 GL 路径要逐行翻转，
// 于是"白色带最终落在图像顶部"同时证明了新鲜度和上下方向。
// ---------------------------------------------------------------------------

HDC g_gl_dc = nullptr;
HGLRC g_gl_rc = nullptr;

// --gl-extra-swap：第二个 GL 窗口 + 独立线程 SwapBuffers。
// 用来覆盖"一个进程里两个 GL 窗口"：共享段是按被绑定窗口命名的，另一个窗口的
// SwapBuffers 也进钩子，若不按窗口过滤就会把两边的帧混进同一个段（§72.23）。
bool g_gl_extra_swap = false;
bool InitOpenGLExtra();
void ReleaseOpenGLExtra();

bool InitOpenGL(HWND hwnd) {
    g_gl_dc = ::GetDC(hwnd);
    if (!g_gl_dc) {
        std::printf("HOST_FAIL GetDC err=%lu\n", ::GetLastError());
        return false;
    }

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    const int format = ::ChoosePixelFormat(g_gl_dc, &pfd);
    if (format == 0) {
        std::printf("HOST_FAIL ChoosePixelFormat err=%lu\n", ::GetLastError());
        return false;
    }
    if (!::SetPixelFormat(g_gl_dc, format, &pfd)) {
        std::printf("HOST_FAIL SetPixelFormat err=%lu\n", ::GetLastError());
        return false;
    }
    g_gl_rc = ::wglCreateContext(g_gl_dc);
    if (!g_gl_rc) {
        std::printf("HOST_FAIL wglCreateContext err=%lu\n", ::GetLastError());
        return false;
    }
    if (!::wglMakeCurrent(g_gl_dc, g_gl_rc)) {
        std::printf("HOST_FAIL wglMakeCurrent err=%lu\n", ::GetLastError());
        return false;
    }
    ::glViewport(0, 0, g_width, g_height);
    ::glDisable(GL_DEPTH_TEST);
    if (g_gl_extra_swap) {
        InitOpenGLExtra();
    }
    return true;
}

void DrawOpenGL(int index) {
    ::glViewport(0, 0, g_width, g_height);
    ::glDisable(GL_SCISSOR_TEST);
    ::glClearColor(kColors[index][0], kColors[index][1], kColors[index][2], 1.0f);
    ::glClear(GL_COLOR_BUFFER_BIT);
    if (g_marker) {
        ::glEnable(GL_SCISSOR_TEST);
        ::glScissor(0, g_height - g_marker_rows, g_width, g_marker_rows);
        ::glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        ::glClear(GL_COLOR_BUFFER_BIT);
        ::glDisable(GL_SCISSOR_TEST);
    }
    if (g_gl_entry == 1) {
        // 合法但不画任何三角形的 glBegin/glEnd：只为了让挂在 glBegin 上的钩子被调用。
        ::glBegin(GL_TRIANGLES);
        ::glEnd();
    } else if (g_gl_entry == 2) {
        ::glFinish();
    }
    if (g_gl_swap_wgl) {
        ::wglSwapBuffers(g_gl_dc);
    } else {
        ::SwapBuffers(g_gl_dc);
    }
}

void ReleaseOpenGL() {
    if (g_gl_rc) {
        ::wglMakeCurrent(nullptr, nullptr);
        ::wglDeleteContext(g_gl_rc);
        g_gl_rc = nullptr;
    }
    if (g_gl_dc) {
        // DC 由窗口持有，只解除占用，不 ReleaseDC（窗口销毁时会一起回收）。
        g_gl_dc = nullptr;
    }
}

// ---------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

// --gl-extra-swap：第二个 GL 窗口，独立线程里清屏 + SwapBuffers
// 这条窗口**故意不画标记带**：宿主一旦读到它，"顶部是白色标记带"的断言立刻红。
// ---------------------------------------------------------------------------

const wchar_t *kGlExtraClassName = L"OpGfxHostWindow_gl_b";
HWND g_gl_extra_hwnd = nullptr;
HDC g_gl_extra_dc = nullptr;
HGLRC g_gl_extra_rc = nullptr;
HANDLE g_gl_extra_thread = nullptr;
volatile LONG g_gl_extra_stop = 0;
int g_gl_extra_color_index = 0;

DWORD WINAPI GlExtraSwapThread(LPVOID) {
    if (!::wglMakeCurrent(g_gl_extra_dc, g_gl_extra_rc)) {
        std::printf("HOST_NOTE wglMakeCurrent(extra) err=%lu\n", ::GetLastError());
        return 0;
    }
    ULONGLONG last_switch = ::GetTickCount64();
    while (::InterlockedCompareExchange(&g_gl_extra_stop, 0, 0) == 0) {
        if (::GetTickCount64() - last_switch >= kColorPeriodMs) {
            g_gl_extra_color_index = (g_gl_extra_color_index + 1) % 3;
            last_switch = ::GetTickCount64();
        }
        ::glViewport(0, 0, g_width, g_height);
        ::glDisable(GL_SCISSOR_TEST);
        ::glClearColor(kColors[g_gl_extra_color_index][0], kColors[g_gl_extra_color_index][1],
                       kColors[g_gl_extra_color_index][2], 1.0f);
        ::glClear(GL_COLOR_BUFFER_BIT);
        // 和主窗口一样按 --gl-swap 决定要不要额外走 glBegin/glFinish：
        // 这样"第二个窗口也在 glBegin/glFinish 入口上触发钩子"的场景才成立，
        // 才能验证按窗口过滤在 std/fi 两条入口上也生效。
        if (g_gl_entry == 1) {
            ::glBegin(GL_TRIANGLES);
            ::glEnd();
        } else if (g_gl_entry == 2) {
            ::glFinish();
        }
        ::SwapBuffers(g_gl_extra_dc);
    }
    ::wglMakeCurrent(nullptr, nullptr);
    return 0;
}

bool InitOpenGLExtra() {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = ::GetModuleHandleW(nullptr);
    wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kGlExtraClassName;
    if (!::RegisterClassExW(&wc) && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        std::printf("HOST_NOTE RegisterClassExW(gl extra) err=%lu\n", ::GetLastError());
        return false;
    }
    RECT rc = {0, 0, g_width, g_height};
    ::AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    // 不抢焦点（WS_EX_NOACTIVATE + SW_SHOWNOACTIVATE）：这条窗口只是为了"进程里多一条 GL 窗口"，
    // 抢到前台会把同批其它用例的输入/截图目标顶掉。
    g_gl_extra_hwnd = ::CreateWindowExW(WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW, kGlExtraClassName,
                                        L"OP OPENGL HOST (extra window)", WS_POPUP, 0, 0, 320, 240, nullptr,
                                        nullptr, wc.hInstance, nullptr);
    if (!g_gl_extra_hwnd) {
        std::printf("HOST_NOTE CreateWindowExW(gl extra) err=%lu\n", ::GetLastError());
        return false;
    }
    ::ShowWindow(g_gl_extra_hwnd, SW_SHOWNOACTIVATE);
    ::UpdateWindow(g_gl_extra_hwnd);

    g_gl_extra_dc = ::GetDC(g_gl_extra_hwnd);
    if (!g_gl_extra_dc) {
        std::printf("HOST_NOTE GetDC(gl extra) err=%lu\n", ::GetLastError());
        return false;
    }
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;
    const int format = ::ChoosePixelFormat(g_gl_extra_dc, &pfd);
    if (format == 0 || !::SetPixelFormat(g_gl_extra_dc, format, &pfd)) {
        std::printf("HOST_NOTE SetPixelFormat(gl extra) err=%lu\n", ::GetLastError());
        return false;
    }
    g_gl_extra_rc = ::wglCreateContext(g_gl_extra_dc);
    if (!g_gl_extra_rc) {
        std::printf("HOST_NOTE wglCreateContext(gl extra) err=%lu\n", ::GetLastError());
        return false;
    }
    g_gl_extra_thread = ::CreateThread(nullptr, 0, GlExtraSwapThread, nullptr, 0, nullptr);
    if (!g_gl_extra_thread) {
        std::printf("HOST_NOTE CreateThread(gl extra) err=%lu\n", ::GetLastError());
        return false;
    }
    return true;
}

void ReleaseOpenGLExtra() {
    ::InterlockedExchange(&g_gl_extra_stop, 1);
    if (g_gl_extra_thread) {
        ::WaitForSingleObject(g_gl_extra_thread, 3000);
        ::CloseHandle(g_gl_extra_thread);
        g_gl_extra_thread = nullptr;
    }
    if (g_gl_extra_rc) {
        // 线程退出前已经 wglMakeCurrent(nullptr)，这里删是安全的。
        ::wglDeleteContext(g_gl_extra_rc);
        g_gl_extra_rc = nullptr;
    }
    if (g_gl_extra_hwnd) {
        ::DestroyWindow(g_gl_extra_hwnd);
        g_gl_extra_hwnd = nullptr;
    }
    g_gl_extra_dc = nullptr;
}

// ---------------------------------------------------------------------------

bool InitApi(HWND hwnd) {
    switch (g_api) {
    case API_D3D9:
        return InitD3D9(hwnd);
    case API_D3D10:
        return InitD3D10(hwnd);
    case API_D3D12:
        return InitD3D12(hwnd);
    case API_OPENGL:
        return InitOpenGL(hwnd);
    }
    return false;
}

void DrawFrame() {
    switch (g_api) {
    case API_D3D9:
        DrawD3D9(g_color_index);
        break;
    case API_D3D10:
        DrawD3D10(g_color_index);
        break;
    case API_D3D12:
        DrawD3D12(g_color_index);
        break;
    case API_OPENGL:
        DrawOpenGL(g_color_index);
        break;
    }
}

void ReleaseApi() {
    switch (g_api) {
    case API_D3D9:
        ReleaseD3D9();
        break;
    case API_D3D10:
        ReleaseD3D10();
        break;
    case API_D3D12:
        ReleaseD3D12();
        break;
    case API_OPENGL:
        ReleaseOpenGLExtra();
        ReleaseOpenGL();
        break;
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_ERASEBKGND:
        // GL 自己负责画，别让 GDI 先刷一遍背景（否则会闪，也会让 FRONT 缓冲内容不稳定）。
        return 1;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return ::DefWindowProcW(hwnd, msg, wp, lp);
}

bool ParseApi(const std::wstring &name) {
    if (name == L"d3d9") {
        g_api = API_D3D9;
        g_title = L"OP D3D9 HOST";
        return true;
    }
    if (name == L"d3d10") {
        g_api = API_D3D10;
        g_title = L"OP D3D10 HOST";
        return true;
    }
    if (name == L"d3d12") {
        g_api = API_D3D12;
        g_title = L"OP D3D12 HOST";
        return true;
    }
    if (name == L"opengl") {
        g_api = API_OPENGL;
        g_title = L"OP OPENGL HOST";
        return true;
    }
    return false;
}

} // namespace

int wmain(int argc, wchar_t **argv) {
    int seconds = 60;
    std::wstring api_name;
    bool marker_explicit = false;

    for (int i = 1; i < argc; ++i) {
        const wchar_t *arg = argv[i];
        if (::wcsncmp(arg, L"--api=", 6) == 0) {
            api_name = arg + 6;
        } else if (::wcsncmp(arg, L"--seconds=", 10) == 0) {
            seconds = _wtoi(arg + 10);
        } else if (::wcscmp(arg, L"--static") == 0) {
            g_static = true;
        } else if (::wcsncmp(arg, L"--width=", 8) == 0) {
            g_width = _wtoi(arg + 8);
        } else if (::wcsncmp(arg, L"--height=", 9) == 0) {
            g_height = _wtoi(arg + 9);
        } else if (::wcsncmp(arg, L"--gl-swap=", 10) == 0) {
            g_gl_swap_wgl = (::wcscmp(arg + 10, L"wgl") == 0);
            if (::wcscmp(arg + 10, L"begin") == 0) {
                g_gl_entry = 1;
            } else if (::wcscmp(arg + 10, L"finish") == 0) {
                g_gl_entry = 2;
            }
        } else if (::wcscmp(arg, L"--no-marker") == 0) {
            g_marker = false;
            marker_explicit = true;
        } else if (::wcscmp(arg, L"--d3d12-extra-swap") == 0) {
            g_d3d12_extra_swap = true;
        } else if (::wcscmp(arg, L"--gl-extra-swap") == 0 || ::wcscmp(arg, L"--extra-swap") == 0) {
            g_gl_extra_swap = true;
        }
    }

    if (api_name.empty()) {
        std::printf("HOST_FAIL missing --api=d3d9|d3d10|d3d12|opengl\n");
        return 2;
    }
    if (!ParseApi(api_name)) {
        std::printf("HOST_FAIL unknown --api\n");
        return 2;
    }
    if (g_width < 64) {
        g_width = 64;
    }
    if (g_height < 64) {
        g_height = 64;
    }
    // 本程序的四个后端现在都能画标记带：d3d9 走 Clear 的矩形参数，d3d10/d3d12
    // 走一条白色纹理 CopySubresourceRegion，opengl 走 glScissor。所以默认全开，
    // 只有显式 --no-marker 才关（少数只想看纯色的手工调试场景）。
    if (!marker_explicit) {
        g_marker = true;
    }
    g_marker_rows = g_height / 4;

    // 类名带上 api，测试按类名找窗口，同时避免多个 host 程序的窗口互相干扰。
    g_class_name = std::wstring(L"OpGfxHostWindow_") + api_name;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = ::GetModuleHandleW(nullptr);
    wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = g_class_name.c_str();
    if (!::RegisterClassExW(&wc)) {
        std::printf("HOST_FAIL RegisterClassExW err=%lu\n", ::GetLastError());
        return 2;
    }

    RECT rc = {0, 0, g_width, g_height};
    ::AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hwnd = ::CreateWindowExW(0, g_class_name.c_str(), g_title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                  CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, wc.hInstance,
                                  nullptr);
    if (!hwnd) {
        std::printf("HOST_FAIL CreateWindowExW err=%lu\n", ::GetLastError());
        return 2;
    }

    if (!InitApi(hwnd)) {
        ::DestroyWindow(hwnd);
        return 3;
    }

    ::ShowWindow(hwnd, SW_SHOWNORMAL);
    ::UpdateWindow(hwnd);

    // 首帧先画出来，再报就绪，测试拿到 hwnd 时共享段里已经有内容。
    DrawFrame();
    std::printf("HOST_READY hwnd=0x%llX pid=%lu api=%ls\n", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(hwnd)),
                ::GetCurrentProcessId(), api_name.c_str());
    std::fflush(stdout);

    const ULONGLONG deadline = ::GetTickCount64() + static_cast<ULONGLONG>(seconds) * 1000ULL;
    ULONGLONG last_switch = ::GetTickCount64();
    bool running = true;
    int frame_count = 0;
    while (running) {
        MSG msg;
        while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
        if (!running) {
            break;
        }
        if (::GetTickCount64() >= deadline) {
            break;
        }

        if (!g_static && ::GetTickCount64() - last_switch >= kColorPeriodMs) {
            g_color_index = (g_color_index + 1) % 3;
            ++g_color_switches;
            last_switch = ::GetTickCount64();
        }

        DrawFrame();
        if (++frame_count % 20 == 0) {
            std::printf("HOST_FRAMES n=%d\n", frame_count);
            std::fflush(stdout);
        }
    }

    ReleaseApi();
    ::DestroyWindow(hwnd);
    return 0;
}
