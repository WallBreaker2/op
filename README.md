# OP - Windows 自动化插件

[![GitHub Release](https://img.shields.io/github/v/release/WallBreaker2/op?style=flat-square)](https://github.com/WallBreaker2/op/releases)
[![CI](https://github.com/WallBreaker2/op/actions/workflows/ci.yml/badge.svg)](https://github.com/WallBreaker2/op/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows-blue.svg)](https://microsoft.com/windows)

[English Version](README_EN.md)

## 📖 简介 (Introduction)

**OP** (Operator & Open) 是一款专为 Windows 设计的开源自动化插件。作为一款现代化的桌面自动化解决方案，它提供了屏幕读取、输入模拟和图像处理等功能。

项目特点：
*   **原生开发**: 基于 C++17 编写，注重运行效率。
*   **跨架构**: 完整支持 32 位和 64 位应用程序。
*   **易于集成**: 标准 COM 接口，兼容 Python, C#, C++, Lua 等多种语言。

## 📖 文档与工具 (Docs & Tools)

*   **官方文档**: [Wiki](https://github.com/WallBreaker2/op/wiki)
*   **GUI测试工具**: [OPTestTool](https://github.com/flaot/OPTestTool) (由 float 提供)

## ✨ 主要特性 (Key Features)

### 🖥️ 窗口与输入自动化
*   **后台操作**: 向非激活或最小化的窗口发送按键和鼠标点击。
*   **输入模拟**: 基于 Windows API (SendInput/SendMessage) 的系统级模拟。
*   **窗口管理**: 查找、移动、调整大小以及查询窗口状态。

### 🖼️ 图像与颜色处理
*   **智能找图**: 支持透明度和颜色偏差的模糊找图功能。
*   **内存找色**: 高速的多点找色算法，直接读取内存数据。
*   **多引擎截图**: 支持 GDI, DirectX (DX), 和 OpenGL 屏幕截图。
*   **安卓支持**: 专为各类主流安卓模拟器定制的截图功能。

### 📝 OCR (光学字符识别)
*   **双引擎支持**: 
    *   **Tesseract**: 由 Google Tesseract 引擎驱动，处理复杂文本识别。
    *   **Native**: 轻量级、高速的字典匹配算法，适用于固定字体。
*   **极速响应**: 专为实时游戏文字识别优化。

## 📦 安装 (Installation)

1.  **下载**: 前往 [GitHub Releases](https://github.com/WallBreaker2/op/releases) 下载最新版本的压缩包。
2.  **解压**: 将文件解压到本地目录。
3.  **注册 COM 组件**:
    以 **管理员身份** 打开终端，并运行以下命令：
    ```powershell
    # 对于 32 位应用程序
    regsvr32 op_x86.dll

    # 对于 64 位应用程序
    regsvr32 op_x64.dll
    ```

## 🚀 快速开始 (Python)

以下是一个使用 Python 调用插件进行找图的简单示例。

```python
from win32com.client import Dispatch
import os

# 1. 初始化 COM 对象
op = Dispatch("op.opsoft")
print(f"插件版本: {op.Ver()}")

# 2. 设置路径 (可选，设置图片文件的基础路径)
# op.SetPath("C:\\Your\\Image\\Folder")

# 3. 执行找图
# 参数: x1, y1, x2, y2, pic_name, delta_color, sim, dir
# 分别为: 左上x, 左上y, 右下x, 右下y, 图片名, 偏色, 相似度, 方向
ret, x, y = op.FindPic(0, 0, 1920, 1080, "test.png", "000000", 0.9, 0)

if ret == 1:
    print(f"找到图片: ({x}, {y})")
    op.MoveTo(x, y)
    op.LeftClick()
else:
    print("未找到图片.")
```

## 🛠️ 源码编译 (Build from Source)

### 环境要求
*   **Visual Studio 2022** (MSVC v143 toolset)
*   **CMake** (3.24 或更高版本)
*   **Windows SDK** (10.0.19041.0 或更高版本)

### 依赖项
本项目依赖以下第三方库：
*   **Blackbone**: 用于进程内存操作和注入 (静态链接)。
*   **Kiero**: 用于 DirectX Hook (源码集成)。
*   **Tesseract**: 用于 OCR 功能 (动态链接)。

在配置之前，您需要提供 BlackBone 的头文件和库路径。推荐设置 `BLACKBONE_ROOT` 指向 BlackBone 仓库根目录，CMake 会自动探测常见输出路径（包括命令行构建和旧版 VS 方案输出）。

```powershell
set BLACKBONE_ROOT="D:\path\to\Blackbone"
```

如果自动探测失败，可显式指定：

```bash
cmake -S . -B build -DBLACKBONE_INCLUDE_DIR="D:/path/to/Blackbone/src" -DBLACKBONE_LIBRARY="D:/path/to/Blackbone/build/x64/BlackBone/Release/BlackBone.lib"
```

### 编译步骤

1.  克隆仓库:
    ```bash
    git clone https://github.com/WallBreaker2/op.git
    cd op
    ```

2.  创建构建目录:
    ```bash
    mkdir build && cd build
    ```

3.  配置并编译:
    ```bash
    cmake ..
    cmake --build . --config Release
    ```

## 🤝 社区与支持

*   **Issues**: [GitHub Issues](https://github.com/WallBreaker2/op/issues) - 报告 Bug 或 提交功能建议。
*   **Discussions**: [GitHub Discussions](https://github.com/WallBreaker2/op/discussions) - 提问或分享想法。
*   **QQ 群**: 
    *   1群: `743710486` (已满)
    *   2群: `27872381` (活跃)

## 📜 许可证

本项目基于 [MIT License](LICENSE) 开源。

## 📚 参考项目 (References)

*   [TSPLUG](https://github.com/tcplugins/tsplug) - 参考了部分函数实现
*   [Kiero](https://github.com/Rebzzel/kiero) - 提供了D3DHook的基础
