
<h1 align="center">kiero v2</h1>
<p align="center">
A lightweight C++ library that <strike>hooks</strike> locates graphics API methods
at runtime.
</p>
<br>

> [!IMPORTANT]
> ## Breaking Changes (compared to v1)
>
> You bring your own hooking library. kiero just finds the
> addresses.
>
> Check [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md) for more details.

## Official Supported APIs

- DirectX 9 (kiero_d3d9.hpp)
- DirectX 10 (kiero_d3d10.hpp)
- DirectX 11 (kiero_d3d11.hpp)
- DirectX 12 (kiero_d3d12.hpp)
- OpenGL (kiero_opengl.hpp)
  - [x] Windows
  - [x] Linux
  - [ ] MacOS
- Vulkan (kiero_vulkan.hpp)
  - [x] Windows
  - [ ] Linux
  - [ ] MacOS

## Requirements

- C++17
- Windows SDK (for DirectX backends)
- DirectX SDK (for DirectX backends)
- Vulkan SDK (for Vulkan backends)

## Installing

### CMake

`kiero` now provides a standard CMake target and works well with
`FetchContent`:

```cmake
include(FetchContent)

FetchContent_Declare(
  kiero
  GIT_REPOSITORY https://github.com/kirchesz/kiero2.git
  GIT_TAG master
)

FetchContent_MakeAvailable(kiero)

target_link_libraries(your_target PRIVATE kiero::kiero)
```

> [!WARNING]
> When you use `FetchContent`, CMake also generates `kiero.generated.hpp`.
>
> You must include this after `kiero.hpp`.

Available options:

- `KIERO_BUILD_D3D9`
- `KIERO_BUILD_D3D10`
- `KIERO_BUILD_D3D11`
- `KIERO_BUILD_D3D12`
- `KIERO_BUILD_OPENGL`
- `KIERO_BUILD_VULKAN`

### Without CMake
1. Download repository.
2. Copy `kiero.hpp`, `kiero_intern.hpp`, `kiero_intern.cpp`, and the backend files you want to use into your project.

For example, for DirectX 11:

```text
kiero.hpp
kiero_intern.hpp
kiero_intern.cpp
kiero_d3d11.hpp
kiero_d3d11.cpp
```

3. Then add those `.cpp` files to your build system and make sure your compiler
can find the headers.

## Quick start

```cpp
#include "kiero.hpp"
#include <kiero.generated.h> // if you use CMake FetchContent
#include "kiero_d3d9.hpp"    // order is important!
                             // kiero.hpp must be included before any backend header
                             // also order changes implementation indices
                             // for example if you include kiero_d3d11.hpp before
                             // kiero_d3d9.hpp, then kiero::Implementation_D3D9
                             // will be 2 instead of 1

kiero::D3D9Output d3d9;
auto err = kiero::locate<kiero::Implementation_D3D9>(
  nullptr, &d3d9
);
if (err != kiero::Error_Nil) {
  // handle error
}

// d3d9.device_methods[42] is the address of
// IDirect3DDevice9::EndScene — hook it however you like
```

## How it works

Each backend do some magic inside (check each backend cpp file for details), and results
stores in a typed output structs. The temporary
resources are released before `locate` returns.

The single entry point is a function template:

```cpp
template<int Impl>
kiero::Error kiero::locate(void* in, void* out);
```

Each backend provides a full specialization. Include only
the headers you need — unused backends add zero overhead.

## Error handling

`locate` returns `kiero::Error`. Shared codes:

| Code | Meaning |
|---|---|
| `Error_Nil` | Success |
| `Error_ModuleNotFound` | Required DLL not loaded |
| `Error_MethodNotFound` | Export not found in DLL |

Each backend defines additional codes.
Check the corresponding header for the full list.

## Adding a new backend

1. Create `kiero_myapi.hpp`:
   - Define `Implementation_MyAPI = KIERO_IMPL_FREE_SLOT`
   - Define backend-specific errors from `Error_BaseIndex`
   - Define your `MyAPIOutput` struct
   - `#undef` and `#define KIERO_IMPL_CURR_SLOT`

2. Create `kiero_myapi.cpp`:
   - Include `kiero.hpp`, your header, `kiero_intern.hpp`
   - Provide `template<> kiero::locate<...>(...)`

## License

kiero v2 is licensed under the MIT License. See [LICENSE](LICENSE) for details.
