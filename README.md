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
│  ├─ com/         COM 注册、IDL、类型库和 IOpInterface 对外接口实现
│  ├─ background/  窗口绑定、截图输入源、后台显示/鼠标/键盘调度
│  ├─ imageProc/   找色、找图、点阵 OCR、OCR HTTP 服务封装
│  ├─ opencv/      OpenCV 模板匹配、特征匹配、预处理和桥接层
│  ├─ winapi/      窗口、进程、内存、注入等 Windows API 封装
│  ├─ core/        公共工具、路径、环境、管道、窗口布局等基础能力
│  ├─ include/     图像、颜色、字库、共享内存等内部基础结构
│  ├─ algorithm/   A* 等通用算法
│  ├─ libop.cpp    C++ 主接口实现，COM/SWIG 最终调用到这里
│  └─ libop.h      C++ 主接口声明
├─ include/        对外头文件和导出接口
├─ tools/          免注册加载工具源码，生成 tools.dll
├─ swig/           Python SWIG 绑定文件
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

Python 最小示例：

```python
from win32com.client import Dispatch

op = Dispatch("op.opsoft")
print("op version:", op.Ver())
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
python build.py -t Release -a x64 -g vs2026
```

Release 产物会安装到 `bin/x86` 或 `bin/x64`。

## 社区

- [GitHub Issues](https://github.com/WallBreaker2/op/issues)
- [GitHub Discussions](https://github.com/WallBreaker2/op/discussions)
- QQ 群：`743710486`、`27872381`

## 许可证

[MIT License](LICENSE)

## 参考项目

- [TSPLUG](https://github.com/tcplugins/tsplug)
- [Kiero](https://github.com/Rebzzel/kiero)
