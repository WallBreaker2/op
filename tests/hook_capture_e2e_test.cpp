// 后台（hook / dx.d3d11）截图的**跨进程**端到端验证。
//
// 为什么单独一个文件、单独一个 ctest 项：
//   1. 它要启动一个真实 D3D11 目标进程（d3d11_host_app.exe）并注入 op_x64.dll，
//      注入会改变进程级全局状态（hook 表、共享段名），和别的用例互相污染；
//   2. 它验证的是"另一进程的 Present 钩子把帧写进命名共享段、本进程读出来"这条链路，
//      而不是某个函数返回值 —— 这条链路在改共享段映射缓存之后必须重新证明一遍：
//      映射缓存意味着本进程持有的是**当初**打开的那块视图，如果宿主侧重建了段，
//      我们可能一直读到陈旧数据。这里用"目标进程定时换清屏色 + 读到的颜色必须跟着变"
//      来证明读到的确实是新帧。

#include "test_support.h"

#include <chrono>
#include <set>
#include <string>

namespace {

// 目标进程：d3d11_host_app.exe 按 500ms 轮换绿/红/蓝清屏。
const wchar_t *kHostWindowClass = L"OpD3D11HostWindow";
const wchar_t *kHostExeName = L"d3d11_host_app.exe";

std::wstring ExeDir() {
    wchar_t path[MAX_PATH] = {};
    const DWORD n = ::GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (n == 0) {
        return std::wstring();
    }
    std::wstring full(path, n);
    const size_t pos = full.find_last_of(L"\\/");
    return pos == std::wstring::npos ? std::wstring() : full.substr(0, pos);
}

std::wstring TempBmpPath(const wchar_t *name) {
    wchar_t dir[MAX_PATH] = {};
    if (::GetTempPathW(MAX_PATH, dir) == 0) {
        return std::wstring(name);
    }
    return std::wstring(dir) + name;
}

// 三种纯色轮换的可能取值（BGRA 里的 RGB 十六进制，GetColor 给的就是 "RRGGBB"）。
const std::set<std::wstring> &HostColors() {
    static const std::set<std::wstring> colors = {L"00FF00", L"FF0000", L"0000FF"};
    return colors;
}

// gfx_host_app.exe 的清屏色：红 / 青 / 品红（只取 0 和 255，float->UNORM 精确）。
// 选这三色的原因见 tests/gfx_host_app.cpp 里 kColors 的注释 —— 红绿蓝在通道互换后
// 仍然落回原集合，测试就抓不到 R/B 颠倒；红在 R/B 互换后变蓝、R/G 互换后变绿，
// 都不在本集合内。
const std::set<std::wstring> &GfxHostColors() {
    static const std::set<std::wstring> colors = {L"FF0000", L"00FFFF", L"FF00FF"};
    return colors;
}

// gtest 的失败消息拼的是 std::string；直接把 wstring 逐字符塞进 std::string 会在
// x86 上触发 C4244（wchar_t → char 可能丢数据）。路径都是 ASCII，这里显式窄化。
std::string NarrowForMessage(const std::wstring &wide) {
    std::string narrow;
    narrow.reserve(wide.size());
    for (const wchar_t c : wide) {
        narrow.push_back(c >= 0x20 && c < 0x7F ? static_cast<char>(c) : '?');
    }
    return narrow;
}

// 排查用：设 OP_HOOK_E2E_VERBOSE=1 才打印每步，正常跑不留噪音。
void StepLog(const char *what) {    if (test_support::GetEnvString(L"OP_HOOK_E2E_VERBOSE").empty()) {
        return;
    }
    std::fprintf(stderr, "[step] %s\n", what);
    std::fflush(stderr);
}

void PumpMessagesFor(int milliseconds) {
    const auto deadline = ::GetTickCount64() + static_cast<ULONGLONG>(milliseconds);
    MSG msg = {};
    while (::GetTickCount64() < deadline) {
        while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
        ::Sleep(10);
    }
}

// `Op::GetScreenDataBmp` 返回内存里的 32bpp BMP（§3.2 的像素比对用它，避免落盘再解码）。
struct BmpView {
    const unsigned char *pixels = nullptr;
    int width = 0;
    int height = 0;
    bool top_down = true;
};

bool ParseBmp32(const unsigned char *data, size_t size, BmpView &out) {
    if (!data || size < 54 || data[0] != 'B' || data[1] != 'M') {
        return false;
    }
    const unsigned int offset = *reinterpret_cast<const unsigned int *>(data + 10);
    const int width = *reinterpret_cast<const int *>(data + 18);
    const int height = *reinterpret_cast<const int *>(data + 22);
    const unsigned short bit_count = *reinterpret_cast<const unsigned short *>(data + 28);
    if (bit_count != 32 || width <= 0 || height == 0) {
        return false;
    }
    const size_t rows = static_cast<size_t>(height < 0 ? -height : height);
    if (size < offset + static_cast<size_t>(width) * rows * 4) {
        return false;
    }
    out.pixels = data + offset;
    out.width = width;
    out.height = static_cast<int>(rows);
    out.top_down = height < 0;
    return true;
}

// 返回 BGRA 四字节指针（行序已按 top_down 归一）。
const unsigned char *BmpPixel(const BmpView &view, int x, int y) {
    if (x < 0 || y < 0 || x >= view.width || y >= view.height) {
        return nullptr;
    }
    const int row = view.top_down ? y : (view.height - 1 - y);
    return view.pixels + (static_cast<size_t>(row) * static_cast<size_t>(view.width) + static_cast<size_t>(x)) * 4;
}

class HookCaptureE2ETest : public ::testing::Test {
  protected:
    void SetUp() override {
        // 需要交互式桌面 + 能向其它进程注入，CI 里单独关掉。
        if (!test_support::GetEnvString(L"OP_SKIP_HOOK_E2E_TESTS").empty()) {
            OP_HOOK_E2E_SKIP_OR_FAIL("Hook cross-process tests are disabled by OP_SKIP_HOOK_E2E_TESTS.");
        }
        const std::wstring dir = ExeDir();
        host_exe_ = dir + L"\\" + kHostExeName;
        ASSERT_NE(::GetFileAttributesW(host_exe_.c_str()), INVALID_FILE_ATTRIBUTES)
            << "找不到目标程序 " << NarrowForMessage(host_exe_)
            << "（应当由 tests/CMakeLists.txt 与 op_test 一起构建并安装到同目录）";

        STARTUPINFOW si = {sizeof(si)};
        PROCESS_INFORMATION pi = {};
        std::wstring cmdline = L"\"" + host_exe_ + L"\" --seconds=90 --width=640 --height=360";
        // 自证用：让目标进程不换色，检验「必须读到不止一种颜色」这条断言确实有牙
        // （若产品读到的是陈旧共享段，等价于永远一种颜色，此时必须变红）。
        if (!test_support::GetEnvString(L"OP_HOOK_E2E_STATIC").empty()) {
            cmdline += L" --static";
        }
        // 派生 fixture 在构造函数里设置（构造先于 SetUp），用来给目标程序加"额外形状"，
        // 例如 --extra-swap（两条交换链、各自在不同线程 Present）。
        if (extra_args_ && *extra_args_) {
            cmdline += L" ";
            cmdline += extra_args_;
        }
        ASSERT_TRUE(::CreateProcessW(nullptr, cmdline.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, dir.c_str(),
                                     &si, &pi))
            << "CreateProcessW 失败 err=" << ::GetLastError();
        host_process_ = pi.hProcess;
        if (pi.hThread) {
            ::CloseHandle(pi.hThread);
        }

        // 等目标窗口出现（含 D3D11 初始化时间）。
        const ULONGLONG deadline = ::GetTickCount64() + 15000;
        while (::GetTickCount64() < deadline) {
            host_window_ = ::FindWindowW(kHostWindowClass, nullptr);
            if (host_window_) {
                break;
            }
            ::Sleep(50);
        }
        ASSERT_TRUE(host_window_ != nullptr) << "目标进程 15s 内没有创建窗口";
    }

