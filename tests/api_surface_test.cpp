// API 表面覆盖：审计 R-10 指出「C API 216 导出只调 19 个（8.8%）」，这里按组把
// 从未被任何用例碰过的公开方法补上。文件按组拆成多个 TEST_F，每组只依赖一个
// 内存窗口/本进程，不引入新的目标程序。
//
// 组 1：鼠标扩展按键（中键 / 右键 / XButton2）—— 缺口 9 个
//        MiddleClick/MiddleDown/MiddleUp/RightDown/RightUp/
//        XButton2Click/XButton2DoubleClick/XButton2Down/XButton2Up
// 说明：MouseEventWindow 已经区分 xbutton1/xbutton2（见 test_support.cpp 的
//       WM_XBUTTONDOWN 分支），所以断言可以精确到具体按键，而不是"有事件就算过"。

#include "test_support.h"

#include "op_c_api.h"
#include <vector>
#include <cstring>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <string>

namespace {

using test_support::MouseEventWindow;

class ApiSurfaceTest : public ::testing::Test {
  protected:
    void SetUp() override { op_.SetShowErrorMsg(3, &ret_); }

    void TearDown() override {
        if (bound_) {
            long unbind_ret = 0;
            op_.UnBindWindow(&unbind_ret);
            bound_ = false;
        }
    }

