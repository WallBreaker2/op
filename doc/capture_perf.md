# 截图效率优化记录

本轮改动只针对“取一帧像素”这条热路径，不改任何公开 API、COM 接口和 SWIG 绑定。

## 1. 结论

单帧耗时（每格为 3 次运行 200~300 次迭代的平均值区间）。
**所有数字都来自 §3 的 standalone 等价管线，不是 `op_x64.dll` 的实测值**，且 GDI 对显示适配器强敏感，
验收请用 min / median 不退化而不是绝对倍数：

| 后端 | 场景 | 优化前 | 优化后 | 提升 | 像素一致性 |
|---|---|---|---|---|---|
| GDI (`normal`) | 整窗 1600x900 | 3.34 ~ 4.77 ms | 1.18 ~ 1.65 ms | **2.4x ~ 2.9x** | 逐字节一致 |
| GDI (`normal`) | 区域 320x200 | 0.14 ~ 0.20 ms | 0.11 ms | 1.2x ~ 1.9x | 逐字节一致 |
| GDI (`normal`) | 小块 10x10（GetColor） | 0.05 ~ 0.06 ms | 0.04 ~ 0.05 ms | ~1.0x ~ 1.4x（噪声范围内） | 逐字节一致 |
| DXGI (`normal.dxgi` / `normal.auto`) | 整屏 1920x1080 | 1.14 ~ 1.17 ms | 0.96 ~ 1.01 ms | 1.15x ~ 1.19x | 逐字节一致 |
| DXGI (`normal.dxgi` / `normal.auto`) | 区域 320x200 | 0.41 ~ 0.43 ms | 0.04 ~ 0.06 ms | **7x ~ 11x** | 逐字节一致 |
| DXGI (`normal.dxgi` / `normal.auto`) | 小块 10x10（GetColor） | 0.38 ~ 0.40 ms | 0.015 ~ 0.018 ms | **21x ~ 26x** | 逐字节一致 |

读法：

- GDI 的收益来自去掉一次整块拷贝 + 一次 `GetDIBits` 格式转换，所以**区域越大越明显**；
  10x10 这种小块两次实现都被 `BitBlt` 的固定开销支配，几乎没有差别。
- DXGI 的收益来自“只把请求矩形从显存读回”，所以**区域越小相对提升越大**；
  整屏截图两次都要搬一整帧，差别只在 staging 纹理复用和免掉整屏清底。

WGC (`normal.wgc`) 的收益是**延迟**而不是单帧 CPU 时间：原来每次截图固定等待 50 ms（GUI 线程路径）/
100 ms（Windows 10 worker 路径）等“新帧”，静态画面下这部分等待占一次截图耗时的绝大部分。
现在改成**有界等待**（`kStaleFrameWaitMs = 16`），有新帧立刻返回。
注意这不是“等价于画面没变”：帧池为空只说明合成器还没出帧，见 §2.3 与 §7 的语义说明。

测量环境：Windows 10 19045 / i7-12700 / RTX 3060 Ti / MSVC 14.44 (-O2)。

## 2. 各后端的瓶颈与改法

### 2.1 GDI（`libop/capture/backends/GdiCapture.*`）

改动前每帧的动作：

```
CreateCompatibleBitmap(w,h)          // 每帧重新分配位图
SelectObject(mem_dc, bmp)
BitBlt(mem_dc, x, y, w, h, screen)   // 第 1 次整块拷贝
GetDIBits(mem_dc, bmp -> shmem)      // 第 2 次拷贝 + 格式转换
for each row: memcpy(img, shmem, h-1-i) // 第 3 次拷贝 + 逐行倒置
SelectObject(mem_dc, old); DeleteObject(bmp)
```

改动后：

```
CreateDIBSection(32bpp, biHeight = -h)  // 复用；容量不够才重建
SelectObject(mem_dc, dib)               // 只做一次
BitBlt(mem_dc, ..., screen)             // 直接 BLT 进最终像素内存
memcpy(img, dib_bits, w*h*4)            // 一次连续 memcpy（负高度 = top-down，不用倒置）
```

关键点：

- `biHeight` 取负值得到 top-down DIB，行序与 `op::Image` 一致，逐行倒置整段消失；
- 32bpp 的 DIB 行距恒等于 `width*4`，所以整行取块退化成一次大 `memcpy`；
- DIB 按“够用就复用”的策略缓存，`GetColor`（10x10）这类小块不会反复建位图；
- 删掉了 GDI 路径往共享内存写一遍像素的无效动作（共享内存像素只被注入式 hook 后端使用），
  帧头 `FrameInfo` 仍然照写，`GetScreenFrameInfo` 行为不变；
- 新增 `GdiCapture::refreshMetrics()` override：窗口缩放/换 DPI 后刷新客户区尺寸和边框偏移，`RectConvert`
  裁剪不再用旧尺寸。（注意：基类虚函数 `ICaptureBackend::refreshMetrics` 与 `RectConvert` 里的调用点早于本次
  改动，是 WGC 时代就有的机制；本次新增的是 GDI 这一侧的实现。）

### 2.2 DXGI（`libop/capture/backends/DxgiCapture.*`）

改动前每次 `requestCapture`：新建整屏 staging 纹理 → `CopyResource` 整屏 → `Map` 整屏
→ `img.fill(0xff000000)` 整屏清底 → 逐行 `memcpy` 取区域。即使只取 10x10，也要把 8 MB 桌面从显存读回内存。

改动后：

- 整屏 staging 纹理只在输出尺寸/格式变化时重建（`ensureDesktopStagingTexture`）；
- 请求区域用 **GPU 侧** `CopySubresourceRegion` 搬到一张区域大小的 staging 纹理（`ensureRegionTexture`，同样复用），
  只有这一小块参与 `Map`，PCIe 读回量随请求尺寸而不是随屏幕尺寸增长；
- 清底只在请求区域被输出边界夹取时才做；
- 行距与目标一致时一次 `memcpy` 完成，否则逐行；
- 区域 staging 纹理按**容量**复用（`>=` 请求尺寸即可），不是精确尺寸：脚本在"整窗找图"和"10x10 取色"
  之间切换时，精确尺寸缓存会每帧销毁/重建一张 staging 纹理；
- 区域 staging 纹理**与 D3D11 设备同生命周期**：`InitD3D11Device()` 换设备时作废（`invalidateRegionTexture()`）。
  D3D11 资源不能跨设备使用，漏掉这一步会让 `Map()` 永久失败，而按 (w,h,format) 的缓存命中会把自愈路径堵死
  —— 拖到副屏、UAC/锁屏导致 `ACCESS_LOST`、驱动 TDR 之后的典型症状；
- duplication 重建失败后退避 300 ms，不在热路径上每帧重复"枚举适配器 + CreateD3D11Device + DuplicateOutput"。

### 2.3 WGC（`libop/capture/backends/WgcCapture.cpp`）

