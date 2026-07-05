# OP - Windows 自动化插件

[![GitHub Release](https://img.shields.io/github/v/release/WallBreaker2/op?style=flat-square)](https://github.com/WallBreaker2/op/releases)
[![CI](https://github.com/WallBreaker2/op/actions/workflows/ci.yml/badge.svg)](https://github.com/WallBreaker2/op/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows-blue.svg)](https://microsoft.com/windows)

[English](README_EN.md)

OP（Operator & Open）是一个面向 Windows 的自动化插件。它把窗口查找、后台绑定、截图、键鼠输入、找色找图、OCR、OpenCV、YOLO HTTP 检测和进程内存读写放在同一套接口里，方便脚本工具和桌面自动化程序直接调用。

核心代码使用 C++ 实现，提供 COM、C API、Python 和 Go 绑定，支持 x86/x64。截图后端覆盖普通窗口、GDI、DXGI、WGC、DirectX Hook、OpenGL 和 OpenGL ES；字库、图片模板可以从本地文件加载，也可以从内存加载，适合把资源随程序一起打包。

OCR 目前有两条路线：固定字体场景使用点阵字库，兼容 OP 二进制字库和大漠文本点阵字库；通用 OCR 可以接入独立 HTTP 服务 [op_ocr_engine](https://github.com/WallBreaker2/op_ocr_engine)，由 Tesseract、PaddleOCR 等后端完成识别。

YOLO 检测同样走外部 HTTP 服务模式。OP 负责截图、读图和请求封装，模型推理由独立服务完成。接口格式见 [YOLO HTTP 检测](doc/yolo.md)，仓库里也放了一个最小服务样例：`tools/op_yolo_engine.py`。

## 文档

- [GitHub Wiki](https://github.com/WallBreaker2/op/wiki)：安装、接口说明、OpenCV、OCR、免注册、多语言示例
- [插件下载](https://github.com/WallBreaker2/op/releases)
- [本地 OCR 说明](doc/ocr.md)
- [YOLO HTTP 检测](doc/yolo.md)
- [测试工具 OPTestTool](https://github.com/flaot/OPTestTool)

## 主要能力

- 窗口枚举、进程查询、窗口状态控制、批量窗口布局
- 普通绑定、后台绑定、显示句柄和输入句柄分离绑定
- GDI、DXGI、WGC、DX Hook、OpenGL、OpenGL ES 等截图方式
- 前台/后台鼠标键盘模拟，支持平滑移动、轨迹移动和 DX 输入锁定
- 找色、找图、透明图片模板、OpenCV 模板匹配和特征匹配
- 点阵字库 OCR，支持本地字库和内存字库
- OCR / YOLO HTTP 服务接入
- C API、COM、Python、Go 等调用方式
- 进程内存读写、汇编调用和常用算法工具

## 目录概览

```text
op/
├─ libop/            核心 C++ 源码
│  ├─ op/            op::Op 对外接口的分文件实现
│  ├─ c_api/         C API 封装
│  ├─ com/           COM 组件、IDL 和自动化接口实现
│  ├─ binding/       窗口绑定和后台模式调度
│  ├─ capture/       GDI、DXGI、WGC、Hook 等截图后端
│  ├─ hook/          远端注入、显示 hook、输入 hook 和共享帧写入
│  ├─ input/         鼠标、键盘输入后端
│  ├─ image/         图片加载、找色、找图和图像搜索服务
│  ├─ ocr/           字库管理和 OCR 识别
│  ├─ opencv/        OpenCV 桥接、模板匹配和图像处理
│  ├─ window/        窗口、进程和 DLL 注入相关能力
│  ├─ base/          基础类型、运行环境和工具函数
│  ├─ ipc/           共享内存、互斥量、管道等进程间通信封装
│  ├─ memory/        目标进程内存读写
│  ├─ network/       HTTP 客户端等网络辅助能力
│  ├─ algorithm/     内部算法与通用计算逻辑
│  └─ yolo/          YOLO 检测器封装
├─ include/          对外头文件
├─ bindings/         C API 的 Python、Go 包装
├─ swig/             SWIG 绑定生成文件
├─ python/           `pyop` Python 包源码
├─ tools/            免注册加载工具和 YOLO 服务样例
├─ examples/         本地示例和测试资源
├─ tests/            C++ 单元测试和集成测试
├─ doc/              项目内补充文档和流程图
├─ scripts/          wheel 构建脚本
├─ ci/               CI triplet 和辅助配置
├─ 3rd_party/        第三方源码或本地依赖
├─ build.py          推荐的一键构建入口
└─ CMakeLists.txt    CMake 工程入口
```

## 快速开始

下载 Release 后，根据宿主程序位数使用对应 DLL。COM 方式需要注册：

```powershell
# 32 位宿主程序
regsvr32 .\op_x86.dll

# 64 位宿主程序
regsvr32 .\op_x64.dll
```

Python 通过 COM 调用：

```python
from win32com.client import Dispatch

op = Dispatch("op.opsoft")
print("op version:", op.Ver())
```

免注册调用请看 Wiki：

- [安装与免注册](https://github.com/WallBreaker2/op/wiki/docs/install)
- [Python 免注册示例](https://github.com/WallBreaker2/op/wiki/demo/python-regfree)
- [C# / Lua / Golang / Rust / Node.js / Java 示例](https://github.com/WallBreaker2/op/wiki/Home)

## Python

`op-plugins` wheel 面向直接使用 `pyop` 的用户，支持 Python 3.9-3.12，提供 `win32` 和 `win_amd64` 两种架构：

```powershell
pip install op-plugins
```

```python
from pyop import Op

op = Op()
print("op version:", op.Ver())
```

如果从 Release 安装指定 wheel，把 `<tag>` 和 `<wheel>` 换成实际文件名：

```powershell
pip install https://github.com/WallBreaker2/op/releases/download/<tag>/<wheel>.whl
```

注意：

- 64 位 Python 使用 `win_amd64` wheel，32 位 Python 使用 `win32` wheel
- wheel 已内置对应架构的 OP 运行文件，不需要再手动复制 zip 里的 DLL
- `bindings/python` 是基于 `ctypes` 的 C API 包装，和 SWIG 版 `pyop` 相互独立，适合需要直接调用 `op_c_api_*.dll` 的场景

验证安装：

```powershell
python -c "from pyop import Op; print(Op().Ver())"
```

## 源码编译

环境要求：

- Windows 10 或更新版本
- Visual Studio 2022 或更新版本
- CMake 3.24 或更新版本
- Python 3.12（用于构建脚本、SWIG 绑定和测试工具）

推荐使用根目录 `build.py`：

```powershell
# 当前 CI 使用的 Release x64 构建
python build.py -g vs2026 -t Release -a x64

# 构建 x86
python build.py -g vs2026 -t Release -a x86

# Debug
python build.py -g vs2026 -t Debug -a x64

# 如果本机是 VS2022，可以把 -g 改成 vs2022
python build.py -g vs2022 -t Release -a x64
```

Release 产物会安装到 `bin/x86` 或 `bin/x64`。发布包通常包含：

```text
op_x86.dll / op_x64.dll
op_c_api_x86.dll / op_c_api_x64.dll
tools.dll
_pyop.pyd
pyop.py
lib/op_c_api_x86.lib / lib/op_c_api_x64.lib
```

本地构建 wheel：

```powershell
pip install scikit-build-core setuptools-scm
.\scripts\build_wheel.ps1
```

如果手动构建，先跑一次 `build.py` 完成依赖引导：

```powershell
python build.py -g vs2026 -t Release -a x64
pip wheel . --no-deps -w wheelhouse
```

## 社区

- [GitHub Issues](https://github.com/WallBreaker2/op/issues)
- [GitHub Discussions](https://github.com/WallBreaker2/op/discussions)
- QQ 群：`743710486`、`27872381`

## 许可证

[MIT License](LICENSE)

## 参考项目

- [TSPLUG](https://github.com/tcplugins/tsplug)
- [Kiero](https://github.com/Rebzzel/kiero)
