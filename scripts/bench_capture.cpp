// 截图管线微基准：对比 GDI 截图的两条实现，并逐字节校验结果一致。
//
// 管线 A（改动前，libop/capture/backends/GdiCapture.cpp 旧实现）
//   CreateCompatibleBitmap -> SelectObject -> BitBlt -> GetDIBits(格式转换+整块拷贝)
//   -> 逐行倒置 memcpy(自底向上 DIB) -> DeleteObject
// 管线 B（改动后）
//   复用 top-down DIB(32bpp) -> BitBlt 直接写该 DIB -> 一次 memcpy -> Image
//
// 编译并运行（VS2022 环境）：
//   cl /nologo /O2 /std:c++17 /EHsc /utf-8 /Fe:bench_capture.exe scripts\bench_capture.cpp user32.lib gdi32.lib
//   bench_capture.exe [迭代次数]
//
// 说明：这里用窗口 DC(GetDC(hwnd)) 做源，等价于 GdiCapture 的 gdi/PrintWindow 取像素路径，
// 与本仓库 normal 模式的差异仅是源坐标，两条管线的相对成本结构一致。

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

namespace {

constexpr int kClientWidth = 1600;
constexpr int kClientHeight = 900;

struct Sink {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels;

    void create(int w, int h) {
        const size_t bytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 4;
        if (pixels.size() != bytes) {
            pixels.resize(bytes);
        }
        width = w;
        height = h;
    }
};

// 内容固定的测试窗口，保证两条管线拿到的是同一帧画面。
void paint_pattern(HDC dc, int width, int height) {
    for (int y = 0; y < height; ++y) {
        RECT row = {0, y, width, y + 1};
        const COLORREF color = RGB((y * 3) & 0xFF, (y * 7 + 32) & 0xFF, (y * 11 + 64) & 0xFF);
        HBRUSH brush = CreateSolidBrush(color);
        FillRect(dc, &row, brush);
        DeleteObject(brush);
    }
    for (int i = 0; i < 24; ++i) {
        RECT block = {40 + i * 61, 60 + (i % 5) * 120, 40 + i * 61 + 48, 60 + (i % 5) * 120 + 48};
        HBRUSH brush = CreateSolidBrush(RGB(i * 10, 255 - i * 8, (i * 23) & 0xFF));
        FillRect(dc, &block, brush);
        DeleteObject(brush);
    }
}

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps = {};
        HDC dc = BeginPaint(hwnd, &ps);
        RECT client = {0};
        GetClientRect(hwnd, &client);
        paint_pattern(dc, client.right - client.left, client.bottom - client.top);
        EndPaint(hwnd, &ps);
        return 0;
    }
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void pump_messages() {
    MSG msg = {};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

// ---------- 管线 A：改动前的实现 ----------
void capture_legacy(HDC src_dc, HDC mem_dc, int src_x, int src_y, int w, int h, std::vector<unsigned char> &scratch,
                    Sink &out) {
    HBITMAP screen = CreateCompatibleBitmap(src_dc, w, h);
    if (!screen) {
        return;
    }
    HGDIOBJ old = SelectObject(mem_dc, screen);
    BitBlt(mem_dc, 0, 0, w, h, src_dc, src_x, src_y, SRCCOPY);

    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;
    bih.biHeight = h; // 正数：自底向上
    bih.biPlanes = 1;
    bih.biSizeImage = w * h * 4;
    bih.biWidth = w;

    scratch.resize(static_cast<size_t>(w) * h * 4);
    GetDIBits(mem_dc, screen, 0, static_cast<DWORD>(h), scratch.data(), reinterpret_cast<BITMAPINFO *>(&bih),
              DIB_RGB_COLORS);

    out.create(w, h);
    for (int i = 0; i < h; ++i) {
        memcpy(out.pixels.data() + static_cast<size_t>(i) * w * 4,
               scratch.data() + static_cast<size_t>(h - 1 - i) * w * 4, static_cast<size_t>(w) * 4);
    }

    if (old) {
        SelectObject(mem_dc, old);
    }
    DeleteObject(screen);
}

// ---------- 管线 B：改动后的实现 ----------
struct DibTarget {
    HBITMAP dib = nullptr;
    HGDIOBJ old = nullptr;
    void *bits = nullptr;
    int width = 0;
    int height = 0;

    bool ensure(HDC dc, HDC mem_dc, int w, int h) {
        if (dib && width >= w && height >= h) {
            return true;
        }
        release(mem_dc);
        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = w;
        bi.bmiHeader.biHeight = -h; // 负数：top-down，行序与 Image 一致
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        void *pixels = nullptr;
        dib = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &pixels, nullptr, 0);
        if (!dib || !pixels) {
            return false;
        }
        bits = pixels;
        width = w;
        height = h;
        old = SelectObject(mem_dc, dib);
        return old != nullptr;
    }

    void release(HDC mem_dc) {
        if (mem_dc && dib && old) {
            SelectObject(mem_dc, old);
        }
        old = nullptr;
        if (dib) {
            DeleteObject(dib);
            dib = nullptr;
        }
        bits = nullptr;
        width = 0;
        height = 0;
    }
};

