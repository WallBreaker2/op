#include <gtest/gtest.h>

#include <atlbase.h>
#include <oaidl.h>
#include <windows.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// MIDL 生成的接口声明（tests/CMakeLists.txt 已把 OP_COM_GENERATED_DIR 加进 include）。
// 这里直接走 IOpAutomation 的 vtable，不用 IDispatch::Invoke —— 后者需要注册 typelib，
// 而本测试刻意**不注册 DLL**：op_x*.dll 导出了 DllGetClassObject，拿 IClassFactory 即可建对象。
#include "op_i.h"

namespace {

// 1x1 32bpp 的最小 BMP，用来驱动 LoadMemPic -> Image::read -> Image::create 这条链。
std::vector<unsigned char> MakeMinimalBmp32() {
    std::vector<unsigned char> data(58, 0);
    data[0] = 'B';
    data[1] = 'M';
    const uint32_t file_size = static_cast<uint32_t>(data.size());
    const uint32_t off_bits = 54;
    std::memcpy(&data[2], &file_size, sizeof(file_size));
    std::memcpy(&data[10], &off_bits, sizeof(off_bits));
    const uint32_t header_size = 40;
    std::memcpy(&data[14], &header_size, sizeof(header_size));
    const int32_t width = 1;
    const int32_t height = 1;
    std::memcpy(&data[18], &width, sizeof(width));
    std::memcpy(&data[22], &height, sizeof(height));
    const uint16_t planes = 1;
    std::memcpy(&data[26], &planes, sizeof(planes));
    const uint16_t bpp = 32;
    std::memcpy(&data[28], &bpp, sizeof(bpp));
    const uint32_t image_size = 4;
    std::memcpy(&data[34], &image_size, sizeof(image_size));
    data[54] = 0;   // B
    data[55] = 0;   // G
    data[56] = 255; // R
    data[57] = 0;   // A
    return data;
}

bool ForceImageThrowRequested() {
    wchar_t value[8] = {};
    const DWORD len = ::GetEnvironmentVariableW(L"OP_TEST_FORCE_IMAGE_THROW", value, 8);
    return len > 0 && value[0] == L'1';
}

const wchar_t *OpDllName() {
#ifdef _WIN64
    return L"op_x64.dll";
#else
    return L"op_x86.dll";
#endif
}

class ComAutomation {
  public:
    bool Create() {
        module_ = ::LoadLibraryW(OpDllName());
        if (!module_) {
            last_error = "LoadLibrary failed: " + std::to_string(::GetLastError());
            return false;
        }
        using GetClassObjectFn = HRESULT(STDAPICALLTYPE *)(REFCLSID, REFIID, void **);
        auto get_class_object = reinterpret_cast<GetClassObjectFn>(::GetProcAddress(module_, "DllGetClassObject"));
        if (!get_class_object) {
            last_error = "DllGetClassObject not exported";
            return false;
        }
        CComPtr<IClassFactory> factory;
        HRESULT hr = get_class_object(__uuidof(OpAutomation), __uuidof(IClassFactory),
                                      reinterpret_cast<void **>(&factory));
        if (FAILED(hr) || !factory) {
            last_error = "DllGetClassObject failed";
            return false;
        }
        hr = factory->CreateInstance(nullptr, __uuidof(IOpAutomation), reinterpret_cast<void **>(&automation_));
        if (FAILED(hr) || !automation_) {
            last_error = "CreateInstance failed";
            return false;
        }
        return true;
    }

    ~ComAutomation() {
        automation_.Release();
        if (module_) {
            ::FreeLibrary(module_);
        }
    }

    CComPtr<IOpAutomation> automation_;
    std::string last_error;

  private:
    HMODULE module_ = nullptr;
};

} // namespace

TEST(ComAutomationTest, CreatesObjectWithoutRegistrationAndReturnsVersion) {
    ComAutomation automation;
    ASSERT_TRUE(automation.Create()) << automation.last_error;

    CComBSTR version;
    ASSERT_TRUE(SUCCEEDED(automation.automation_->Ver(&version))) << "Ver() 调用失败";
    EXPECT_GT(version.Length(), 0u);
}

// 脚本传 Null（BSTR == NULL）时，门面层会把它当空串（见 defect_fixes.md §8）。
// 这里从真正的 COM 入口再验一遍：不许崩进程，并且对象之后仍然可用。
TEST(ComAutomationTest, NullBstrArgumentDoesNotCrashHost) {
    ComAutomation automation;
    ASSERT_TRUE(automation.Create()) << automation.last_error;

    long ret = -1;
    const HRESULT hr = automation.automation_->SetPath(nullptr, &ret);
    EXPECT_TRUE(SUCCEEDED(hr)) << "SetPath(NULL) 不该失败到 HRESULT";

    CComBSTR version;
    EXPECT_TRUE(SUCCEEDED(automation.automation_->Ver(&version))) << "调用后对象被破坏了";
}