    op::Op op_;
    long ret_ = 0;
    bool bound_ = false;
};

// 中键 / 右键 / XButton2 的按下抬起必须落到目标窗口的对应消息上。
// 这条用例的判据是"计数器精确等于 1"而不是"大于 0"：SendMessage 模式下每次
// 调用只应产生一条消息，多出来就说明产品重复投递了。
TEST_F(ApiSurfaceTest, ExtendedMouseButtonsReachTargetWindow) {
    MouseEventWindow window;
    ASSERT_TRUE(window.Create()) << "目标窗口创建失败";

    op_.BindWindow((long)(intptr_t)window.hwnd, L"normal", L"windows", L"windows", 0, &ret_);
    ASSERT_EQ(ret_, 1) << "BindWindow(normal/windows) 失败";
    bound_ = true;

    window.ResetCounts();
    op_.MiddleDown(&ret_);
    EXPECT_EQ(ret_, 1);
    op_.MiddleUp(&ret_);
    EXPECT_EQ(ret_, 1);
    EXPECT_EQ(window.middle_down, 1) << "中键按下没有到达目标窗口";
    EXPECT_EQ(window.middle_up, 1) << "中键抬起没有到达目标窗口";

    window.ResetCounts();
    op_.RightDown(&ret_);
    EXPECT_EQ(ret_, 1);
    op_.RightUp(&ret_);
    EXPECT_EQ(ret_, 1);
    EXPECT_EQ(window.right_down, 1);
    EXPECT_EQ(window.right_up, 1);

    window.ResetCounts();
    op_.XButton2Down(&ret_);
    EXPECT_EQ(ret_, 1);
    op_.XButton2Up(&ret_);
    EXPECT_EQ(ret_, 1);
    EXPECT_EQ(window.xbutton2_down, 1) << "XBUTTON2 按下丢了（可能是 XBUTTON1）";
    EXPECT_EQ(window.xbutton2_up, 1);
    EXPECT_EQ(window.xbutton1_down, 0) << "XBUTTON2 被错投成了 XBUTTON1";
}

// 中键双击与 XButton2 双击走的是 WM_*BUTTONDBLCLK 分支，和单点消息不是同一条路径。
TEST_F(ApiSurfaceTest, ExtendedMouseDoubleClicksProduceDoubleClickMessages) {
    MouseEventWindow window;
    ASSERT_TRUE(window.Create());

    op_.BindWindow((long)(intptr_t)window.hwnd, L"normal", L"windows", L"windows", 0, &ret_);
    ASSERT_EQ(ret_, 1);
    bound_ = true;

    window.ResetCounts();
    op_.MiddleDoubleClick(&ret_);
    EXPECT_EQ(ret_, 1);
    EXPECT_GE(window.middle_double, 1) << "中键双击没有产生 WM_MBUTTONDBLCLK";

    window.ResetCounts();
    op_.XButton2DoubleClick(&ret_);
    EXPECT_EQ(ret_, 1);
    EXPECT_GE(window.xbutton2_double, 1) << "XBUTTON2 双击没有产生 WM_XBUTTONDBLCLK";

    // 单击入口（与 Down/Up 分属两条实现路径，必须各自验证）
    window.ResetCounts();
    op_.MiddleClick(&ret_);
    EXPECT_EQ(ret_, 1);
    EXPECT_EQ(window.middle_down, 1) << "MiddleClick 没有产生按下";
    EXPECT_EQ(window.middle_up, 1) << "MiddleClick 没有产生抬起";

    window.ResetCounts();
    op_.XButton2Click(&ret_);
    EXPECT_EQ(ret_, 1);
    EXPECT_EQ(window.xbutton2_down, 1) << "XButton2Click 没有产生 XBUTTON2 按下";
    EXPECT_EQ(window.xbutton2_up, 1);
}

// 组 2：内存标量读写（缺口 6 个）+ 绑定状态 / 延时 / 图色缓存（缺口 3 个）
// hwnd=0 且未绑定时 resolve_memory_hwnd 返回 0 → ProcessMemory 走本进程 memcpy
// 路径，不涉及跨进程 attach。地址按十六进制解析（ProcessMemory::str2address
// 里是 wcstoull(...,16)），所以这里必须写成 0xNNN 形式。
TEST_F(ApiSurfaceTest, ScalarMemoryAndStateApisWork) {
    float f = 1.25f;
    wchar_t addr[32] = {0};
    std::swprintf(addr, 32, L"0x%llX", (unsigned long long)(uintptr_t)&f);

    float rf = 0.0f;
    op_.ReadFloat(0, addr, &rf);
    EXPECT_FLOAT_EQ(rf, 1.25f) << "ReadFloat 没读到本进程的值";

    op_.WriteFloat(0, addr, -3.5f, &ret_);
    EXPECT_EQ(ret_, 1);
    EXPECT_FLOAT_EQ(f, -3.5f) << "WriteFloat 没写回本进程";

    double d = 2.5;
    std::swprintf(addr, 32, L"0x%llX", (unsigned long long)(uintptr_t)&d);
    double rd = 0.0;
    op_.ReadDouble(0, addr, &rd);
    EXPECT_DOUBLE_EQ(rd, 2.5);
    op_.WriteDouble(0, addr, -0.125, &ret_);
    EXPECT_EQ(ret_, 1);
    EXPECT_DOUBLE_EQ(d, -0.125);

    // type=1 是 UTF-16，len 是字节数
    const wchar_t *text = L"OP-API-SURFACE";
    const long text_bytes = (long)((wcslen(text) + 1) * sizeof(wchar_t));
    std::swprintf(addr, 32, L"0x%llX", (unsigned long long)(uintptr_t)text);
    std::wstring read_back;
    op_.ReadString(0, addr, 1, text_bytes, read_back);
    EXPECT_EQ(read_back, text) << "ReadString(type=1) 读 UTF-16 不对";

    wchar_t buf[64] = {0};
    std::swprintf(addr, 32, L"0x%llX", (unsigned long long)(uintptr_t)buf);
    op_.WriteString(0, addr, 1, L"hello-op", &ret_);
    EXPECT_EQ(ret_, 1);
    EXPECT_STREQ(buf, L"hello-op") << "WriteString(type=1) 没写成 UTF-16";

    long is_bind = -1;
    op_.IsBind(&is_bind);
    EXPECT_EQ(is_bind, 0) << "未绑定时 IsBind 应为 0";

    op_.EnablePicCache(1, &ret_);
    EXPECT_EQ(ret_, 1);
    op_.EnablePicCache(0, &ret_);
    EXPECT_EQ(ret_, 1);

    // Delays 是随机延时：验证落在区间内，而不是"睡够了就行"
    const auto t0 = std::chrono::steady_clock::now();
    op_.Delays(10, 20, &ret_);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - t0)
                             .count();
    EXPECT_EQ(ret_, 1);
    EXPECT_GE(elapsed, 10) << "Delays 的实际延时小于下界";
    EXPECT_LE(elapsed, 300) << "Delays 的实际延远超上界（调度抖动不该有这么大）";

    // 绑定后 IsBind 必须是 1，否则脚本无法判断当前状态
    MouseEventWindow window;
    ASSERT_TRUE(window.Create());
    op_.BindWindow((long)(intptr_t)window.hwnd, L"normal", L"windows", L"windows", 0, &ret_);
    ASSERT_EQ(ret_, 1);
    bound_ = true;
    op_.IsBind(&is_bind);
    EXPECT_EQ(is_bind, 1) << "绑定后 IsBind 仍为 0";
}