- 去掉静态画面下固定 50 ms（GUI 线程）/ 100 ms（Windows 10 worker）的“等新帧”轮询，改成**有界等待**
  （`kStaleFrameWaitMs = 16`）：帧池取空后再等最多一帧合成间隔，有新帧立即返回；
- 为什么不是“完全不等待”：WGC 是变化驱动的，**帧池为空只说明合成器还没出帧，不等于画面没变**。
  输入动作（点击/键入/局部重绘）后立刻 `GetColor`/`FindPic` 时，如果直接复用 staging，会返回动作前的像素
  并且**报成功**——这是 read-after-write 语义被破坏，比慢 16 ms 严重得多。这里是"语义变化被限制在一帧内"，
  不是"语义不变"；
- 整窗截图（最常见）走一次 `memcpy`，不再逐行 1000+ 次 `memcpy`；
- `UnBindExInternal` 释放 `stagingTexture_`/`d3dDevice*` 时持 `frameMutex_`，与 `copyFrameToStaging` 互斥；
  重建路径的 `FrameArrived` 回调补上 `captureStopping_` 判定（首次建会话的回调本来就有）。

### 2.4 其它顺带的小改动

| 文件 | 改动 |
|---|---|
| `libop/image/Image.h` | `create()` 尺寸没变直接返回，不再每帧 `realloc`；`write()` 由逐像素改整行 `memcpy`（保存整屏 BMP/PNG 明显变快） |
| `libop/binding/BindingSession.cpp` | `pic:` / `mem:` 源整行取块时一次 `memcpy`，并补上越界检查 |
| `libop/capture/backends/HookCapture.cpp` | 注入式后端 top-down 帧整行取块时一次 `memcpy`（OpenGL 翻转路径保持逐行） |

## 3. 复现测量（standalone 等价管线，**不是发布路径**）

> 口径说明：下面两个基准是**独立可执行文件里的等价重实现**（`capture_legacy` / `capture_fast`），
> 不链接 libop、不实例化 `GdiCapture`/`DxgiCapture`，源也不同（bench 用 `GetDC(hwnd)` 窗口 DC，
> 发布路径 `RDT_NORMAL` 用 `GetDC(NULL)` 桌面 DC + 窗口偏移）。所以：
> - "pixels identical / pixel-exact" 只证明**两条等价管线互相一致**，不覆盖发布路径独有的
>   DIB 复用、越界清底、区域夹取、设备重建分支；
> - 表里的倍数是这两条管线的比值，不是 `op_x64.dll` 的实测值；GDI 的倍数对显示适配器敏感
>   （虚拟机/虚拟显示适配器上实测 1.7x~2.3x，低于本表 2.4x~2.9x），**不要当验收门槛**。

两个基准都是独立可执行文件，不依赖 vcpkg/OpenCV/BlackBone，只链接系统库：

```bat
call "…\VC\Auxiliary\Build\vcvars64.bat"
cl /nologo /O2 /std:c++17 /EHsc /utf-8 /Fe:bench_capture.exe scripts\bench_capture.cpp user32.lib gdi32.lib
cl /nologo /O2 /std:c++17 /EHsc /utf-8 /Fe:bench_dxgi.exe    scripts\bench_dxgi.cpp d3d11.lib dxgi.lib user32.lib

bench_capture.exe 300
bench_dxgi.exe    200
```

两个基准都会在跑完计时后对两条管线做**逐字节比较**，不一致时返回码为 2 并打印 `pixel mismatch`。
两者都会先打印环境元数据（CPU 架构/核数、接到桌面上的显示适配器名），并输出
`avg / median / p95 / min / stdev` 五档统计——只报 avg 会被尖峰带偏，验收用 **min 与 median 不退化**。

实测（2026-10-20，本机 i7-12700 / Win10 19045 / MSVC 14.44 / 显示适配器只有 2×RTX 3060 Ti，无虚拟显示适配器）：

```
env: x64, 20 logical cpus
display adapters:
  [0] NVIDIA GeForce RTX 3060 Ti
  [1] NVIDIA GeForce RTX 3060 Ti

client 1600x900

[full client] 1600x900
  legacy : avg 4.811  median 4.768  p95 6.489  min 3.669  stdev 0.903 ms
  fast   : avg 1.562  median 1.391  p95 2.396  min 0.998  stdev 0.526 ms
  speedup: avg 3.08x  median 3.43x  min 3.68x
  pixels identical: YES

[region 320x200] 320x200
  legacy : avg 0.156  median 0.130  p95 0.254  min 0.111  stdev 0.095 ms
  fast   : avg 0.109  median 0.094  p95 0.146  min 0.088  stdev 0.075 ms
  speedup: avg 1.43x  median 1.38x  min 1.26x
  pixels identical: YES

[block 10x10] 10x10
  legacy : avg 0.049  median 0.045  p95 0.063  min 0.043  stdev 0.022 ms
  fast   : avg 0.053  median 0.042  p95 0.069  min 0.039  stdev 0.071 ms
  speedup: avg 0.92x  median 1.09x  min 1.08x
  pixels identical: YES

RESULT: pixel-exact
```

DXGI（同一台机器，同一次会话）：

```
device: hardware (feature level 0xB000)
desktop texture: 1920x1080

[full desktop] 1920x1080
  legacy : avg 1.111  median 0.990  p95 1.727  min 0.932  stdev 0.336 ms
  fast   : avg 0.945  median 0.883  p95 1.284  min 0.799  stdev 0.188 ms
  speedup: avg 1.18x  median 1.12x  min 1.17x
  pixels identical: YES

[region 320x200] 320x200
  legacy : avg 0.409  median 0.367  p95 0.616  min 0.358  stdev 0.111 ms
  fast   : avg 0.050  median 0.036  p95 0.054  min 0.035  stdev 0.067 ms
  speedup: avg 8.24x  median 10.15x  min 10.30x
  pixels identical: YES

[block 10x10] 10x10
  legacy : avg 0.376  median 0.342  p95 0.576  min 0.334  stdev 0.103 ms
  fast   : avg 0.015  median 0.015  p95 0.016  min 0.014  stdev 0.002 ms
  speedup: avg 25.32x  median 23.58x  min 23.87x
  pixels identical: YES

RESULT: pixel-exact
```

**可复现性结论**：DXGI 的三档倍数稳定落在本表区间；GDI 的倍数对显示适配器强敏感——
同型号 CPU/GPU 的另一台/另一次会话里（桌面接了 OrayIddDriver / 虚拟显示适配器）实测
整窗只有 1.67x~2.25x、区域用例甚至出现过 fast 比 legacy 慢（0.39x）。所以：
**倍数是环境相关的参考值，不是验收门槛；验收看 min / median 不退化。**
（这也是两个基准现在先打印 `display adapters` 的原因。）

## 4. 正确性验证

