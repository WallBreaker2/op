// 最小 EGL/GLES2 目标程序：用来覆盖 `opengl.es`（产品侧把钩子挂在 `eglSwapBuffers` 上）。
//
// 为什么要单独一个程序：EGL 的 DLL 不在系统目录里，只有 Chromium 系应用自带 ANGLE。
// 本机装了夸克网盘，`%ProgramFiles%\QuarkCloudDrive\<版本>\` 里就有 libEGL.dll +
// libGLESv2.dll + d3dcompiler_47.dll，正好可以当被测目标。
//
// 用法：
//   egl_host_app.exe [--seconds=60] [--width=640] [--height=360] [--static]
//                    [--no-marker] [--egl-dir=<含 libEGL.dll 的目录>]
//
// 约定（与 d3d11_host_app / gfx_host_app 一致）：
//   * 窗口类名 OpEglHostWindow、标题 OP EGL HOST；
//   * 每 500ms 换一次清屏色（绿/红/蓝），用来证明"读到的是新帧"；
//   * 默认在画面顶部画一条白带（GL 坐标 y 向上，所以顶部是 y=height-marker_rows），
//     用来证明"上下没有颠倒"；
//   * 就绪时打 `HOST_READY hwnd=0x... pid=... api=opengl.es`，每 20 帧打一次 HOST_FRAMES。
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>
#include <cstring>
#include <string>

