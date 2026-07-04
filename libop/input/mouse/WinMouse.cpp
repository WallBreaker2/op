// #include "stdafx.h"
#include "WinMouse.h"
#include "CursorShape.h"
#include "../../runtime/AutomationModes.h"
#include "../../runtime/RuntimeUtils.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cwctype>
#include <functional>
#include <sstream>
#include <vector>

namespace {
long send_message_result(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    return ::SendMessageTimeout(hwnd, message, wparam, lparam, SMTO_BLOCK, 2000, nullptr) ? 1L : 0L;
}

double point_distance(POINT a, POINT b) {
    const double dx = static_cast<double>(b.x - a.x);
    const double dy = static_cast<double>(b.y - a.y);
    return std::sqrt(dx * dx + dy * dy);
}

int movement_steps(POINT from, POINT to, int duration) {
    const double distance = point_distance(from, to);
    const int by_distance = static_cast<int>(std::ceil(distance / 10.0));
    const int by_duration = duration > 0 ? static_cast<int>(std::ceil(duration / 8.0)) : 0;
    int steps = by_distance;
    steps = (std::max)(steps, by_duration);
    steps = (std::max)(steps, 3);
    return std::clamp(steps, 3, 120);
}

double smoothstep(double t) {
    return t * t * (3.0 - 2.0 * t);
}

double random_signed_unit() {
    return (static_cast<double>(rand()) / static_cast<double>(RAND_MAX)) * 2.0 - 1.0;
}

int random_offset(int value) {
    if (value == 0)
        return 0;
    const int span = value == INT_MIN ? INT_MAX : std::abs(value);
    const int offset = rand() % span;
    return value > 0 ? offset : -offset;
}

POINT bezier_point(POINT p0, POINT p1, POINT p2, POINT p3, double t) {
    const double u = 1.0 - t;
    const double uu = u * u;
    const double tt = t * t;
    const double x = uu * u * p0.x + 3.0 * uu * t * p1.x + 3.0 * u * tt * p2.x + tt * t * p3.x;
    const double y = uu * u * p0.y + 3.0 * uu * t * p1.y + 3.0 * u * tt * p2.y + tt * t * p3.y;
    return POINT{static_cast<LONG>(std::lround(x)), static_cast<LONG>(std::lround(y))};
}

std::vector<POINT> build_linear_path(POINT from, POINT to, int duration) {
    if (point_distance(from, to) < 1.0)
        return {to};

    std::vector<POINT> path;
    const int steps = movement_steps(from, to, duration);
    path.reserve(static_cast<size_t>(steps));
    POINT last{LONG_MIN, LONG_MIN};
    for (int i = 1; i <= steps; ++i) {
        const double t = smoothstep(static_cast<double>(i) / static_cast<double>(steps));
        POINT pt{static_cast<LONG>(std::lround(from.x + (to.x - from.x) * t)),
                 static_cast<LONG>(std::lround(from.y + (to.y - from.y) * t))};
        if (pt.x != last.x || pt.y != last.y) {
            path.push_back(pt);
            last = pt;
        }
    }
    if (path.empty() || path.back().x != to.x || path.back().y != to.y)
        path.push_back(to);
    return path;
}

std::vector<POINT> build_smooth_path(POINT from, POINT to, int duration, int jitter) {
    const double distance = point_distance(from, to);
    if (distance < 1.0)
        return {to};

    const double dx = static_cast<double>(to.x - from.x);
    const double dy = static_cast<double>(to.y - from.y);
    const double nx = -dy / distance;
    const double ny = dx / distance;
    const double raw_bend = distance * static_cast<double>(jitter) / 100.0;
    const double bend = raw_bend > 0.0 ? std::clamp(raw_bend, 3.0, 80.0) : 0.0;
    const double bend1 = bend * random_signed_unit();
    const double bend2 = bend * random_signed_unit();

    // 控制点只做轻微偏移，保留方向感，同时避免完全笔直的机械轨迹。
    POINT c1{static_cast<LONG>(std::lround(from.x + dx * 0.35 + nx * bend1)),
             static_cast<LONG>(std::lround(from.y + dy * 0.35 + ny * bend1))};
    POINT c2{static_cast<LONG>(std::lround(from.x + dx * 0.70 + nx * bend2)),
             static_cast<LONG>(std::lround(from.y + dy * 0.70 + ny * bend2))};

    std::vector<POINT> path;
    const int steps = movement_steps(from, to, duration);
    path.reserve(static_cast<size_t>(steps));
    POINT last{LONG_MIN, LONG_MIN};
    for (int i = 1; i <= steps; ++i) {
        const double t = smoothstep(static_cast<double>(i) / static_cast<double>(steps));
        POINT pt = bezier_point(from, c1, c2, to, t);
        if (pt.x != last.x || pt.y != last.y) {
            path.push_back(pt);
            last = pt;
        }
    }
    if (path.empty() || path.back().x != to.x || path.back().y != to.y)
        path.push_back(to);
    return path;
}

std::vector<POINT> build_mouse_path(POINT from, POINT to, int duration, int mode, int jitter) {
    if (mode == 1)
        return build_linear_path(from, to, duration);
    return build_smooth_path(from, to, duration, jitter);
}

bool parse_path_point(const std::wstring &text, POINT &pt) {
    std::wistringstream stream(text);
    long x = 0;
    long y = 0;
    wchar_t comma = 0;
    if (!(stream >> x >> comma >> y) || comma != L',')
        return false;
    while (stream && std::iswspace(stream.peek()))
        stream.get();
    if (!stream.eof())
        return false;
    pt = POINT{static_cast<LONG>(x), static_cast<LONG>(y)};
    return true;
}

bool parse_mouse_path(const std::wstring &path_text, std::vector<POINT> &path) {
    path.clear();
    size_t start = 0;
    while (start <= path_text.size()) {
        const size_t end = path_text.find(L'|', start);
        const std::wstring item = path_text.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start);
        POINT pt{};
        if (!parse_path_point(item, pt))
            return false;
        path.push_back(pt);
        if (end == std::wstring::npos)
            break;
        start = end + 1;
    }
    return path.size() >= 2;
}