    void TearDown() override {
        if (bound_) {
            StepLog("before UnBindWindow");
            long ret = 0;
            op_.UnBindWindow(&ret);
            StepLog("after UnBindWindow");
            bound_ = false;
        }
        if (host_process_) {
            ::TerminateProcess(host_process_, 0);
            ::WaitForSingleObject(host_process_, 3000);
            ::CloseHandle(host_process_);
            host_process_ = nullptr;
        }
    }

    op::Op op_;
    HWND host_window_ = nullptr;
    HANDLE host_process_ = nullptr;
    std::wstring host_exe_;
    bool bound_ = false;
    // 目标程序的额外命令行参数，派生 fixture 在构造函数里改（构造先于 SetUp）。
    const wchar_t *extra_args_ = L"";

    // 采样 samples 次（每次间隔 600ms，目标进程 500ms 换一次色），要求：
    //   1. 每次颜色都在目标进程的调色板里 —— 否则共享段里根本不是目标进程的帧；
    //   2. 至少出现两种颜色 —— 只读到一种说明帧没更新（陈旧共享段 / 钩子未生效）。
    void ExpectFreshFrames(int samples, const char *tag) {
        // 给钩子装上 + 首帧写进共享段的时间。
        PumpMessagesFor(1200);
        std::set<std::wstring> seen;
        std::wstring first_color;
        for (int i = 0; i < samples; ++i) {
            StepLog("before GetColor");
            std::wstring color;
            op_.GetColor(320, 180, color);
            StepLog("after GetColor");
            ASSERT_EQ(color.size(), static_cast<size_t>(6))
                << tag << " 第 " << i << " 次取色返回异常值:" << NarrowForMessage(color);
            EXPECT_TRUE(HostColors().count(color) == 1)
                << tag << " 第 " << i << " 次取到的颜色 " << NarrowForMessage(color)
                << " 不在目标进程的清屏色集合 {FF0000, 00FFFF, FF00FF} 里：共享段里不是目标进程画的帧";
            if (i == 0) {
                first_color = color;
            }
            seen.insert(color);
            PumpMessagesFor(600);
        }
        EXPECT_GT(seen.size(), static_cast<size_t>(1))
            << tag << " " << samples << " 次采样只读到一种颜色 " << NarrowForMessage(first_color)
            << "：帧没有更新（陈旧共享段 / 钩子未生效）";
    }
};

// 多交换链形状的 d3d11 目标：目标进程除了主链，还在独立线程里 Present 第二条交换链。
// 注入侧的 staging 缓存 / 设备上下文是**全进程共享**的静态状态（D3D11Capture.cpp 的
// g_staging_cache + g_staging_lock），两条链并发进来必须靠锁串行化，否则会互相顶掉
// staging、读到别的链的尺寸（§72.21 在 d3d12 上就是这么炸的，§72.22 在这里验）。
class HookCaptureMultiChainE2ETest : public HookCaptureE2ETest {
  public:
    HookCaptureMultiChainE2ETest() {
        extra_args_ = L"--extra-swap";
    }
};

// ---------------------------------------------------------------------------
// 其余四个后端：d3d9 / d3d10 / d3d12 / opengl
//
// 五个后端共用同一段"钩子侧把整帧写进命名共享段、宿主侧读出来"的搬运代码
// （libop/hook/DxCaptureCommon.cpp + SharedFrameChannel.*），但本轮之前只有 d3d11
// 被真注入验过。这里给剩下四个后端各配一份真实目标进程（gfx_host_app.exe），
// 跑和 d3d11 同样的两条用例：颜色必须跟着目标进程换、解绑重绑后映射必须跟上新段。
// ---------------------------------------------------------------------------

// opengl.es 的目标程序需要 ANGLE（本机来自夸克网盘）。没有就跳过用例 ——
// 这是环境依赖，不是产品缺陷。
bool HasAngleDll() {
    // 32 位测试进程的 ProgramFiles 被重定向到 "Program Files (x86)"，夸克网盘在 64 位目录下，
    // 所以先读 ProgramW6432。
    wchar_t program_files[MAX_PATH] = {};
    if (::GetEnvironmentVariableW(L"ProgramW6432", program_files, MAX_PATH) == 0 &&
        ::GetEnvironmentVariableW(L"ProgramFiles", program_files, MAX_PATH) == 0) {
        return false;
    }
    const std::wstring pattern = std::wstring(program_files) + L"\\QuarkCloudDrive\\*";
    WIN32_FIND_DATAW fd = {};
    HANDLE find = ::FindFirstFileW(pattern.c_str(), &fd);
    if (find == INVALID_HANDLE_VALUE) {
        return false;
    }
    bool found = false;
    do {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 || fd.cFileName[0] == L'.') {
            continue;
        }
        const std::wstring dll =
            std::wstring(program_files) + L"\\QuarkCloudDrive\\" + fd.cFileName + L"\\libEGL.dll";
        if (::GetFileAttributesW(dll.c_str()) != INVALID_FILE_ATTRIBUTES) {
            found = true;
            break;
        }
    } while (::FindNextFileW(find, &fd));
    ::FindClose(find);
    return found;
}

struct GfxHostApi {
    const wchar_t *api;          // gfx_host_app.exe 的 --api= 取值
    const wchar_t *window_class; // 目标进程窗口类名（= OpGfxHostWindow_<api>）
    const wchar_t *render_type;  // BindWindow 的 render type 字符串
    bool has_marker;             // 目标进程是否在屏幕顶部画了白色标记带
};

