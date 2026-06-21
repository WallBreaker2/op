#pragma once

#include <vector>
#include <windows.h>

namespace op::window_layout {

constexpr int kMinWindowWidth = 50;
constexpr int kMinWindowHeight = 50;

enum class Type {
    Grid,     // 宫格
    Diagonal  // 对角线
};

enum class SizeMode {
    Keep,   // 保持原窗口大小
    Uniform // 所有窗口统一客户区大小
};

enum class AnchorMode {
    Window, // 按窗口外框排列
    Client  // 按客户区排列
};

struct Options {
    Type type = Type::Grid;

    // 宫格模式下每行几个窗口。
    // columns = 1 时，效果等同于单列排列。
    int columns = 2;

    // 排列起点
    int start_x = 0;
    int start_y = 0;

    // 窗口间距
    int gap_x = 10;
    int gap_y = 10;

    // 窗口大小策略
    SizeMode size_mode = SizeMode::Uniform;

    // 排列时使用哪一层矩形作为贴边基准
    AnchorMode anchor_mode = AnchorMode::Window;

    // size_mode = Uniform 时使用，表示目标客户区大小。
    int window_width = 320;
    int window_height = 240;
};

struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct Metrics {
    Rect window_rect = {};
    Rect visible_rect = {};
    Rect client_rect = {};
};

bool GetWindowMetrics(HWND hwnd, Metrics &metrics);

std::vector<Rect> Calculate(const std::vector<HWND> &windows, const Options &options);

long Apply(const std::vector<HWND> &windows, const std::vector<Rect> &rects, const Options &options);

long Layout(const std::vector<HWND> &windows, const Options &options);

} // namespace op::window_layout