namespace {

const wchar_t *kClassName = L"OpEglHostWindow";
const wchar_t *kTitle = L"OP EGL HOST";

// 清屏色：绿 / 红 / 蓝（GL 是 0..1 的 float）。
// 和 gfx_host_app 用同一套颜色（红/青/品红），这样 e2e 用例可以共用 GfxHostColors()。
const float kColors[3][4] = {
    {1.0f, 0.0f, 0.0f, 1.0f}, // 红 FF0000
    {0.0f, 1.0f, 1.0f, 1.0f}, // 青 00FFFF
    {1.0f, 0.0f, 1.0f, 1.0f}, // 品红 FF00FF
};
const ULONGLONG kColorPeriodMs = 500;

// ---- 最小 EGL/GLES 类型与常量（本仓库不依赖 EGL 头，全部自己声明）----
typedef void *EGLDisplay;
typedef void *EGLConfig;
typedef void *EGLSurface;
typedef void *EGLContext;
typedef int32_t EGLint;
typedef uint32_t EGLBoolean;
typedef uint32_t EGLenum;

const EGLint EGL_NONE = 0x3038;
const EGLint EGL_SUCCESS = 0x3000;
const EGLint EGL_SURFACE_TYPE = 0x3033;
const EGLint EGL_WINDOW_BIT = 0x0004;
const EGLint EGL_RENDERABLE_TYPE = 0x3040;
const EGLint EGL_OPENGL_ES2_BIT = 0x0004;
const EGLint EGL_RED_SIZE = 0x3024;
const EGLint EGL_GREEN_SIZE = 0x3023;
const EGLint EGL_BLUE_SIZE = 0x3022;
const EGLint EGL_ALPHA_SIZE = 0x3021;
const EGLint EGL_CONTEXT_CLIENT_VERSION = 0x3098;
const EGLint EGL_PLATFORM_ANGLE_ANGLE = 0x3202;
const EGLint EGL_PLATFORM_ANGLE_TYPE_ANGLE = 0x3203;
const EGLint EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE = 0x3208;
const EGLint EGL_PLATFORM_ANGLE_TYPE_DEFAULT_ANGLE = 0x3204;

const unsigned int GL_COLOR_BUFFER_BIT = 0x00004000;
const unsigned int GL_SCISSOR_TEST = 0x00000C11;

// ---- EGL 入口（从 libEGL.dll 里按名字取）----
struct EglApi {
    EGLDisplay (*GetPlatformDisplayEXT)(EGLenum platform, void *native_display, const EGLint *attribs);
    EGLDisplay (*GetDisplay)(void *native_display);
    EGLBoolean (*Initialize)(EGLDisplay dpy, EGLint *major, EGLint *minor);
    EGLBoolean (*ChooseConfig)(EGLDisplay dpy, const EGLint *attribs, EGLConfig *configs, EGLint count,
                               EGLint *num_config);
    EGLSurface (*CreateWindowSurface)(EGLDisplay dpy, EGLConfig config, HWND win, const EGLint *attribs);
    EGLContext (*CreateContext)(EGLDisplay dpy, EGLConfig config, EGLContext share, const EGLint *attribs);
    EGLBoolean (*MakeCurrent)(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
    EGLBoolean (*SwapBuffers)(EGLDisplay dpy, EGLSurface surface);
    EGLint (*GetError)();
    EGLBoolean (*Terminate)(EGLDisplay dpy);
};

// ---- GLES2 入口（从 libGLESv2.dll 里按名字取）----
struct GlesApi {
    void (*Viewport)(int x, int y, int w, int h);
    void (*ClearColor)(float r, float g, float b, float a);
    void (*Clear)(unsigned int mask);
    void (*Enable)(unsigned int cap);
    void (*Disable)(unsigned int cap);
    void (*Scissor)(int x, int y, int w, int h);
};

EglApi g_egl = {};
GlesApi g_gl = {};

HWND g_hwnd = nullptr;
EGLDisplay g_display = nullptr;
EGLSurface g_surface = nullptr;
EGLContext g_context = nullptr;
HMODULE g_egl_module = nullptr;
HMODULE g_gles_module = nullptr;

int g_width = 640;
int g_height = 360;
bool g_static = false;
bool g_marker = true;
int g_marker_rows = 0;
int g_color_index = 0;

template <typename T>
bool ResolveFrom(HMODULE mod, const char *name, T &out) {
    out = reinterpret_cast<T>(::GetProcAddress(mod, name));
    return out != nullptr;
}

// 找 `%ProgramFiles%\QuarkCloudDrive\<版本最高>\`（里面有 libEGL.dll）。
std::wstring DiscoverQuarkAngleDir() {
    // 32 位进程看到的 ProgramFiles 会被重定向到 "Program Files (x86)"，而夸克网盘装在
    // 64 位 Program Files 下，所以先问 ProgramW6432（32 位进程也能读到这个变量）。
    wchar_t program_files[MAX_PATH] = {};
    DWORD len = ::GetEnvironmentVariableW(L"ProgramW6432", program_files, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        len = ::GetEnvironmentVariableW(L"ProgramFiles", program_files, MAX_PATH);
    }
    if (len == 0 || len >= MAX_PATH) {
        return std::wstring();
    }
    std::wstring pattern = std::wstring(program_files) + L"\\QuarkCloudDrive\\*";
    WIN32_FIND_DATAW fd = {};
    HANDLE find = ::FindFirstFileW(pattern.c_str(), &fd);
    if (find == INVALID_HANDLE_VALUE) {
        return std::wstring();
    }
    std::wstring best;
    do {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            continue;
        }
        if (fd.cFileName[0] == L'.') {
            continue;
        }
        std::wstring dir = std::wstring(program_files) + L"\\QuarkCloudDrive\\" + fd.cFileName;
        std::wstring dll = dir + L"\\libEGL.dll";
        if (::GetFileAttributesW(dll.c_str()) == INVALID_FILE_ATTRIBUTES) {
            continue;
        }
        // 版本号是纯数字加点，直接按字典序比就是版本序（7.4.0.818 > 7.3.5.810）。
        if (best.empty() || wcscmp(fd.cFileName, best.c_str() + best.find_last_of(L'\\') + 1) > 0) {
            best = dir;
        }
    } while (::FindNextFileW(find, &fd));
    ::FindClose(find);
    return best;
}

bool LoadAngle(const std::wstring &dir) {
    if (dir.empty()) {
        std::printf("HOST_FAIL no ANGLE directory (pass --egl-dir=<dir with libEGL.dll>)\n");
        return false;
    }
    std::wstring egl_path = dir + L"\\libEGL.dll";
    std::wstring gles_path = dir + L"\\libGLESv2.dll";
    // LOAD_WITH_ALTERED_SEARCH_PATH：让 ANGLE 自己去找同目录的 libGLESv2.dll /
    // d3dcompiler_47.dll，不用改进程的 DLL 搜索路径。
    g_egl_module = ::LoadLibraryExW(egl_path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!g_egl_module) {
        std::printf("HOST_FAIL LoadLibrary(libEGL.dll) err=%lu\n", ::GetLastError());
        return false;
    }
    g_gles_module = ::LoadLibraryExW(gles_path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!g_gles_module) {
        std::printf("HOST_FAIL LoadLibrary(libGLESv2.dll) err=%lu\n", ::GetLastError());
        return false;
    }

    bool ok = true;
    ok &= ResolveFrom(g_egl_module, "eglGetPlatformDisplayEXT", g_egl.GetPlatformDisplayEXT);
    ok &= ResolveFrom(g_egl_module, "eglGetDisplay", g_egl.GetDisplay);
    ok &= ResolveFrom(g_egl_module, "eglInitialize", g_egl.Initialize);
    ok &= ResolveFrom(g_egl_module, "eglChooseConfig", g_egl.ChooseConfig);
    ok &= ResolveFrom(g_egl_module, "eglCreateWindowSurface", g_egl.CreateWindowSurface);
    ok &= ResolveFrom(g_egl_module, "eglCreateContext", g_egl.CreateContext);
    ok &= ResolveFrom(g_egl_module, "eglMakeCurrent", g_egl.MakeCurrent);
    ok &= ResolveFrom(g_egl_module, "eglSwapBuffers", g_egl.SwapBuffers);
    ok &= ResolveFrom(g_egl_module, "eglGetError", g_egl.GetError);
    ok &= ResolveFrom(g_egl_module, "eglTerminate", g_egl.Terminate);

    ok &= ResolveFrom(g_gles_module, "glViewport", g_gl.Viewport);
    ok &= ResolveFrom(g_gles_module, "glClearColor", g_gl.ClearColor);
    ok &= ResolveFrom(g_gles_module, "glClear", g_gl.Clear);
    ok &= ResolveFrom(g_gles_module, "glEnable", g_gl.Enable);
    ok &= ResolveFrom(g_gles_module, "glDisable", g_gl.Disable);
    ok &= ResolveFrom(g_gles_module, "glScissor", g_gl.Scissor);
    if (!ok) {
        std::printf("HOST_FAIL ANGLE exports missing\n");
        return false;
    }
    return true;
}

bool InitEgl(HWND hwnd) {
    // 优先走 ANGLE 的平台扩展（明确要 D3D11 后端）；拿不到就退回默认显示。
    const EGLint platform_attribs[] = {
        EGL_PLATFORM_ANGLE_TYPE_ANGLE,
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        EGL_NONE,
    };
    if (g_egl.GetPlatformDisplayEXT) {
        g_display = g_egl.GetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, nullptr, platform_attribs);
    }
    if (!g_display) {
        g_display = g_egl.GetDisplay(nullptr);
    }
    if (!g_display) {
        std::printf("HOST_FAIL eglGetDisplay failed err=0x%04X\n", g_egl.GetError ? g_egl.GetError() : 0u);
        return false;
    }
    EGLint major = 0;
    EGLint minor = 0;
    if (!g_egl.Initialize(g_display, &major, &minor)) {
        std::printf("HOST_FAIL eglInitialize err=0x%04X\n", g_egl.GetError());
        return false;
    }
    const EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_RED_SIZE,
        8,                EGL_GREEN_SIZE, 8,                 EGL_BLUE_SIZE,  8,           EGL_ALPHA_SIZE,
        8,                EGL_NONE,
    };
    EGLConfig config = nullptr;
    EGLint num_config = 0;
    if (!g_egl.ChooseConfig(g_display, config_attribs, &config, 1, &num_config) || num_config < 1) {
        std::printf("HOST_FAIL eglChooseConfig err=0x%04X n=%d\n", g_egl.GetError(), (int)num_config);
        return false;
    }
    g_surface = g_egl.CreateWindowSurface(g_display, config, hwnd, nullptr);
    if (!g_surface) {
        std::printf("HOST_FAIL eglCreateWindowSurface err=0x%04X\n", g_egl.GetError());
        return false;
    }
    const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    g_context = g_egl.CreateContext(g_display, config, nullptr, context_attribs);
    if (!g_context) {
        std::printf("HOST_FAIL eglCreateContext err=0x%04X\n", g_egl.GetError());
        return false;
    }
    if (!g_egl.MakeCurrent(g_display, g_surface, g_surface, g_context)) {
        std::printf("HOST_FAIL eglMakeCurrent err=0x%04X\n", g_egl.GetError());
        return false;
    }
    return true;
}