std::vector<POINT> expand_mouse_path(const std::vector<POINT> &key_points) {
    if (key_points.empty())
        return {};

    std::vector<POINT> path;
    path.push_back(key_points.front());
    for (size_t i = 1; i < key_points.size(); ++i) {
        const POINT from = key_points[i - 1];
        const POINT to = key_points[i];
        const int steps = std::clamp(static_cast<int>(std::ceil(point_distance(from, to) / 10.0)), 1, 60);
        POINT last = path.back();
        for (int step = 1; step <= steps; ++step) {
            const double t = static_cast<double>(step) / static_cast<double>(steps);
            POINT pt{static_cast<LONG>(std::lround(from.x + (to.x - from.x) * t)),
                     static_cast<LONG>(std::lround(from.y + (to.y - from.y) * t))};
            if (pt.x != last.x || pt.y != last.y) {
                path.push_back(pt);
                last = pt;
            }
        }
    }
    return path;
}

long run_mouse_path(const std::vector<POINT> &path, int duration,
                    const std::function<long(POINT)> &move_to_point) {
    if (path.empty() || duration < 0)
        return 0;

    const int delay = duration > 0 && path.size() > 1 ? duration / static_cast<int>(path.size() - 1) : 0;
    for (size_t i = 0; i < path.size(); ++i) {
        if (!move_to_point(path[i]))
            return 0;
        if (delay > 0 && i + 1 < path.size())
            ::Delay(delay);
    }
    return 1;
}

void delay_if_needed(int delay) {
    if (delay > 0)
        ::Delay(delay);
}
}