**2026-10-20 补做：ATL 组件装好后本机完成了 x64 Release 全量构建并真实跑了测试**，结果如下。

| 项 | 状态 |
|---|---|
| `python build.py`（x64 Release，VS2022 17.14 / MSVC 14.44） | **已执行，成功**：产出 `build/vs2022-x64-Release/libop/Release/op_x64.dll`（10.3 MB）、`tests/Release/op_test.exe`，并 install 到 `bin/x64` |
| `bench_capture` / `bench_dxgi` 逐字节比对 + 统计口径 | **已执行**：GDI `300` 次 / DXGI `200` 次均 `RESULT: pixel-exact`；已补 `median/p95/stdev` 与环境元数据，见 §3 |
| `CaptureBackendTest.*`（3 条，含新增 DXGI 区域用例） | **3/3 通过**（本次修复直接相关） |
| x86 Release（`python build.py -a x86`） | **已执行，构建成功** 并 install 到 `bin/x86`；T1 ×33 次 0 AV（其中 1 次 `0xC0000409` 未能复现），全量 4 次稳定 7 通过 / 7 失败，与 x64 一致 |
| `WgcTest.*`（11 条） | **4 条稳定通过**；其余 7 条被一个**既有缺陷**连带失败，见 §8 |
| `WgcTest.NormalDxgiMaximizeAfterBindCapturesClippedClientArea` | **失败，但修复前基线同样失败**（对照实验：纯修复前代码单跑 4/4 失败，取到 `FFFFFF`），属环境/用例本身问题，与本次修复无关 |
| `image_color_test` / `mouse_keyboard_test` / `ocr_test` / `opencv_test` 等 | **未跑**（本阶段只跑了 capture 相关用例；OCR 用例需要额外起 OCR 服务） |
| DXGI 设备重建后自愈、WGC 并发解绑 | 仍无自动化覆盖（设备重建需要 test-only 接缝），只有代码级审查 |

复现命令：

```bat
python build.py
cd build\vs2022-x64-Release && ctest -C Release --output-on-failure
:: 只跑本次相关用例（必须从 bin\x64 跑，op_x64.dll 与 op_test.exe 同目录）
cd ..\..\bin\x64 && op_test.exe --gtest_filter="CaptureBackendTest.*:WgcTest.*"
```

无交互桌面的 CI 可用 `OP_SKIP_WGC_TESTS=1` 跳过真实截屏用例（新增测试沿用同一开关）。
注意：该开关是**整文件级**跳过，所以无交互桌面的 CI 里本次优化是零覆盖。

## 5. 变更文件清单

新增：

- `scripts/bench_capture.cpp`、`scripts/bench_dxgi.cpp`（性能/一致性基准）
- `tests/capture_backend_test.cpp`（回归测试）、`tests/CMakeLists.txt` 里登记
- `doc/capture_perf.md`（本文件）

修改：

- `libop/capture/backends/GdiCapture.h`、`GdiCapture.cpp`（整体重写取帧路径）
- `libop/capture/backends/DxgiCapture.h`、`DxgiCapture.cpp`（区域 GPU 拷贝 + staging 复用）
- `libop/capture/backends/WgcCapture.cpp`（去掉固定等帧 + 单次 memcpy）
- `libop/capture/backends/HookCapture.cpp`（单次 memcpy 快路径）
- `libop/image/Image.h`（`create()` 复用、`write()` 整行拷贝）
- `libop/binding/BindingSession.cpp`（`pic:`/`mem:` 取块 + 越界检查）

## 6. 回滚

本目录不是 git 工作区（没有 `.git`），**没有上游可以"恢复"**，所以回滚只能依赖本地快照。
本次评审修复把动手前的 9 个文件快照到了 `_backup/fix_20261020_164105/`（文件名里的 `__` 就是路径分隔符）：

```bat
:: 撤销本次评审修复（回到"优化已落地但未修复"的状态）
cd /d E:\AIWork\github\op-master
copy /y "_backup\fix_20261020_164105\libop__capture__backends__GdiCapture.h"     "libop\capture\backends\GdiCapture.h"
copy /y "_backup\fix_20261020_164105\libop__capture__backends__GdiCapture.cpp"   "libop\capture\backends\GdiCapture.cpp"
copy /y "_backup\fix_20261020_164105\libop__capture__backends__DxgiCapture.h"    "libop\capture\backends\DxgiCapture.h"
copy /y "_backup\fix_20261020_164105\libop__capture__backends__DxgiCapture.cpp"  "libop\capture\backends\DxgiCapture.cpp"
copy /y "_backup\fix_20261020_164105\libop__capture__backends__WgcCapture.cpp"   "libop\capture\backends\WgcCapture.cpp"
copy /y "_backup\fix_20261020_164105\doc__capture_perf.md"                       "doc\capture_perf.md"
```

只想撤销某一个后端的修复：把对应 `.cpp`/`.h` 按上面单独 `copy` 回去即可，三个后端互不依赖
（`BindingSession::createDisplay` 仍按 `RDT_NORMAL`/`RDT_NORMAL_DXGI`/`RDT_NORMAL_WGC` 选后端，接口没变）。

**注意**：快照是"本次修复前"的版本，不是"性能优化前"的原始版本——原始版本在这个工作区里已经不存在了
（无 `.git`、无 `*.orig`）。要回到性能优化之前的状态，得从你自己的上游代码库取那 6 个源文件。

两个基准脚本（`scripts/bench_capture.cpp`、`bench_dxgi.cpp`）没有留修复前快照（改成输出 `median/p95/stdev`
之前没有先备份）。要撤销这部分：把 `print_stats()` / `print_environment()` 两个函数和它们的两处调用删掉，
把 `Stats` 结构里的 `median_ms/p95_ms/stdev_ms` 三行删掉，并把报告那几行改回
`"  legacy : avg %.3f ms  min %.3f ms\n"`。这两个文件是纯诊断代码，不进 DLL，不回滚也不影响出货。

删除新增的基准与测试（可选）：`scripts/bench_capture.cpp`、`scripts/bench_dxgi.cpp`、
`tests/capture_backend_test.cpp`，并把 `tests/CMakeLists.txt` 里的 `"capture_backend_test.cpp"` 一行删掉。

公开接口与 `Image` 尺寸/行序、`FrameInfo` 语义都没变，所以回滚不需要改调用方代码；唯一例外是
**WGC 的首帧新鲜度**（见 §2.3 与 §7），回滚 WGC 修复会让它重新变成"可能返回动作前的一帧"。

## 7. 评审修复记录（2026-10-20）

代码评审提出 5 项阻断，本轮修掉其中 4 项代码/文档问题，第 5 项（构建+测试证据）受环境阻塞，见 §4。