void ReleaseEgl() {
    if (g_egl.MakeCurrent && g_display) {
        g_egl.MakeCurrent(g_display, nullptr, nullptr, nullptr);
    }
    if (g_egl.Terminate && g_display) {
        g_egl.Terminate(g_display);
    }
    g_context = nullptr;
    g_surface = nullptr;
    g_display = nullptr;
    if (g_gles_module) {
        ::FreeLibrary(g_gles_module);
        g_gles_module = nullptr;
    }
    if (g_egl_module) {
        ::FreeLibrary(g_egl_module);
        g_egl_module = nullptr;
    }
}

void DrawFrame() {
    g_gl.Viewport(0, 0, g_width, g_height);
    const float *color = kColors[g_color_index];
    g_gl.ClearColor(color[0], color[1], color[2], color[3]);
    g_gl.Clear(GL_COLOR_BUFFER_BIT);
    if (g_marker && g_marker_rows > 0) {
        // GL 坐标原点在左下角：y=height-marker_rows 起的这一段就是画面**顶部**。
        g_gl.Enable(GL_SCISSOR_TEST);
        g_gl.Scissor(0, g_height - g_marker_rows, g_width, g_marker_rows);
        g_gl.ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        g_gl.Clear(GL_COLOR_BUFFER_BIT);
        g_gl.Disable(GL_SCISSOR_TEST);
    }
    g_egl.SwapBuffers(g_display, g_surface);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_DESTROY) {
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hwnd, msg, wparam, lparam);
}