namespace op::input {

WinMouse::WinMouse()
    : _hwnd(NULL), _mode(0), _x(0), _y(0), _button_state(0), _trajectory_mode(0),
      _trajectory_min_duration(0), _trajectory_max_duration(0), _trajectory_jitter(18),
      _trajectory_start_delay(0), _trajectory_end_delay(0) {
}

WinMouse::~WinMouse() {
    _hwnd = NULL;
}

long WinMouse::Bind(HWND h, int mode) {
    _hwnd = h;
    _mode = mode;
    _x = _y = 0;
    _button_state = 0;
    return 1;
}

long WinMouse::UnBind() {
    _hwnd = 0;
    _mode = 0;
    _x = _y = 0;
    _button_state = 0;
    return 1;
}

long WinMouse::GetCursorPos(long &x, long &y) {
    BOOL ret = FALSE;
    POINT pt;
    ret = ::GetCursorPos(&pt);
    if (_hwnd && _hwnd != ::GetDesktopWindow()) {
        ret = ::ScreenToClient(_hwnd, &pt);
    }
    x = pt.x;
    y = pt.y;
    return ret;
}

long WinMouse::GetCursorShape(std::wstring &ret) {
    CursorShapeInfo info;
    if (!cursor_shape::FromSystem(info)) {
        ret.clear();
        return 0;
    }

    ret = cursor_shape::Format(info);
    return 1;
}

long WinMouse::MoveR(int rx, int ry) {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        // https://learn.microsoft.com/zh-cn/windows/win32/api/winuser/ns-winuser-mouseinput
        _x += rx;
        _y += ry;

        INPUT Input = {0};
        Input.type = INPUT_MOUSE;
        Input.mi.dwFlags = MOUSEEVENTF_MOVE;
        Input.mi.dx = static_cast<LONG>(rx);
        Input.mi.dy = static_cast<LONG>(ry);
        return ::SendInput(1, &Input, sizeof(INPUT)) > 0 ? 1 : 0;
    }
    }
    return MoveTo(_x + rx, _y + ry);
}

long WinMouse::MoveTo(int x, int y) {
    long ret = 0;
    POINT client_pt{x, y};
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        POINT pt = client_pt;
        if (_hwnd)
            ::ClientToScreen(_hwnd, &pt);

        const int screen_x = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
        const int screen_y = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
        const int screen_width = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
        const int screen_height = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
        const double width = screen_width > 1 ? static_cast<double>(screen_width - 1) : 1.0;
        const double height = screen_height > 1 ? static_cast<double>(screen_height - 1) : 1.0;
        const double fx = (pt.x - screen_x) * (65535.0 / width);
        const double fy = (pt.y - screen_y) * (65535.0 / height);

        INPUT Input = {0};
        Input.type = INPUT_MOUSE;
        Input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
        Input.mi.dx = static_cast<LONG>(fx);
        Input.mi.dy = static_cast<LONG>(fy);
        ret = ::SendInput(1, &Input, sizeof(INPUT)) > 0 ? 1 : 0;
        break;
    }
    case INPUT_TYPE::IN_WINDOWS: {
        ret = send_message_result(_hwnd, WM_MOUSEMOVE, button_state(), MAKELPARAM(client_pt.x, client_pt.y));
        break;
    }
    }
    _x = client_pt.x, _y = client_pt.y;
    return ret;
}

POINT WinMouse::current_client_point() const {
    return POINT{_x, _y};
}

long WinMouse::sync_system_cursor() {
    POINT pt = current_client_point();
    if (_hwnd)
        ::ClientToScreen(_hwnd, &pt);
    return ::SetCursorPos(pt.x, pt.y) ? 1 : 0;
}

WPARAM WinMouse::button_state() const {
    return _button_state;
}

WPARAM WinMouse::button_state_with(WPARAM button, bool down) const {
    return down ? (_button_state | button) : (_button_state & ~button);
}

