// #include "stdafx.h"
#include "BindingSession.h"
#include "../runtime/AutomationModes.h"
#include "../runtime/RuntimeUtils.h"
#include "../runtime/WindowsVersion.h"
#include <algorithm>
#include <cwctype>
#include <tuple>
#include <vector>

#include "../capture/backends/DxgiCapture.h"
#include "../capture/backends/GdiCapture.h"
#include "../capture/backends/HookCapture.h"
#ifdef OP_ENABLE_WGC
#include "../capture/backends/WgcCapture.h"
#endif

#include "../capture/sources/MemoryImageSource.h"
#include "../input/keyboard/DxKeyboard.h"
#include "../input/keyboard/WinKeyboard.h"
#include "../input/mouse/DxMouse.h"
#include <memory>

namespace op::binding {

using op::capture::DxgiCapture;
using op::capture::GdiCapture;
using op::capture::HookCapture;
using op::capture::ICaptureBackend;
#ifdef OP_ENABLE_WGC
using op::capture::WgcCapture;
#endif
using op::input::DxKeyboard;
using op::input::DxMouse;
using op::input::KeyboardBackend;
using op::input::WinKeyboard;
using op::input::WinMouse;

namespace {

std::wstring to_lower_ascii(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towlower(ch));
    });
    return value;
}

std::wstring window_class_name(HWND hwnd) {
    std::wstring buffer(256, L'\0');
    for (;;) {
        const int copied = ::GetClassNameW(hwnd, buffer.data(), static_cast<int>(buffer.size()));
        if (copied <= 0)
            return L"";
        if (static_cast<size_t>(copied) < buffer.size() - 1) {
            buffer.resize(static_cast<size_t>(copied));
            return buffer;
        }
        buffer.assign(buffer.size() * 2, L'\0');
    }
}

bool class_contains(const std::wstring &class_name, const wchar_t *needle) {
    return to_lower_ascii(class_name).find(to_lower_ascii(needle)) != std::wstring::npos;
}

bool class_equals(const std::wstring &class_name, const wchar_t *needle) {
    return to_lower_ascii(class_name) == to_lower_ascii(needle);
}

bool prefers_wgc(HWND hwnd) {
    const std::wstring class_name = window_class_name(hwnd);
    if (class_name.empty())
        return false;

    const wchar_t *partial_matches[] = {L"Chrome", L"Mozilla", nullptr};
    for (const wchar_t **match = partial_matches; *match; ++match) {
        if (class_contains(class_name, *match))
            return true;
    }

    const wchar_t *whole_matches[] = {
        L"ApplicationFrameWindow",
        L"Windows.UI.Core.CoreWindow",
        L"WinUIDesktopWin32WindowClass",
        L"GAMINGSERVICESUI_HOSTING_WINDOW_CLASS",
        L"XLMAIN",
        L"PPTFrameClass",
        L"screenClass",
        L"PodiumParent",
        L"OpusApp",
        L"OMain",
        L"Framework::CFrame",
        L"rctrl_renwnd32",
        L"MSWinPub",
        L"OfficeApp-Frame",
        L"SDL_app",
        nullptr,
    };
    for (const wchar_t **match = whole_matches; *match; ++match) {
        if (class_equals(class_name, *match))
            return true;
    }

    return false;
}

std::vector<int> choose_auto_displays(HWND hwnd) {
    std::vector<int> displays;
    // 对已知容易被 GDI/DXGI 截黑的窗口优先 WGC，其余普通窗口优先 DXGI。
#ifdef OP_ENABLE_WGC
    if (IsWindows10BuildOrGreater(kWindows10Build1903) && prefers_wgc(hwnd))
        displays.push_back(RDT_NORMAL_WGC);
#else
    (void)hwnd;
#endif
    if (IsWindowsVersionAtLeast(6, 2, 0))
        displays.push_back(RDT_NORMAL_DXGI);
    displays.push_back(RDT_NORMAL);
    return displays;
}

const wchar_t *display_mode_name(int display) {
    switch (display) {
    case RDT_NORMAL:
        return L"normal";
    case RDT_NORMAL_DXGI:
        return L"normal.dxgi";
    case RDT_NORMAL_WGC:
        return L"normal.wgc";
    default:
        return L"normal.auto";
    }
}

} // namespace

BindingSession::BindingSession()
    : _display_hwnd(0), _input_hwnd(0), _is_bind(0), _capture(nullptr), _mouse(std::make_unique<WinMouse>()),
      _keyboard(std::make_unique<WinKeyboard>()) {
    _display_method = std::make_pair<wstring, wstring>(L"screen", L"");
}

BindingSession::~BindingSession() {
    reset_bind_state(false);
}

long BindingSession::BindWindow(LONG_PTR hwnd, const wstring &sdisplay, const wstring &smouse, const wstring &skeypad,
                              long mode) {
    return BindWindowEx(hwnd, hwnd, sdisplay, smouse, skeypad, mode);
}

