# OP 自动化插件

[![GitHub Release](https://img.shields.io/github/v/release/WallBreaker2/op?style=flat-square)](https://github.com/WallBreaker2/op/releases)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

## 📖 项目概述

**OP**（Operator & Open）是一款开源自动化工具，专为Windows平台设计。核心功能包括：

- 🔍 **窗口自动化**：键鼠消息模拟/后台截图
- 🖼️ **图像处理**：智能找图/色值识别
- ✨ **OCR引擎**：传统算法+AI引擎双模式
- 🛠️ **平台支持**：原生32/64位双版本

![架构图](doc/class_struct.svg)

## 🚀 核心特性

### 自动化控制
- Windows消息级键鼠模拟
- 多渲染引擎（GDI/DirectX/OpenGL）截图
- 主流安卓模拟器最小化截图

### 图像识别
- 智能模糊匹配（支持偏色/透明度）
- 多区域并行搜索
- 自适应屏幕缩放识别

### OCR引擎
```python
# 双模式文字识别示例
text = op.Ocr(0,0,2000,2000,"eng",0.8)  # Tesseract引擎
legacy_text = op.OcrEx(0,0,500,500,"sysfont")  # 传统算法（最大255x255点阵）

## 📦 快速开始

### 环境准备
1. 下载最新发行版：[GitHub Releases](https://github.com/WallBreaker2/op/releases)
2. 注册COM组件（管理员权限）：
```powershell
regsvr32 op_x86.dll  # 32位
regsvr32 op_x64.dll  # 64位
```

### Python示例
```python
from win32com.client import Dispatch

op = Dispatch("op.opsoft")
print(f"SDK版本: {op.Ver()}")

# 图像搜索（返回坐标元组）
found, x, y = op.FindPic(
    0, 0, 1920, 1080,
    "button.png",
    "101010",  # 偏色值
    0.9,       # 相似度
    0          # 搜索模式
)
```

## 🛠️ 开发指南

### 编译要求
| 组件              | 版本要求          |
|-------------------|------------------|
| Visual Studio     | 2022 (MSVC 143)  |
| CMake             | ≥3.24            |
| Windows SDK       | 10.0.19041.0     |

### 依赖管理
```bash
# 配置环境变量
set BLACKBONE_ROOT="D:\libs\Blackbone"
```

| 依赖库            | 编译方式 | 备注                 |
|-------------------|----------|----------------------|
| Blackbone         | 静态链接 | 进程注入核心         |
| Kiero             | 源码集成 | DirectX Hook实现     |
| Tesseract         | 动态链接 | OCR引擎（默认包含）  |

## 💬 社区支持
- 官方讨论区: [GitHub Discussions](https://github.com/WallBreaker2/op/discussions)
- QQ交流群: 27872381（新）743710486（满）
- 问题追踪: [Issues](https://github.com/WallBreaker2/op/issues)

## 📜 开源协议
本项目基于 [MIT License](LICENSE) 开源，特别感谢：
- [TSPLUG](https://github.com/tcplugins/tsplug) - Windows部分函数参考实现
- [Kiero](https://github.com/Rebzzel/kiero) - DirectX注入方案
``` 