void capture_fast(HDC src_dc, HDC mem_dc, int src_x, int src_y, int w, int h, DibTarget &target, Sink &out) {
    if (!target.ensure(src_dc, mem_dc, w, h)) {
        return;
    }
    BitBlt(mem_dc, 0, 0, w, h, src_dc, src_x, src_y, SRCCOPY);

    out.create(w, h);
    const auto *base = static_cast<const unsigned char *>(target.bits);
    const size_t pitch = static_cast<size_t>(target.width) * 4;
    const size_t row_bytes = static_cast<size_t>(w) * 4;
    if (pitch == row_bytes) {
        memcpy(out.pixels.data(), base, row_bytes * static_cast<size_t>(h));
    } else {
        for (int i = 0; i < h; ++i) {
            memcpy(out.pixels.data() + static_cast<size_t>(i) * row_bytes, base + static_cast<size_t>(i) * pitch,
                   row_bytes);
        }
    }
}

// 只报 avg 会被偶发尖峰带偏（同一台机器整窗 avg 能差 30%），
// 所以同时给 median / p95 / stdev，验收用 min 与 median 不退化，不用绝对倍数。
struct Stats {
    double avg_ms = 0.0;
    double min_ms = 0.0;
    double median_ms = 0.0;
    double p95_ms = 0.0;
    double stdev_ms = 0.0;
};

Stats run_benchmark(int iterations, const std::function<void()> &step) {
    // 预热一轮，避免首帧的分支/分配影响统计。
    step();
    std::vector<double> samples;
    samples.reserve(static_cast<size_t>(iterations));
    for (int i = 0; i < iterations; ++i) {
        const auto begin = std::chrono::steady_clock::now();
        step();
        const auto end = std::chrono::steady_clock::now();
        samples.push_back(std::chrono::duration<double, std::milli>(end - begin).count());
    }
    double sum = 0.0;
    for (double v : samples) {
        sum += v;
    }
    Stats stats;
    stats.avg_ms = sum / static_cast<double>(samples.size());
    stats.min_ms = *std::min_element(samples.begin(), samples.end());

    std::sort(samples.begin(), samples.end());
    const size_t n = samples.size();
    stats.median_ms = samples[n / 2];
    const size_t p95_index = static_cast<size_t>(static_cast<double>(n - 1) * 0.95);
    stats.p95_ms = samples[p95_index];

    double sq = 0.0;
    for (double v : samples) {
        sq += (v - stats.avg_ms) * (v - stats.avg_ms);
    }
    stats.stdev_ms = std::sqrt(sq / static_cast<double>(n));
    return stats;
}

void print_stats(const char *label, const Stats &s) {
    std::printf("  %-6s : avg %.3f  median %.3f  p95 %.3f  min %.3f  stdev %.3f ms\n", label, s.avg_ms, s.median_ms,
                s.p95_ms, s.min_ms, s.stdev_ms);
}