// 组 3：进程 / 窗口查询（缺口 12 个）。判据尽量对齐 Win32 的权威结果，
// 而不是"调用了不崩"：EnumProcess 里必须有自己的 pid、FindWindowByProcessId
// 必须返回自己的窗口、GetWindow 必须与 ::GetWindow 一致。
TEST_F(ApiSurfaceTest, ProcessAndWindowQueriesReflectReality) {
    const DWORD self_pid = ::GetCurrentProcessId();

    std::wstring pids;
    op_.EnumProcess(L"op_test.exe", pids);
    ASSERT_FALSE(pids.empty()) << "EnumProcess 没枚举到自己";
    EXPECT_NE(pids.find(std::to_wstring(self_pid)), std::wstring::npos)
        << "EnumProcess 结果里没有当前 pid，实际：" << pids;

    // GetProcessInfo 的格式是 name|path|cpu|mem（WindowProcess.cpp:206）
    std::wstring info;
    op_.GetProcessInfo((long)self_pid, info);
    ASSERT_FALSE(info.empty()) << "GetProcessInfo 返回空";
    int bars = 0;
    for (wchar_t c : info) {
        if (c == L'|')
            ++bars;
    }
    EXPECT_EQ(bars, 3) << "GetProcessInfo 应该是四段，实际：" << info;

    std::wstring op_path;
    op_.GetPath(op_path);
    // curr_path 由 SetPath 决定；未 SetPath 时是空串。这里锁"不崩且不是垃圾指针内容"
    EXPECT_TRUE(op_path.empty() || op_path.find(L'\\') != std::wstring::npos)
        << "GetPath 返回了不像路径的内容：" << op_path;

    MouseEventWindow window;
    ASSERT_TRUE(window.Create());
    wchar_t class_name[128] = {0};
    ::GetClassNameW(window.hwnd, class_name, 128);

    long pid = 0;
    op_.GetWindowProcessId((LONG_PTR)window.hwnd, &pid);
    EXPECT_EQ(pid, (long)self_pid);

    std::wstring proc_path;
    op_.GetWindowProcessPath((LONG_PTR)window.hwnd, proc_path);
    ASSERT_FALSE(proc_path.empty()) << "GetWindowProcessPath 返回空";
    EXPECT_NE(proc_path.find(L"op_test.exe"), std::wstring::npos)
        << "GetWindowProcessPath 返回：" << proc_path;

    LONG_PTR found = 0;
    op_.FindWindowByProcessId((long)self_pid, class_name, L"", &found);
    EXPECT_EQ(found, (LONG_PTR)window.hwnd) << "FindWindowByProcessId 没定位到自己的窗口";

    // flag 语义见 WindowState.cpp:135（0=存在 2=可见 3=最小化 7=可用）
    long state = -1;
    op_.GetWindowState((LONG_PTR)window.hwnd, 0, &state);
    EXPECT_EQ(state, 1) << "flag=0 判断窗口存在，应为 1";
    op_.GetWindowState((LONG_PTR)window.hwnd, 3, &state);
    EXPECT_EQ(state, 0) << "新建窗口不该是最小化";
    op_.GetWindowState((LONG_PTR)window.hwnd, 7, &state);
    EXPECT_EQ(state, 1) << "新建窗口默认可用";

    // 无效 hwnd 必须回 0，而不是留调用方原值
    op_.GetWindowState(0, 0, &state);
    EXPECT_EQ(state, 0) << "无效 hwnd 的 GetWindowState 没归零";

    // flag 语义见 WindowState.cpp:95（0=父 6=owner）
    LONG_PTR parent = 0;
    op_.GetWindow((LONG_PTR)window.hwnd, 0, &parent);
    EXPECT_EQ(parent, (LONG_PTR)::GetParent(window.hwnd));
    LONG_PTR owner = 12345;
    op_.GetWindow((LONG_PTR)window.hwnd, 6, &owner);
    EXPECT_EQ(owner, (LONG_PTR)::GetWindow(window.hwnd, GW_OWNER));

    // 0=桌面 1=任务栏；未定义 flag 必须回 0
    LONG_PTR desktop = 0;
    op_.GetSpecialWindow(0, &desktop);
    EXPECT_EQ(desktop, (LONG_PTR)::GetDesktopWindow());
    LONG_PTR tray = 0;
    op_.GetSpecialWindow(1, &tray);
    EXPECT_EQ(tray, (LONG_PTR)::FindWindowW(L"Shell_TrayWnd", nullptr));
    LONG_PTR unknown = 12345;
    op_.GetSpecialWindow(99, &unknown);
    EXPECT_EQ(unknown, 0) << "未定义的 flag 没把出参归零";

    // 把窗口置顶摆到 (0,0)，这样 GetPointWindow 的期望值是确定的
    ::SetWindowPos(window.hwnd, HWND_TOPMOST, 0, 0, 220, 180, SWP_SHOWWINDOW);
    ::Sleep(80);
    LONG_PTR at_point = 0;
    op_.GetPointWindow(10, 10, &at_point);
    EXPECT_TRUE(::IsWindow(reinterpret_cast<HWND>(at_point))) << "GetPointWindow 返回的不是有效窗口";
    EXPECT_EQ(at_point, (LONG_PTR)window.hwnd) << "置顶后 (10,10) 应该落在自己的窗口上";

    LONG_PTR under_mouse = 0;
    op_.GetMousePointWindow(&under_mouse);
    EXPECT_TRUE(under_mouse == 0 || ::IsWindow(reinterpret_cast<HWND>(under_mouse)))
        << "GetMousePointWindow 返回的不是有效窗口";
}

}  // namespace


