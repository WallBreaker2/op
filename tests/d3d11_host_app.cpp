// 端到端验证用的「真实 D3D11 目标进程」。
//
// 为什么需要它：dx.d3d11 / dx.dxgi 这类后台截图把 op_x64.dll 注入到**别的进程**里，
// 在目标进程的 Present 里干活，把整帧写进命名共享段，再由本进程的 HookCapture 读出来。
// 这条链路只有在"两个进程 + 真实交换链"下才成立：
//   - 只有跨进程才能验证共享段映射是两边各自 open 的（缓存映射后才有可能读到陈旧数据）；
//   - 只有真交换链才会真正走 Present 钩子。
// 所以这里做个最小 D3D11 窗口程序，按固定节奏换清屏色，让测试能判断"读到的是新帧"。
//
// 用法：d3d11_host_app.exe [--seconds=N] [--static] [--width=W] [--height=H] [--extra-swap]
//   --seconds    自动退出时间（默认 60 秒），测试结束会直接 TerminateProcess
//   --static     不换色（用于判断像素值稳定）
//   --extra-swap 再开一条交换链（第二个窗口），在**独立线程**里 Present。
//                真游戏/编辑器常见"一个进程多条交换链、各自在不同线程 Present"，
//                注入侧的单例抓取状态必须靠锁串行化才不会被两条链互相踩死
//                （见 doc/defect_fixes.md §72.21 的 d3d12 版本、§72.22 的 d3d11 验证）。
// 输出：首帧就绪后打印一行 "HOST_READY hwnd=0x... pid=..."，类名固定 OpD3D11HostWindow。

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <cstdio>
#include <cstring>

