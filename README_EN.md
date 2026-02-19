# OP - Windows Automation Plugin

[![GitHub Release](https://img.shields.io/github/v/release/WallBreaker2/op?style=flat-square)](https://github.com/WallBreaker2/op/releases)
[![CI](https://github.com/WallBreaker2/op/actions/workflows/ci.yml/badge.svg)](https://github.com/WallBreaker2/op/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows-blue.svg)](https://microsoft.com/windows)

## 📖 Introduction

**OP** (Operator & Open) is an open-source automation plugin designed for Windows. It serves as a modern desktop automation solution, offering functionality for screen reading, input simulation, and image processing.

Project Features:
*   **Native Development**: Written in C++17, focusing on efficiency.
*   **Cross-Architecture**: Full support for both 32-bit and 64-bit applications.
*   **Easy Integration**: Standard COM interface compatible with Python, C#, C++, Lua, and more.

## 📖 Documentation & Tools

*   **Documentation**: [Wiki](https://github.com/WallBreaker2/op/wiki)
*   **GUI Test Tool**: [OPTestTool](https://github.com/flaot/OPTestTool) (Provided by float)

## ✨ Key Features

### 🖥️ Window & Input Automation
*   **Background Operation**: Send keys and mouse clicks to non-active or minimized windows.
*   **Input Simulation**: System-level simulation via Windows API (SendInput/SendMessage).
*   **Window Management**: Find, move, resize, and query window states.

### 🖼️ Image & Color Processing
*   **Smart Search**: Fuzzy image search with transparency and color deviation support.
*   **Memory Search**: High-speed multi-point color finding in memory.
*   **Capture Engines**: Support for GDI, DirectX (DX), and OpenGL screen capture.
*   **Android Support**: Specialized capture for popular Android emulators.

### 📝 OCR (Optical Character Recognition)
*   **Dual Engine**: 
    *   **Tesseract**: Powered by Google's Tesseract engine for complex text.
    *   **Native**: Lightweight, high-speed dict-based algorithm for fixed fonts.
*   **Speed**: Optimized for real-time game text recognition.

## 📦 Installation

1.  **Download**: Get the latest binary release from [GitHub Releases](https://github.com/WallBreaker2/op/releases).
2.  **Unpack**: Extract the files to a local directory.
3.  **Register COM Component**:
    Open a terminal as **Administrator** and run:
    ```powershell
    # For 32-bit applications
    regsvr32 op_x86.dll

    # For 64-bit applications
    regsvr32 op_x64.dll
    ```

## 🚀 Quick Start (Python)

Here is a simple example using Python to load the plugin and find an image.

```python
from win32com.client import Dispatch
import os

# 1. Initialize the COM object
op = Dispatch("op.opsoft")
print(f"OP Plugin Version: {op.Ver()}")

# 2. Setup path (optional, sets base path for images)
# op.SetPath("C:\\Your\\Image\\Folder")

# 3. Perform an image search
# Arguments: x1, y1, x2, y2, pic_name, delta_color, sim, dir
ret, x, y = op.FindPic(0, 0, 1920, 1080, "test.png", "000000", 0.9, 0)

if ret == 1:
    print(f"Found image at: ({x}, {y})")
    op.MoveTo(x, y)
    op.LeftClick()
else:
    print("Image not found.")
```

## 🛠️ Build from Source

### Prerequisites
*   **Visual Studio 2022** (MSVC v143 toolset)
*   **CMake** (3.24 or newer)
*   **Windows SDK** (10.0.19041.0 or newer)

### Dependencies
The project relies on several third-party libraries:
*   **Blackbone**: For process memory and injection (Static Link).
*   **Kiero**: For DirectX hooking (Source integration).
*   **Tesseract**: For OCR capabilities (Dynamic Link).

Before configuring, you need to provide BlackBone headers and library. The recommended way is setting `BLACKBONE_ROOT` to the BlackBone repository root; CMake will auto-detect common output layouts (both command-line builds and legacy VS-generated layouts).

```powershell
set BLACKBONE_ROOT="D:\path\to\Blackbone"
```

If auto-detection fails, pass explicit paths:

```bash
cmake -S . -B build -DBLACKBONE_INCLUDE_DIR="D:/path/to/Blackbone/src" -DBLACKBONE_LIBRARY="D:/path/to/Blackbone/build/x64/BlackBone/Release/BlackBone.lib"
```

### Build Steps

1.  Clone the repository:
    ```bash
    git clone https://github.com/WallBreaker2/op.git
    cd op
    ```

2.  Create a build directory:
    ```bash
    mkdir build && cd build
    ```

3.  Configure and Build:
    ```bash
    cmake ..
    cmake --build . --config Release
    ```

## 🤝 Community & Support

*   **Issues**: [GitHub Issues](https://github.com/WallBreaker2/op/issues) - Report bugs or request features.
*   **Discussions**: [GitHub Discussions](https://github.com/WallBreaker2/op/discussions) - Ask questions and share ideas.
*   **QQ Groups**: 
    *   Group 1: `743710486` (Full)
    *   Group 2: `27872381` (Active)

## 📜 License

This project is licensed under the [MIT License](LICENSE).

## 📚 References

*   [TSPLUG](https://github.com/tcplugins/tsplug) - Referenced for some function implementations.
*   [Kiero](https://github.com/Rebzzel/kiero) - Basis for D3DHook.