int ParseIntArg(const wchar_t *arg, const wchar_t *prefix, int fallback) {
    const size_t len = wcslen(prefix);
    if (::wcsncmp(arg, prefix, len) != 0) {
        return fallback;
    }
    return _wtoi(arg + len);
}

} // namespace

int wmain(int argc, wchar_t **argv) {
    int seconds = 60;
    std::wstring egl_dir;
    for (int i = 1; i < argc; ++i) {
        const wchar_t *arg = argv[i];
        if (::wcscmp(arg, L"--static") == 0) {
            g_static = true;
        } else if (::wcscmp(arg, L"--no-marker") == 0) {
            g_marker = false;
        } else if (::wcsncmp(arg, L"--seconds=", 10) == 0) {
            seconds = ParseIntArg(arg, L"--seconds=", seconds);
        } else if (::wcsncmp(arg, L"--width=", 8) == 0) {
            g_width = ParseIntArg(arg, L"--width=", g_width);
        } else if (::wcsncmp(arg, L"--height=", 9) == 0) {
            g_height = ParseIntArg(arg, L"--height=", g_height);
        } else if (::wcsncmp(arg, L"--egl-dir=", 10) == 0) {
            egl_dir = arg + 10;
        }
    }
    if (g_width < 64) {
        g_width = 64;
    }
    if (g_height < 64) {
        g_height = 64;
    }
    g_marker_rows = g_height / 4;

    if (egl_dir.empty()) {
        egl_dir = DiscoverQuarkAngleDir();
    }
    if (!LoadAngle(egl_dir)) {
        return 1;
    }

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = ::GetModuleHandleW(nullptr);
    wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = kClassName;
    if (!::RegisterClassExW(&wc)) {
        std::printf("HOST_FAIL RegisterClassEx err=%lu\n", ::GetLastError());
        return 1;
    }
    RECT rc = {0, 0, g_width, g_height};
    ::AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hwnd = ::CreateWindowExW(0, kClassName, kTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                               rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, wc.hInstance, nullptr);
    if (!g_hwnd) {
        std::printf("HOST_FAIL CreateWindowEx err=%lu\n", ::GetLastError());
        return 1;
    }
    if (!InitEgl(g_hwnd)) {
        ::DestroyWindow(g_hwnd);
        return 1;
    }
    ::ShowWindow(g_hwnd, SW_SHOWNORMAL);
    ::UpdateWindow(g_hwnd);
    DrawFrame();
    std::printf("HOST_READY hwnd=0x%llX pid=%lu api=opengl.es\n", (unsigned long long)(uintptr_t)g_hwnd,
                (unsigned long)::GetCurrentProcessId());
    std::fflush(stdout);

    const ULONGLONG deadline = ::GetTickCount64() + (ULONGLONG)seconds * 1000;
    ULONGLONG last_switch = ::GetTickCount64();
    int frames = 0;
    for (;;) {
        MSG msg = {};
        while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
        if (::GetTickCount64() >= deadline) {
            break;
        }
        if (!g_static && ::GetTickCount64() - last_switch >= kColorPeriodMs) {
            g_color_index = (g_color_index + 1) % 3;
            last_switch = ::GetTickCount64();
        }
        DrawFrame();
        if (++frames % 20 == 0) {
            std::printf("HOST_FRAMES n=%d\n", frames);
            std::fflush(stdout);
        }
        ::Sleep(1);
    }

    ReleaseEgl();
    ::DestroyWindow(g_hwnd);
    return 0;
}