// d3d9 用 Clear 的矩形、opengl 用 glScissor 画标记带；d3d10/d3d12 的
// ClearRenderTargetView 没有矩形参数（画带要引着色器/离屏纹理），只验颜色新鲜度。
const GfxHostApi kApiD3D9 = {L"d3d9", L"OpGfxHostWindow_d3d9", L"dx.d3d9", true};
const GfxHostApi kApiD3D10 = {L"d3d10", L"OpGfxHostWindow_d3d10", L"dx.d3d10", true};
// d3d11 复用原来的 d3d11_host_app.exe（它只认 --seconds/--static/--width/--height，
// 多给一个 --api=d3d11 不会影响它），窗口类名和 exe 名都是它自己的。
const GfxHostApi kApiD3D11 = {L"d3d11", L"OpD3D11HostWindow", L"dx.d3d11", false};
const GfxHostApi kApiD3D12 = {L"d3d12", L"OpGfxHostWindow_d3d12", L"dx.d3d12", true};
const GfxHostApi kApiOpenGL = {L"opengl", L"OpGfxHostWindow_opengl", L"opengl", true};
// 同一个目标程序、同一个窗口，只是产品侧的钩子挂在不同入口上：
// opengl.std 挂 glBegin、opengl.fi 挂 glFinish。两条入口原来都没有覆盖。
const GfxHostApi kApiOpenGLStd = {L"opengl", L"OpGfxHostWindow_opengl", L"opengl.std", true};
const GfxHostApi kApiOpenGLFi = {L"opengl", L"OpGfxHostWindow_opengl", L"opengl.fi", true};
// opengl.es：钩子挂在 libEGL!eglSwapBuffers 上，目标程序是 egl_host_app.exe
// （它加载夸克网盘自带的 ANGLE）。窗口类名是它自己的 OpEglHostWindow。
const GfxHostApi kApiOpenGLES = {L"opengl", L"OpEglHostWindow", L"opengl.es", true};

const wchar_t *kGfxHostExeName = L"gfx_host_app.exe";
const wchar_t *kEglHostExeName = L"egl_host_app.exe";
// opengl.es 的目标程序必须是 64 位：夸克网盘那份 ANGLE 是 64 位的，32 位进程加载不了。
// 所以 x86 构建里跑的是 stage 过来的 egl_host_app_x64.exe（宿主会用 op_inject64 助手注入）。
const wchar_t *EglHostExeName() {
#if defined(_WIN64)
    return kEglHostExeName;
#else
    return L"egl_host_app_x64.exe";
#endif
}

// 目标程序是否已经在 op_test 旁边（x86 上要靠 x64 构建产物 stage 过来）。
bool HasEglHost() {
    const std::wstring path = ExeDir() + L"\\" + EglHostExeName();
    return ::GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}
// 64 位目标程序：由 tests/CMakeLists.txt 在 x86 构建里从 x64 产物 stage 过来（改名避免重名）。
const wchar_t *kGfxHostExeNameX64 = L"gfx_host_app_x64.exe";

class HookCaptureGfxE2ETest : public ::testing::Test {
  protected:
    void SetUp() override {
        if (!test_support::GetEnvString(L"OP_SKIP_HOOK_E2E_TESTS").empty()) {
            OP_HOOK_E2E_SKIP_OR_FAIL("Hook cross-process tests are disabled by OP_SKIP_HOOK_E2E_TESTS.");
        }
    }

    void TearDown() override {
        if (bound_) {
            long ret = 0;
            op_.UnBindWindow(&ret);
            bound_ = false;
        }
        if (host_process_) {
            ::TerminateProcess(host_process_, 0);
            ::WaitForSingleObject(host_process_, 3000);
            ::CloseHandle(host_process_);
            host_process_ = nullptr;
        }
    }

    // 起目标进程并等它的窗口出现。extra_args 用来打开目标程序的额外形状
    // （目前只有 d3d12 的 --d3d12-extra-swap：第二条交换链 + 独立 Present 线程）。
    void StartHost(const GfxHostApi &api, const wchar_t *exe_name = kGfxHostExeName,
                   const wchar_t *extra_args = L"") {
        const std::wstring dir = ExeDir();
        host_exe_ = dir + L"\\" + exe_name;
        ASSERT_NE(::GetFileAttributesW(host_exe_.c_str()), INVALID_FILE_ATTRIBUTES)
            << "找不到目标程序 " << NarrowForMessage(host_exe_)
            << "（应当由 tests/CMakeLists.txt 与 op_test 一起构建并安装到同目录）";

        STARTUPINFOW si = {sizeof(si)};
        PROCESS_INFORMATION pi = {};
        std::wstring cmdline = L"\"" + host_exe_ + L"\" --api=" + api.api + L" --seconds=120 --width=640 --height=360";
        if (extra_args && *extra_args) {
            cmdline += L" ";
            cmdline += extra_args;
        }
        if (!test_support::GetEnvString(L"OP_HOOK_E2E_STATIC").empty()) {
            cmdline += L" --static";
        }
        // GL 的交换入口可切：默认 gdi32!SwapBuffers（真实 GL 程序走的路），
        // 排查时可切到 opengl32!wglSwapBuffers（钩子直接挂的入口）。
        const std::wstring gl_swap = test_support::GetEnvString(L"OP_HOOK_E2E_GL_SWAP");
        if (!gl_swap.empty()) {
            cmdline += L" --gl-swap=" + gl_swap;
        }
        ASSERT_TRUE(::CreateProcessW(nullptr, cmdline.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                                     dir.c_str(), &si, &pi))
            << "CreateProcessW 失败 err=" << ::GetLastError();
        host_process_ = pi.hProcess;
        if (pi.hThread) {
            ::CloseHandle(pi.hThread);
        }

        // 20s：d3d12 要建设备 + 队列 + 交换链，比 d3d11 慢一些。
        const ULONGLONG deadline = ::GetTickCount64() + 20000;
        while (::GetTickCount64() < deadline) {
            host_window_ = ::FindWindowW(api.window_class, nullptr);
            if (host_window_) {
                break;
            }
            ::Sleep(50);
        }
        ASSERT_TRUE(host_window_ != nullptr) << "目标进程 20s 内没有创建窗口（api=" << NarrowForMessage(api.api)
                                             << "，可能初始化失败，看 HOST_FAIL 输出）";
    }