long BindingSession::BindWindowEx(LONG_PTR display_hwnd, LONG_PTR input_hwnd, const wstring &sdisplay,
                                const wstring &smouse, const wstring &skeypad, long mode) {
    // step 1.避免重复绑定
    reset_bind_state(false);

    HWND displayWnd = display_hwnd == 0 ? GetDesktopWindow() : HWND(display_hwnd);
    HWND inputWnd = input_hwnd == 0 ? displayWnd : HWND(input_hwnd);
    // step 2.check hwnd
    if (!::IsWindow(displayWnd) || !::IsWindow(inputWnd)) {
        setlog("Invalid window handles display=%p input=%p", displayWnd, inputWnd);
        return 0;
    }

    int display, mouse, keypad;
    std::vector<int> display_candidates;
    const bool auto_display = sdisplay == L"normal.auto";
    // step 3.check display... mode
    if (auto_display) {
        display_candidates = choose_auto_displays(displayWnd);
        display = display_candidates.front();
    } else if (sdisplay == L"normal")
        display = RDT_NORMAL;
    else if (sdisplay == L"normal.dxgi")
        display = RDT_NORMAL_DXGI;
    else if (sdisplay == L"normal.wgc")
        display = RDT_NORMAL_WGC;
    else if (sdisplay == L"gdi")
        display = RDT_GDI;
    else if (sdisplay == L"gdi2")
        display = RDT_GDI2;
    else if (sdisplay == L"dx2")
        display = RDT_GDI_DX2;
    else if (sdisplay == L"dx")
        display = RDT_DX_DEFAULT;
    else if (sdisplay == L"dx.d3d9")
        display = RDT_DX_D3D9;
    else if (sdisplay == L"dx.d3d10")
        display = RDT_DX_D3D10;
    else if (sdisplay == L"dx.d3d11")
        display = RDT_DX_D3D11;
    else if (sdisplay == L"dx.d3d12")
        display = RDT_DX_D3D12;
    else if (sdisplay == L"opengl")
        display = RDT_GL_DEFAULT;
    else if (sdisplay == L"opengl.std")
        display = RDT_GL_STD;
    else if (sdisplay == L"opengl.nox")
        display = RDT_GL_NOX;
    else if (sdisplay == L"opengl.es")
        display = RDT_GL_ES;
    else if (sdisplay == L"opengl.fi") // glFinish
        display = RDT_GL_FI;
    else {
        setlog(L"error display mode: %s", sdisplay.c_str());
        return 0;
    }
    if (!auto_display)
        display_candidates.push_back(display);
    // check mouse
    if (smouse == L"normal")
        mouse = INPUT_TYPE::IN_NORMAL;
    else if (smouse == L"windows")
        mouse = INPUT_TYPE::IN_WINDOWS;
    else if (smouse == L"dx")
        mouse = INPUT_TYPE::IN_DX;
    else {
        setlog(L"error mouse mode: %s", smouse.c_str());
        return 0;
    }
    // check keypad
    if (skeypad == L"normal")
        keypad = INPUT_TYPE::IN_NORMAL;
    else if (skeypad == L"normal.hd")
        keypad = INPUT_TYPE::IN_NORMAL2;
    else if (skeypad == L"windows")
        keypad = INPUT_TYPE::IN_WINDOWS;
    else if (skeypad == L"dx")
        keypad = INPUT_TYPE::IN_DX;
    else {
        setlog(L"error keypad mode: %s", skeypad.c_str());
        return 0;
    }
    // DXGI 只在 Windows 8 及以上开放。
    if (display == RDT_NORMAL_DXGI && !IsWindowsVersionAtLeast(6, 2, 0)) {
        setlog(L"normal.dxgi requires Windows 8 or later");
        return 0;
    }

    // WGC 窗口捕获从 Windows 10 1903 开始可用。
    if (display == RDT_NORMAL_WGC && !IsWindows10BuildOrGreater(kWindows10Build1903)) {
        setlog(L"normal.wgc requires Windows 10 build 18362 or later");
        return 0;
    }

    auto prepare_backends = [&]() {
        _mode = mode;
        _display = display;
        _display_hwnd = displayWnd;
        _input_hwnd = inputWnd;
        set_display_method(L"screen");

        _capture = createDisplay(display);
        _mouse = createMouse(mouse);
        _keyboard = createKeypad(keypad);
        return _capture && _mouse && _keyboard;
    };

    auto try_bind = [&]() {
        const long display_ret = _capture->Bind(displayWnd, display);
        const long mouse_ret = display_ret == 1 ? _mouse->Bind(inputWnd, mouse) : 0;
        const long keypad_ret = (display_ret == 1 && mouse_ret == 1) ? _keyboard->Bind(inputWnd, keypad) : 0;
        return std::make_tuple(display_ret, mouse_ret, keypad_ret);
    };

    // normal.auto 逐个尝试候选后端，前一个失败时释放资源再进入下一个。
    long display_ret = 0;
    long mouse_ret = 0;
    long keypad_ret = 0;
    bool bind_ok = false;
    for (size_t i = 0; i < display_candidates.size(); ++i) {
        if (i > 0)
            reset_bind_state(false);

        display = display_candidates[i];
        if (!prepare_backends()) {
            setlog("create instance error!");
            continue;
        }

        std::tie(display_ret, mouse_ret, keypad_ret) = try_bind();
        if (display_ret == 1 && mouse_ret == 1 && keypad_ret == 1) {
            bind_ok = true;
            break;
        }

        if (auto_display && i + 1 < display_candidates.size()) {
            setlog(L"normal.auto selected %s but bind failed, fallback to %s", display_mode_name(display),
                   display_mode_name(display_candidates[i + 1]));
            reset_bind_state(false);
        }
    }
    if (!bind_ok) {
        setlog(L"BindWindowEx failed. display_hwnd=%p input_hwnd=%p display=%s(%d) ret=%d mouse=%s(%d) ret=%d "
               L"keypad=%s(%d) ret=%d",
               displayWnd, inputWnd, sdisplay.c_str(), display, display_ret, smouse.c_str(), mouse, mouse_ret,
               skeypad.c_str(), keypad, keypad_ret);
        UnBindWindow();
        return 0;
    }

    _capture->waitForBindReady();

    _is_bind = 1;
    return 1;
}