| # | 位置 | 修前 | 修后 |
|---|---|---|---|
| 1 | `DxgiCapture.cpp` `invalidateRegionTexture()` / `InitD3D11Device()` / `RebuildDuplication()` / `UnBindEx()` | 换设备后 `regionTexture_` 仍是旧设备的纹理，`ensureRegionTexture` 按 (w,h,format) 命中缓存永不重建 → `Map` 每帧失败，拖副屏/锁屏/TDR 后截图永久失败 | 设备换手、duplication 重建、解绑三处统一作废区域纹理 |
| 2 | `DxgiCapture.cpp` `ensureRegionTexture()` | 精确尺寸命中，交替尺寸每帧 `CreateTexture2D`/`Release` | 容量命中（`>=`），行距不一致走已有逐行分支 |
| 3 | `DxgiCapture.cpp` `requestCapture()` / `RebuildDuplication()` + `DxgiCapture.h` 新增 `rebuild_backoff_until_ms_` | 重建失败后每帧重试（monitor 变化那一帧连做两次）完整重建，几十 ms 长尾 | 失败退避 300 ms；`refreshWindowMetrics` 失败后不再重复重建 |
| 4 | `WgcCapture.cpp` `requestCapture()` 非 metrics 分支 + `kStaleFrameWaitMs` | 帧池为空即复用 staging → 输入动作后首帧可能返回动作前像素且报成功 | 补一次有界等待（≤16 ms），有新帧立即返回 |
| 5 | `WgcCapture.cpp` `UnBindExInternal()` / 重建路径 `FrameArrived` 回调 | 解绑时释放 staging 不持 `frameMutex_`；重建路径回调不看 `captureStopping_` | 释放区持锁；回调补停止判定 |
| 6 | `GdiCapture.cpp` `copy_from_dib()`（改 `bool`）/ 两处调用点 | 越界时静默 return，`requestCapture` 仍返回 true（配合 `create()` 复用 = 返回上一帧像素） | 失败向上传播，`requestCapture` 返回 false |
| 7 | `GdiCapture.cpp` `ensure_dib()` SelectObject 失败分支 | `_dib_old` 为空 → `release_dib()` 跳过解选直接 `DeleteObject` 已选中位图（失败 + 泄漏） | 失败分支自己 `DeleteObject` 并清空指针 |
| 8 | `GdiCapture.cpp` `write_frame_info()` | 写 `FrameInfo` 不持 `_pmutex`（DXGI/WGC 都持） | 统一持锁 |
| 9 | `GdiCapture.cpp` `RDT_GDI_DX2` 分支 | `img.read(&image)` 返回值被丢弃，失败也返回 true | 失败报 false 并记日志 |
| 10 | 本文档 §2.1 / §2.2 / §2.3 / §3 / §4 / §6 | "语义不变""已通过""新增 refreshMetrics" | 逐条改为与代码一致（WGC 语义有界变化、测的是 standalone 管线、验证状态未核验、回滚指向本地快照） |
| 11 | `scripts/bench_capture.cpp` / `scripts/bench_dxgi.cpp` | 只输出 avg/min，无环境信息，验收门槛容易被尖峰带偏 | 输出 `avg/median/p95/min/stdev` + `speedup` 三口径 + CPU/显示适配器元数据（dxgi bench 因此多链 `user32.lib`） |
| 12 | `tests/wgc_test.cpp` / `tests/capture_backend_test.cpp` | WGC 首帧新鲜度、DXGI 区域复用无覆盖 | 新增 `WgcTest.FirstCaptureAfterContentChangeIsFresh`、`CaptureBackendTest.DxgiRegionCaptureReusesLargerStagingTexture`。已真实运行：DXGI 那条 **通过**；WGC 那条因 §8 的卡死缺陷连带失败（绑定阶段就 ret=0），修掉 §8 后才能判定 |

验证状态（2026-10-20 更新，已真实构建并跑过测试）：
- **已执行**：两个基准在本机重跑（GDI `300` 次、DXGI `200` 次），`RESULT: pixel-exact`，输出见 §3；
  统计口径已从"只报 avg"改为 `avg/median/p95/min/stdev` + 显示适配器列表。
- **已执行**：`python build.py` x64 Release 全量构建成功；`CaptureBackendTest.*` **3/3 通过**（含新增的
  DXGI 区域复用用例，直接覆盖第 1/2 项修复），`WgcTest` 4 条稳定通过（覆盖第 4/5 项修复不破坏正常路径）。
- **未通过**：`WgcTest` 其余 7 条被既有缺陷连带失败（§8），对照实验证明**修复前基线同样失败**。
- **未执行**：`image_color_test` / `mouse_keyboard_test` / `ocr_test` / `opencv_test`（需要 OCR 服务或耗时较长）。
- DXGI 设备重建的自愈路径仍缺 test-only 接缝，目前只能靠人工冒烟
  （双屏拖动 / 锁屏恢复 / 拔插 HDMI 后连续 `GetColor`）。

## 8. 已定位并修掉 AV；WGC 第二会话 init 失败仍未修（2026-10-20 第二轮）

> **本轮修正了本节原来的根因判断**，两处：
> 1. 卡点**不是** `applySessionOptions` 里的 `RequestAccessAsync(Borderless).get()`：`IsBorderRequired`
>    是 10.0.20348+ 的 API，本机 19045 上 `wgcSessionPropertyPresent()` 门禁直接跳过该分支
>    （旁证：14 条跑批的日志里从未出现 `Request WGC borderless access failed`）。原判断作废。
> 2. 进程级计数 `g_win10WgcCleanupInFlight` **是放大器，不是根因**：最小复现（同进程第 2 个 WGC 会话、
>    不同 HWND）崩溃时日志里并没有 `previous WGC cleanup is still pending`，说明计数已正常归零。
>    两个已回退的修法（门禁改"继续"、计数加令牌）失败也是这个原因——它们都在修放大器。

### 8.1 已修：catch 处理器里的 WinRT 误用导致 worker 线程 AV（本轮修复，已实测）

**符号栈（Debug + PDB，`cdb -cf` 最小复现，两次运行都复现）**：

```
AV c0000005  op_x64!winrt::Windows::Foundation::IUnknown::add_ref+0x27   (rax=0)
  <- __FrameHandler4::CxxCallCatchBlock
  <- runInMtaApartment<...>+0x4b          [WgcCapture.cpp:50  result = func(); 之后进入 handler]
  <- WgcCapture::runWin10Worker+0x5a      [WgcCapture.cpp:293 (void)runInMtaApartment(...)]
  <- BindExOnWindows10::lambda_1::operator()  <- std::thread::_Invoke <- thread_start
```

**机制**：`Init()` 抛出的异常**既不是** `winrt::hresult_error`、也**不是** `std::exception`（加了
`catch (const std::exception&)` 后仍落到 `catch (...)`），于是进入 `runInMtaApartment` 的 catch-all；
原来那里调 `winrt::to_hresult()` —— 按 C++/WinRT 契约它只能在 WinRT 异常上调用，对非 WinRT 异常是
未定义行为，实测就是在 handler 里对空指针 `AddRef` 崩溃。`catch (winrt::hresult_error&)` 里的
`err.message()` / `winrt::to_string()` 同理（异常对象内的 `IRestrictedErrorInfo` 可能已失效）。