void WinMouse::set_button_state(WPARAM button, bool down) {
    _button_state = button_state_with(button, down);
}

long WinMouse::send_input_mouse(DWORD flags, DWORD mouse_data, bool sync_cursor) {
    if (sync_cursor && !sync_system_cursor())
        return 0;

    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = flags;
    input.mi.mouseData = mouse_data;
    return ::SendInput(1, &input, sizeof(INPUT)) > 0 ? 1 : 0;
}

long WinMouse::send_input_click(DWORD down_flags, DWORD up_flags, DWORD mouse_data, long delay) {
    if (!sync_system_cursor())
        return 0;

    const long r1 = send_input_mouse(down_flags, mouse_data, false);
    ::Delay(delay);
    const long r2 = send_input_mouse(up_flags, mouse_data, false);
    return r1 && r2 ? 1 : 0;
}

long WinMouse::send_windows_button(UINT message, WPARAM button, bool down) {
    const POINT pt = current_client_point();
    const WPARAM state = button_state_with(button, down);
    const long ret = send_message_result(_hwnd, message, state, MAKELPARAM(pt.x, pt.y));
    if (ret)
        set_button_state(button, down);
    return ret;
}

long WinMouse::send_windows_xbutton(UINT message, WORD xbutton, WPARAM button, bool down) {
    const POINT pt = current_client_point();
    const WPARAM state = button_state_with(button, down);
    const long ret = send_message_result(_hwnd, message, MAKEWPARAM(static_cast<WORD>(state), xbutton),
                                         MAKELPARAM(pt.x, pt.y));
    if (ret)
        set_button_state(button, down);
    return ret;
}

long WinMouse::button_click(long (WinMouse::*down)(), long (WinMouse::*up)(), long delay) {
    const long r1 = (this->*down)();
    ::Delay(delay);
    const long r2 = (this->*up)();
    return r1 && r2 ? 1 : 0;
}

long WinMouse::normal_double_click(long (WinMouse::*click)(), long delay) {
    const long r1 = (this->*click)();
    ::Delay(delay);
    const long r2 = (this->*click)();
    return r1 && r2 ? 1 : 0;
}

long WinMouse::button_double_click(long (WinMouse::*click)(), UINT message, UINT up_message, WPARAM button,
                                   long delay) {
    const long r1 = (this->*click)();
    ::Delay(delay);
    const POINT pt = current_client_point();
    const WPARAM state = button_state_with(button, true);
    const long r2 = send_message_result(_hwnd, message, state, MAKELPARAM(pt.x, pt.y));
    if (r2)
        set_button_state(button, true);
    ::Delay(delay);
    const long r3 = send_windows_button(up_message, button, false);
    return r1 && r2 && r3 ? 1 : 0;
}

long WinMouse::xbutton(WORD xbutton_id, WPARAM button, bool down) {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL:
        return send_input_mouse(down ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP, xbutton_id);
    case INPUT_TYPE::IN_WINDOWS:
        return send_windows_xbutton(down ? WM_XBUTTONDOWN : WM_XBUTTONUP, xbutton_id, button, down);
    }
    return 0;
}

long WinMouse::xbutton_double_click(long (WinMouse::*click)(), WORD xbutton_id, WPARAM button, long delay) {
    const long r1 = (this->*click)();
    ::Delay(delay);
    const long r2 = send_windows_xbutton(WM_XBUTTONDBLCLK, xbutton_id, button, true);
    ::Delay(delay);
    const long r3 = send_windows_xbutton(WM_XBUTTONUP, xbutton_id, button, false);
    return r1 && r2 && r3 ? 1 : 0;
}