namespace {

const wchar_t *kClassName = L"OpD3D11HostWindow";
const wchar_t *kTitle = L"OP D3D11 HOST";

// 三种纯色轮换。测试读到其中任意一种都算对，读到两种以上说明帧在更新。
const float kColors[3][4] = {
    {0.0f, 1.0f, 0.0f, 1.0f}, // 绿
    {1.0f, 0.0f, 0.0f, 1.0f}, // 红
    {0.0f, 0.0f, 1.0f, 1.0f}, // 蓝
};
const ULONGLONG kColorPeriodMs = 500;

ID3D11Device *g_device = nullptr;
ID3D11DeviceContext *g_context = nullptr;
IDXGISwapChain *g_swap = nullptr;
ID3D11RenderTargetView *g_rtv = nullptr;

int g_width = 640;
int g_height = 360;
bool g_static = false;
int g_color_index = 0;
int g_color_switches = 0;

bool InitD3D(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = static_cast<UINT>(g_width);
    sd.BufferDesc.Height = static_cast<UINT>(g_height);
    sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL obtained = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = ::D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels,
                                                 _countof(levels), D3D11_SDK_VERSION, &sd, &g_swap, &g_device,
                                                 &obtained, &g_context);
    if (FAILED(hr)) {
        std::printf("HOST_FAIL D3D11CreateDeviceAndSwapChain hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }

    ID3D11Texture2D *backbuffer = nullptr;
    hr = g_swap->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&backbuffer));
    if (FAILED(hr) || !backbuffer) {
        std::printf("HOST_FAIL GetBuffer hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }
    hr = g_device->CreateRenderTargetView(backbuffer, nullptr, &g_rtv);
    backbuffer->Release();
    if (FAILED(hr)) {
        std::printf("HOST_FAIL CreateRenderTargetView hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }
    return true;
}

void ReleaseD3D() {
    if (g_rtv) {
        g_rtv->Release();
        g_rtv = nullptr;
    }
    if (g_swap) {
        g_swap->Release();
        g_swap = nullptr;
    }
    if (g_context) {
        g_context->Release();
        g_context = nullptr;
    }
    if (g_device) {
        g_device->Release();
        g_device = nullptr;
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_DESTROY) {
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hwnd, msg, wp, lp);
}

// ---------------------------------------------------------------------------
// 第二条交换链：同一个 ID3D11Device、另一个窗口、另一个线程 Present
// ---------------------------------------------------------------------------

const wchar_t *kExtraClassName = L"OpD3D11HostWindowExtra";

HWND g_extra_hwnd = nullptr;
IDXGISwapChain *g_extra_swap = nullptr;
ID3D11RenderTargetView *g_extra_rtv = nullptr;
HANDLE g_extra_thread = nullptr;
volatile LONG g_extra_stop = 0;
int g_extra_color_index = 0;

bool InitExtraSwap() {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = ::GetModuleHandleW(nullptr);
    wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kExtraClassName;
    if (!::RegisterClassExW(&wc) && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        std::printf("HOST_NOTE RegisterClassExW(extra) err=%lu\n", ::GetLastError());
        return false;
    }
    RECT rc = {0, 0, g_width, g_height};
    ::AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    // 同 gfx_host_app 的额外窗口：不抢焦点，避免把同批其它用例的前台目标顶掉。
    g_extra_hwnd = ::CreateWindowExW(WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW, kExtraClassName,
                                     L"OP D3D11 HOST (extra swap)", WS_POPUP, 0, 0, 320, 240, nullptr, nullptr,
                                     wc.hInstance, nullptr);
    if (!g_extra_hwnd) {
        std::printf("HOST_NOTE CreateWindowExW(extra) err=%lu\n", ::GetLastError());
        return false;
    }
    ::ShowWindow(g_extra_hwnd, SW_SHOWNOACTIVATE);
    ::UpdateWindow(g_extra_hwnd);

    // 复用主链的设备：交换链必须由"创建该设备的工厂"来建，
    // 所以从设备反查 adapter → factory，而不是另外 D3D11CreateDeviceAndSwapChain。
    IDXGIDevice *dxgi_device = nullptr;
    IDXGIAdapter *adapter = nullptr;
    IDXGIFactory *factory = nullptr;
    HRESULT hr = g_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void **>(&dxgi_device));
    if (SUCCEEDED(hr) && dxgi_device) {
        hr = dxgi_device->GetAdapter(&adapter);
    }
    if (SUCCEEDED(hr) && adapter) {
        hr = adapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void **>(&factory));
    }
    if (SUCCEEDED(hr) && factory) {
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = static_cast<UINT>(g_width);
        sd.BufferDesc.Height = static_cast<UINT>(g_height);
        sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = g_extra_hwnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        hr = factory->CreateSwapChain(g_device, &sd, &g_extra_swap);
    }
    if (dxgi_device) {
        dxgi_device->Release();
    }
    if (adapter) {
        adapter->Release();
    }
    if (factory) {
        factory->Release();
    }
    if (FAILED(hr) || !g_extra_swap) {
        std::printf("HOST_NOTE CreateSwapChain(extra) hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }

    ID3D11Texture2D *backbuffer = nullptr;
    hr = g_extra_swap->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&backbuffer));
    if (FAILED(hr) || !backbuffer) {
        std::printf("HOST_NOTE GetBuffer(extra) hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }
    hr = g_device->CreateRenderTargetView(backbuffer, nullptr, &g_extra_rtv);
    backbuffer->Release();
    if (FAILED(hr) || !g_extra_rtv) {
        std::printf("HOST_NOTE CreateRenderTargetView(extra) hr=0x%08X\n", static_cast<unsigned>(hr));
        return false;
    }
    return true;
}

// 独立线程：换色 + Clear + Present。immediate context 本身是线程安全的，
// 这里要的正是"两条交换链从两个线程同时进 Present"这个形状。
DWORD WINAPI ExtraSwapThread(LPVOID) {
    ULONGLONG last_switch = ::GetTickCount64();
    while (::InterlockedCompareExchange(&g_extra_stop, 0, 0) == 0) {
        if (!g_static && ::GetTickCount64() - last_switch >= kColorPeriodMs) {
            g_extra_color_index = (g_extra_color_index + 1) % 3;
            last_switch = ::GetTickCount64();
        }
        g_context->ClearRenderTargetView(g_extra_rtv, kColors[g_extra_color_index]);
        const HRESULT hr = g_extra_swap->Present(1, 0);
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            std::printf("HOST_NOTE device removed(extra) hr=0x%08X\n", static_cast<unsigned>(hr));
            break;
        }
    }
    return 0;
}

void ReleaseExtraSwap() {
    ::InterlockedExchange(&g_extra_stop, 1);
    if (g_extra_thread) {
        ::WaitForSingleObject(g_extra_thread, 5000);
        ::CloseHandle(g_extra_thread);
        g_extra_thread = nullptr;
    }
    if (g_extra_rtv) {
        g_extra_rtv->Release();
        g_extra_rtv = nullptr;
    }
    if (g_extra_swap) {
        g_extra_swap->Release();
        g_extra_swap = nullptr;
    }
    if (g_extra_hwnd) {
        ::DestroyWindow(g_extra_hwnd);
        g_extra_hwnd = nullptr;
    }
}

} // namespace

int wmain(int argc, wchar_t **argv) {
    int seconds = 60;
    bool extra_swap = false;
    for (int i = 1; i < argc; ++i) {
        if (::wcsncmp(argv[i], L"--seconds=", 10) == 0) {
            seconds = _wtoi(argv[i] + 10);
        } else if (::wcscmp(argv[i], L"--static") == 0) {
            g_static = true;
        } else if (::wcscmp(argv[i], L"--extra-swap") == 0) {
            extra_swap = true;
        } else if (::wcsncmp(argv[i], L"--width=", 8) == 0) {
            g_width = _wtoi(argv[i] + 8);
        } else if (::wcsncmp(argv[i], L"--height=", 9) == 0) {
            g_height = _wtoi(argv[i] + 9);
        }
    }
    if (g_width < 64) {
        g_width = 64;
    }
    if (g_height < 64) {
        g_height = 64;
    }

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = ::GetModuleHandleW(nullptr);
    wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kClassName;
    if (!::RegisterClassExW(&wc)) {
        std::printf("HOST_FAIL RegisterClassExW err=%lu\n", ::GetLastError());
        return 2;
    }

    RECT rc = {0, 0, g_width, g_height};
    ::AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hwnd = ::CreateWindowExW(0, kClassName, kTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                  rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, wc.hInstance, nullptr);
    if (!hwnd) {
        std::printf("HOST_FAIL CreateWindowExW err=%lu\n", ::GetLastError());
        return 2;
    }

    if (!InitD3D(hwnd)) {
        return 3;
    }

    ::ShowWindow(hwnd, SW_SHOWNORMAL);
    ::UpdateWindow(hwnd);

    // 首帧先画出来，再报就绪，测试拿到 hwnd 时共享段里已经有内容。
    g_context->ClearRenderTargetView(g_rtv, kColors[g_color_index]);
    g_swap->Present(1, 0);
    std::printf("HOST_READY hwnd=0x%llX pid=%lu\n", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(hwnd)),
                ::GetCurrentProcessId());
    std::fflush(stdout);

    // 第二条交换链：建失败只记 HOST_NOTE 继续跑主链，不把整个目标进程弄死
    // （测试里那条用例会因此读到单一颜色而变红，比静默通过安全）。
    if (extra_swap) {
        if (InitExtraSwap()) {
            g_extra_thread = ::CreateThread(nullptr, 0, ExtraSwapThread, nullptr, 0, nullptr);
            if (!g_extra_thread) {
                std::printf("HOST_NOTE CreateThread(extra) err=%lu\n", ::GetLastError());
            }
        }
    }

    const ULONGLONG deadline = ::GetTickCount64() + static_cast<ULONGLONG>(seconds) * 1000ULL;
    ULONGLONG last_switch = ::GetTickCount64();
    bool running = true;
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

        g_context->ClearRenderTargetView(g_rtv, kColors[g_color_index]);
        // Present 里就是注入的 dx11 钩子在干活，这里不能加锁也不能省。
        const HRESULT hr = g_swap->Present(1, 0);
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            std::printf("HOST_FAIL device removed hr=0x%08X\n", static_cast<unsigned>(hr));
            break;
        }
    }

    ReleaseExtraSwap();
    ReleaseD3D();
    ::DestroyWindow(hwnd);
    return 0;
}