    // 主链路：注入 -> 目标进程钩子里写共享段 -> 本进程读出来，且读到的必须是新帧。
    void ExpectFreshFrames(const GfxHostApi &api) {
        if (!host_window_) {
            return; // StartHost 已经报过致命错误
        }
        long ret = 0;
        op_.SetShowErrorMsg(3, &ret);

        ret = 0;
        op_.BindWindow((long)(intptr_t)host_window_, api.render_type, L"normal", L"normal", 0, &ret);
        ASSERT_EQ(ret, 1) << "BindWindow(" << NarrowForMessage(api.render_type)
                          << ") 失败：注入或 SetDisplayHook 未成功（op 日志见 bin/x64 下 op.log）";
        bound_ = true;

        // 给钩子装上 + 首帧写进共享段的时间（d3d12 的抓帧是"本次提交、下次读回"，
        // 所以它的第一帧本来就晚一拍，这里多等一点）。
        PumpMessagesFor(1500);

        std::set<std::wstring> seen;
        std::wstring first_color;
        for (int i = 0; i < 6; ++i) {
            std::wstring color;
            op_.GetColor(320, 180, color);
            ASSERT_EQ(color.size(), static_cast<size_t>(6)) << "第 " << i << " 次取色返回异常值:" << NarrowForMessage(color);
            EXPECT_TRUE(GfxHostColors().count(color) == 1)
                << "第 " << i << " 次取到的颜色 " << NarrowForMessage(color)
                << " 不在目标进程的清屏色集合 {FF0000, 00FFFF, FF00FF} 里：共享段里不是目标进程画的帧";
            if (i == 0) {
                first_color = color;
            }
            seen.insert(color);
            PumpMessagesFor(600);
        }
        EXPECT_GT(seen.size(), static_cast<size_t>(1))
            << "6 次采样只读到一种颜色 " << NarrowForMessage(first_color) << "：帧没有更新（陈旧共享段 / 钩子未生效）";

        if (api.has_marker) {
            // 目标进程在**屏幕顶部** 1/4 画白带、其余部分刷轮换色。
            // 产品侧传统 GL 路径要逐行翻转、DX 路径不翻转，两种情况下白带都必须落在
            // 图像顶部；翻转方向反了的话白带会跑到图像底部，这两个断言会同时变红。
            std::wstring top;
            std::wstring bottom;
            op_.GetColor(320, 20, top);
            op_.GetColor(320, 340, bottom);
            EXPECT_EQ(top, std::wstring(L"FFFFFF"))
                << "图像顶部不是目标进程画的白色标记带（" << NarrowForMessage(top) << "）：画面上下颠倒";
            EXPECT_NE(bottom, std::wstring(L"FFFFFF")) << "白色标记带出现在图像底部：逐行翻转方向反了";
            EXPECT_TRUE(GfxHostColors().count(bottom) == 1) << "图像底部颜色 " << NarrowForMessage(bottom) << " 不是轮换色";
        }

        // 整块读回路径（HookCapture::requestCapture 的整行 memcpy）也要通。
        const std::wstring bmp_path = TempBmpPath(L"hook_e2e_gfx_capture.bmp");
        ::DeleteFileW(bmp_path.c_str());
        ret = 0;
        op_.Capture(0, 0, 320, 180, bmp_path.c_str(), &ret);
        ASSERT_EQ(ret, 1) << "Capture 失败";
        WIN32_FILE_ATTRIBUTE_DATA fad = {};
        ASSERT_TRUE(::GetFileAttributesExW(bmp_path.c_str(), GetFileExInfoStandard, &fad));
        EXPECT_GT((static_cast<unsigned long long>(fad.nFileSizeHigh) << 32) | fad.nFileSizeLow, 1024ULL);
        ::DeleteFileW(bmp_path.c_str());
    }

    // 绑定/解绑多次：共享段映射是被缓存复用的，重复绑定必须重新 open 到新段。
    void ExpectRebindWorks(const GfxHostApi &api) {
        if (!host_window_) {
            return;
        }
        long ret = 0;
        op_.SetShowErrorMsg(3, &ret);

        for (int round = 0; round < 2; ++round) {
            ret = 0;
            op_.BindWindow((long)(intptr_t)host_window_, api.render_type, L"normal", L"normal", 0, &ret);
            ASSERT_EQ(ret, 1) << "第 " << round << " 轮 BindWindow(" << NarrowForMessage(api.render_type) << ") 失败";
            bound_ = true;
            PumpMessagesFor(900);

            std::set<std::wstring> seen;
            for (int i = 0; i < 4; ++i) {
                std::wstring color;
                op_.GetColor(320, 180, color);
                if (GfxHostColors().count(color) == 1) {
                    seen.insert(color);
                }
                PumpMessagesFor(600);
            }
            EXPECT_GT(seen.size(), static_cast<size_t>(1))
                << "第 " << round << " 轮只读到一种颜色：重新绑定后共享段映射没有跟上新段";

            ret = 0;
            op_.UnBindWindow(&ret);
            bound_ = false;
            PumpMessagesFor(300);
        }
    }

    // -----------------------------------------------------------------------
    // §3.2 只搬请求区域
    //
    // 宿主 `Capture(x, y, w, h)` 现在把想要的矩形发布到共享段尾部的协商区，注入侧按矩形搬
    // （D3D 走 CopySubresourceRegion，GL 只拷那几行），宿主再按注入侧回写的响应解释像素区。
    // 这里验两件事：
    //   1. 区域读回的像素必须和整窗读回的对应位置逐字节一致（布局/偏移没串行）；
    //   2. 行索引必须落在正确的窗口行上 —— 纯色帧看不出行错位，所以用目标进程顶部的
    //      白色标记带（只有 d3d9 / opengl 画）当标尺：取一条跨越带子下边界的横条，
    //      白色必须正好在带内那几行结束。
    // -----------------------------------------------------------------------

    void BindForRegion(const GfxHostApi &api) {
        long ret = 0;
        op_.SetShowErrorMsg(3, &ret);
        ret = 0;
        op_.BindWindow((long)(intptr_t)host_window_, api.render_type, L"normal", L"normal", 0, &ret);
        ASSERT_EQ(ret, 1) << "BindWindow(" << NarrowForMessage(api.render_type) << ") 失败";
        bound_ = true;
        PumpMessagesFor(1500);
    }

    void UnbindForRegion() {
        long ret = 0;
        op_.UnBindWindow(&ret);
        bound_ = false;
    }