long WinMouse::send_wheel(DWORD input_flag, UINT window_message, int delta) {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL:
        return send_input_mouse(input_flag, static_cast<DWORD>(delta));
    case INPUT_TYPE::IN_WINDOWS: {
        POINT pt = current_client_point();
        ::ClientToScreen(_hwnd, &pt);
        return send_message_result(_hwnd, window_message,
                                   MAKEWPARAM(static_cast<WORD>(button_state()), static_cast<WORD>(delta)),
                                   MAKELPARAM(pt.x, pt.y));
    }
    }
    return 0;
}

long WinMouse::MoveToEx(int x, int y, int w, int h, int &dst_x, int &dst_y) {
    dst_x = x + random_offset(w);
    dst_y = y + random_offset(h);
    return MoveTo(dst_x, dst_y);
}

int WinMouse::resolve_trajectory_duration(int duration) const {
    if (duration < 0)
        return -1;

    int resolved = duration > 0 ? duration : _trajectory_min_duration;
    resolved = (std::max)(resolved, _trajectory_min_duration);
    if (_trajectory_max_duration > 0)
        resolved = (std::min)(resolved, _trajectory_max_duration);
    return resolved;
}

long WinMouse::MoveToSmooth(int x, int y, int duration) {
    const int resolved_duration = resolve_trajectory_duration(duration);
    if (resolved_duration < 0)
        return 0;

    POINT from = current_client_point();
    if (_mode == INPUT_TYPE::IN_NORMAL) {
        long current_x = 0;
        long current_y = 0;
        if (GetCursorPos(current_x, current_y)) {
            from.x = static_cast<LONG>(current_x);
            from.y = static_cast<LONG>(current_y);
        }
    }

    const POINT to{x, y};
    const std::vector<POINT> path =
        build_mouse_path(from, to, resolved_duration, _trajectory_mode, _trajectory_jitter);
    delay_if_needed(_trajectory_start_delay);
    const long ret = run_mouse_path(path, resolved_duration, [this](POINT pt) { return MoveTo(pt.x, pt.y); });
    if (ret)
        delay_if_needed(_trajectory_end_delay);
    return ret;
}

long WinMouse::MoveToExSmooth(int x, int y, int w, int h, int duration, int &dst_x, int &dst_y) {
    dst_x = x + random_offset(w);
    dst_y = y + random_offset(h);
    return MoveToSmooth(dst_x, dst_y, duration);
}

long WinMouse::MovePath(const std::wstring &path_text, int duration) {
    std::vector<POINT> key_points;
    const int resolved_duration = resolve_trajectory_duration(duration);
    if (resolved_duration < 0 || !parse_mouse_path(path_text, key_points))
        return 0;

    // MovePath 接收的是关键点，段内补点后再发送，避免长距离两点之间直接跳过去。
    const std::vector<POINT> path = expand_mouse_path(key_points);
    if (path.size() < 2)
        return 0;
    delay_if_needed(_trajectory_start_delay);
    const long ret = run_mouse_path(path, resolved_duration, [this](POINT pt) { return MoveTo(pt.x, pt.y); });
    if (ret)
        delay_if_needed(_trajectory_end_delay);
    return ret;
}

long WinMouse::DragPath(const std::wstring &path_text, int duration) {
    std::vector<POINT> key_points;
    const int resolved_duration = resolve_trajectory_duration(duration);
    if (resolved_duration < 0 || !parse_mouse_path(path_text, key_points))
        return 0;

    const std::vector<POINT> path = expand_mouse_path(key_points);
    if (path.size() < 2)
        return 0;
    if (!MoveTo(path.front().x, path.front().y))
        return 0;

    const long down_ret = LeftDown();
    if (!down_ret)
        return 0;

    delay_if_needed(_trajectory_start_delay);
    std::vector<POINT> moving_path(path.begin() + 1, path.end());
    const long move_ret =
        run_mouse_path(moving_path, resolved_duration, [this](POINT pt) { return MoveTo(pt.x, pt.y); });
    // 拖拽到终点后先停一下再松手，更接近按住后确认落点的操作习惯。
    delay_if_needed(_trajectory_end_delay);
    const long up_ret = LeftUp();
    return move_ret && up_ret ? 1 : 0;
}