TEST_F(ApiSurfaceTest, OpenCvImageFileApisWork) {
    // 一半蓝一半红的 40x30 位图：灰度化与裁剪都能产生可验证的差异。
    const int width = 40;
    const int height = 30;
    std::vector<unsigned char> bgra(static_cast<size_t>(width) * height * 4, 0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned char *pixel = &bgra[(static_cast<size_t>(y) * width + x) * 4];
            pixel[3] = 255;
            if (x < width / 2) {
                pixel[0] = 255;  // B
            } else {
                pixel[2] = 255;  // R
            }
        }
    }

    auto write_bmp = [](const std::wstring &bmp_path, int bmp_width, int bmp_height,
                        const std::vector<unsigned char> &pixels) -> bool {
        const auto bytes = test_support::BuildBmp32TopDown(bmp_width, bmp_height, pixels);
        if (bytes.empty()) {
            return false;
        }
        FILE *file = nullptr;
        if (_wfopen_s(&file, bmp_path.c_str(), L"wb") != 0 || file == nullptr) {
            return false;
        }
        const size_t written = fwrite(bytes.data(), 1, bytes.size(), file);
        fclose(file);
        return written == bytes.size();
    };

    const std::wstring source = test_support::GetTempBmpPath(L"api_surface_cv_src.bmp");
    DeleteFileW(source.c_str());
    ASSERT_TRUE(write_bmp(source, width, height, bgra)) << "无法写出测试用 BMP";

    // CvToGray / CvCrop / CvResize：纯文件变换，产物必须真的落到磁盘上。
    const std::wstring gray = test_support::GetTempBmpPath(L"api_surface_cv_gray.bmp");
    const std::wstring cropped = test_support::GetTempBmpPath(L"api_surface_cv_crop.bmp");
    const std::wstring resized = test_support::GetTempBmpPath(L"api_surface_cv_resize.bmp");
    DeleteFileW(gray.c_str());
    DeleteFileW(cropped.c_str());
    DeleteFileW(resized.c_str());

    op_.CvToGray(source.c_str(), gray.c_str(), &ret_);
    EXPECT_EQ(1, ret_) << "CvToGray 失败";
    EXPECT_NE(INVALID_FILE_ATTRIBUTES, GetFileAttributesW(gray.c_str())) << "灰度图没有落盘";

    op_.CvCrop(source.c_str(), 4, 4, 8, 8, cropped.c_str(), &ret_);
    EXPECT_EQ(1, ret_) << "CvCrop 失败";
    EXPECT_NE(INVALID_FILE_ATTRIBUTES, GetFileAttributesW(cropped.c_str())) << "裁剪图没有落盘";

    op_.CvResize(source.c_str(), 20, 15, resized.c_str(), &ret_);
    EXPECT_EQ(1, ret_) << "CvResize 失败";
    EXPECT_NE(INVALID_FILE_ATTRIBUTES, GetFileAttributesW(resized.c_str())) << "缩放图没有落盘";

    // 产物尺寸必须真的等于请求的尺寸（读 BMP 头验证，而不是只看返回值）。
    auto read_bmp_size = [](const std::wstring &bmp_path, int &out_width, int &out_height) -> bool {
        FILE *file = nullptr;
        if (_wfopen_s(&file, bmp_path.c_str(), L"rb") != 0 || file == nullptr) {
            return false;
        }
        unsigned char header[26] = {0};
        const size_t read = fread(header, 1, sizeof(header), file);
        fclose(file);
        if (read < sizeof(header)) {
            return false;
        }
        out_width = header[18] | (header[19] << 8) | (header[20] << 16) | (header[21] << 24);
        out_height = header[22] | (header[23] << 8) | (header[24] << 16) | (header[25] << 24);
        return true;
    };

    int out_width = 0;
    int out_height = 0;
    ASSERT_TRUE(read_bmp_size(cropped, out_width, out_height)) << "裁剪产物不是合法 BMP";
    EXPECT_EQ(8, out_width);
    EXPECT_EQ(8, out_height);
    ASSERT_TRUE(read_bmp_size(resized, out_width, out_height)) << "缩放产物不是合法 BMP";
    EXPECT_EQ(20, out_width);
    EXPECT_EQ(15, out_height);
    ASSERT_TRUE(read_bmp_size(gray, out_width, out_height)) << "灰度产物不是合法 BMP";
    EXPECT_EQ(width, out_width) << "灰度化不应改变尺寸";
    EXPECT_EQ(height, out_height);

    // 无效入参必须返回 0，不能静默成功。
    op_.CvToGray(nullptr, gray.c_str(), &ret_);
    EXPECT_EQ(0, ret_) << "空源路径应当被拒绝";
    op_.CvResize(source.c_str(), 0, 15, resized.c_str(), &ret_);
    EXPECT_EQ(0, ret_) << "零宽度缩放应当被拒绝";
    op_.CvLoadTemplate(nullptr, source.c_str(), &ret_);
    EXPECT_EQ(0, ret_) << "空模板名应当被拒绝";

    // 模板生命周期：加载 / 存在 / 计数 / 改名 / 删除。
    op_.CvRemoveAllTemplates(&ret_);
    op_.CvGetTemplateCount(&ret_);
    EXPECT_EQ(0, ret_) << "RemoveAllTemplates 之后模板数应为 0";

    op_.CvLoadTemplate(L"api_surface_full", source.c_str(), &ret_);
    EXPECT_EQ(1, ret_) << "CvLoadTemplate 失败";
    op_.CvHasTemplate(L"api_surface_full", &ret_);
    EXPECT_EQ(1, ret_);
    op_.CvGetTemplateCount(&ret_);
    EXPECT_EQ(1, ret_);

    std::wstring names;
    op_.CvGetAllTemplateNames(names);
    EXPECT_NE(std::wstring::npos, names.find(L"api_surface_full")) << "模板名清单里没有刚加载的模板";

    // 掩模模板：拿同一张图当掩模也应被接受（尺寸一致）。
    op_.CvLoadMaskedTemplate(L"api_surface_masked", source.c_str(), source.c_str(), &ret_);
    EXPECT_EQ(1, ret_) << "CvLoadMaskedTemplate 失败";
    op_.CvGetTemplateCount(&ret_);
    EXPECT_EQ(2, ret_);

    // 批量加载：name,path|name,path 形式。
    std::wstring template_list = L"api_surface_batch," + source;
    op_.CvLoadTemplateList(template_list.c_str(), &ret_);
    EXPECT_EQ(1, ret_) << "CvLoadTemplateList 失败";
    op_.CvHasTemplate(L"api_surface_batch", &ret_);
    EXPECT_EQ(1, ret_);

    // 无效清单必须返回 0。
    op_.CvLoadTemplateList(L"no_comma_here", &ret_);
    EXPECT_EQ(0, ret_) << "缺少逗号分隔的清单应当被拒绝";

    std::wstring version;
    op_.CvGetOpenCvVersion(version);
    EXPECT_FALSE(version.empty()) << "CvGetOpenCvVersion 返回空";

    op_.CvRemoveTemplate(L"api_surface_full", &ret_);
    EXPECT_EQ(1, ret_);
    op_.CvHasTemplate(L"api_surface_full", &ret_);
    EXPECT_EQ(0, ret_);

    op_.CvRemoveAllTemplates(&ret_);
    op_.CvGetTemplateCount(&ret_);
    EXPECT_EQ(0, ret_) << "RemoveAllTemplates 之后模板数应回到 0";

    DeleteFileW(source.c_str());
    DeleteFileW(gray.c_str());
    DeleteFileW(cropped.c_str());
    DeleteFileW(resized.c_str());
}


