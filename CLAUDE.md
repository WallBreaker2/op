# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

OP is a Windows desktop automation COM plugin written in C++17. It provides screen capture (GDI/DirectX/OpenGL), input simulation (SendInput/SendMessage/hooks), image search with fuzzy matching, and OCR (local dict-based + HTTP service-backed). The plugin exposes a COM `IDispatch`-based interface consumable from Python, C#, C++, and Lua.

## Build commands

```bash
# One-click bootstrap + build (x64 Release, VS2022)
python build.py

# x86 target
python build.py -a x86

# Debug build
python build.py -t Debug

# Skip dependency bootstrapping (already have vcpkg + BlackBone)
python build.py --no-bootstrap-deps

# Direct CMake build (after build.py has set up the build directory)
cmake --build build/vs2022-x64-Release --config Release
cmake --build build/vs2022-x86-Release --config Release
```

## Test commands

```bash
# Run all tests via ctest (build first, then from the build directory)
cd build/vs2022-x64-Release
ctest -C Release --output-on-failure

# Run a single test by name (GoogleTest filter)
cd build/vs2022-x64-Release
./tests/op_test.exe --gtest_filter="OcrTest.OcrAutoFromGeneratedConsoleLikeBmpContainsExpectedText"

# OCR tests require an OCR server running. Set environment variables to override the endpoint:
#   OP_OCR_BACKEND=tesseract (default) → http://127.0.0.1:8080/api/v1/ocr
#   OP_OCR_BACKEND=paddle            → http://127.0.0.1:8081/api/v1/ocr
#   OP_OCR_URL=http://...            → explicit override
#   OP_OCR_TIMEOUT_MS=5000
```

## Architecture

```
include/libop.h          — Public API declaration (class libop, exported via OP_API)
libop/libop.h            — Private libop declaration wrapping op_context via pimpl
libop/libop.cpp          — Main implementation: delegates to WinApi, opBackground, ImageProc
libop/com/op.idl         — MIDL COM interface definition (IOpInterface, ~300 dispids)
libop/com/OpInterface.*  — COM IDispatch implementation, calls through to libop
libop/core/optype.h      — Core types: point_t, rect_t, ocr_rec_t, bytearray aliases
libop/core/globalVar.h   — Render type enums, input types, shared constants, version string
libop/core/helpfunc.*    — Utility functions (string conversion, hex dump, error helpers)
libop/background/        — Screen capture engines (GDI, DXGI, D3D9-12, OpenGL, WGC),
                           mouse/key input simulators (win32 + DX hook),
                           DLL injection hooks for display/input interception
libop/imageProc/         — Image search (ImageLoc), image processing (ImageProc),
                           OCR via HTTP service (OcrWrapper, singleton with mutex),
                           dict-based OCR (tess_ocr)
libop/winapi/            — Win32 wrappers, process injection (BlackBone-based), memory ops
libop/algorithm/         — A* pathfinding (AStar.hpp)
tests/                   — GoogleTest suite with custom test environment (OpEnvironment)
                           Tests are split by category (core_algorithm_winapi, mouse_keyboard,
                           image_color, ocr). main.cpp defines its own main() and registers
                           OpEnvironment for global setup/teardown.
tools/                   — EasyCom DLL (COM helper, separate from the main plugin)
swig/                    — Python bindings via SWIG (_pyop.pyd + pyop.py)
3rd_party/               — Pre-built libs (x86/x64) + headers + Kiero source
ci/triplets/             — vcpkg overlay triplets for custom build configuration
build.py                 — Unified build: bootstraps vcpkg, clones+builds BlackBone,
                           configures CMake, and builds the project
```

### Key architectural patterns

- **Pimpl + COM**: `class libop` (in `include/libop.h`) is the public DLL-exported facade. It holds a `unique_ptr<op_context>` which bundles `WinApi`, `opBackground`, and `ImageProc` instances. The COM layer (`IOpInterface` / `OpInterface`) converts `BSTR`/`VARIANT` arguments and delegates to `libop`.
- **COINIT_APARTMENTTHREADED**: The COM object uses apartment threading. All calls from a single thread share the same `op_context`.
- **Screen capture modes**: Controlled by `SetDisplayInput()` — supports GDI (`normal`), DXGI, WGC, and `mem:<ptr>` for external pixel buffers.
- **OCR has two code paths**: Local dict-based (`SetDict`/`UseDict`, fast for fixed fonts) and HTTP service-backed (`OcrEx`/`OcrAuto`, via `OcrWrapper` → `ocr_server.exe` or PaddleOCR).
- **Input modes**: `mouse`/`keypad` accept `normal`, `windows`, or `dx` backends. `dx` mode uses MinHook-injected DLLs to intercept/replay input in the target process.
- **Pre-commit**: clang-format (v16) runs on `.c/.h/.cpp/.hpp/.cc/.cxx` files.

## Platform constraints

- **Windows only**. Compiles with MSVC v143 (VS2022), `/MT` static CRT, `/EHa` exception handling.
- Dual-architecture output: `op_x86.dll` (32-bit) and `op_x64.dll` (64-bit). Callers must match bitness.
- Build directory naming isolates architectures: `build/vs2022-{x64|x86}-{Release|Debug}`. Never mix x86 and x64 in the same build directory.