**修复**（`WgcCapture.cpp` 的 `runInMtaApartment` + 全文件 9 处 `catch(...)`）：

| 修前 | 修后 |
|---|---|
| `catch (...)` 里 `setlog("... (0x%08X)", winrt::to_hresult().value)` ×9 | 固定 `0x8000FFFF`（E_UNEXPECTED），不再触碰异常对象 |
| `catch (winrt::hresult_error &err)` 里 `winrt::to_string(err.message()).c_str()` ×10 | 只记 `err.code().value` |
| 只有 `catch (winrt::hresult_error&)` / `catch (...)` | 补 `catch (const std::exception&)`，把非 WinRT 异常单独落日志 |

**实测效果（Release，最小复现 T1 = `WgcTest.CapturesStaticWindowFromStart:WgcTest.NormalAutoUsesWgcForKnownBrowserClasses`）**：

| 指标 | 修前（基线 DLL） | 修后 |
|---|---|---|
| T1 连跑 AV 次数 | **3/4** | **0/6** |
| 全量 14 条 | 3/7 次运行 AV 终止，跑完时 7~8 条失败 | 连跑 3 次**均跑完 14 条**，6 通过 / 8 失败，0 AV |
| 级联 `previous WGC cleanup is still pending` | 每条后续用例都有（6~8 条） | **消失**（worker 现在能正常退出并递减计数） |

结论：**AV 与本轮升级的"进程级计数永久污染"是同一个根因的两个后果**——worker 死在 handler 里，
既崩了进程，也没机会执行 `fetch_sub`。修掉 handler 误用，两者一起消失。

### 8.2 仍未修：第 2 个 WGC 会话的 `Init()` 失败（已定位到第一步，且已不再产生黑图）

**本轮把失败点钉死到 `Init()` 的第 0→1 步**（加了 10 个带 tick 的 step 埋点后实测）：

```
wgc init step=0 activation_factory t=34467890      <- 第 1 个会话，全部 10 步通过
...  step=9 session assigned t=141                  <-   Init 成功
wgc init step=0 activation_factory t=34469516      <- 第 2 个会话
wgc init activation factory unknown exception        <-   step=1 再也没打印
```

即死在
`winrt::get_activation_factory<GraphicsCaptureItem>()` 或紧随的
`activation_factory.as<IGraphicsCaptureItemInterop>()` 上，抛出的异常**既不是
`winrt::hresult_error` 也不是 `std::exception`**（两者都已单独 catch + 落日志，从未命中）。

**已排除的假设（都做了 A/B，不要重复试）**：

| 假设 | 实验 | 结果 | 处置 |
|---|---|---|---|
| `uninit_apartment()` 破坏进程级 WinRT 激活 | 去掉 worker 线程里的 uninit | 现象**完全不变** | 已恢复原实现 |
| 计数门禁 / detach 引起 | §8.1 修掉后面临同一现象 | 无关 | — |
| `RequestAccessAsync(Borderless)` 阻塞 | API 可用性门禁在 19045 跳过该分支 | 本机根本不执行 | — |
| 上一个会话拆除太慢 → detach → 会话泄漏 → broker 拒绝新会话 | 把 `kWin10CloseWaitMs` 1500→8000 | **无变化**；且日志里没有 `stop timed out`，说明根本没走 detach | 已回退到 1500 |
| 激活失败是瞬时竞态，重试可解 | 有界重试 3 次 × 50 ms | **3/3 次全部失败**（确定性，不是竞态） | 已回退为单次 |
| 每次重新激活太贵，缓存 factory 可解 | 进程级缓存 interop factory | 两个用例都通过，但**进程退出时 AV 3/3**（静态 `com_ptr` 在 COM 反初始化后释放） | 已回退（更糟） |

**当前结论（本机 Win10 19045）**：同一进程内**只能成功创建一个 `GraphicsCaptureItem` 会话**；
第 2 次及以后的 `get_activation_factory<GraphicsCaptureItem>()` 确定性失败（抛出的异常既不是
`hresult_error` 也不是 `std::exception`）。这与代码里的计数/锁/detach/拆除时序都无关，
属于该 OS/驱动环境的 WGC 行为。**产品侧已用两条兜住**：
① `BindExOnWindows10` 有界等就绪、失败返回 0（不再静默黑图）；
② `normal.auto` 因此能回退到 DXGI（实测日志 `normal.auto selected normal.wgc but bind failed, fallback to normal.dxgi`）。
`normal.wgc` 显式指定时则诚实报绑定失败——这比"绑定成功 + 全黑图"正确。

**现象**：修掉 AV 后，第 2 个会话不再崩，但 `Init()` 仍然失败，日志稳定复现：

```
x64info: WGC worker thread unknown exception 0x8000FFFF
x64info: requestCapture: WGC worker init is not ready
```

即 `runInMtaApartment` 的 catch-all 接住了一个**未知类型**的异常（`/EHa` 下 SEH 也会落到这里），
`Init()` 返回失败 → `win10WorkerInitSucceeded_ = false` → 首帧取到 `000000`；随后
`UnBindExOnWindows10` 等满 `kWin10CloseWaitMs = 1500` 才返回，每个失败会话固定多付 1.5 s。

**下一步（还没做）**：在 `Init()` 的 389/424/443/450/453/455/507 各加一条带时间戳的 `setlog`，
定位是哪一步抛、抛的是什么（HRESULT 或 SEH code）；同时给 `BindExOnWindows10` 加有界的就绪等待，
让 `normal.auto` 在 WGC 起不来时能真正回退到 DXGI（现在 `BindExOnWindows10` 起完线程就 `return 1`，
Win10 的 `waitForBindReady()` 又是空实现，所以 auto 永远不会回退）。

### 8.3 本节的验收口径（由验收角色提出，未执行部分一律算计划）

- **G1**：T1 连跑 10 次 → 0 AV、0 次 `worker init is not ready`、第二会话取色 `FF0000`、总耗时 ≤1.5s；
- **G2**：全量连跑 10 次 → 不出现 `previous WGC cleanup is still pending` / `worker init is not ready` / `WGC worker stop timed out`；
- **G3**：Debug + cdb 同一命令 → `IUnknown::add_ref` 帧不再出现；
- **G4**：`RepeatedBindUnbindAndMaximizedRepeatedCapture` 改成**每轮新建窗口**、20 轮全绿、句柄差值为 0；
- **G5**：Debug + Release 各一轮、x86 Release 构建并跑 T1；
- **G6**：改动前快照可回滚，回滚后 T1 能复现本缺陷。
当前状态：**G1 未达成（0 AV 达成，`init is not ready` 仍在）**；G3 待复跑；G4/G5/G6 未做。