TEST_F(ApiSurfaceTest, OpenCvScreenMatchersWork) {
    // 公开 CvMatch* 的入参是"当前捕获图上的矩形"，所以必须真的绑一个窗口再截图。
    op_.BindWindow(0, L"normal", L"normal", L"normal", 0, &ret_);
    bound_ = (1 == ret_);

    const std::wstring frame = test_support::GetTempBmpPath(L"api_surface_cv_frame.bmp");
    const std::wstring piece = test_support::GetTempBmpPath(L"api_surface_cv_piece.bmp");
    DeleteFileW(frame.c_str());
    DeleteFileW(piece.c_str());

    op_.Capture(0, 0, 320, 240, frame.c_str(), &ret_);
    if (1 != ret_) {
        GTEST_SKIP() << "当前会话抓不到屏幕（锁屏或无交互桌面），无法构造匹配源图";
    }

    op_.CvCrop(frame.c_str(), 10, 10, 60, 60, piece.c_str(), &ret_);
    ASSERT_EQ(1, ret_) << "从截图上裁出模板失败";

    op_.CvRemoveAllTemplates(&ret_);
    op_.CvLoadTemplate(L"api_surface_piece", piece.c_str(), &ret_);
    ASSERT_EQ(1, ret_);

    // 模板就是从这张源图的 (10,10) 裁出来的，匹配必须回到原点。
    std::wstring retjson;
    op_.CvMatchTemplate(0, 0, 320, 240, L"api_surface_piece", 0.9, 0, 0, 5, 0, retjson, &ret_);
    EXPECT_EQ(1, ret_) << "刚裁出的模板在自己的源图上匹配不到";
    EXPECT_NE(std::wstring::npos, retjson.find(L"\"ok\":1"));
    EXPECT_NE(std::wstring::npos, retjson.find(L"\"x\":10")) << "命中位置的 x 不是裁切原点";
    EXPECT_NE(std::wstring::npos, retjson.find(L"\"y\":10")) << "命中位置的 y 不是裁切原点";

    // 单档位 1.0 与多模板入口，在同样的图上也必须命中同一位置。
    op_.CvMatchTemplateScale(0, 0, 320, 240, L"api_surface_piece", L"1.0", 0.9, 5, 0, retjson, &ret_);
    EXPECT_EQ(1, ret_) << "CvMatchTemplateScale 单档位 1.0 应当命中";
    EXPECT_NE(std::wstring::npos, retjson.find(L"\"ok\":1"));

    op_.CvMatchAnyTemplate(0, 0, 320, 240, L"api_surface_piece", 0.9, 0, 0, 5, 0, retjson, &ret_);
    EXPECT_EQ(1, ret_) << "CvMatchAnyTemplate 应当命中";
    EXPECT_NE(std::wstring::npos, retjson.find(L"\"ok\":1"));

    // 形状/边缘/特征走的是完全不同的算法，同一块截图不保证有可用特征，
    // 所以这里锁的是接口契约（返回 0/1，且出参始终是合法 JSON），而不是"必须命中"。
    op_.CvShapeMatchTemplate(0, 0, 320, 240, L"api_surface_piece", 0.5, retjson, &ret_);
    EXPECT_TRUE(0 == ret_ || 1 == ret_) << "CvShapeMatchTemplate 返回值越界";
    EXPECT_NE(std::wstring::npos, retjson.find(L"\"ok\":")) << "CvShapeMatchTemplate 出参不是 JSON";

    op_.CvEdgeMatchTemplate(0, 0, 320, 240, L"api_surface_piece", 0.5, retjson, &ret_);
    EXPECT_TRUE(0 == ret_ || 1 == ret_) << "CvEdgeMatchTemplate 返回值越界";
    EXPECT_NE(std::wstring::npos, retjson.find(L"\"ok\":")) << "CvEdgeMatchTemplate 出参不是 JSON";

    op_.CvFeatureMatchTemplate(0, 0, 320, 240, L"api_surface_piece", 0.5, retjson, &ret_);
    EXPECT_TRUE(0 == ret_ || 1 == ret_) << "CvFeatureMatchTemplate 返回值越界";
    EXPECT_NE(std::wstring::npos, retjson.find(L"\"ok\":")) << "CvFeatureMatchTemplate 出参不是 JSON";

    // 未加载的模板名必须干净失败，不能给出 ok JSON。
    op_.CvMatchTemplate(0, 0, 320, 240, L"api_surface_missing_name", 0.9, 0, 0, 5, 0, retjson, &ret_);
    EXPECT_EQ(0, ret_);
    EXPECT_NE(std::wstring::npos, retjson.find(L"\"ok\":0"));

    op_.CvRemoveAllTemplates(&ret_);
    DeleteFileW(frame.c_str());
    DeleteFileW(piece.c_str());
}


