#include "test_support.h"
#include <cstdint>
#include <windows.h>

namespace {

class TemporaryWindow {
  public:
    TemporaryWindow() {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"OpHandleCompatWindow";
        RegisterClassW(&wc);

        hwnd_ = CreateWindowExW(0, wc.lpszClassName, L"Handle Compat", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                CW_USEDEFAULT, 400, 300, nullptr, nullptr, wc.hInstance, nullptr);
        ShowWindow(hwnd_, SW_SHOW);
        SetForegroundWindow(hwnd_);
        SetFocus(hwnd_);
        UpdateWindow(hwnd_);
    }

    ~TemporaryWindow() {
        if (hwnd_)
            DestroyWindow(hwnd_);
    }

    HWND hwnd() const {
        return hwnd_;
    }

  private:
    HWND hwnd_ = nullptr;
};

} // namespace

TEST(HandleCompatTest, ForegroundWindowHandleFitsInLongOnlyWhenNoTruncation) {
    TemporaryWindow window;
    ASSERT_TRUE(window.hwnd() != nullptr);

    libop op;
    LONG_PTR returned = 0;
    op.GetForegroundWindow(&returned);

    const auto expected = reinterpret_cast<std::uintptr_t>(::GetForegroundWindow());
    const auto actual = static_cast<std::uintptr_t>(returned);

    EXPECT_EQ(actual, expected) << "Foreground hwnd was truncated when returned through long";
}

TEST(HandleCompatTest, LongCannotRepresentAllHwndValuesOn64Bit) {
#if defined(_WIN64)
    EXPECT_EQ(sizeof(HWND), 8u);
    EXPECT_EQ(sizeof(LONG), 4u);

    const std::uintptr_t synthetic = 0x1234567887654321ull;
    const LONG api_value = static_cast<LONG>(synthetic);
    const std::uintptr_t round_trip = static_cast<std::uintptr_t>(static_cast<unsigned long>(api_value));

    EXPECT_NE(round_trip, synthetic) << "LONG unexpectedly preserved a 64-bit HWND value";
#else
    GTEST_SKIP() << "This test only applies to 64-bit builds";
#endif
}