long WinMouse::SetMouseTrajectory(int mode, int min_duration, int max_duration, int jitter, int start_delay,
                                  int end_delay) {
    if (mode < 0 || mode > 2 || min_duration < 0 || max_duration < 0 || jitter < 0 || jitter > 100 ||
        start_delay < 0 || end_delay < 0)
        return 0;
    if (max_duration > 0 && min_duration > max_duration)
        return 0;

    // max_duration 为 0 表示不封顶，旧脚本传入的 duration 会按原样生效。
    _trajectory_mode = mode;
    _trajectory_min_duration = min_duration;
    _trajectory_max_duration = max_duration;
    _trajectory_jitter = jitter;
    _trajectory_start_delay = start_delay;
    _trajectory_end_delay = end_delay;
    return 1;
}

long WinMouse::LeftClick() {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        return send_input_click(MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP, 0, MOUSE_NORMAL_DELAY);
    }

    case INPUT_TYPE::IN_WINDOWS: {
        return button_click(&WinMouse::LeftDown, &WinMouse::LeftUp, MOUSE_WINDOWS_DELAY);
    }
    }
    return 0;
}

long WinMouse::LeftDoubleClick() {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        return normal_double_click(&WinMouse::LeftClick, MOUSE_NORMAL_DELAY);
    }
    case INPUT_TYPE::IN_WINDOWS: {
        return button_double_click(&WinMouse::LeftClick, WM_LBUTTONDBLCLK, WM_LBUTTONUP, MK_LBUTTON,
                                   MOUSE_WINDOWS_DELAY);
    }
    }
    return 0;
}

long WinMouse::LeftDown() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        ret = send_input_mouse(MOUSEEVENTF_LEFTDOWN);
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        ret = send_windows_button(WM_LBUTTONDOWN, MK_LBUTTON, true);
        break;
    }
    }
    return ret;
}

long WinMouse::LeftUp() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        ret = send_input_mouse(MOUSEEVENTF_LEFTUP);
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        ret = send_windows_button(WM_LBUTTONUP, MK_LBUTTON, false);
        break;
    }
    }
    return ret;
}

long WinMouse::MiddleClick() {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL:
        return send_input_click(MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_MIDDLEUP, 0, MOUSE_NORMAL_DELAY);
    case INPUT_TYPE::IN_WINDOWS:
        return button_click(&WinMouse::MiddleDown, &WinMouse::MiddleUp, MOUSE_WINDOWS_DELAY);
    }
    return 0;
}

long WinMouse::MiddleDoubleClick() {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        return normal_double_click(&WinMouse::MiddleClick, MOUSE_NORMAL_DELAY);
    }
    case INPUT_TYPE::IN_WINDOWS:
        return button_double_click(&WinMouse::MiddleClick, WM_MBUTTONDBLCLK, WM_MBUTTONUP, MK_MBUTTON,
                                   MOUSE_WINDOWS_DELAY);
    }
    return 0;
}

long WinMouse::MiddleDown() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        ret = send_input_mouse(MOUSEEVENTF_MIDDLEDOWN);
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        ret = send_windows_button(WM_MBUTTONDOWN, MK_MBUTTON, true);
        break;
    }
    }
    return ret;
}

long WinMouse::MiddleUp() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        ret = send_input_mouse(MOUSEEVENTF_MIDDLEUP);
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        ret = send_windows_button(WM_MBUTTONUP, MK_MBUTTON, false);
        break;
    }
    }
    return ret;
}

long WinMouse::RightClick() {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        return send_input_click(MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP, 0, MOUSE_NORMAL_DELAY);
    }

    case INPUT_TYPE::IN_WINDOWS: {
        return button_click(&WinMouse::RightDown, &WinMouse::RightUp, MOUSE_WINDOWS_DELAY);
    }
    }
    return 0;
}