TEST_F(ApiSurfaceTest, ScreenDataModeFlipsRows) {
    op_.BindWindow(0, L"normal", L"normal", L"normal", 0, &ret_);
    bound_ = (1 == ret_);

    constexpr int kWidth = 64;
    constexpr int kHeight = 48;
    constexpr size_t kBytes = static_cast<size_t>(kWidth) * kHeight * 4;

    // SetScreenDataMode 只记一个模式位：0 = 从上到下（默认），1 = 从下到上。
    op_.SetScreenDataMode(0, &ret_);
    EXPECT_EQ(1, ret_);

    size_t top_down = 0;
    op_.GetScreenData(0, 0, kWidth, kHeight, &top_down, &ret_);
    EXPECT_EQ(1, ret_) << "GetScreenData 抓不到屏幕（锁屏或无交互桌面）";
    if (1 != ret_ || 0 == top_down) {
        GTEST_SKIP() << "当前会话没有可抓取的屏幕内容";
    }

    // 缓冲区是内部复用的，下一次调用会覆盖它，所以必须先拷出来。
    const std::vector<unsigned char> top_copy(reinterpret_cast<const unsigned char *>(top_down),
                                              reinterpret_cast<const unsigned char *>(top_down) + kBytes);

    // 再抓一次同模式，用来判断桌面内容在两次抓取之间是否稳定。
    size_t probe = 0;
    op_.GetScreenData(0, 0, kWidth, kHeight, &probe, &ret_);
    ASSERT_EQ(1, ret_);
    ASSERT_NE(static_cast<size_t>(0), probe);
    const std::vector<unsigned char> probe_copy(reinterpret_cast<const unsigned char *>(probe),
                                                reinterpret_cast<const unsigned char *>(probe) + kBytes);
    if (top_copy != probe_copy) {
        GTEST_SKIP() << "屏幕内容在两次抓取之间发生变化，翻转语义无法判定";
    }

    op_.SetScreenDataMode(1, &ret_);
    EXPECT_EQ(1, ret_);
    size_t bottom_up = 0;
    op_.GetScreenData(0, 0, kWidth, kHeight, &bottom_up, &ret_);
    EXPECT_EQ(1, ret_);
    ASSERT_NE(static_cast<size_t>(0), bottom_up);

    const std::vector<unsigned char> bottom_copy(reinterpret_cast<const unsigned char *>(bottom_up),
                                                 reinterpret_cast<const unsigned char *>(bottom_up) + kBytes);

    // 两种模式对同一矩形取到的内容只应差一个行序：
    // 把 bottom-up 的第 47-y 行和 top-down 的第 y 行对齐后必须逐字节相同。
    int mismatched_rows = 0;
    for (int y = 0; y < kHeight; ++y) {
        const unsigned char *a = &top_copy[static_cast<size_t>(y) * kWidth * 4];
        const unsigned char *b = &bottom_copy[static_cast<size_t>(kHeight - 1 - y) * kWidth * 4];
        if (0 != memcmp(a, b, static_cast<size_t>(kWidth) * 4)) {
            ++mismatched_rows;
        }
    }
    EXPECT_EQ(0, mismatched_rows) << "SC_DATA_BOTTOM 模式下 GetScreenData 没有按行翻转";

    op_.SetScreenDataMode(0, &ret_);
    EXPECT_EQ(1, ret_);
}

