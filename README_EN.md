# OP - Windows Automation Plugin

[![GitHub Release](https://img.shields.io/github/v/release/WallBreaker2/op?style=flat-square)](https://github.com/WallBreaker2/op/releases)
[![CI](https://github.com/WallBreaker2/op/actions/workflows/ci.yml/badge.svg)](https://github.com/WallBreaker2/op/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows-blue.svg)](https://microsoft.com/windows)

[中文](README.md)

OP (Operator & Open) is a Windows automation plugin. It brings window discovery, background binding, screen capture, mouse and keyboard input, color and image search, OCR, OpenCV, YOLO HTTP detection, and process memory access into one set of APIs for scripting tools and desktop automation programs.

The core is written in C++. It exposes COM, C API, Python, and Go bindings, with x86 and x64 builds. Capture backends cover normal windows, GDI, DXGI, WGC, DirectX hooks, OpenGL, and OpenGL ES. Image templates and OCR dictionaries can be loaded from files or from memory, which makes it easier to ship resources inside your own program.

OCR has two practical paths. Fixed-font scenes can use local bitmap dictionaries, including OP binary dictionaries and the text bitmap dictionary format compatible with DaMo. General OCR can be delegated to the standalone HTTP service [op_ocr_engine](https://github.com/WallBreaker2/op_ocr_engine), backed by engines such as Tesseract or PaddleOCR.

YOLO detection follows the same external-service model. OP captures or loads the image and wraps the HTTP request; model inference runs in a separate service. See [YOLO HTTP detection](doc/yolo.md) for the API format. A minimal sample service is included at `tools/op_yolo_engine.py`.

## Documentation

- [GitHub Wiki](https://github.com/WallBreaker2/op/wiki): installation, API reference, OpenCV, OCR, registration-free usage, and language demos
- [Releases](https://github.com/WallBreaker2/op/releases)
- [Local OCR notes](doc/ocr.md)
- [YOLO HTTP detection](doc/yolo.md)
- [OPTestTool](https://github.com/flaot/OPTestTool)

## Features

- Window enumeration, process lookup, window state control, and batch window layout
- Normal binding, background binding, and separated display/input window handles
- GDI, DXGI, WGC, DX hook, OpenGL, and OpenGL ES capture modes
- Foreground/background mouse and keyboard input, smooth movement, path movement, and DX input locking
- Color search, image search, transparent templates, OpenCV template matching, and feature matching
- Bitmap-dictionary OCR with file and memory dictionary loading
- OCR and YOLO HTTP service integration
- COM, C API, Python, and Go access
- Process memory read/write, assembly calls, and utility algorithms

## Repository Layout

```text
op/
├─ libop/            Core C++ source
│  ├─ op/            Split implementation of the public op::Op API
│  ├─ c_api/         C API wrapper
│  ├─ com/           COM component, IDL, and automation interface
│  ├─ binding/       Window binding and background-mode dispatch
│  ├─ capture/       GDI, DXGI, WGC, Hook, and related capture backends
│  ├─ hook/          Remote injection, display/input hooks, and shared-frame writing
│  ├─ input/         Mouse and keyboard input backends
│  ├─ image/         Image loading, color search, image search, and image services
│  ├─ ocr/           Dictionary management and OCR implementation
│  ├─ opencv/        OpenCV bridge, template matching, and image processing
│  ├─ window/        Window, process, and DLL injection helpers
│  ├─ base/          Basic types, runtime environment, and utility functions
│  ├─ ipc/           Shared memory, mutexes, pipes, and other IPC helpers
│  ├─ memory/        Target-process memory access
│  ├─ network/       HTTP client and network helpers
│  ├─ algorithm/     Internal algorithms and shared calculation logic
│  └─ yolo/          YOLO detector wrapper
├─ include/          Public headers
├─ bindings/         Python and Go wrappers over the C API
├─ swig/             SWIG-generated binding files
├─ python/           `pyop` Python package source
├─ tools/            Registration-free loader tools and YOLO sample service
├─ examples/         Local examples and test assets
├─ tests/            C++ unit and integration tests
├─ doc/              In-repository notes and diagrams
├─ scripts/          Wheel build scripts
├─ ci/               CI triplets and helper configuration
├─ 3rd_party/        Third-party source or local dependencies
├─ build.py          Recommended one-command build entry
└─ CMakeLists.txt    CMake project entry
```

## Quick Start

Download a release package and use the DLL that matches the bitness of your host process. COM usage requires registration:

```powershell
# 32-bit host process
regsvr32 .\op_x86.dll

# 64-bit host process
regsvr32 .\op_x64.dll
```

Minimal Python example through COM:

```python
from win32com.client import Dispatch

op = Dispatch("op.opsoft")
print("op version:", op.Ver())
```

For registration-free usage, see the Wiki:

- [Installation and registration-free usage](https://github.com/WallBreaker2/op/wiki/docs/install)
- [Python registration-free example](https://github.com/WallBreaker2/op/wiki/demo/python-regfree)
- [C# / Lua / Golang / Rust / Node.js / Java demos](https://github.com/WallBreaker2/op/wiki/Home)

## Python

The `op-plugins` wheel is for users who want to use `pyop` directly. It supports Python 3.9-3.12 and provides both `win32` and `win_amd64` builds:

```powershell
pip install op-plugins
```

```python
from pyop import Op

op = Op()
print("op version:", op.Ver())
```

To install a specific wheel from GitHub Releases, replace `<tag>` and `<wheel>` with the actual names:

```powershell
pip install https://github.com/WallBreaker2/op/releases/download/<tag>/<wheel>.whl
```

Notes:

- 64-bit Python needs a `win_amd64` wheel; 32-bit Python needs a `win32` wheel
- The wheel already includes the OP runtime files for the matching architecture
- `bindings/python` is a separate `ctypes` wrapper over the C API. Use it when you want to call `op_c_api_*.dll` directly instead of going through the SWIG `pyop` module

Verify the installation:

```powershell
python -c "from pyop import Op; print(Op().Ver())"
```

## Build

Requirements:

- Windows 10 or newer
- Visual Studio 2022 or newer
- CMake 3.24 or newer
- Python 3.12 for build scripts, SWIG bindings, and test tools

Use `build.py` from the repository root:

```powershell
# Release x64, matching the current CI setup
python build.py -g vs2026 -t Release -a x64

# Build x86
python build.py -g vs2026 -t Release -a x86

# Debug build
python build.py -g vs2026 -t Debug -a x64

# If your machine uses VS2022, switch the generator
python build.py -g vs2022 -t Release -a x64
```

Release artifacts are installed to `bin/x86` or `bin/x64`. A release package normally contains:

```text
op_x86.dll / op_x64.dll
op_c_api_x86.dll / op_c_api_x64.dll
tools.dll
_pyop.pyd
pyop.py
lib/op_c_api_x86.lib / lib/op_c_api_x64.lib
```

Build a local wheel:

```powershell
pip install scikit-build-core setuptools-scm
.\scripts\build_wheel.ps1
```

For a manual wheel build, run `build.py` once first so native dependencies are bootstrapped:

```powershell
python build.py -g vs2026 -t Release -a x64
pip wheel . --no-deps -w wheelhouse
```

## Community

- [GitHub Issues](https://github.com/WallBreaker2/op/issues)
- [GitHub Discussions](https://github.com/WallBreaker2/op/discussions)
- QQ groups: `743710486`, `27872381`

## License

[MIT License](LICENSE)

## References

- [TSPLUG](https://github.com/tcplugins/tsplug)
- [Kiero](https://github.com/Rebzzel/kiero)
