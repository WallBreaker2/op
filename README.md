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
*   **服务化 OCR**:
    *   通过独立 `ocr_server`（来自 `op_ocr_engine`）提供 OCR 能力。
    *   `op` 侧通过 HTTP 调用服务，统一 x86/x64 调用路径。
*   **Native 字典识别**: 轻量级、高速的字典匹配算法，适用于固定字体。
*   **极速响应**: 专为实时游戏文字识别优化。

### OCR 服务模式（OcrEx / OcrAuto）

- 默认后端为 Tesseract，默认地址：`http://127.0.0.1:8080/api/v1/ocr`
- 若切换到 PaddleOCR，推荐地址：`http://127.0.0.1:8081/api/v1/ocr`
- 先启动服务（示例）：
  ```bash
  # Tesseract HTTP 服务
  ocr_server.exe --datapath ./tessdata --lang chi_sim --port 8080

  # PaddleOCR HTTP 服务
  python -m py_paddle_server.app --host 127.0.0.1 --port 8081 --print-results
  ```
- 可通过 `SetOcrEngine` 指定服务地址、后端别名与超时：
  - `path_of_engine` 或 `dll_name` 可直接传完整 URL
  - 也可直接传后端别名：`tesseract` / `paddle`
  - `argv` 支持：`--url=http://127.0.0.1:8080/api/v1/ocr --timeout=5000`
- 新增默认切换环境变量：
  - `OP_OCR_BACKEND=tesseract`：默认走 `http://127.0.0.1:8080/api/v1/ocr`
  - `OP_OCR_BACKEND=paddle`：默认走 `http://127.0.0.1:8081/api/v1/ocr`
  - `OP_OCR_URL=http://127.0.0.1:9000/api/v1/ocr`：显式覆盖默认服务地址
  - `OP_OCR_TIMEOUT_MS=5000`：覆盖默认超时

Python 示例：

```python
from win32com.client import Dispatch

op = Dispatch("op.opsoft")

# 1) 直接指定后端别名
op.SetOcrEngine("paddle", "", "--timeout=5000")

# 2) 或直接指定完整 URL
# op.SetOcrEngine("http://127.0.0.1:8081/api/v1/ocr", "", "--timeout=5000")

text = op.OcrAutoFromFile("screen.bmp", 0.8)
print(text)
```

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

### 内存图像输入（SetDisplayInput）

`SetDisplayInput` 支持两种 `mem:` 形式：

- `mem:<ptr>`：`ptr` 指向 BMP 内存（兼容历史行为）。
- `mem:<ptr>,<w>,<h>,<fmt>`：`ptr` 指向原始像素内存，`fmt` 支持 `bgra` 或 `bgr`（默认 `bgra`）。

示例（Python，OpenCV BGR 内存）：

```python
img_bgr = cv2.cvtColor(np.asarray(client.screen), cv2.COLOR_RGB2BGR)
mode = f"mem:{img_bgr.ctypes.data},{img_bgr.shape[1]},{img_bgr.shape[0]},bgr"
ret = op.SetDisplayInput(mode)
```

说明：`<ptr>` 同时支持十进制与十六进制地址字符串（例如 `0x7FF...`）。

### 文字发送说明（SendString / SendStringIme）

- `SendString(hwnd, str)`：优先向目标窗口当前焦点控件发送 `WM_CHAR`，适合英文、数字和大多数符号输入。
- `SendStringIme(hwnd, str)`：同时兼容 `WM_CHAR` 与 `WM_IME_CHAR` 路径，用于中文等 IME 相关输入场景。
- 建议：在后台窗口场景下，先确保目标输入框已获得焦点，再调用发送接口。
- 键盘布局差异（如全角/半角、非 US 布局）可能影响部分符号字符，请优先在目标机器实测。

### 后台键鼠模式说明

- `mouse=normal/windows/dx` 均可用，推荐在游戏窗口优先尝试 `dx`。
- `keypad=normal/normal.hd/windows` 可用；`keypad=dx` 当前未提供。
- `dx` 模式下，滚轮与按键状态依赖目标进程输入 Hook；若目标进程重启，建议重新 `BindWindow`。

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

从现在开始，推荐直接使用根目录 `build.py` 一键引导依赖并编译。脚本会自动处理：
*   安装/复用 `vcpkg`（优先复用已安装的 `VCPKG_ROOT` 或 `%USERPROFILE%\\vcpkg`）。
*   一次性安装 `x86/x64` 所需的 `gtest`、`minhook`。
*   拉取并构建 `BlackBone`（缓存到 `build/_deps`）。

如果您需要手动配置，仍可通过环境变量提供 BlackBone 路径。推荐设置 `BLACKBONE_ROOT` 指向 BlackBone 仓库根目录，CMake 会自动探测常见输出路径（包括命令行构建和旧版 VS 方案输出）。

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

2.  一键安装依赖并编译（推荐）:
    ```bash
    python build.py
    ```
    默认会生成目录：`build/vs2022-x64-Release`

3.  常用参数:
    ```bash
    # 指定架构构建
    python build.py -a x86
    # 输出目录: build/vs2022-x86-Release

    # 跳过依赖引导（已有完整环境时）
    python build.py --no-bootstrap-deps

    # 指定已有 vcpkg 路径
    python build.py --vcpkg-root D:/path/to/vcpkg
    ```

4.  使用 CMake 直接编译（推荐在 build.py 生成目录上执行）:
    ```bash
    cmake --build build/vs2022-x64-Release --config Release
    # x86:
    cmake --build build/vs2022-x86-Release --config Release
    ```

5.  传统 CMake 流程（高级用户可选）:
    ```bash
    cmake -S . -B build/Release
    cmake --build build/Release --config Release
    ```
    注意：该流程请避免在同一 `-B` 目录内混用 x86/x64，否则会触发 CMake 平台缓存冲突。

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