### 8.4 本轮后半段的第二个修复：`normal.auto` 的回退链恢复（已实测有效）

`BindExOnWindows10()` 原来「起完 worker 就直接 `return 1`」，而 Win10 的 `waitForBindReady()` 是空实现
——于是"绑定成功"不代表会话可用：第 2 个会话 init 失败时脚本拿到的是**全黑图**，`normal.auto`
也永远不回退到 DXGI。改为**有界就绪等待**（复用 `waitForWin10WorkerInit(kWin10RequestInitWaitMs)`，
失败置 `win10WorkerStop_` 并 `return 0`）。

**实测（Release，本机 19045）**：

| 场景 | 改前 | 改后 |
|---|---|---|
| T1（两会话：`CapturesStaticWindowFromStart` + `NormalAutoUsesWgcForKnownBrowserClasses`） | 第 2 条 FAILED，1840 ms，取色 `000000` | **3/3 PASS**，第 2 条 167~205 ms，颜色正确（auto 已回退到 DXGI） |
| 全量 14 条 | 6 通过 / 8 失败（含 1 次级联 6 条） | **8 通过 / 6 失败**，0 级联、0 AV |
| 失败用例耗时 | 1800~5100 ms（每会话固定等 1500 ms） | **24~1031 ms**（绑定阶段即失败，不再假装绑定成功） |

剩下 6 条失败全部是**显式 `normal.wgc`** 的用例（`FirstFrameResizeAndCloseScenarios`、
`MinimizeRestoreStillCaptures`、`MaximizeAfterBindCapturesFullClientArea`、
`MaximizedWindowBindCapturesFullClientArea`、`RepeatedBindUnbindAndMaximizedRepeatedCapture`、
`FirstCaptureAfterContentChangeIsFresh`），原因都是同一件事：本机第 2 个 WGC 会话起不来。
它们现在是**诚实地 bind 失败**，而不是"绑定成功 + 返回黑图"。

## 8-旧. 第一轮的缺陷记录（保留作历史，根因以 §8 为准）

**现象**（`bin\x64\op_test.exe --gtest_filter="CaptureBackendTest.*:WgcTest.*"`，14 条用例）：

1. `WgcTest.NormalAutoUsesWgcForKnownBrowserClasses` 起报
   `requestCapture: WGC worker init is not ready`，取色得到 `L"000000"`；
2. 其后的**每一条** `normal.wgc` 绑定都失败：
   `BindEx: previous WGC cleanup is still pending on Windows 10` + `BindWindowEx failed ... ret=0`
   （连续 6 条用例级联失败）；
3. 约 3/7 次运行会在上述阶段直接 `0xC0000005`（AV）退出。

**根因链**（`libop/capture/backends/WgcCapture.cpp`）：

- `WgcCapture::runWin10Worker()` 在 worker 线程里做 `Init()`（包含 `GraphicsCaptureAccess::RequestAccessAsync(...).get()`
  这类阻塞的 WinRT 调用），成功后才置 `win10WorkerInitReady_`；
- `requestCapture()` 用 `waitForWin10WorkerInit(kWin10RequestInitWaitMs)` 等它：实测这条路径上 Init
  **超过 4.5 s 仍未返回**（把预算从 300ms 提到 3000ms 也没等到），于是首帧直接失败；
- worker 卡在 Init 里就永远不会执行到末尾的 `g_win10WgcCleanupInFlight.fetch_sub(1)`；而
  `BindExOnWindows10()` 用进程级 `waitForWin10WgcCleanup(200ms)` 做门禁 → **计数永久 >0 → 本进程后续所有
  `normal.wgc` 绑定全部返回 0**；
- `UnBindExOnWindows10()` 超时后退化为 `win10Worker_.detach()`，此时"计数归谁减"和"谁还能碰这个 WinRT
  对象"都变得不确定，AV 就出在这条路径上。

**已尝试且已回退的两个修法**（留档，避免重复踩）：
- 把门禁改成"告警并继续"：级联消失，但 crash 从"干净返回 0"变成 AV（新会话与 stuck worker 并发创建），**更糟**；
- 给计数加 `shared_ptr<atomic<bool>>` 令牌让实例侧/worker 侧只减一次：计数不再泄漏，但 Init 仍卡死、
  AV 仍在（崩溃与计数无关），属语义变更而非修复，已回退。

**正确的修法（需要下一步做）**：
1. 用 **Debug 配置**（带 PDB）复现 AV，在 `cdb` 里看清是哪个 WinRT 调用卡住 / 哪一帧崩的
   （当前 Release 无 PDB，`cdb` 只能给到导出符号+偏移，定位不到行）；
2. 让 worker 初始化**不阻塞**：`RequestAccessAsync` 不要 `.get()` 死等，改成可取消/带超时的异步；
   初始化失败要把 worker 拉回来（置 stop + 跳出），而不是留在原地；
3. 去掉进程级 `g_win10WgcCleanupInFlight` 门禁，或改成按 HWND 维度 + 有上限的等待；
   同时**禁止 detach**：要么 join（对象由 `shared_from_this` 保活，worker 退出后再析构），
   要么把 WinRT 对象所有权整体搬进一个 `shared_ptr<WorkerState>`，让 worker 不再触碰实例成员；
4. 修完后 `WgcTest.*` 应全绿（`NormalDxgiMaximizeAfterBindCapturesClippedClientArea` 需另修，它修复前就失败）。

## 9. 跨进程命名约定（**含历史互换，禁止单方面改名**）

注入式后端（`dx.*` / `opengl.*`）靠两个**命名内核对象**与注入 DLL 通信，名字由
`libop/base/AutomationModes.h` 的两个 helper 生成，主控侧（`ICaptureBackend::bind_init`、`WgcCapture`）
与注入侧（`DisplayHook`）**都调用同一对 helper**：

| 用途 | 真实名字 | 生成函数 |
|---|---|---|
| 命名共享段（`SharedMemory`，放 `FrameInfo` + 像素） | `op_mutex_<hwnd 十进制>` | `MakeOpSharedResourceName(hwnd)` |
| 命名互斥体（`ProcessMutex`，保护这块段） | `op_shared_mem_<hwnd 十进制>` | `MakeOpMutexName(hwnd)` |

**名字是故意互换的**：共享段带 `mutex` 前缀、互斥体带 `shared_mem` 前缀。这是历史遗留，
但它是**线格式**——已经部署的旧注入 DLL 会按旧名字去找对象。

因此：

- **不要**"顺手改成语义正确的名字"。单方面改名会让旧注入 DLL 直接找不到对象（表现为注入成功但一直取不到帧）；
  要改必须**主控端 + 注入端同步发版**，并且新旧版本不能同时运行在同一进程里。