bool report_case(const char *name, int iterations, HDC src_dc, int src_x, int src_y, int w, int h) {
    HDC mem_legacy = CreateCompatibleDC(src_dc);
    HDC mem_fast = CreateCompatibleDC(src_dc);
    if (!mem_legacy || !mem_fast) {
        std::printf("CreateCompatibleDC failed\n");
        return false;
    }

    std::vector<unsigned char> scratch;
    Sink legacy_out;
    Sink fast_out;
    DibTarget target;

    Stats legacy = run_benchmark(
        iterations, [&]() { capture_legacy(src_dc, mem_legacy, src_x, src_y, w, h, scratch, legacy_out); });
    Stats fast =
        run_benchmark(iterations, [&]() { capture_fast(src_dc, mem_fast, src_x, src_y, w, h, target, fast_out); });

    pump_messages();

    const bool size_ok = legacy_out.width == fast_out.width && legacy_out.height == fast_out.height;
    const bool pixels_ok =
        size_ok && memcmp(legacy_out.pixels.data(), fast_out.pixels.data(), legacy_out.pixels.size()) == 0;

    std::printf("[%s] %dx%d\n", name, w, h);
    print_stats("legacy", legacy);
    print_stats("fast", fast);
    const double min_ratio = legacy.min_ms / (fast.min_ms > 0.0 ? fast.min_ms : 1e-9);
    const double median_ratio = legacy.median_ms / (fast.median_ms > 0.0 ? fast.median_ms : 1e-9);
    std::printf("  speedup: avg %.2fx  median %.2fx  min %.2fx\n",
                legacy.avg_ms / (fast.avg_ms > 0.0 ? fast.avg_ms : 1e-9), median_ratio, min_ratio);
    std::printf("  pixels identical: %s\n\n", pixels_ok ? "YES" : "NO");

    target.release(mem_fast);
    DeleteDC(mem_legacy);
    DeleteDC(mem_fast);
    return pixels_ok;
}

// 打印能解释"同一份代码在不同机器上倍数不同"的环境元数据。
// GDI 的 BitBlt/CreateCompatibleBitmap 开销对显示适配器非常敏感：
// 虚拟显示适配器（远程桌面、串流、虚拟机、投屏驱动）上倍数会明显低于物理显卡。
void print_environment() {
    SYSTEM_INFO si = {};
    GetNativeSystemInfo(&si);
    std::printf("env: %s, %u logical cpus\n",
                si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "x64" : "x86", si.dwNumberOfProcessors);

    std::printf("display adapters:\n");
    DISPLAY_DEVICEW dd = {};
    dd.cb = sizeof(dd);
    for (DWORD i = 0; EnumDisplayDevicesW(nullptr, i, &dd, 0); ++i) {
        if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) {
            // 名字里带 Mirror/Idd/Indirect/Virtual 的基本都是虚拟显示适配器。
            std::wprintf(L"  [%lu] %s\n", i, dd.DeviceString);
        }
        dd = {};
        dd.cb = sizeof(dd);
    }
    std::printf("\n");
}

} // namespace

int main(int argc, char **argv) {
    int iterations = 300;
    if (argc > 1) {
        // windows.h 把 max 定义成宏，加括号抑制宏展开。
        iterations = (std::max)(1, std::atoi(argv[1]));
    }
    print_environment();

    const HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"op_capture_bench";
    if (!RegisterClassExW(&wc)) {
        std::printf("RegisterClassExW failed: %lu\n", GetLastError());
        return 1;
    }

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"op capture bench", WS_OVERLAPPEDWINDOW, 40, 40,
                                kClientWidth + 16, kClientHeight + 39, nullptr, nullptr, instance, nullptr);
    if (!hwnd) {
        std::printf("CreateWindowExW failed: %lu\n", GetLastError());
        return 1;
    }
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    pump_messages();

    RECT client = {0};
    GetClientRect(hwnd, &client);
    POINT origin = {0};
    ClientToScreen(hwnd, &origin);
    const int client_w = client.right - client.left;
    const int client_h = client.bottom - client.top;
    std::printf("client %dx%d\n\n", client_w, client_h);
    HDC window_dc = GetDC(hwnd);
    if (!window_dc) {
        std::printf("GetDC(hwnd) failed: %lu\n", GetLastError());
        return 1;
    }

    bool ok = true;
    // 整窗截图：最常见的整屏/整窗抓取。
    ok = report_case("full client", iterations, window_dc, 0, 0, client_w, client_h) && ok;
    // 局部截图：FindPic 指定区域时的典型调用。
    ok = report_case("region 320x200", iterations, window_dc, 120, 90, 320, 200) && ok;
    // GetColor/CmpColor 的 10x10 小块：验证 DIB 容量复用后不再反复建位图。
    ok = report_case("block 10x10", iterations, window_dc, 300, 300, 10, 10) && ok;

    ReleaseDC(hwnd, window_dc);
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, instance);

    std::printf(ok ? "RESULT: pixel-exact\n" : "RESULT: pixel mismatch\n");
    return ok ? 0 : 2;
}