    // 注意这条用例验的是**像素正确性**，不是"注入侧真的只搬了矩形"：
    // 宿主就算退回整窗读再自己裁，读回的像素也和这里期望的一致。两种传输路径的区分
    // 靠 scripts/hook_capture_probe.cpp 的 CheckRegionTransport（它直接读共享段，
    // 断言响应 mode=Region 且像素区只有 rw*4 的行距）。这里守的是另一类错误：
    // 宿主把"按矩形搬的帧"当整窗解释（§72.19 的 D6）、行索引/翻转算错、越界钳错。
    void ExpectRegionMatchesFullFrame(const GfxHostApi &api) {
        BindForRegion(api);

        const int full_w = 640;
        const int full_h = 360;
        const int rx = 160;
        // 矩形必须**跨过顶部标记带**（目标进程把上面 1/4 刷成白色，360 高就是 0..89 行）。
        // 原来取 ry=90 正好整块落在纯背景区：上下颠倒读回来还是同一片纯色，GL 的
        // Y 原点错误因此漏过了（见 defect_fixes.md §72.19）。
        const int ry = 40;
        const int rw = 320;
        const int rh = 180;

        // 目标进程 500ms 换一次色，整窗和区域是两次独立读，跨了换色就对不上；
        // 对不上就重来，最多 8 轮。
        bool matched = false;
        std::string mismatch;
        for (int attempt = 0; attempt < 8 && !matched; ++attempt) {
            size_t full_data = 0;
            long full_size = 0;
            long ret = 0;
            op_.GetScreenDataBmp(0, 0, full_w, full_h, &full_data, &full_size, &ret);
            ASSERT_EQ(ret, 1) << "整窗 GetScreenDataBmp 失败";
            // GetScreenDataBmp 返回的指针指向 Op 内部复用的缓冲区，**下一次调用就会把它覆盖**。
            // 这里先拷一份再读下面的区域图，否则 full 会指向区域图的数据（§72.19 的 FF00FF）。
            std::vector<unsigned char> full_hold(
                reinterpret_cast<const unsigned char *>(full_data),
                reinterpret_cast<const unsigned char *>(full_data) + static_cast<size_t>(full_size));
            BmpView full;
            ASSERT_TRUE(ParseBmp32(full_hold.data(), full_hold.size(), full));
            ASSERT_EQ(full.width, full_w);
            ASSERT_EQ(full.height, full_h);
            // 注入侧要等**下一帧**才按矩形搬：第一次读同一矩形，宿主采样到的响应还是上一帧的
            // 整窗布局（mode=full），走的是整窗读。先读一次丢掉，第二次才是"按矩形搬 → 按矩形读"
            // 的 §3.2 路径（见 defect_fixes.md §72.19）。
            size_t warm_data = 0;
            long warm_size = 0;
            long warm_ret = 0;
            op_.GetScreenDataBmp(rx, ry, rx + rw, ry + rh, &warm_data, &warm_size, &warm_ret);
            ASSERT_EQ(warm_ret, 1) << "区域预热读失败";

            size_t region_data = 0;
            long region_size = 0;
            ret = 0;
            op_.GetScreenDataBmp(rx, ry, rx + rw, ry + rh, &region_data, &region_size, &ret);
            ASSERT_EQ(ret, 1) << "区域 GetScreenDataBmp 失败";
            BmpView region;
            ASSERT_TRUE(ParseBmp32(reinterpret_cast<const unsigned char *>(region_data),
                                   static_cast<size_t>(region_size), region));
            // 顺便锁住 x2/y2 是排他的（否则这里会是 321x181）。
            ASSERT_EQ(region.width, rw);
            ASSERT_EQ(region.height, rh);

            // 只比对顶部标记带覆盖的行：那条带是纯白、不随目标进程 500ms 换色变化，
            // 而整窗读和区域读之间必然隔着一帧（甚至一次换色）。比对整块会偶发不匹配，
            // 而带内的行足够抓住"区域搬错位置/上下颠倒/读到垃圾"（见 §72.19）。
            const int band_rows = full_h / 4; // 目标进程把上面 1/4 刷成白色
            const int compare_rows = (ry < band_rows) ? std::min(rh, band_rows - ry) : 0;
            matched = compare_rows > 0;
            mismatch.clear();
            if (!matched) {
                mismatch = "比对矩形没有覆盖顶部标记带，用例写错了";
            }
            for (int y = 0; y < compare_rows && matched; ++y) {
                for (int x = 0; x < rw; ++x) {
                    const unsigned char *a = BmpPixel(full, rx + x, ry + y);
                    const unsigned char *b = BmpPixel(region, x, y);
                    if (!a || !b || a[0] != b[0] || a[1] != b[1] || a[2] != b[2]) {
                        char detail[160] = {};
                        std::snprintf(detail, sizeof(detail),
                                      "区域(%d,%d) 与整窗(%d,%d) 不一致（第 %d 轮）：区域=%02X%02X%02X 整窗=%02X%02X%02X",
                                      x, y, rx + x, ry + y, attempt, b ? b[2] : 0, b ? b[1] : 0, b ? b[0] : 0,
                                      a ? a[2] : 0, a ? a[1] : 0, a ? a[0] : 0);
                        mismatch = detail;
                        matched = false;
                        break;
                    }
                }
            }
            if (!matched) {
                PumpMessagesFor(120);
            }
        }
        EXPECT_TRUE(matched) << mismatch << "：区域读回的像素与整窗对应位置不一致";

        // 越界矩形在 Op 层就被夹到窗口内（不是报错），所以这里验夹取结果：
        // 右边界超出 100 像素、只该拿回 630..639 这 10 列。
        size_t out_data = 0;
        long out_size = 0;
        long ret = 0;
        op_.GetScreenDataBmp(full_w - 10, 0, full_w + 100, 100, &out_data, &out_size, &ret);
        EXPECT_EQ(ret, 1) << "夹取后的读回失败了";
        BmpView clamped;
        EXPECT_TRUE(ParseBmp32(reinterpret_cast<const unsigned char *>(out_data), static_cast<size_t>(out_size), clamped));
        EXPECT_EQ(clamped.width, 10) << "越界矩形没有被夹到窗口内";
        EXPECT_EQ(clamped.height, 100);

        UnbindForRegion();
    }

    void ExpectMarkerBandBoundaryInRegion(const GfxHostApi &api) {
        BindForRegion(api);

        const int band_rows = 90;      // gfx_host_app: g_marker_rows = height / 4
        const int y0 = band_rows - 30; // 60
        const int rows = 60;           // 60..119，正好跨过带子下边界

        bool ok = false;
        std::string detail;
        for (int attempt = 0; attempt < 8 && !ok; ++attempt) {
            size_t data = 0;
            long size = 0;
            long ret = 0;
            op_.GetScreenDataBmp(0, y0, 640, y0 + rows, &data, &size, &ret);
            ASSERT_EQ(ret, 1) << "横条 GetScreenDataBmp 失败";
            BmpView view;
            ASSERT_TRUE(ParseBmp32(reinterpret_cast<const unsigned char *>(data), static_cast<size_t>(size), view));
            ASSERT_EQ(view.width, 640);
            ASSERT_EQ(view.height, rows);

            const unsigned char *in_first = BmpPixel(view, 320, 0);        // 窗口行 60：带内
            const unsigned char *in_last = BmpPixel(view, 320, 29);        // 窗口行 89：带内最后一行
            const unsigned char *out_first = BmpPixel(view, 320, 30);      // 窗口行 90：带外第一行
            const unsigned char *out_last = BmpPixel(view, 320, rows - 1); // 窗口行 119：带外

            const auto is_white = [](const unsigned char *p) {
                return p && p[0] == 255 && p[1] == 255 && p[2] == 255;
            };
            if (!is_white(in_first) || !is_white(in_last)) {
                detail = "带内那 30 行不是白色：区域搬运的行索引落在错误位置";
                PumpMessagesFor(120);
                continue;
            }
            if (is_white(out_first) || is_white(out_last)) {
                // 换色瞬间可能读到上一帧的边界，重试一轮再判。
                detail = "带外那 30 行出现白色：标记带边界错位（行偏移/翻转方向错）";
                PumpMessagesFor(120);
                continue;
            }
            ok = true;
        }
        EXPECT_TRUE(ok) << detail;

        UnbindForRegion();
    }

    op::Op op_;
    HWND host_window_ = nullptr;
    HANDLE host_process_ = nullptr;
    std::wstring host_exe_;
    bool bound_ = false;
};

} // namespace