// 注意：FindWindow(NULL, NULL) 按 Win32 语义会返回"任意一个顶层窗口"，不是 0，
// 所以这里只能断言"不崩且返回的是合法窗口或 0"。
TEST(ComAutomationTest, FindWindowWithNullBstrDoesNotCrash) {
    ComAutomation automation;
    ASSERT_TRUE(automation.Create()) << automation.last_error;

    LONGLONG hwnd = -1;
    EXPECT_TRUE(SUCCEEDED(automation.automation_->FindWindow(nullptr, nullptr, &hwnd)));
    EXPECT_TRUE(hwnd == 0 || ::IsWindow(reinterpret_cast<HWND>(hwnd)) != FALSE);

    // FindWindowEx 同一家族，同样可能在入口 [0] 解引用
    LONGLONG child = -1;
    EXPECT_TRUE(SUCCEEDED(automation.automation_->FindWindowEx(reinterpret_cast<LONGLONG>(::GetDesktopWindow()),
                                                               nullptr, nullptr, &child)));
}

// FindWindowByProcess 内部会多次 wcslen(class_name)/wcslen(title)，
// BSTR 为 NULL 时会在 wcslen 上直接崩（本轮修复点）。
TEST(ComAutomationTest, FindWindowByProcessWithNullFiltersDoesNotCrash) {
    ComAutomation automation;
    ASSERT_TRUE(automation.Create()) << automation.last_error;

    wchar_t exe_path[MAX_PATH] = {};
    ASSERT_NE(::GetModuleFileNameW(nullptr, exe_path, MAX_PATH), 0u);
    const wchar_t *exe_name = wcsrchr(exe_path, L'\\');
    exe_name = exe_name ? exe_name + 1 : exe_path;

    LONGLONG hwnd = -1;
    // 进程名合法、class/title 为 NULL：会走到 wcslen(class_name)/wcslen(title) 那段
    EXPECT_TRUE(SUCCEEDED(
        automation.automation_->FindWindowByProcess(CComBSTR(exe_name), nullptr, nullptr, &hwnd)));

    // 之后对象仍然可用
    CComBSTR version;
    EXPECT_TRUE(SUCCEEDED(automation.automation_->Ver(&version)));
}

// GetCursorPos 的输出是 VARIANT*：自动化层先写 vt = VT_I4，再把 &lVal 传下去。
// 用 VT_EMPTY 这类"不寻常但脚本能给"的槽位，确认不会踩空。
TEST(ComAutomationTest, GetCursorPosToleratesUnexpectedOutputVariants) {
    ComAutomation automation;
    ASSERT_TRUE(automation.Create()) << automation.last_error;

    VARIANT x;
    VARIANT y;
    ::VariantInit(&x);
    ::VariantInit(&y);
    x.vt = VT_EMPTY;
    y.vt = VT_EMPTY;

    long ret = -1;
    EXPECT_TRUE(SUCCEEDED(automation.automation_->GetCursorPos(&x, &y, &ret)));
    ::VariantClear(&x);
    ::VariantClear(&y);

    CComBSTR version;
    EXPECT_TRUE(SUCCEEDED(automation.automation_->Ver(&version)));
}

// 这一类修的是"参数进 std::wstring 之前没判空"：SendString / SendStringIme 的形参是
// const std::wstring&，RunApp 同；WinExec 走 _ws2string（内部构造 wstring）；
// InjectDll 内部对 dllPath 做 wcslen。BSTR 为 NULL 时原来会 AV。
TEST(ComAutomationTest, NullBstrForStringArgumentsDoesNotCrash) {
    ComAutomation automation;
    ASSERT_TRUE(automation.Create()) << automation.last_error;

    const LONGLONG hwnd = reinterpret_cast<LONGLONG>(::GetDesktopWindow());
    long ret = -1;

    EXPECT_TRUE(SUCCEEDED(automation.automation_->SetWindowText(hwnd, nullptr, &ret)));
    EXPECT_TRUE(SUCCEEDED(automation.automation_->SendString(hwnd, nullptr, &ret)));
    EXPECT_TRUE(SUCCEEDED(automation.automation_->SendStringIme(hwnd, nullptr, &ret)));

    ULONG pid = 0;
    long run_ret = -1;
    EXPECT_TRUE(SUCCEEDED(automation.automation_->RunApp(nullptr, 0, &pid, &run_ret)));
    EXPECT_EQ(run_ret, 0) << "空命令行不该启动进程";

    long exec_ret = -1;
    EXPECT_TRUE(SUCCEEDED(automation.automation_->WinExec(nullptr, 0, &exec_ret)));

    long inject_ret = -1;
    EXPECT_TRUE(SUCCEEDED(automation.automation_->InjectDll(CComBSTR(L"op_test.exe"), nullptr, &inject_ret)));

    CComBSTR version;
    EXPECT_TRUE(SUCCEEDED(automation.automation_->Ver(&version))) << "调用后对象被破坏了";
}

