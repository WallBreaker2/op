# OP - Windows Automation Plugin

[![GitHub Release](https://img.shields.io/github/v/release/WallBreaker2/op?style=flat-square)](https://github.com/WallBreaker2/op/releases)
[![CI](https://github.com/WallBreaker2/op/actions/workflows/ci.yml/badge.svg)](https://github.com/WallBreaker2/op/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows-blue.svg)](https://microsoft.com/windows)

[中文](README.md)

OP (Operator & Open) is an open-source automation plugin for Windows. It provides window automation, background mouse and keyboard input, screen capture, image/color search, OCR, YOLO HTTP detection, OpenCV image processing, memory access, and related desktop automation features. The core is written in C++ and exposed through COM interfaces with x86/x64 support.

OCR supports two paths: fixed-font scenarios can use local bitmap dictionaries, including OP binary dictionaries and the text bitmap dictionary format compatible with DaMo; when no bitmap dictionary is used, OP can call the standalone OCR HTTP service [op_ocr_engine](https://github.com/WallBreaker2/op_ocr_engine), with Tesseract, PaddleOCR, and other general/model OCR backends. YOLO detection uses the same external HTTP service pattern: OP captures or loads images, then sends them to a separate YOLO11/YOLOv11 backend. See [YOLO HTTP detection](doc/yolo.md). A minimal sample service is available at `tools/op_yolo_engine.py`.

## Documentation

- [GitHub Wiki](https://github.com/WallBreaker2/op/wiki): installation, APIs, OpenCV, OCR, registration-free usage, and language demos
- [Releases](https://github.com/WallBreaker2/op/releases)
- [OPTestTool](https://github.com/flaot/OPTestTool)

## Features

- Window search, window state control, window layout, and background binding
- Foreground and background mouse/keyboard simulation
- GDI, DXGI, WGC, DirectX, and OpenGL capture modes
- Color search, image search, image input sources, and memory image input
- Bitmap-dictionary OCR, compatible with OP dictionaries and DaMo text bitmap dictionaries
- Standalone OCR HTTP service integration for Tesseract, PaddleOCR, and other general/model OCR backends
- Standalone YOLO HTTP detection service integration for YOLO11/YOLOv11 and other external detection backends
- OpenCV template matching, feature matching, and file preprocessing
- Process memory access, assembly calls, and utility algorithms

## Repository Layout

```text
op/
├─ libop/          Core plugin source
│  ├─ com/         COM registration, IDL, type library, and IOpAutomation implementation
│  ├─ binding/     Window binding and background-mode dispatch
│  ├─ capture/     Capture sources and GDI/DXGI/WGC/Hook capture backends
│  ├─ input/       Mouse, keyboard, and DX input backends
│  ├─ hook/        Display/input hooks, injection protocol, and exported hook entrypoints
│  ├─ image/       Color/image search, bitmap OCR, OCR HTTP wrapper, and YOLO HTTP wrapper
│  ├─ opencv/      OpenCV template matching, feature matching, preprocessing, and bridge layer
│  ├─ windows/      Windows API wrappers for windows, processes, memory, and injection
│  ├─ core/        Shared utilities, paths, environment, pipes, and window layout helpers
│  ├─ common/      Internal image, color, dictionary, and shared-memory structures
│  ├─ algorithm/   Common algorithms such as A*
│  ├─ libop.cpp    Main C++ interface implementation used by COM/SWIG
├─ include/        Public headers and exported interfaces
├─ tools/          Registration-free loader source, builds tools.dll
├─ swig/           Python SWIG binding files
├─ examples/       Local examples and test assets
├─ tests/          C++ unit and integration tests
├─ doc/op.wiki/    GitHub Wiki documentation source
├─ 3rd_party/      Third-party source or local dependencies
├─ ci/             CI helper scripts
├─ bin/            Runtime files or built binaries
├─ out/            Historical or optional output directory
├─ build.py        Recommended one-command build entry
└─ CMakeLists.txt  CMake project entry
```

## Quick Start

Download a release package, then register the DLL that matches your host process bitness:

```powershell
# 32-bit host process
regsvr32 op_x86.dll

# 64-bit host process
regsvr32 op_x64.dll
```

Minimal Python example:

```python
from win32com.client import Dispatch

op = Dispatch("op.opsoft")
print("op version:", op.Ver())
```

For registration-free usage, see the Wiki:

- [Installation and registration-free usage](https://github.com/WallBreaker2/op/wiki/docs/install)
- [Python registration-free example](https://github.com/WallBreaker2/op/wiki/demo/python-regfree)
- [C# / Lua / Golang / Rust / Node.js / Java demos](https://github.com/WallBreaker2/op/wiki/Home)

## Build

Requirements:

- Visual Studio 2022 or newer
- CMake 3.24 or newer
- Windows SDK 10.0.19041.0 or newer

Recommended build entry:

```powershell
# Default: Release + x64
python build.py

# Build x86
python build.py -a x86

# Debug build
python build.py -t Debug
```

Release artifacts are installed to `bin/x86` or `bin/x64`.

## Community

- [GitHub Issues](https://github.com/WallBreaker2/op/issues)
- [GitHub Discussions](https://github.com/WallBreaker2/op/discussions)
- QQ groups: `743710486`, `27872381`

## License

[MIT License](LICENSE)

## References

- [TSPLUG](https://github.com/tcplugins/tsplug)
- [Kiero](https://github.com/Rebzzel/kiero)