// 主链路：注入 → 目标进程 Present 里写共享段 → 本进程读出来，且读到的必须是新帧。
TEST_F(HookCaptureE2ETest, D3D11BackgroundCaptureReadsFreshFrames) {
    long ret = 0;
    op_.SetShowErrorMsg(3, &ret);

    StepLog("before BindWindow");
    ret = 0;
    op_.BindWindow((long)(intptr_t)host_window_, L"dx.d3d11", L"normal", L"normal", 0, &ret);
    StepLog("after BindWindow");
    ASSERT_EQ(ret, 1) << "BindWindow(dx.d3d11) 失败：注入或 SetDisplayHook 未成功（op 日志见 bin/x64 下 op.log）";
    bound_ = true;

    ExpectFreshFrames(6, "d3d11");

    // 整块读回路径（HookCapture::requestCapture 的 memcpy）也要通。
    const std::wstring bmp_path = TempBmpPath(L"hook_e2e_capture.bmp");
    ::DeleteFileW(bmp_path.c_str());
    ret = 0;
    op_.Capture(0, 0, 320, 180, bmp_path.c_str(), &ret);
    ASSERT_EQ(ret, 1) << "Capture 失败";
    WIN32_FILE_ATTRIBUTE_DATA fad = {};
    ASSERT_TRUE(::GetFileAttributesExW(bmp_path.c_str(), GetFileExInfoStandard, &fad));
    EXPECT_GT((static_cast<unsigned long long>(fad.nFileSizeHigh) << 32) | fad.nFileSizeLow, 1024ULL);
    ::DeleteFileW(bmp_path.c_str());
}

// 绑定/解绑多次：共享段映射是被缓存复用的，重复绑定必须重新 open 到新段，不能残留旧映射。
TEST_F(HookCaptureE2ETest, D3D11RebindAfterUnbindStillReadsFrames) {
    long ret = 0;
    op_.SetShowErrorMsg(3, &ret);

    for (int round = 0; round < 2; ++round) {
        ret = 0;
        op_.BindWindow((long)(intptr_t)host_window_, L"dx.d3d11", L"normal", L"normal", 0, &ret);
        ASSERT_EQ(ret, 1) << "第 " << round << " 轮 BindWindow 失败";
        bound_ = true;
        PumpMessagesFor(900);

        std::set<std::wstring> seen;
        for (int i = 0; i < 4; ++i) {
            std::wstring color;
            op_.GetColor(320, 180, color);
            if (HostColors().count(color) == 1) {
                seen.insert(color);
            }
            PumpMessagesFor(600);
        }
        EXPECT_GT(seen.size(), static_cast<size_t>(1))
            << "第 " << round << " 轮只读到一种颜色：重新绑定后共享段映射没有跟上新段";

        ret = 0;
        op_.UnBindWindow(&ret);
        bound_ = false;
        PumpMessagesFor(300);
    }
}

// 两条交换链 + 两个线程同时 Present：帧必须继续更新，而且不能崩。
// 这条用例第一次跑在"注入侧没有互斥"的实现上会红（d3d12 的 §72.21 就是这样抓到缺陷的），
// d3d11 侧因为 g_staging_lock 已经是独占锁，用来回归"别把锁删了"。
TEST_F(HookCaptureMultiChainE2ETest, D3D11TwoSwapChainsKeepsReadingFrames) {
    long ret = 0;
    op_.SetShowErrorMsg(3, &ret);

    ret = 0;
    op_.BindWindow((long)(intptr_t)host_window_, L"dx.d3d11", L"normal", L"normal", 0, &ret);
    ASSERT_EQ(ret, 1) << "BindWindow(dx.d3d11) 失败：多交换链目标下注入或 SetDisplayHook 未成功";
    bound_ = true;

    ExpectFreshFrames(6, "d3d11+extra-swap");
}

// ---------------------------------------------------------------------------
// 四个新后端的用例。每条都起一份真实目标进程 + 注入，所以成对出现：
// 一条验"读到的必须是新帧"，一条验"解绑重绑后映射要跟上新段"。
// ---------------------------------------------------------------------------

TEST_F(HookCaptureGfxE2ETest, D3D9BackgroundCaptureReadsFreshFrames) {
    StartHost(kApiD3D9);
    ExpectFreshFrames(kApiD3D9);
}

TEST_F(HookCaptureGfxE2ETest, D3D9RebindAfterUnbindStillReadsFrames) {
    StartHost(kApiD3D9);
    ExpectRebindWorks(kApiD3D9);
}

TEST_F(HookCaptureGfxE2ETest, D3D10BackgroundCaptureReadsFreshFrames) {
    StartHost(kApiD3D10);
    ExpectFreshFrames(kApiD3D10);
}

TEST_F(HookCaptureGfxE2ETest, D3D10RebindAfterUnbindStillReadsFrames) {
    StartHost(kApiD3D10);
    ExpectRebindWorks(kApiD3D10);
}

TEST_F(HookCaptureGfxE2ETest, D3D12BackgroundCaptureReadsFreshFrames) {
    StartHost(kApiD3D12);
    ExpectFreshFrames(kApiD3D12);
}

TEST_F(HookCaptureGfxE2ETest, D3D12RebindAfterUnbindStillReadsFrames) {
    StartHost(kApiD3D12);
    ExpectRebindWorks(kApiD3D12);
}

TEST_F(HookCaptureGfxE2ETest, OpenGLBackgroundCaptureReadsFreshFrames) {
    StartHost(kApiOpenGL);
    ExpectFreshFrames(kApiOpenGL);
}

// 一个进程里两个 GL 窗口（目标程序 --gl-extra-swap）：只有被绑定的那个窗口的帧该被搬。
// 另一个窗口故意不画标记带，所以"顶部是白色标记带"这条断言同时证明了没有串到另一个窗口。
TEST_F(HookCaptureGfxE2ETest, OpenGLTwoWindowsKeepsReadingFrames) {
    StartHost(kApiOpenGL, kGfxHostExeName, L"--gl-extra-swap");
    ExpectFreshFrames(kApiOpenGL);
}

// opengl.std：钩子挂在 glBegin 上。目标程序在 SwapBuffers 之前走一次 glBegin/glEnd。
TEST_F(HookCaptureGfxE2ETest, OpenGLStdEntryCapturesFrames) {
    StartHost(kApiOpenGL, kGfxHostExeName, L"--gl-swap=begin");
    ExpectFreshFrames(kApiOpenGLStd);
}

// opengl.fi：钩子挂在 glFinish 上。目标程序在 SwapBuffers 之前走一次 glFinish。
// Windows 上「交换」有两条入口：gdi32!SwapBuffers 与 opengl32!wglSwapBuffers。
// 产品把钩子挂在后者；默认用例走的是 gdi32!SwapBuffers（真实 GL 程序的路），
// 这里补一条直接调 wglSwapBuffers 的目标程序，确认钩子在直接入口上也生效（§72.27）。
TEST_F(HookCaptureGfxE2ETest, OpenGLWglEntryCapturesFrames) {
    StartHost(kApiOpenGL, kGfxHostExeName, L"--gl-swap=wgl");
    ExpectFreshFrames(kApiOpenGL);
}