- 名字里的 hwnd 用**完整指针宽度**（x64 是 64 位）。曾经的
  `SHARED_RES_NAME_FORMAT = L"op_mutex_%d"` / `MUTEX_NAME_FORMAT = L"op_shared_mem_%d"` 两个宏因为
  从未使用、且 `%d` 会把 x64 句柄截断，已于 2026-10-21 删除。
- 该约定有回归护栏：`tests/op_algorithm_windows_test.cpp` 的
  `DisplayInputTest.SharedResourceNamesAreLockedWireFormat` 把两个名字**逐字**钉死
  （含"64 位句柄不能被截断"的断言），
  `tests/capture_backend_test.cpp` 也改成直接用 helper 拼名字，不再自己 `swprintf_s`。

## 10. DXGI 取不到新帧时的环境判定清单（2026-10-22 实测）

> **先跑这个**：`powershell -ExecutionPolicy Bypass -File scripts\check_env.ps1`
> 它会把下面四步的事实一次性打出来，并直接给 `[OK]`/`[WARN]` 判定（会话类型、显示适配器是否含 IDD/虚拟、
> 显示器布局、DWM、OCR 端点可达性、Tesseract 是否存在、DXGI 本次能否取到帧）。
> 下面保留手工命令，用于脚本不可用时逐条核对。

`dx.d3d12` 走的是另一条链路（注入 hook Present + 自建 DIRECT 队列），它的抓帧正确性有独立自检：

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_d3d12_probe.ps1
```

探针在本进程建真实 D3D12 交换链，直接调 `op::hook::D3D12Capture::GrabBackBuffer` 逐像素比对
（第一帧、清屏色变化、连续 8 帧、状态栅栏还原），**按 stdout 的 PASS/FAIL 判定**——调试层开着时
进程退出码会被改成 2173，别信退出码。细节见 `doc\defect_fixes.md` §70。

`DxgiRegionCaptureReusesLargerStagingTexture` 在整套回归里约 **2/3 轮**取不到新帧（三条 2s 等待全超时），
单独跑又总是通过。已排除的**代码侧**因素：窗口钉主显示器、`SetThreadExecutionState` 保持唤醒、`DwmFlush`。

排查这类"只有整套跑才失败"的问题，先按四步确认环境，再怀疑代码：

```powershell
# 1) 会话类型：Console 才有真实桌面；RDP/服务会话下桌面复制不稳定
$env:SESSIONNAME

# 2) 显示适配器：出现 IDD / 虚拟显示适配器（Oray、GameViewer、AskLink…）时，
#    桌面可能由虚拟驱动呈现，DXGI Desktop Duplication 的帧更新不可靠
Get-CimInstance Win32_VideoController | Select-Object Name, Status, PNPDeviceID

# 3) 显示器布局（多屏时窗口落在哪个输出很关键；本项目用例已钉到主屏）
Add-Type -AssemblyName System.Windows.Forms
[System.Windows.Forms.Screen]::AllScreens | Select-Object DeviceName, Primary, Bounds, WorkingArea

# 4) DWM 是否在跑（桌面复制依赖 DWM 合成）
Get-Process dwm -ErrorAction SilentlyContinue
```

**本机实测（2026-10-22）**：

| 项 | 值 | 结论 |
|---|---|---|
| `SESSIONNAME` | `Console` | 不是 RDP，排除"无桌面" |
| 显示适配器 | `OrayIddDriver Device`、`GameViewer Virtual Display Adapter`、`AskLink Display Adapter (SDR)`、`NVIDIA GeForce RTX 3060 Ti` | **有 3 个虚拟显示适配器** |
| 显示器 | 2 个：主屏 `(0,0,1920×1080)`、副屏 `(-1920,-5,1920×1080)` | 双屏、负坐标 |
| `dwm.exe` | 运行中 | —— |
| 该 DXGI 用例单独跑 ×5 | **0 失败** | 单跑稳定 |
| 整套 `ctest` 连跑 | 约 **2/3 轮**失败 | 只有"连跑"复现 |

**当前判定**：最可能是**虚拟显示适配器下桌面复制的帧更新不可靠**（连跑时前一个进程刚释放输出，
驱动没及时出新帧），而不是 `DxgiCapture` 的逻辑错误 —— 同族逻辑在单跑与 GDI 变体上稳定通过。

**换机器复跑时的判定方法**：
1. 只有物理 GPU、Console 会话仍然复现 → 是后端问题，回到代码；
2. 也有 IDD/虚拟适配器 → 与本机同类，属环境；
3. 无论哪种，先把上表四项填出来再下结论，别只写一句"偶发失败"。

**当前处理（不许伪装成通过）**：该用例拿不到帧时 `GTEST_SKIP` 并打印实测/期望颜色，
跳过会出现在 `[ ENV-SKIP ]` 账目里；同族逻辑仍由 GDI 变体与其它 DXGI 用例严格覆盖。

## 11. 后台（hook 注入式）截图的每帧成本（2026-10-23 实测）

`normal` / `dxgi` / `wgc` 之外还有一类截图：`dx.dxgi`、`dx.d3d11`、`dx.d3d10`、`dx.d3d9`、`dx.gl`。
它们不是"从桌面复制像素"，而是**往目标进程注入 `op_x64.dll` 并 Hook `Present`**，
在**目标的渲染线程里**把后备缓冲读回写进命名共享段。

所以这条路径的耗时不是"宿主读图慢一点"，而是**直接从玩家的帧时间里扣**：
1080p、目标 60fps（每帧 16.7ms）时，优化前的 2.66ms 等于固定吃掉 16%；144fps 时吃 37%。

### 11.1 现在的数字（1080p，本机 RTX 3060 Ti）

| 阶段 | 优化前 | 现在 |
|---|---|---|
| `GetDesc` + `GetBuffer` | 0.001 ms | 0.001 ms |
| `CopyResource` | 0.000 ms | 0.000 ms |
| `Map`（等 GPU 就绪） | 0.399 ms | 0.407 ms |
| `open` 共享段 + 互斥体 | 0.009 ms | **0.007 ms（复用，仅每 64 帧重开一次）** |
| 整帧写入共享段 | **1.874 ms** | **0.657 ms** |
| `Unmap` | 0.005 ms | 0.005 ms |
| **合计（mean）** | **2.7268 ms** | **1.05 ~ 1.10 ms** |

（"现在"这一列是探针 `--phase` 分阶段实测，和同一进程里直接跑 `dx11_capture` 的均值 1.05~1.10 ms 对得上。
`--phase` 走的是产品同一套 `SharedFrameChannelLease`，所以量到的就是真实路径。）

尺寸相关性（每帧均值，本轮实测）：

| 分辨率 | 像素量 | mean | p95 |
|---|---|---|---|
| 640×360 | 0.23 Mpx | 0.12 ~ 0.17 ms | 0.30 ~ 0.48 ms |
| 1280×720 | 0.92 Mpx | 0.47 ms | 0.88 ms |
| 1920×1080 | 2.07 Mpx | 1.05 ~ 1.10 ms | 1.5 ~ 1.6 ms |
| 2560×1440 | 3.69 Mpx | 2.165 ms | 3.41 ms |

基本还是随像素线性（约 0.55 ms/Mpx，优化前是 1.27 ms/Mpx），2560×1440 略超线性是因为那一档
`Map` 等 GPU 的时间本身变长。交换链是 `R8G8B8A8` 时（要走 R/B 通道交换，已改用 SSE2）1080p 约 1.05ms。

### 11.2 病根不是"建纹理"，是每帧重开映射

直觉会怀疑每帧 `CreateTexture2D`，实测把它缓存住之后**收益为零**（A/B 三轮都在噪声内）。
真正的账在共享段这边：

| 写法 | 1080p | 带宽 |
|---|---|---|
| 每帧新映射 + `memcpy` | 1.87 ms | ≈4.4 GB/s |
| 复用同一映射 + `memcpy` | 0.63 ms | ≈12.2 GB/s |
| 复用同一映射 + SSE2 非临时写 | 0.47 ms | ≈16.4 GB/s |

（这两张表的 0.47 和上表的 0.657 都是真的，量的东西不同：0.47 是探针 `--copybench` 里**纯搬运**一段的裸速度，
0.657 是 `--phase` 里经 `CopyImageData` 完整路径 —— 多了写 28 字节帧头、取一次跨进程互斥体、以及首帧不预热。）

`MapViewOfFile` 本身只要几微秒（它是惰性的），代价全在随后第一次写那块内存时的**软缺页**上 ——
每帧换一块新视图就等于每帧把整帧重新缺页一次。

三处改动（详见 `doc/defect_fixes.md` §71）：

1. `libop/hook/SharedFrameChannel.*`：映射与跨进程互斥体按名字缓存复用，每 64 帧自愈式重开一次
   （宿主重绑窗口会建新段，旧映射句柄还吊着旧对象，必须定期对齐）；
2. `libop/hook/DxCaptureCommon.cpp`：≥256KB 的整帧搬运走 SSE2 非临时存储；`R8G8B8A8` 的通道交换同样 SIMD 化；
3. 一次瞬时失败（设备重置、驱动升级、DWM 重建交换链）不再永久停抓 —— 连续失败 30 次才关。

### 11.3 复现测量

```powershell
# 注入侧每帧耗时 + 像素正确性 + 分阶段计时
powershell -ExecutionPolicy Bypass -File scripts\run_hook_capture_probe.ps1 -Phase