TEST_F(ApiSurfaceTest, LineYoloClipboardAndCoordApisWork) {
    op_.BindWindow(0, L"normal", L"normal", L"normal", 0, &ret_);
    bound_ = (1 == ret_);

    // 桌面窗口覆盖整个屏幕且客户区原点就是屏幕原点，所以屏幕坐标转它的客户区是恒等变换。
    long x = 1234;
    long y = 5678;
    const HWND desktop = ::GetDesktopWindow();
    op_.ScreenToClient(reinterpret_cast<LONG_PTR>(desktop), &x, &y, &ret_);
    EXPECT_EQ(1, ret_);
    EXPECT_EQ(1234, x) << "桌面客户区坐标应当与屏幕坐标一致";
    EXPECT_EQ(5678, y) << "桌面客户区坐标应当与屏幕坐标一致";

    // 与 ClientToScreen 互逆：来回一趟必须回到原点。
    long rx = x;
    long ry = y;
    op_.ClientToScreen(reinterpret_cast<LONG_PTR>(desktop), &rx, &ry, &ret_);
    EXPECT_EQ(1, ret_);
    EXPECT_EQ(1234, rx);
    EXPECT_EQ(5678, ry);

    // 无效窗口必须干净失败，不能把入参原样吐回来并声称成功。
    long bx = 111;
    long by = 222;
    op_.ScreenToClient(0, &bx, &by, &ret_);
    EXPECT_EQ(0, ret_);
    EXPECT_EQ(111, bx);
    EXPECT_EQ(222, by);

    // 剪贴板往返 + SendPaste 的契约。
    op_.SetClipboard(L"op_api_surface_paste_probe", &ret_);
    EXPECT_EQ(1, ret_);
    std::wstring clip;
    op_.GetClipboard(clip);
    EXPECT_EQ(std::wstring(L"op_api_surface_paste_probe"), clip);

    op_.SendPaste(reinterpret_cast<LONG_PTR>(desktop), &ret_);
    EXPECT_TRUE(0 == ret_ || 1 == ret_) << "SendPaste 返回值越界";
    // 无效窗口在旧实现里会无条件返回成功（只发 SendMessage 不看结果），已改为先校验窗口。
    op_.SendPaste(0, &ret_);
    EXPECT_EQ(0, ret_) << "无效窗口的 SendPaste 不应当成功";

    // FindLine：找不到线时出参为空，找到时是坐标串；两种都是合法契约。
    std::wstring line;
    op_.FindLine(0, 0, 200, 160, L"FFFFFF", 0.9, line);
    if (!line.empty()) {
        // 只断言格式，不把内容打出来：wstring 逐字符转 string 会触发 C4244
        // （wchar_t -> char 可能丢数据），而这个出参本身就是 ASCII 坐标串。
        EXPECT_TRUE(std::wstring::npos != line.find(L',')) << "FindLine 命中时出参不是坐标串";
    }

    // YoloDetect：没有本地 YOLO 服务时必须干净失败（返回 0），不能挂死或抛异常。
    std::wstring yolo_json;
    op_.YoloDetect(0, 0, 64, 48, 0.25, 0.45, yolo_json, &ret_);
    EXPECT_TRUE(0 == ret_ || 1 == ret_) << "YoloDetect 返回值越界";
}