// 同一条直接入口 + 第二个 GL 窗口：按窗口过滤（wglGetCurrentDC + WindowFromDC）
// 在这条入口上同样要生效，否则两个窗口的帧会混进同一个共享段。
TEST_F(HookCaptureGfxE2ETest, OpenGLWglEntryIgnoresSecondWindow) {
    StartHost(kApiOpenGL, kGfxHostExeName, L"--gl-swap=wgl --gl-extra-swap");
    ExpectFreshFrames(kApiOpenGL);
}

TEST_F(HookCaptureGfxE2ETest, OpenGLFinishEntryCapturesFrames) {
    StartHost(kApiOpenGL, kGfxHostExeName, L"--gl-swap=finish");
    ExpectFreshFrames(kApiOpenGLFi);
}

// opengl.std + 第二个 GL 窗口：过滤走的是 wglGetCurrentDC() 那条分支（glBegin 拿不到 hdc），
// 另一窗口也在 glBegin 入口上触发钩子，所以这条用例专门盯那个分支。
TEST_F(HookCaptureGfxE2ETest, OpenGLStdEntryIgnoresSecondWindow) {
    StartHost(kApiOpenGL, kGfxHostExeName, L"--gl-swap=begin --gl-extra-swap");
    ExpectFreshFrames(kApiOpenGLStd);
}

// opengl.es：钩子挂在 libEGL!eglSwapBuffers 上，目标程序加载夸克网盘自带的 ANGLE。
TEST_F(HookCaptureGfxE2ETest, OpenGLESBackgroundCaptureReadsFreshFrames) {
    if (!HasAngleDll() || !HasEglHost()) {
        OP_HOOK_E2E_SKIP_OR_FAIL("本机没有 ANGLE（夸克网盘自带的 libEGL.dll）或没有 " +
                                 NarrowForMessage(EglHostExeName()) + "，跳过 opengl.es 用例");
    }
    StartHost(kApiOpenGLES, EglHostExeName());
    ExpectFreshFrames(kApiOpenGLES);
}

TEST_F(HookCaptureGfxE2ETest, OpenGLESRebindAfterUnbindStillReadsFrames) {
    if (!HasAngleDll() || !HasEglHost()) {
        OP_HOOK_E2E_SKIP_OR_FAIL("本机没有 ANGLE（夸克网盘自带的 libEGL.dll）或没有 " +
                                 NarrowForMessage(EglHostExeName()) + "，跳过 opengl.es 用例");
    }
    StartHost(kApiOpenGLES, EglHostExeName());
    ExpectRebindWorks(kApiOpenGLES);
}

TEST_F(HookCaptureGfxE2ETest, OpenGLRebindAfterUnbindStillReadsFrames) {
    StartHost(kApiOpenGL);
    ExpectRebindWorks(kApiOpenGL);
}

// ---------------------------------------------------------------------------
// 跨位宽：32 位宿主 + 原生 64 位目标。
//
// 真实场景里 32 位自动化程序（32 位 Python / 老 C# 客户端）绑定 64 位游戏很常见：
// 宿主跑在 op_x86.dll 上，要注入的却是 op_x64.dll。BlackBone 的远程调用按"目标是否
// WOW64"选汇编器，32 位宿主对原生 64 位目标会选到错位宽的 stub —— 注入报成功、
// 导出也解析得到，但远端一行代码都不执行，偶发还卡 111 秒（证据见 §72.13）。
//
// 现在改由 64 位助手 op_inject64.exe 代劳：它自己就是 64 位，注入与远程调用同构。
// 所以这条用例和别的后端一样验"能绑上、读到的必须是新帧、解绑不把目标带崩"。
//
// 依赖三份 x64 产物，由 tests/CMakeLists.txt 的 stage_x64_artifacts.cmake 放到
// op_test.exe 同目录：op_x64.dll、gfx_host_app_x64.exe、op_inject64.exe。
// 没 stage 就跳过（纯 x86 环境没编过 x64，本来也拿不到）。
// ---------------------------------------------------------------------------
#if !defined(_WIN64)
TEST_F(HookCaptureGfxE2ETest, X86HostToX64TargetReadsFreshFrames) {
    const std::wstring dir = ExeDir();
    const std::wstring x64_dll = dir + L"\\op_x64.dll";
    const std::wstring x64_host = dir + L"\\" + kGfxHostExeNameX64;
    const std::wstring x64_helper = dir + L"\\op_inject64.exe";
    if (::GetFileAttributesW(x64_dll.c_str()) == INVALID_FILE_ATTRIBUTES ||
        ::GetFileAttributesW(x64_host.c_str()) == INVALID_FILE_ATTRIBUTES ||
        ::GetFileAttributesW(x64_helper.c_str()) == INVALID_FILE_ATTRIBUTES) {
        OP_HOOK_E2E_SKIP_OR_FAIL("没有 stage 齐全的 x64 产物（op_x64.dll / " +
                                 NarrowForMessage(kGfxHostExeNameX64) +
                                 " / op_inject64.exe），跳过跨位宽用例；先编一次 x64 再编 x86 即可");
    }

    // 用 d3d9：目标进程初始化最快，且 kiero 的 d3d9 定位不依赖设备创建时序。
    StartHost(kApiD3D9, kGfxHostExeNameX64);

    ExpectFreshFrames(kApiD3D9);

    // 解绑走的是另一条跨位宽路径（助手 --release），同样不能把目标带崩。
    long ret = 0;
    op_.UnBindWindow(&ret);
    bound_ = false;
    PumpMessagesFor(300);
    EXPECT_NE(::WaitForSingleObject(host_process_, 0), WAIT_OBJECT_0)
        << "跨位宽解绑后目标进程被带崩了";
}
#endif

// ---------------------------------------------------------------------------
// §3.2 只搬请求区域：五个后端的区域读回必须和整窗读回逐字节一致，
// d3d9/opengl 再用顶部标记带的下边界验一次行索引（GL 走逐行翻转，DX 不翻转）。
// ---------------------------------------------------------------------------
TEST_F(HookCaptureGfxE2ETest, D3D9RegionCaptureMatchesFullFrame) {
    StartHost(kApiD3D9);
    ExpectRegionMatchesFullFrame(kApiD3D9);
}

TEST_F(HookCaptureGfxE2ETest, D3D10RegionCaptureMatchesFullFrame) {
    StartHost(kApiD3D10);
    ExpectRegionMatchesFullFrame(kApiD3D10);
}