long BindingSession::UnBindWindow() {
    return reset_bind_state(true);
}

long BindingSession::reset_bind_state(bool restore_default_input) {
    _display_hwnd = NULL;
    _input_hwnd = NULL;
    _display = 0;
    _is_bind = 0;
    _mode = 0;

    if (_capture) {
        _capture->UnBind();
        _capture.reset();
    }
    if (_mouse) {
        _mouse->UnBind();
        _mouse.reset();
    }
    if (_keyboard) {
        _keyboard->UnBind();
        _keyboard.reset();
    }

    // 恢复默认前台输入对象。
    if (restore_default_input) {
        _mouse = std::make_unique<WinMouse>();
        _keyboard = std::make_unique<WinKeyboard>();
    }

    return 1;
}

LONG_PTR BindingSession::GetBindWindow() {
    return reinterpret_cast<LONG_PTR>(_display_hwnd);
}

long BindingSession::IsBind() {
    return _is_bind;
}

// long bkbase::GetCursorPos(int& x, int& y) {
//	POINT pt;
//	auto r = ::GetCursorPos(&pt);
//	x = pt.x; y = pt.y;
//	return r;
// }

long BindingSession::GetDisplay() {
    return _display;
}

// byte* bkbase::GetScreenData() {
//	if (get_display_method().first == L"screen") {
//		return _capture ? _capture->get_data() : nullptr;
//	}
//	else {
//		if (get_display_method().first == L"pic") {
//			return _pic.pdata;
//		}
//		if (get_display_method().first == L"mem") {
//
// #if OP64==1
//			auto ptr= (byte*)_wtoi64(get_display_method().second.data());
// #else
//			auto ptr = (byte*)_wtoi(get_display_method().second.data());
// #endif //
//
//			auto pbfh = (BITMAPFILEHEADER*)ptr;
//			return ptr + pbfh->bfOffBits;
//	}
//		return nullptr;
// }
//
//
// }

void BindingSession::lock_data() {
    if (_capture) {
        auto p = _capture->get_mutex();
        if (p)
            p->lock();
    }
}

void BindingSession::unlock_data() {
    if (_capture) {
        auto p = _capture->get_mutex();
        if (p)
            p->unlock();
    }
}

long BindingSession::get_height() {
    const auto &method = get_display_method().first;
    if (method == L"pic" || method == L"mem") {
        return _pic.height;
    }
    return _capture ? _capture->get_height() : 0;
}

long BindingSession::get_width() {
    const auto &method = get_display_method().first;
    if (method == L"pic" || method == L"mem") {
        return _pic.width;
    }
    return _capture ? _capture->get_width() : 0;
}