# 交换链是 R8G8B8A8（走通道交换路径）
powershell -ExecutionPolicy Bypass -File scripts\run_hook_capture_probe.ps1 -Rgba

# 整帧搬运基准（memcpy / 流式写 / 读回）
powershell -ExecutionPolicy Bypass -File scripts\run_hook_capture_probe.ps1 -CopyBench
```

探针在本进程里建真 D3D11 交换链并直接调 `op::hook::dx11_capture()`（同一份源码，不复制），
判定看 stdout 的 `PASS: 0 failure(s)`，**不看退出码**（沿用 §10 里 d3d12 探针的约定）。

跨进程链路（宿主 `requestCapture`、共享帧头、重绑时序）由 ctest 项 `hook_e2e` 覆盖：
`tests/d3d11_host_app.cpp` 是真目标程序，用例断言"6 次采样读到不止一种颜色"，并自证该断言有牙
（`OP_HOOK_E2E_STATIC=1` 让目标不换色时必须变红）。

## 12. 下一步要做什么（与截图性能相关）

完整交接清单在 **`doc/next_steps.md`**，这里只摘与本文件相关的部分。

### 12.1 可以立刻做

| 项 | 为什么 | 改哪里 |
|---|---|---|
| 其余四个后端的跨进程 e2e | 五个后端改的是同一段共享段搬运代码，但只有 `dx.d3d11` 被真注入验过；探针是进程内直调，绕过了注入与跨进程 | `tests/d3d11_host_app.cpp` 扩成 `--api=d3d9/d3d10/d3d11`；`tests/CMakeLists.txt:219` 起加 target，用例加进 `HookCaptureE2ETest` |
| x86 的 `hook_e2e` 进常规回归 | 本轮手动验过一次（13.21s 通过），但平时 `ctest` 只跑 x64；`FrameInfo` 是 28 字节线格式，混搭架构只有真注入才验得出来 | 发布前检查里加一条 x86 的 `ctest -R "hook_e2e|capture_backend"` |
| 探针 `--pipeline` 是空开关 | `scripts/hook_capture_probe.cpp:69` 有这个字段、第 90 行解析，但没有任何代码读它 —— 给了参数照常跑、什么都不多测 | 删掉（建议）或真去模拟"注入侧写一帧 → 宿主侧读一帧"的全链路延迟 |

### 12.2 需要拍板才能做（都会改外部可观察行为）

- ~~**staging ping-pong**~~ **已实现并实测（第 67 批，§72.17），结果和当初的预期不一致**：
  1080p 无 vsync（探针 `Present(0,0)`，600 帧）**持平**（1.0540 → 1.0593 ms）——
  省下的 `Map` 等待被"CPU 整帧搬运与 GPU 拷贝抢带宽"抵掉了；
  有 vsync（`Present(1,0)` 60Hz，240 帧）**快 23%**（980.0 → 757.3 µs）。
  因为收益取决于目标进程是否 vsync、代价是 `Capture` 拿到上一帧，**默认仍是 sync**，
  `OP_HOOK_PINGPONG=1` 显式开启。代码在 `libop/hook/D3D11Capture.cpp` 的 `capture_once()`。
- **只搬请求区域**：宿主常常只要一小块，但跨进程传矩形等于改共享段线格式（`FrameInfo` 锁死 28 字节），
  与 §9 的"禁止单方面改名"同一条约束。详见 `doc/next_steps.md` §3.2。

### 12.3 需要外部条件

- **Win10 上 WGC 单进程第二次会话必失败**（§8.2）：平台限制，每条单独起进程才全绿，
  所以 11 条 WGC 用例各注册成一个 ctest 项。**不要为了省时间把它们合回一个进程。**
- **`dx.d3d12` 需要真实 D3D12 目标**：自写探针 7 条断言全过（其中一条钉住"`GrabBackBuffer` 回填的格式
  等于交换链格式"，见 `doc/defect_fixes.md` §72.4/§72.9），但测的是我们自己的交换链；
  多交换链、多队列、跨线程 `ExecuteCommandLists` 只有真游戏能覆盖。另注意探针退出码不可信
  （调试层会把它改成 2173/0x87D），一律看 stdout 的 `PASS:`/`FAIL:`。

### 12.4 明确不做

`normal.wgc` 在 Win10 失败后自动降级到 DXGI —— **操作者明确禁止**：不实现、不启用、不建议，
DXGI 降级的范围与触发条件也不动。
