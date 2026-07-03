# OP - Windows 自动化插件

[![GitHub Release](https://img.shields.io/github/v/release/WallBreaker2/op?style=flat-square)](https://github.com/WallBreaker2/op/releases)
[![CI](https://github.com/WallBreaker2/op/actions/workflows/ci.yml/badge.svg)](https://github.com/WallBreaker2/op/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows-blue.svg)](https://microsoft.com/windows)

[English](README_EN.md)

OP (Operator & Open) 是一个面向 Windows 的开源自动化插件，提供窗口操作、后台键鼠、截图、找色找图、OCR、OpenCV 图像处理、内存读写等能力。项目以 C++ 实现，提供 COM 接口，支持 x86/x64。

OCR 同时支持两条路径：固定字体场景可以使用本地点阵字库，兼容 OP 二进制字库和大漠文本点阵字库格式；不使用点阵字库时，可以接入独立 OCR HTTP 服务 [op_ocr_engine](https://github.com/WallBreaker2/op_ocr_engine)，支持 Tesseract 和 PaddleOCR 等通用/模型 OCR 后端。

YOLO 检测采用同类外部 HTTP 服务模式：OP 负责截图、读图和 HTTP 调用，YOLO11/YOLOv11 模型推理由独立服务完成。接口与返回格式见 [YOLO HTTP 检测](doc/yolo.md)。
仓库内提供了最小服务样例：`tools/op_yolo_engine.py`。

## 文档

- [GitHub Wiki](https://github.com/WallBreaker2/op/wiki)：安装、接口说明、OpenCV、OCR、免注册、多语言示例
- [插件下载](https://github.com/WallBreaker2/op/releases)
- [测试工具 OPTestTool](https://github.com/flaot/OPTestTool)

## 主要能力

- 窗口查询、窗口状态、窗口布局和后台绑定
- 前台/后台鼠标键盘模拟
- GDI、DXGI、WGC、DirectX、OpenGL 等截图方式
- 找色、找图、图片输入源和内存图片输入
- 点阵字库 OCR，兼容 OP 字库和大漠文本点阵字库格式
- 独立 OCR HTTP 服务接入，支持 Tesseract、PaddleOCR 等通用/模型 OCR 后端
- 独立 YOLO HTTP 检测服务接入，支持 YOLO11/YOLOv11 等外部检测后端
- OpenCV 模板匹配、特征匹配和文件预处理
- 进程内存读写、汇编调用和基础算法工具

## 目录概览

```text
op/
├─ libop/          核心插件源码
│  ├─ com/         COM 注册、IDL、类型库和 IOpAutomation 对外接口实现
│  ├─ binding/     窗口绑定与后台模式调度
│  ├─ capture/     截图输入源和 GDI/DXGI/WGC/Hook 采集后端
│  ├─ input/       鼠标、键盘和 DX 输入后端
│  ├─ hook/        显示/输入 hook、注入协议和导出入口
│  ├─ image/       找色、找图、点阵 OCR、OCR HTTP 服务封装
│  ├─ opencv/      OpenCV 模板匹配、特征匹配、预处理和桥接层
│  ├─ windows/      窗口、进程、内存、注入等 Windows API 封装
│  ├─ op/          C++ 主接口 op::Op 的分文件实现
│  ├─ common/      图像、颜色、字库、共享内存等内部基础结构
│  ├─ algorithm/   A* 等通用算法
│  ├─ libop.cpp    op::Op 构造、析构和上下文初始化
├─ include/        对外头文件和导出接口
├─ tools/          免注册加载工具源码，生成 tools.dll
├─ swig/           Python SWIG 绑定文件
├─ python/         pip wheel 包源码（python/pyop）
├─ examples/       本地测试示例和测试资源
├─ tests/          C++ 单元测试和集成测试
├─ doc/op.wiki/    GitHub Wiki 文档源码
├─ 3rd_party/      第三方源码或本地依赖
├─ ci/             CI 辅助脚本
├─ bin/            预置或构建后的运行文件
├─ out/            历史或可选输出目录
├─ build.py        推荐的一键构建入口
└─ CMakeLists.txt  CMake 工程入口
```

## 快速开始

下载 Release 后，根据宿主程序位数注册对应 DLL：

```powershell
# 32 位宿主程序
regsvr32 op_x86.dll

# 64 位宿主程序
regsvr32 op_x64.dll
```

Python 最小示例（COM，与 Python 版本无关）：

```python
from win32com.client import Dispatch

op = Dispatch("op.opsoft")
print("op version:", op.Ver())
```

### pip 安装（推荐 Python 用户使用）

`op-plugins` 通过 PyPI 发布多版本 wheel（Python 3.9–3.12，win32/win_amd64），自动匹配本地 Python 版本，无需手动挑选 `_pyop.pyd`：

```powershell
# 从 PyPI 安装（推荐）
pip install op-plugins

# 或从 GitHub Release 安装指定 wheel（将 <tag> 和 <wheel> 替换为实际文件名）
pip install https://github.com/WallBreaker2/op/releases/download/<tag>/<wheel>.whl

# 示例：64 位 Python 3.12
pip install https://github.com/WallBreaker2/op/releases/download/v1.0.0/op_plugins-1.0.0-cp312-cp312-win_amd64.whl
```

安装后使用 SWIG 绑定：

```python
from pyop import Op

op = Op()
print("op version:", op.Ver())
```

注意：

- 64 位 Python 请安装 `win_amd64` wheel；32 位 Python / 32 位游戏场景请安装 `win32` wheel
- wheel 已内置 `op_x64.dll` / `op_x86.dll` 和 `tools.dll`，无需单独下载 zip
- 若仍使用 zip 分发，须确保 `_pyop.pyd` 与本地 Python 版本一致（如 cp312 对应 Python 3.12）

验证安装：

```powershell
python -c "from pyop import Op; print(Op().Ver())"
```

免注册调用请参考 Wiki：

- [安装与免注册](https://github.com/WallBreaker2/op/wiki/install)
- [Python 免注册示例](https://github.com/WallBreaker2/op/wiki/python-regfree)
- [C# / Lua / Golang / Rust / Node.js / Java 示例](https://github.com/WallBreaker2/op/wiki#demo)

## 源码编译

环境要求：

- Visual Studio 2022 或更新版本
- CMake 3.24 或更新版本
- Windows SDK 10.0.19041.0 或更新版本

推荐使用根目录 `build.py`：

```powershell
# 默认 Release + x64
python build.py

# 构建 x86
python build.py -a x86

# Debug
python build.py -t Debug

# 指定环境
python build.py -t Release -a x64 -g vs2022
```

Release 产物会安装到 `bin/x86` 或 `bin/x64`。

本地构建 pip wheel（推荐用脚本自动 bootstrap 依赖并设置 CMake 参数）：

```powershell
# 先确保已安装构建依赖
pip install scikit-build-core setuptools-scm

# 一键构建（默认 x64，自动检测 VS 版本）
./scripts/build_wheel.ps1

# 或手动：先 bootstrap，再 pip wheel
python build.py -t Release -a x64
pip wheel . --no-deps -w wheelhouse

# 本地模拟 CI 的单版本双架构 wheel 构建
pip install cibuildwheel==2.23.4
$env:CIBW_BUILD="cp312-*"
$env:CIBW_ARCHS_WINDOWS="AMD64"
$env:CIBW_BEFORE_BUILD_WINDOWS="powershell ./scripts/cibw_before_build.ps1"
$env:CIBW_ENVIRONMENT_WINDOWS='CMAKE_GENERATOR="Visual Studio 17 2022" CMAKE_ARGS="-A x64 -DOP_PYTHON_WHEEL=ON -DOP_BUILD_TESTS=OFF -Dbuild_swig_py=ON"'
python -m cibuildwheel --platform windows

$env:CIBW_ARCHS_WINDOWS="x86"
$env:CIBW_ENVIRONMENT_WINDOWS='CMAKE_GENERATOR="Visual Studio 17 2022" CMAKE_ARGS="-A Win32 -DOP_PYTHON_WHEEL=ON -DOP_BUILD_TESTS=OFF -Dbuild_swig_py=ON"'
python -m cibuildwheel --platform windows
```

若 `pip wheel` 报 BlackBone 未找到，说明尚未运行 `python build.py` 或 `./scripts/build_wheel.ps1` 完成依赖引导。

## 社区

- [GitHub Issues](https://github.com/WallBreaker2/op/issues)
- [GitHub Discussions](https://github.com/WallBreaker2/op/discussions)
- QQ 群：`743710486`、`27872381`

## 许可证

[MIT License](LICENSE)

## 参考项目

- [TSPLUG](https://github.com/tcplugins/tsplug)
- [Kiero](https://github.com/Rebzzel/kiero)