long BindingSession::RectConvert(long &x1, long &y1, long &x2, long &y2) {

    /*if (_capture && (_display == RENDER_TYPE::NORMAL || _display == RENDER_TYPE::GDI)) {
        x1 += _capture->_client_x; y1 += _capture->_client_y;
        x2 += _capture->_client_x; y2 += _capture->_client_y;
    }*/

    // WGC 在窗口最小化/恢复后尺寸会异步变化，裁剪前先给后端一次刷新机会。
    if (_capture) {
        _capture->refreshMetrics();
    }

    x2 = std::min<long>(this->get_width(), x2);
    y2 = std::min<long>(this->get_height(), y2);
    if (x1 < 0 || y1 < 0 || x1 >= x2 || y1 >= y2) {
        setlog(L"invalid rectangle:%d %d %d %d", x1, y1, x2, y2);
        return 0;
    }
    // if (_capture) {
    //	if (_display == RDT_NORMAL) {//cap rect
    //		_capture->rect.left = x1;
    //		_capture->rect.top = y1;
    //		_capture->rect.right = x2;
    //		_capture->rect.bottom = y2;
    //	}
    //	else {
    //		_capture->rect.left = 0;
    //		_capture->rect.top = 0;
    //		_capture->rect.right = _capture->get_width();
    //		_capture->rect.bottom =_capture->get_height();
    //	}
    //
    // }
    return 1;
}

long BindingSession::get_image_type() {

    if (_display_method.first == L"pic")
        return 0;
    else if (_display_method.first == L"mem") {

        return 1;
    } else {
        switch (GET_RENDER_TYPE(_display)) {
        case RENDER_TYPE::NORMAL:
            return -2;
        case RENDER_TYPE::GDI:
            return -1;
        case RENDER_TYPE::DX:
            return 0;
        case RENDER_TYPE::OPENGL:
            return -1;
        default:
            return 0;
        }
    }
}

bool BindingSession::check_bind() {
    // 显示模式非屏幕
    if (get_display_method().first != L"screen") {
        return !_pic.empty();
    }
    // 已绑定
    if (IsBind())
        return true;

    // 绑定前台桌面
    return BindWindow(reinterpret_cast<LONG_PTR>(::GetDesktopWindow()), L"normal", L"normal", L"normal", 0);
}

const std::pair<wstring, wstring> &BindingSession::get_display_method() const {
    return _display_method;
}

long BindingSession::set_display_method(const wstring &method) {
    if (method == L"screen") {
        _display_method = {L"screen", L""};
        return 1;
    }

    auto idx = method.find(L"pic:");
    if (idx != wstring::npos) {
        const auto file_path = method.substr(idx + 4);
        wstring fullpath;
        if (!Path2GlobalPath(file_path, _curr_path, fullpath)) {
            return 0;
        }

        Image image;
        if (!image.read(fullpath.data())) {
            return 0;
        }

        _pic = image;
        _display_method = {L"pic", file_path};
        return 1;
    }

    idx = method.find(L"mem:");
    if (idx != wstring::npos) {
        std::wstring mem_arg;
        Image image;
        if (!op::capture::ParseMemoryImageSource(method.substr(idx + 4), image, mem_arg)) {
            return 0;
        }

        _pic = image;
        _display_method = {L"mem", mem_arg};
        return 1;
    }

    return 0;
}

bool BindingSession::requestCapture(int x1, int y1, int w, int h, Image &img) {
    const auto &method = get_display_method().first;
    if (method == L"screen")
        return _capture && _capture->requestCapture(x1, y1, w, h, img);
    else if (method == L"pic" || method == L"mem") {
        img.create(w, h);
        for (int i = 0; i < h; i++)
            memcpy(img.ptr<uchar>(i), _pic.ptr<uchar>(i + y1) + x1 * 4, w * 4);
        return true;
    }
    return false;
}

std::shared_ptr<ICaptureBackend> BindingSession::createDisplay(int mode) {
    if (mode == RDT_NORMAL || GET_RENDER_TYPE(mode) == RENDER_TYPE::GDI) {
        return std::make_shared<GdiCapture>();
    } else if (mode == RDT_NORMAL_DXGI) {
        return std::make_shared<DxgiCapture>();
    }
#ifdef OP_ENABLE_WGC
    else if (mode == RDT_NORMAL_WGC) {
        return std::make_shared<WgcCapture>();
    }
#endif
    else if (GET_RENDER_TYPE(mode) == RENDER_TYPE::DX) {
        return std::make_shared<HookCapture>();
    } else if (GET_RENDER_TYPE(mode) == RENDER_TYPE::OPENGL)
        return std::make_shared<HookCapture>();
    return nullptr;
}

std::unique_ptr<WinMouse> BindingSession::createMouse(int mode) {
    if (mode == INPUT_TYPE::IN_NORMAL || mode == INPUT_TYPE::IN_WINDOWS)
        return std::make_unique<WinMouse>();
    else if (mode == INPUT_TYPE::IN_DX) {
        return std::make_unique<DxMouse>();
    }
    return nullptr;
}

std::unique_ptr<KeyboardBackend> BindingSession::createKeypad(int mode) {
    if (mode == INPUT_TYPE::IN_DX)
        return std::make_unique<DxKeyboard>();
    return std::make_unique<WinKeyboard>();
}

} // namespace op::binding