// d3d11/d3d12 的注入侧区域路径和 d3d9/d3d10 是两份独立实现（d3d11 走
// CopySubresourceRegion 把矩形摆到 staging 左上角、d3d12 走自建队列的
// CopyTextureRegion + 回读），所以必须各自被真注入验一遍，不能靠别的后端代表。
TEST_F(HookCaptureGfxE2ETest, D3D11RegionCaptureMatchesFullFrame) {
    StartHost(kApiD3D11, kHostExeName);
    ExpectRegionMatchesFullFrame(kApiD3D11);
}

TEST_F(HookCaptureGfxE2ETest, D3D12RegionCaptureMatchesFullFrame) {
    StartHost(kApiD3D12);
    ExpectRegionMatchesFullFrame(kApiD3D12);
}

TEST_F(HookCaptureGfxE2ETest, OpenGLRegionCaptureMatchesFullFrame) {
    StartHost(kApiOpenGL);
    ExpectRegionMatchesFullFrame(kApiOpenGL);
}

TEST_F(HookCaptureGfxE2ETest, D3D9RegionCaptureKeepsMarkerBandBoundary) {
    StartHost(kApiD3D9);
    ExpectMarkerBandBoundaryInRegion(kApiD3D9);
}

TEST_F(HookCaptureGfxE2ETest, OpenGLRegionCaptureKeepsMarkerBandBoundary) {
    StartHost(kApiOpenGL);
    ExpectMarkerBandBoundaryInRegion(kApiOpenGL);
}

// 一个进程里两条交换链、各自在不同线程 Present —— 真游戏/编辑器常见的形状。
// 注入侧 D3D12Capture 是单例（一条拷贝队列 + 一套命令列表/回读缓冲），两条链并发
// 进 Present 钩子时那套状态是共享的：这里验的就是它不会串帧、不会崩。
// 两个窗口用同一套轮换色和同一条白色标记带，所以无论共享段最后落的是哪条链的帧，
// 读到的都必须是"顶部白 + 底部轮换色"，且多次采样能看到不止一种颜色。
TEST_F(HookCaptureGfxE2ETest, D3D12TwoSwapChainsKeepsReadingFrames) {
    StartHost(kApiD3D12, kGfxHostExeName, L"--d3d12-extra-swap");
    ExpectFreshFrames(kApiD3D12);
}

namespace {

HWND g_enum_skip_window = nullptr;
HWND g_enum_found_window = nullptr;
const wchar_t *g_enum_class = nullptr;

BOOL CALLBACK find_other_host_window(HWND wnd, LPARAM) {
    if (wnd == g_enum_skip_window || !g_enum_class) {
        return TRUE;
    }
    wchar_t cls[64] = {};
    ::GetClassNameW(wnd, cls, 64);
    if (wcscmp(cls, g_enum_class) == 0) {
        g_enum_found_window = wnd;
        return FALSE;
    }
    return TRUE;
}

} // namespace

// 同一个 Op 对象从 A 窗口换绑到 B 窗口：换绑时 BindingSession 会先 reset_bind_state()
// （内部 _capture->UnBind() → 远端 ReleaseDisplayHook），再为 B 建新的 HookCapture。
// 这条用例验的就是"换目标之后读到的必须是 B 的帧"，而不是 A 的残留：
// 两个目标程序都是 d3d11、都在轮换清屏色，所以判据是"换绑后仍能读到不止一种颜色"，
// 再加一条"两个窗口属于不同进程"，确保换的确实是另一个目标。
TEST_F(HookCaptureGfxE2ETest, D3D11RebindToAnotherWindowReadsNewTarget) {
    // d3d11 的目标程序是单独的 d3d11_host_app.exe（不吃 --api=），所以这里显式指定。
    StartHost(kApiD3D11, kHostExeName);
    if (!host_window_) {
        return; // StartHost 已经报过致命错误
    }
    long ret = 0;
    op_.SetShowErrorMsg(3, &ret);
    op_.BindWindow((long)(intptr_t)host_window_, kApiD3D11.render_type, L"normal", L"normal", 0, &ret);
    ASSERT_EQ(ret, 1) << "首次绑定失败";
    bound_ = true;
    PumpMessagesFor(1200);

    DWORD first_pid = 0;
    ::GetWindowThreadProcessId(host_window_, &first_pid);

    // 第二个目标进程：同一个类名/标题，所以只能按"句柄不等于 A"来找。
    const std::wstring dir = ExeDir();
    std::wstring exe = dir + L"\\" + kHostExeName;
    std::wstring cmdline = L"\"" + exe + L"\" --seconds=120 --width=640 --height=360";
    STARTUPINFOW si = {sizeof(si)};
    PROCESS_INFORMATION pi = {};
    ASSERT_TRUE(::CreateProcessW(nullptr, cmdline.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                                 dir.c_str(), &si, &pi))
        << "CreateProcessW 失败 err=" << ::GetLastError();
    HANDLE second_process = pi.hProcess;
    if (pi.hThread) {
        ::CloseHandle(pi.hThread);
    }

    g_enum_skip_window = host_window_;
    g_enum_class = kApiD3D11.window_class;
    g_enum_found_window = nullptr;
    const ULONGLONG deadline = ::GetTickCount64() + 20000;
    while (::GetTickCount64() < deadline && !g_enum_found_window) {
        ::EnumWindows(find_other_host_window, 0);
        if (!g_enum_found_window) {
            ::Sleep(50);
        }
    }
    HWND second_window = g_enum_found_window;
    ASSERT_TRUE(second_window != nullptr) << "第二个目标窗口 20s 内没有出现";

    DWORD second_pid = 0;
    ::GetWindowThreadProcessId(second_window, &second_pid);
    EXPECT_NE(first_pid, second_pid) << "两个窗口属于同一个进程，换绑用例的前提不成立";

    // 不解绑，直接换绑到 B。
    ret = 0;
    op_.BindWindow((long)(intptr_t)second_window, kApiD3D11.render_type, L"normal", L"normal", 0, &ret);
    ASSERT_EQ(ret, 1) << "换绑到第二个窗口失败";
    host_window_ = second_window; // TearDown 解绑用
    PumpMessagesFor(1500);

    std::set<std::wstring> seen;
    for (int i = 0; i < 6; ++i) {
        std::wstring color;
        op_.GetColor(320, 180, color);
        ASSERT_EQ(color.size(), 6u) << "换绑后第 " << i << " 次 GetColor 没拿到颜色";
        if (HostColors().count(color)) {
            seen.insert(color);
        }
        PumpMessagesFor(600);
    }
    EXPECT_GT(seen.size(), 1u) << "换绑后读不到第二个窗口的新帧（读到的是旧目标的残留？）";

    // 先解绑再杀目标：否则 TearDown 里的解绑会对着已经销毁的窗口发远端 ReleaseDisplayHook
    // （GetWindowThreadProcessId 拿不到 pid，BlackBone Attach(0) 直接报 C000000B）。
    ret = 0;
    op_.UnBindWindow(&ret);
    bound_ = false;

    ::TerminateProcess(second_process, 0);
    ::WaitForSingleObject(second_process, 3000);
    ::CloseHandle(second_process);
}