// 异常跨 COM 边界的红/绿探针。
// 设了 OP_TEST_FORCE_IMAGE_THROW=1 时，Image::create 会故意抛出（见 libop/image/Image.h）。
// 注意不能走 LoadMemPic：ImageSearchService::LoadMemPic 自己已经有 try/catch（异常在那里就被吃了）。
// 这里走 GetColor -> requestCapture -> 后端 img.create 这条没有任何中间 catch 的链路。
TEST(ComAutomationTest, ImageCreateThrowIsConvertedToHresult) {
    ComAutomation automation;
    ASSERT_TRUE(automation.Create()) << automation.last_error;

    long bind_ret = 0;
    ASSERT_TRUE(SUCCEEDED(automation.automation_->BindWindow(
        reinterpret_cast<LONGLONG>(::GetDesktopWindow()), CComBSTR(L"normal"), CComBSTR(L"windows"),
        CComBSTR(L"windows"), 0, &bind_ret)));
    ASSERT_EQ(bind_ret, 1) << "绑定桌面窗口失败，无法走到取帧路径";

    CComBSTR color;
    const HRESULT hr = automation.automation_->GetColor(0, 0, &color);

    if (ForceImageThrowRequested()) {
        EXPECT_EQ(hr, static_cast<HRESULT>(E_UNEXPECTED)) << "异常应被兜成 E_UNEXPECTED，而不是穿出 COM";
    } else {
        EXPECT_TRUE(SUCCEEDED(hr)) << "正常路径不该返回失败";
        EXPECT_GT(color.Length(), 0u) << "正常路径应取到颜色";
    }

    long unbind_ret = 0;
    EXPECT_TRUE(SUCCEEDED(automation.automation_->UnBindWindow(&unbind_ret)));

    CComBSTR version;
    EXPECT_TRUE(SUCCEEDED(automation.automation_->Ver(&version))) << "调用后对象被破坏了";
}

// FindColor / FindPic 走的是同一条"先截图再匹配"的链路，同样会碰 Image::create。
// 用同一个注入开关确认它们也把异常兜成 HRESULT。
TEST(ComAutomationTest, FindColorAndFindPicConvertImageThrow) {
    ComAutomation automation;
    ASSERT_TRUE(automation.Create()) << automation.last_error;

    long bind_ret = 0;
    ASSERT_TRUE(SUCCEEDED(automation.automation_->BindWindow(
        reinterpret_cast<LONGLONG>(::GetDesktopWindow()), CComBSTR(L"normal"), CComBSTR(L"windows"),
        CComBSTR(L"windows"), 0, &bind_ret)));
    ASSERT_EQ(bind_ret, 1) << "绑定桌面窗口失败";

    VARIANT cx;
    VARIANT cy;
    ::VariantInit(&cx);
    ::VariantInit(&cy);
    long find_ret = -1;
    const HRESULT hr_color =
        automation.automation_->FindColor(0, 0, 64, 64, CComBSTR(L"FFFFFF"), 1.0, 0, &cx, &cy, &find_ret);

    VARIANT px;
    VARIANT py;
    ::VariantInit(&px);
    ::VariantInit(&py);
    long pic_ret = -1;
    const HRESULT hr_pic = automation.automation_->FindPic(0, 0, 64, 64, CComBSTR(L"no_such_pic_name"),
                                                           CComBSTR(L""), 1.0, 0, &px, &py, &pic_ret);

    if (ForceImageThrowRequested()) {
        EXPECT_EQ(hr_color, static_cast<HRESULT>(E_UNEXPECTED)) << "FindColor 的异常没被兜住";
        EXPECT_EQ(hr_pic, static_cast<HRESULT>(E_UNEXPECTED)) << "FindPic 的异常没被兜住";
    } else {
        EXPECT_TRUE(SUCCEEDED(hr_color));
        EXPECT_TRUE(SUCCEEDED(hr_pic));
    }

    ::VariantClear(&cx);
    ::VariantClear(&cy);
    ::VariantClear(&px);
    ::VariantClear(&py);

    long unbind_ret = 0;
    EXPECT_TRUE(SUCCEEDED(automation.automation_->UnBindWindow(&unbind_ret)));

    CComBSTR version;
    EXPECT_TRUE(SUCCEEDED(automation.automation_->Ver(&version))) << "调用后对象被破坏了";
}