long WinMouse::RightDoubleClick() {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        return normal_double_click(&WinMouse::RightClick, MOUSE_NORMAL_DELAY);
    }
    case INPUT_TYPE::IN_WINDOWS:
        return button_double_click(&WinMouse::RightClick, WM_RBUTTONDBLCLK, WM_RBUTTONUP, MK_RBUTTON,
                                   MOUSE_WINDOWS_DELAY);
    }
    return 0;
}

long WinMouse::RightDown() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        ret = send_input_mouse(MOUSEEVENTF_RIGHTDOWN);
        break;
    }
    case INPUT_TYPE::IN_WINDOWS: {
        ret = send_windows_button(WM_RBUTTONDOWN, MK_RBUTTON, true);
        break;
    }
    }
    return ret;
}

long WinMouse::RightUp() {
    long ret = 0;
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        ret = send_input_mouse(MOUSEEVENTF_RIGHTUP);
        break;
    }

    case INPUT_TYPE::IN_WINDOWS: {
        ret = send_windows_button(WM_RBUTTONUP, MK_RBUTTON, false);
        break;
    }
    }
    return ret;
}

long WinMouse::XButton1Click() {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL:
        return send_input_click(MOUSEEVENTF_XDOWN, MOUSEEVENTF_XUP, XBUTTON1, MOUSE_NORMAL_DELAY);
    case INPUT_TYPE::IN_WINDOWS:
        return button_click(&WinMouse::XButton1Down, &WinMouse::XButton1Up, MOUSE_WINDOWS_DELAY);
    }
    return 0;
}

long WinMouse::XButton1DoubleClick() {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        return normal_double_click(&WinMouse::XButton1Click, MOUSE_NORMAL_DELAY);
    }
    case INPUT_TYPE::IN_WINDOWS:
        return xbutton_double_click(&WinMouse::XButton1Click, XBUTTON1, MK_XBUTTON1, MOUSE_WINDOWS_DELAY);
    }
    return 0;
}

long WinMouse::XButton1Down() {
    return xbutton(XBUTTON1, MK_XBUTTON1, true);
}

long WinMouse::XButton1Up() {
    return xbutton(XBUTTON1, MK_XBUTTON1, false);
}

long WinMouse::XButton2Click() {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL:
        return send_input_click(MOUSEEVENTF_XDOWN, MOUSEEVENTF_XUP, XBUTTON2, MOUSE_NORMAL_DELAY);
    case INPUT_TYPE::IN_WINDOWS:
        return button_click(&WinMouse::XButton2Down, &WinMouse::XButton2Up, MOUSE_WINDOWS_DELAY);
    }
    return 0;
}

long WinMouse::XButton2DoubleClick() {
    switch (_mode) {
    case INPUT_TYPE::IN_NORMAL: {
        return normal_double_click(&WinMouse::XButton2Click, MOUSE_NORMAL_DELAY);
    }
    case INPUT_TYPE::IN_WINDOWS:
        return xbutton_double_click(&WinMouse::XButton2Click, XBUTTON2, MK_XBUTTON2, MOUSE_WINDOWS_DELAY);
    }
    return 0;
}

long WinMouse::XButton2Down() {
    return xbutton(XBUTTON2, MK_XBUTTON2, true);
}

long WinMouse::XButton2Up() {
    return xbutton(XBUTTON2, MK_XBUTTON2, false);
}

long WinMouse::Wheel(int delta) {
    return send_wheel(MOUSEEVENTF_WHEEL, WM_MOUSEWHEEL, delta);
}

long WinMouse::HWheel(int delta) {
    return send_wheel(MOUSEEVENTF_HWHEEL, WM_MOUSEHWHEEL, delta);
}

long WinMouse::WheelDown() {
    return Wheel(-WHEEL_DELTA);
}

long WinMouse::WheelUp() {
    return Wheel(WHEEL_DELTA);
}

} // namespace op::input