TEST_F(ApiSurfaceTest, LayoutWindowsPlacesTargetWindows) {
    const HWND console = ::GetConsoleWindow();
    if (nullptr == console) {
        GTEST_SKIP() << "没有控制台窗口可用于布局测试";
    }

    wchar_t text[64] = {0};
    swprintf_s(text, L"%lld", static_cast<long long>(reinterpret_cast<LONG_PTR>(console)));

    // hwnds 的格式是 "123|456|789"（见 include/libop.h:193 的注释）。
    op_.LayoutWindows(text, 0, 1, 100, 120, 8, 8, 0, 320, 200, 0, &ret_);
    EXPECT_EQ(1, ret_) << "对单个窗口做布局应当成功";

    RECT placed = {0, 0, 0, 0};
    ASSERT_TRUE(::GetWindowRect(console, &placed));
    EXPECT_NEAR(100, placed.left, 2) << "布局后的 x 与请求的起点不符";
    EXPECT_NEAR(120, placed.top, 2) << "布局后的 y 与请求的起点不符";

    // 空清单与非法句柄都必须干净失败，不能把"什么都没做"报成成功。
    op_.LayoutWindows(L"", 0, 1, 0, 0, 0, 0, 0, 100, 100, 0, &ret_);
    EXPECT_EQ(0, ret_) << "空句柄清单不应当成功";
    op_.LayoutWindows(L"not-a-handle", 0, 1, 0, 0, 0, 0, 0, 100, 100, 0, &ret_);
    EXPECT_EQ(0, ret_) << "非法句柄不应当成功";

    // C API 走同一条实现（OpLayoutWindows 直接返回结果码，没有出参指针）。
    op_handle handle = OpCreate();
    ASSERT_NE(static_cast<op_handle>(nullptr), handle);
    const int capi_ret = OpLayoutWindows(handle, text, 0, 1, 100, 120, 8, 8, 0, 320, 200, 0);
    OpDestroy(handle);
    EXPECT_EQ(1, capi_ret) << "C API 的 OpLayoutWindows 与 C++ 结果不一致";
}