// Capture 是"打开对象后最容易被调用"的一条分配路径：Capture -> requestCapture -> Image::create。
// 包裹前它的异常会直接穿出 COM 方法边界（客户端没有处理器时进程直接死），包裹后必须变成 HRESULT。
TEST(ComAutomationTest, CaptureConvertsImageThrowToHresult) {
    ComAutomation automation;
    ASSERT_TRUE(automation.Create()) << automation.last_error;

    long bind_ret = 0;
    ASSERT_TRUE(SUCCEEDED(automation.automation_->BindWindow(
        reinterpret_cast<LONGLONG>(::GetDesktopWindow()), CComBSTR(L"normal"), CComBSTR(L"windows"),
        CComBSTR(L"windows"), 0, &bind_ret)));
    ASSERT_EQ(bind_ret, 1) << "绑定桌面窗口失败";

    wchar_t temp_dir[MAX_PATH] = {};
    ASSERT_NE(::GetTempPathW(MAX_PATH, temp_dir), 0u);
    // 必须带 .bmp 扩展名：保存链路按扩展名挑编码器，GetTempFileNameW 生成的 .tmp 会保存失败。
    wchar_t temp_file[MAX_PATH] = {};
    ASSERT_NE(::swprintf_s(temp_file, L"%sop_capture_%lu.bmp", temp_dir, ::GetCurrentProcessId()), -1);
    ::DeleteFileW(temp_file);

    // 预置成一个"看起来成功"的值：异常路径必须把它清零，否则只看 ret 的调用方会误判成功。
    long cap_ret = 1;
    const HRESULT hr = automation.automation_->Capture(0, 0, 32, 32, CComBSTR(temp_file), &cap_ret);

    if (ForceImageThrowRequested()) {
        EXPECT_EQ(hr, static_cast<HRESULT>(E_UNEXPECTED)) << "Capture 的异常没被兜住，会穿出 COM";
        EXPECT_EQ(cap_ret, 0) << "异常路径没有把 ret 清零，调用方会读到上一次的值";
    } else {
        EXPECT_TRUE(SUCCEEDED(hr)) << "正常路径不该返回失败";
        EXPECT_EQ(cap_ret, 1) << "正常路径应截图成功";
        WIN32_FILE_ATTRIBUTE_DATA attrs = {};
        EXPECT_TRUE(::GetFileAttributesExW(temp_file, GetFileExInfoStandard, &attrs));
        EXPECT_GT(attrs.nFileSizeLow, 1024u) << "截图文件过小";
    }

    ::DeleteFileW(temp_file);

    long unbind_ret = 0;
    EXPECT_TRUE(SUCCEEDED(automation.automation_->UnBindWindow(&unbind_ret)));

    CComBSTR version;
    EXPECT_TRUE(SUCCEEDED(automation.automation_->Ver(&version))) << "调用后对象被破坏了";
}

// 批量内存 API 的 COM 形状：入参是竖线分隔的地址串，出参是竖线分隔的值串（失败项留空）。
// 这里从真正的 COM 入口走一遍，锁住"解析 → 批量读 → 拼回字符串"这条链。
TEST(ComAutomationTest, ReadIntBatchRoundTripsThroughPipeSeparatedBstr) {
    ComAutomation automation;
    ASSERT_TRUE(automation.Create()) << automation.last_error;

    // 造一个自己的窗口，让门面能把目标进程解析成本进程。
    HWND hwnd = ::CreateWindowExW(0, L"STATIC", L"op com batch", WS_OVERLAPPEDWINDOW, 0, 0, 120, 80, nullptr,
                                  nullptr, ::GetModuleHandleW(nullptr), nullptr);
    ASSERT_NE(hwnd, nullptr) << "测试窗口创建失败";

    const auto *module_base = reinterpret_cast<const wchar_t *>(::GetModuleHandleW(nullptr));
    ASSERT_NE(module_base, nullptr);
    wchar_t address[32] = {};
    swprintf_s(address, L"%llX", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(module_base)));

    // 两个有效地址 + 一个坏地址：坏项必须留空，且不影响前两项。
    CComBSTR input;
    input.Append(address);
    input.Append(L"|");
    input.Append(address);
    input.Append(L"|0");

    CComBSTR result;
    const HRESULT hr = automation.automation_->ReadIntBatch(reinterpret_cast<LONGLONG>(hwnd), input, 1, &result);
    EXPECT_TRUE(SUCCEEDED(hr)) << "ReadIntBatch 不该失败到 HRESULT";

    const std::wstring got(result ? result.m_str : L"");
    std::string got_narrow;
    for (wchar_t ch : got)
        got_narrow.push_back(ch >= 0x20 && ch < 0x7F ? static_cast<char>(ch) : '?');
    // 'MZ' = 0x5A4D = 23117，两个有效项都该是它，坏项留空。
    EXPECT_EQ(got, std::wstring(L"23117|23117|")) << "实际返回：" << got_narrow;

    ::DestroyWindow(hwnd);
}
