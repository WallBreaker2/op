# 下一步要做什么

这份文件是**交接清单**：已经修了什么、现在什么状态、接下来具体该动哪里。
历史细节在 `doc/defect_fixes.md`（§1~§72），性能数据在 `doc/capture_perf.md`。

**先说结论，避免有人重复劳动：下面第 2 章里的项目全部已经做完并验证过，
不要再把它们当"待办"重开一遍。**

---

## 0. 当前状态（2026-10-24 第 99 批收尾）

| 项 | 状态 |
|---|---|
| `ctest -C Release`（x64） | **34/34 通过**，234.35s（DX 10 条与真实截屏 8 条已拆成逐进程/逐用例项，§72.59；基线 70.71s，涨的是 `hook_e2e` 从 2 条变 28 条） |
| `ctest -C Release`（x86） | **34/34 通过**，243.39s —— 从第 66 批起 x86 进常规回归；含 29 条注入用例。历史上那条 DX 偶发失败已随拆项消失（§72.59 / §4.4） |
| x64 / x86 构建 | 零 error、零 warning（x86 仅 gtest 头里的既有 `C4018`） |
| 三角色审计（架构/安全/验收） | **21 条已修**（R1/R2/R4/R-01/R-02/R-03/R-04/R-05/R-07/R-11/R-12/R-14、安全 #1/#2/#4/#5/#6/#7/#9、B1/B2），**0 条未做**；9 条明确不修并写明理由（R5/R6/R8/R9/D/E/A/C、R-06），见 `doc/defect_fixes.md` §72.51 与 §72.54 |
| OpenCV「真实图片」用例 | 5 条永久 skip 已修：根因是 `FindDemoAsset` 只向上找两级 + 素材树从未进仓库；新增 `scripts/gen_demo_assets.py` 生成 34 组 40 个 PNG，`OpenCvTest` 27 条全跑全过（§72.52.1） |
| 接口覆盖率 | **三面 100%**：C++ `op::Op` 214/214、C API 186/186、COM `IOpAutomation` 214/214。账本由 `scripts/api_coverage.py` 生成（抽符号 + 拿 `tests/` 当语料），本轮补了 53 个此前零调用的函数，新增 `tests/api_surface_test.cpp` 9 条用例；补测中发现 `SendPaste` 不校验 hwnd 就返回成功，已修（§72.55） |
| 跨语言验证 | 托管语言（C#）经 `DllImport` 调 C API 的冒烟已进仓库：`tests/csharp_smoke/` + `scripts/run_csharp_smoke.ps1`，ctest 项 `csharp_smoke`（找不到 dotnet SDK 退 77 记 SKIP，不拖红）。这条用例抓到过 C++ 测试抓不到的真缺陷 —— C API 返回的短字符串指针被下一次调用覆盖（`std::wstring` 的 SSO 让 `swap` 换不掉 buffer 地址），已改为 4 槽环形池（§72.58） |
| 函数调用文档 | 索引 `doc/api.md` + 6 个主题分文件 `doc/api/0*.md` + 手写配方 `doc/api/recipes.md`，**214 个公开方法**全部带说明，由 `scripts/gen_api_doc.py` 从头文件生成（改完头文件重跑即可，不会与代码脱节）。OpenCV 35 个 / 内存读写 8 个 / 杂项 12 个原本没有注释，说明是对着实现补的。`ApiDocTest.DocumentedMethodsMatchHeader` 把「头文件改了文档没跟」变成会失败的用例（§72.56、§72.57） |
| 并发约束 | 同一个 `op::Op` 实例的 OCR/Find 系 API **不是线程安全的**（共用“当前图” `_src`）；HTTP session/connect 池则是进程级共享且并发安全（§72.52.2） |
| hook 用例的跳过门禁 | 5 处「环境缺失就 `GTEST_SKIP`」改走 `OP_HOOK_E2E_SKIP_OR_FAIL`：设 `OP_REQUIRE_HOOK_E2E=1` 时变**红**而不是变绿；新增 ctest 项 `hook_require_e2e`（3 条 / 18 s）把门禁纳入回归；x64 与 x86 各 16 项全过（§72.53） |
| op_test 规模 | x64 **327 条（237 通过 / 90 skip）**；x86 **328 条（236 通过 / 92 skip）** —— skip 全部来自按环境分流的套件（WGC / 截图 / OCR 与 YOLO 契约 / hook e2e），各自都有独立的 ctest 项 |
| 后台截图注入侧 1080p | 整窗每帧 **1.10 ms**；只搬请求区域时 480×270 降到 **0.12 ms**（10.8%，字节 6.2%，§72.18/§72.29）；宿主侧来源视图已收窄到请求矩形（§72.29）；`OP_HOOK_PINGPONG=1` 可再快 23%（仅 vsync 场景，代价滞后一帧，§72.17） |
| 跨进程 e2e 覆盖 | **五个后端全有**（d3d9/d3d10/d3d11/d3d12/opengl），每个后端"读新帧 + 重绑 + 区域 = 整窗"三条；**五个后端都有顶部标记带断言**（上下方向不再是推断，§72.20.3/§72.25）；x86 另加一条跨位宽用例；**多链/多窗口三条**（d3d12 §72.21、d3d11 §72.22、opengl §72.23）；**`wglSwapBuffers` 直接入口一条**（§72.27）；**opengl.std / opengl.fi / opengl.es 三条**（§72.24/§72.25），再加一条"换绑到另一个窗口"（§72.30），共 **28 条（x64）/ 29 条（x86）** |
| 32 位宿主绑 64 位目标 | **支持**：宿主拉起 64 位助手 `op_inject64.exe` 完成注入，真读到帧（§72.16）；助手的模块查找已改成"Toolhelp 重试 + `K32EnumProcessModulesEx` 双来源"，修掉跑全套时三条跨位宽用例必红的问题（§72.32） |
| 共享段/互斥体名字 | **已改回正名**：段 `op_shared_mem_<hwnd>`、互斥体 `op_mutex_<hwnd>`（§72.20.1，主控与注入必须同批发版） |
| API 层审查（窗口/输入/内存/图像） | 修掉 6 处真缺陷：`ClientToScreen` 假成功 + 空出参崩溃、`MoveWindow` 未初始化 `RECT`、`KeyPress` 修饰键不回滚（§72.28）；`GetForegroundFocus` 用线程局部 `GetFocus()` 导致工作线程恒返回 0、两个 delay setter 模式名大小写敏感、`KeyPressStr(L"")` 返回 0（§72.37） |
| 内存 API | 跨进程读不再每次 Attach（pid 不变就复用）、地址表达式解析缓存 16 条、本进程路径绕过 BlackBone（§72.29）；**批量形状已落地**：`ReadIntBatch`/`WriteIntBatch`/`ReadDataBatch` 走完 ProcessMemory → Op → COM(`SAFEARRAY(VARIANT)`) → C API 四层，逐条 2.93 µs → 批量 0.45 µs（**6.55×**，§72.31）；Python 绑定已用 swig 4.5.1 重生成，`ReadIntBatchText`/`WriteIntBatchText` 可直接调（§72.36）；坏地址不再连坐整批（§72.36）；**C API 也复用同一个 `ProcessMemory`**（以前每次调用现造，attach 缓存全丢）：跨进程单条读 5.96 µs → **2.47 µs（2.41×）**、本进程 1.74 µs → 0.77 µs（§72.38） |
| OCR 响应解析 | 手写单遍扫描器替代 `std::regex`：50 条结果 824 µs → **11.6 µs（71×）**；`text` 改 UTF-8 直转，非 GBK 字符不再被压成 `?`（§72.33） |
| YOLO 响应解析 / base64 | 解析同样换成手写扫描器：50 条结果 1118 µs → **11.5 µs（97×）**，标签 UTF-8 直转不丢字；`HttpClient` 的 base64 从 `CryptBinaryToStringA` 换成表驱动：1080p 一张图 **85.6 ms → 5.6 ms（15.3×）**，输出逐字节与 CryptoAPI 一致（§72.34） |
| HTTP 响应体读取 | 不再每块新建临时串再 `append`，直接读进目标串尾部 + 预预留 64 KB；新增一条 3000 条结果（≈156 KB、跨多块）的契约用例钉住"多块拼接不丢尾"（§72.35） |
| HTTP 完成等待 | `http_post_json` 的 `sleep_for(5ms)` 轮询改成条件变量：Windows 默认定时器粒度 15.6ms，旧写法让每次调用白等约 14.5ms。`OcrAutoFromFile` 对本地零延迟服务端 **15530 us → 3585 us（4.33×）**（§72.39） |
| 图片文件读取 | `Image::read(LPCTSTR)` 加了 BMP 直读快路径（`BI_RGB` + 24/32 位 + 行对齐），其余格式仍走 GDI+。GDI+ 解一张 3KB 的 BMP 要 1.8ms，直读降到 ~0.1ms：`OcrAutoFromFile` 对本地零延迟服务端 **3052 us → 938 us（3.25×）**（§72.40）；新增 4 条 `ImageReadBmpTest` 钉住 32 位自上而下 / 24 位自下而上带行填充 / 8 位调色板回落 / 写读往返 |
| HTTP 连接复用 | 会话池（host/port/scheme/UA 四元组 + 8 条上限）让 WinHTTP 的连接池活下来，不再每次重新 `WinHttpOpen`+`Connect`；失败时摘连接并只在“还没收到响应”的阶段重试一次。`OcrAutoFromFile` 对本地零延迟服务端 **938 us → 513 us（1.83×，从 §72.39 起算累计 30.3×）**（§72.41）；新增 3 条 `SessionPool*` 用例（顺序复用 20 次 / 超时后恢复 / 4 线程并发） |
| FindPic 匹配路径 | 积分图 `record_sum` 重写（只清第 0 行/列、内层改行前缀和 + 行指针，不再逐像素 `at<int>()`）：1080p 全图扫描 **7.00 ms → 3.80 ms（1.84×）**，其中积分图 **3.88 ms → 0.67 ms（5.8×）**；`scan` 段（线程池切块）残留抖动记为不做（§72.42） |
| 捕获后端绑定与地板 | **GDI 桌面捕获的地板是 16.7 ms/次（60 Hz vblank）**，不是缺陷：`BitBlt` 被 DWM 合成节奏同步，320×240 与 1920×1080 几乎相同（15.3 vs 19.1 ms）；`normal.dxgi` 捕获只要 1.5 ms/次（§72.44 修正了早前"19.5 ms"的错算）。**绑定成本**：DXGI 168 ms 里 93% 是 `D3D11CreateDevice`，独占时复用进程级共享设备后同进程重绑 **120~137 ms → 11~23 ms（6~11×）**（§72.45）；WGC 140 ms 同样 100% 在设备创建，复用后重绑 **140 ms → 32~47 ms（3~4.4×）**（§72.46）。**注入后端**（`dx.d3d9` / `dx.d3d10` / `dx.d3d11` / `dx.d3d12` / `opengl*`）首次绑定 188 ms 里 140 ms 是 kiero 建临时设备读 vtable，同进程重绑复用 locate 结果后 **63 ms → 31 ms**（§72.47） |
| 累积分批 | 100 批，`doc/defect_fixes.md` 本批新增 §72.60（R4 收敛）与 §72.61（两条偶发红 + 输入侧 `result()` 抛异常），编号从 §72.1 起算 |
| 五个后端的帧发布骨架 | **已收敛到一处**（R4，§72.60）：`libop/hook/SharedFrameWriter.h` 的 `SharedFrameView`（裸指针 + 长度）替掉 `std::span`，头文件不再要求 C++20；D3D9/D3D10/D3D11/D3D12 全走 `make_shared_frame_span` + `write_shared_frame*`，只有 OpenGL 保留 `pshare`（`glReadPixels` 直写共享段，没有中间缓冲可拷） |

验证命令（任何改动之后都应跑这两条）：

```powershell
cd build/vs2022-x64-Release; ctest -C Release --output-on-failure
cd build/vs2022-x86-Release; ctest -C Release --output-on-failure
```

---

## 1. 本轮做完的（第 66~72 批）＋ 下一批候选

### 1.1 ✅ 补齐其余四个后端的跨进程端到端用例

已做完，细节见 `doc/defect_fixes.md` §72.2~§72.7。

- 新增目标进程程序 `tests/gfx_host_app.cpp`（`--api=d3d9|d3d10|d3d12|opengl`），
  `d3d11_host_app.cpp` 保持原样不动。
- `tests/hook_capture_e2e_test.cpp` 新增 `HookCaptureGfxE2ETest` 套件 8 条用例（每个后端"读新帧 + 重绑"），
  `hook_e2e` 从 2 条变 **10 条**，x64 65.86s / x86 同量级全过。
- 过程中抓到两个真缺陷：**d3d12 把共享帧格式写死成 R8G8B8A8**（红蓝颠倒，§72.4）、
  **解绑时清掉正在跑的 detour 的 `old_address`**（目标进程崩溃，§72.5~§72.6）。
- D3D10 的 `E_INVALIDARG` 不是我们的 bug：本机 D3D10 旧 blt 交换链不接受 BGRA，换 R8G8B8A8 即可（§72.3）。
- opengl 额外断言了上下方向（顶部白色标记带 + 中心非白）。
- 顺带修掉 d3d12 探针自身的断裂（§72.9）：`run_d3d12_probe.ps1` 在 §71 改完注入侧之后链接清单没跟着
  更新（缺 `SharedFrameChannel.cpp` / `DxCaptureCommon.cpp`），加上 §72.4 改了 `GrabBackBuffer` 出参，
  本轮之前是编不过的状态；现在编真实现、删掉 `CopyImageData` 空桩，并多了一条格式断言。

### 1.2 ✅ x86 的 `ctest` 进常规回归

已做完：x86 全量 `ctest -C Release` 现在 15/15 通过（123.69s），并写进第 6 章的发布前检查。

顺带暴露并修掉一个**只在 x86 上成立**的用例缺陷：`DisplayInputTest.SharedResourceNamesAreLockedWireFormat`
里"64 位句柄不能被截断成 32 位名字"那两行断言在 32 位构建下构造不出高 32 位非零的场景，
`EXPECT_NE` 必然失败 —— 已用 `#if defined(_WIN64)` 圈住。要守的"互换命名是线格式"没变。

### 1.3 ✅ 探针的 `--pipeline` 空开关

已删：`scripts/hook_capture_probe.cpp` 的声明与解析两处一起删掉。
端到端延迟已有 `hook_e2e` 更可信的证据（`GetColor` 能稳定读到换色后的帧）。

### 1.4 ✅ COM 转发宏补 `try/catch`

`OP_COM_FORWARD(Name)` 宏生成的 **29 个** `(LONG *ret)` 纯转发方法（`Name`、`GetID`、`LeftClick`、
`UnBindWindow` …）原来是裸的，异常会穿出 COM 边界。try/catch 写进宏里，一次覆盖 29 个，
以后用这个宏加方法也自动带上。

### 1.5 ✅ `release()` 等不到静默时的延迟重试

原来 `DisplayHook::release()` 等 500ms 拿不到 `detour_depth == 0` 就放弃清理并 `setlog` 记一笔，
后果是钩子留在原地（`MH_RemoveHook` 没调、`is_hooked` 已清）→ 下次绑同一入口
`MH_CreateHook` 返回 `MH_ERROR_ALREADY_CREATED`，只能重启目标进程。

现在：`release()` 超时分支置 `pending_release` 标记；`setup()` 入口先
`finish_pending_release(1000)` 把上一次的残留收掉，收不掉就拒绝本次绑定并记日志
（宁可绑不上，也不在残留状态上再挂钩子）；`ReleaseDisplayHook()` 超时分支补一次
`finish_pending_release(200)`；`release_module_if_idle()` 增加守卫，待清理期间不卸载模块。
详见 `defect_fixes.md` §72.12。

这条路径要"解绑正好落在 detour 飞行中"才触发，宿主侧用例够不到注入进程里的 `detour_depth`，
所以没有新增用例，靠四个后端的 rebind 用例（共 8 次 release→setup）回归。

### 1.6 ✅ x86 主控 + x64 目标：已由 64 位注入助手打通（第 68 批）

原计划"把 `op_x64.dll` stage 到 x86 测试目录，再写一条混搭用例"。照做之后用例**稳定失败**，
继续挖下去发现不是缺用例，是这条链路在 BlackBone 上不成立：

- 注入报成功（`injected=1`）、导出解析得到、远端调用返回 0，但目标进程里**一行代码都没执行**
  （`__op.log` 无新增、共享段无 `FrameInfo`）；有一次挂了 **111 秒**才返回。
- 根因：`BlackBone/Process/RPC/RemoteExec.cpp:58` 用
  `switchMode = _process.barrier().type == wow_32_64` 选汇编器，而这个条件只在
  "64 位宿主 + WOW64 目标"时成立；32 位宿主 attach 原生 64 位目标时选到错位宽的 stub。
- 顺带修掉 `call_result_t::result()` 在失败时抛 `bad_optional_access` 的隐患（改用 `result(0)` + `success()`）。

已交付：`BindEx` 在注入前显式判定 `sizeof(void *) == 4 && is64` → 记日志 + `Detach` + 返回 `-2`；
x86 构建多一条 `X86HostToX64TargetFailsFastWithoutCrash`（断言返回 0、5 秒内返回、目标进程存活，
70ms 通过）；`tests/stage_x64_artifacts.cmake` 负责把 x64 产物 stage 到 x86 测试目录。
详见 `defect_fixes.md` §72.13。

第 68 批按 §3.4 方案 1 做了 `op_inject64.exe`，这条链路现在**真能绑上并读到新帧**：
x86 用例 `X86HostToX64TargetReadsFreshFrames`（5.8s 通过，绑定/解绑都走助手，目标进程不崩）。
详见 `defect_fixes.md` §72.16。

---

### 1.7 ✅ 只搬请求区域（第 68 批）

`Capture(x, y, w, h)` 只要一块时，注入侧不再整窗搬运：协商区放在像素区之后，
28 字节帧头一个字节没动，三种混搭都向后兼容。探针 1080p 整窗 1.0279 ms →
区域 480×270 **0.1022 ms（10.1×）**。细节见 `doc/defect_fixes.md` §72.18，
"要不要做"的选择题已经结案（`doc/next_steps.md` §3.2）。

### 1.8 ✅ 区域搬运的两个真缺陷：响应不写 + 缓冲区被复用（第 69 批）

§1.7 上线后 `hook_e2e` 里 `D3D9RegionCaptureMatchesFullFrame` 是红的，定位出两个缺陷
（`doc/defect_fixes.md` §72.19）：

- **响应只在"本次请求合法"时才写**：请求 TTL 250ms 一过期，注入侧退回整窗拷贝，
  而响应还停在上一次的矩形上 → 宿主按矩形解释整窗帧，读到错位像素。
  现在五个后端都无条件写响应（"描述缓冲区这一帧的实际布局"），`FrameExchangeAt` 判空
  保证旧宿主/无协商区时自动 no-op。
- **`GetScreenDataBmp` 的缓冲区是复用的**：用例先取整窗、再取区域、然后比对，
  实际比的是同一块被覆盖后的内存。测试侧改为先拷一份 `full_hold` 再比对，
  并加了一次预热读（注入侧要等下一帧才按矩形搬）。

同批修掉：OpenGL 双向翻转（上下颠倒）、等待新帧解锁后的悬垂视图、响应一直无效时白等
满 200ms、`exchange_seen` 粘滞、`img.create` 在失败路径之前污染调用方 `Image`。

补测：区域搬运在五个后端各有一份独立实现，原来只有 d3d9/d3d10/opengl 有区域用例，
d3d11（`CopySubresourceRegion`）和 d3d12（自建队列 `CopyTextureRegion` + 回读）没被真注入验过，
已补 `D3D11RegionCaptureMatchesFullFrame` / `D3D12RegionCaptureMatchesFullFrame`，
`hook_e2e` 变 **17 条（x64）/ 18 条（x86）**。注意这两条验的是**像素正确性**，
"注入侧真的只搬了矩形"由探针的 `CheckRegionTransport` 断言（读共享段看 `mode=Region` 和行距）。

### 1.9 ✅ 名字改回正名 + d3d12 模拟目标游戏化 + d3d10/d3d12 标记带（第 72 批）

三件一起做（`doc/defect_fixes.md` §72.20）：

- **改名**：段 `op_shared_mem_<hwnd>`、互斥体 `op_mutex_<hwnd>`（原来反着）。护栏用例改成钉新格式；
  主控与注入必须同批发版，混装时 `waitForBindReady` 超时会打日志提示。详见 §3.3。
- **d3d12 模拟目标游戏化**：`tests/gfx_host_app.cpp` 改成 3 个后备缓冲 + 2 套（分配器 + 命令列表）
  每帧轮转，替代"需要真游戏"这个外部条件。详见 §4.3。
  `opengl.es` 的目标程序用夸克网盘自带的 ANGLE（`tests/egl_host_app.cpp`），不再需要额外安装（§72.25）。
- **标记带**：d3d10 用 `CopySubresourceRegion`、d3d12 用 `CopyTextureRegion` 各画一条
  `width×height/4` 的白色带，默认四个后端全画；`kApiD3D10` / `kApiD3D12` 的 `has_marker` 改成
  `true`，于是这两个后端的"读新帧"用例也断言"顶部白、底部彩"——上下方向不再是推断。

### 1.10 ✅ d3d11 多交换链目标 + 按窗口过滤交换链（第 73 批）

`tests/d3d11_host_app.cpp` 加 `--extra-swap`（第二个窗口 + 独立线程 Present 的第二条链），
新用例 `HookCaptureMultiChainE2ETest.D3D11TwoSwapChainsKeepsReadingFrames` 首跑红，
根因是**多条链写同一个按 hwnd 命名的共享段**，"像素区"和"响应区"可能来自不同链。
修法是按 `DXGI_SWAP_CHAIN_DESC::OutputWindow` 过滤，别的窗口的链直接 `CaptureResult::Ignored`。
详见 `doc/defect_fixes.md` §72.22。残留：OpenGL 的 `wglSwapBuffers` 只有 HDC，暂未加同样过滤。

### 1.11 ✅ opengl 多窗口 + 额外窗口不抢前台（第 74 批）

`tests/gfx_host_app.cpp` 加 `--gl-extra-swap`（第二个 GL 窗口 + 独立线程 `SwapBuffers`，
故意不画标记带）。用例未过滤时 3 跑 2 红：另一窗口的帧混进同一个按 hwnd 命名的共享段。
修法是 `WindowFromDC(hdc)` 比对 `render_hwnd`（`glBegin`/`glFinish` 用动态取的
`wglGetCurrentDC`）。顺带把两个额外窗口改成 `WS_EX_NOACTIVATE + SW_SHOWNOACTIVATE` ——
它们原来抢前台，导致全量套件里 `capture_backend` / `hook_e2e` 偶发红。详见 `doc/defect_fixes.md` §72.23。

- [x] **1.12 `opengl.std` / `opengl.fi` 两条入口补覆盖**（§72.24，第 75 批）：`--gl-swap=begin|finish` 让目标程序在交换前走一次 `glBegin`/`glFinish`，加三条用例；`glBegin` 分支的按窗口过滤已用注入侧仪表证到（额外窗口 291 次调用全被拒、`gl_capture_impl` 292 次全部来自绑定窗口），A/B 反证不了的原因写在 §72.24.4。

- [x] **1.13 `opengl.es`（EGL）补覆盖，修掉 ES 画面上下颠倒**（§72.25，第 76 批）：新建 `tests/egl_host_app.cpp`（自声明 EGL/GLES2，加载夸克网盘自带 ANGLE），两条用例（新帧 + 重绑）在 x64/x86 上都真跑；宿主翻转判断里 `|| _render_type == RDT_GL_ES` 是错的（ES 也是底朝上），删掉后画面方向正确，A/B 反证见 §72.25.3。
### 1.15 ✅ 注入后端的绑定成本：140 ms 在 kiero 的临时设备上（第 94 批）

`DisplayHook::locate_render_method` 走 kiero，每次都要建一个临时窗口 + 真 D3D 设备读
vtable（140 ms），而同一个进程、同一个模块里那个地址是常量。改成把结果记住
（render_type + 模块基址 + target + detour），重绑时命中缓存就跳过 locate：

| 阶段 | 改前 | 改后 |
|---|---|---|
| 首次 `remote_call` | 188 ms | 188 ms（第一次必须真 locate） |
| 同进程重绑 `remote_call` | 63 ms | **31 ms** |

首次不变是固有的（vtable 还没被任何人读过）。`MH_Uninitialize` 常驻那半截明确不做：省个位数
毫秒，换的是 DLL 卸载后悬垂的 trampoline。单槽缓存在"交替绑两个不同后端"时会退化到重新
locate（慢 140 ms，不会错）。详见 §72.47。

**第 95 批（本轮）**

- **全量测试所有函数**：建覆盖率账本 → 三面接口从 75% 推到 100%，新增 `tests/api_surface_test.cpp` 9 条用例；
  顺手修掉 `WindowService::SendPaste` 不校验 hwnd 的问题。详见 `doc/defect_fixes.md` §72.55。
- `scripts/api_coverage.py` 成为常驻工具：改完代码跑一次就知道哪条公开函数又没人调用。
- README / README_EN 同步：补文档索引、测试章节、跨位宽注入能力、目录概览修正（`examples/` 实际是 `assets/`）。
- **函数调用文档**：新增 `scripts/gen_api_doc.py`，输出索引 `doc/api.md` + 6 个主题分文件
  `doc/api/01-runtime.md` ~ `doc/api/06-ocr.md`，214 个公开方法全覆盖；其中 55 个原本在头文件里
  没有注释的接口，说明是对着实现写的。详见 `doc/defect_fixes.md` §72.56。
- **文档守卫 + 配方**：`tests/api_doc_test.cpp` 双向比对头文件与 `doc/api/*.md`，
  文档漂移会让 ctest 变红；另加手写 `doc/api/recipes.md`（六个按任务组织的完整示例）。
  详见 §72.57。

### 1.16 ✅ 跨语言（C#）验证进仓库 + C API 返回值槽位池（第 96 批）

**做了什么**

1. `tests/csharp_smoke/`（`CSharpSmoke.csproj` + `Program.cs`）：自带 P/Invoke 声明，不依赖任何外部仓库；
   `Platform=x86` 时定义 `OP_X86` 选择 `op_c_api_x86.dll`。
2. `scripts/run_csharp_smoke.ps1`：编、把 `bin/<arch>/` 的两个 DLL 拷到输出目录、跑；
   dotnet 或产物缺失时退 **77**，由 ctest 的 `SKIP_RETURN_CODE` 认成 SKIP。
3. `tests/CMakeLists.txt`：找到 `dotnet` + `powershell` 才注册 `csharp_smoke` ctest 项（`TIMEOUT 300`）。
   **dotnet 不是本仓库的构建依赖**，`python build.py` 不受影响。
4. `libop/c_api/op_c_api.cpp`：返回值缓冲区从「单缓冲 + 上一轮」改成 **4 槽环形池**。

**为什么必须换掉单缓冲**（§72.58）

`call_string` 在 §72.48 之后是 `string_result` + `string_result_prev` 双缓冲，配
`prev.swap(result)` 表达「旧指针活过一次调用」。但 `<= 7` 个 wchar 的返回值
（`GetColor` 的 `"RRGGBB"`、`Ver` 的 `"0.4.8.3"`）落在 MSVC `std::wstring` 的 SSO 里 ——
内容存在对象自身内部数组，`swap` 只交换对象状态、不换 buffer 地址，指向它的指针照样被
下一次调用改写。这个承诺对短字符串从来没成立过。槽位是不同对象，`c_str()` 地址天然不同，
长短一律成立；顺带把 §72.48 的入参自别名也覆盖掉了。

**顺手确认的语义**：`Capture(x1, y1, x2, y2)` 的 `x2/y2` 是**右下角开区间**（宽 = `x2-x1`），
`Capture(0,0,319,239)` 存的是 319×239。已写进 `include/libop.h` 注释并同步到
`doc/api/04-image.md`；指针生命周期约定同步到 `doc/api.md`。

**验收**

```bash
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_csharp_smoke.ps1 -Arch x64
# 10/10 通过，PASS: 全部通过
cd build/vs2022-x64-Release && ctest -C Release -R csharp_smoke    # Passed 1.51s
```

### 1.17 ✅ `dx.d3d9` 绑定偶发失败：重试兜底 + 诊断落盘 + 输入侧同类修复（第 100 批）

x86 全量里 `D3D9RegionCaptureMatchesFullFrame` 偶发红（`BindWindowEx ... display=dx.d3d9 ret=0`，
257 ms 就返回），而目标进程一行日志都没有。根因是 BlackBone 的
`RemoteFunctionBase::Call`（`build/_deps/BlackBone/src/BlackBone/Process/RPC/RemoteFunction.hpp:66-123`）
在远程线程没跑完时也报 `STATUS_SUCCESS`：`RemoteExec.cpp:93-101` 的 `Join()` 是 `INFINITE`，
远程线程异常退出同样返回 `WAIT_OBJECT_0`，读回的 `callResult` 就是 0 ——
"没执行"和"返回 0"在调用侧不可区分。

`libop/capture/backends/HookCapture.cpp` 三处改动：

1. 绑定失败改 `setlog_persistent` 强制写 `__op.log`（测试侧 `SetShowErrorMsg(3)` 只走 stdout，
   ctest 控制台日志会被下一次成功跑覆盖，偶发问题永远留不下证据）；
2. `cret.success() && bind_ret != 1` 时 `release` + **重试一次**（实测代价 59 ms）；
3. `release_remote_display_hook` 自己的失败也不再静默（但**不用返回值判成败** ——
   目标侧本来没绑时 `ReleaseDisplayHook` 返回 0 是正常路径）。

顺带修了输入侧同类缺陷：`libop/hook/InputHookClient.cpp` 7 处无参 `result()` 在远程调用失败时
会抛 `std::bad_optional_access`，改成 `result(0)`。

细节见 §72.61；没做的是"注入后确认目标主线程就绪"，见 §4.7。

## 2. 已经做完的，别重开（曾经的"待办"，现已关闭）
- [x] **1.14 `opengl` 的 wglSwapBuffers 直接入口补覆盖**（§72.27，第 78 批）：目标程序本来就有 `--gl-swap=wgl`，但一直没用例；新增"直接入口读新帧"与"直接入口 + 第二个 GL 窗口"两条，后者顺带证明按窗口过滤在直接入口上同样生效。

写在这里是为了防止下一轮又把它们当新发现翻出来 —— 每一条都有用例或源码标记做证据。

| 曾经的待办 | 现状 | 证据 |
|---|---|---|
| "约 175 个 COM 方法缺 `try/catch`" | **已全补**；第 66 批又把 `OP_COM_FORWARD` 宏生成的 29 个纯转发方法一起兜住（见 1.4） | §67、§72.8；`OpAutomation.cpp` 里 183 个 `try {` + 宏内 1 个 |
| "`gTemplateStore` 模板缓存无上限" | **不存在这个全局**。模板/图片缓存是 `g_pic_cache`，已有 32 条 FIFO 上限 + 显式 `LoadPic` 钉住 | `libop/image/ImageSearchService.cpp:26-34`、§22 |
| "`enable_wgc` 是死开关" | **误判**。`CMakeLists.txt:152` 定义，`libop/CMakeLists.txt:362` 与 `tests/CMakeLists.txt:187` 真用它 | §66；`-Denable_wgc=OFF` 下 `OP_ENABLE_WGC` 命中 0 次 |
| "`dx.d3d12` 没有 command queue 来源" | **已解决**，注入方自建 DIRECT 队列 | §70；`scripts/run_d3d12_probe.ps1` |
| "后台截图每帧 `CreateTexture2D` 很贵" | **不是瓶颈**，缓存纹理收益为零（A/B 三轮在噪声内） | §71.4 |
| "`normal.auto` 回退链没测" | 已用 `-Denable_wgc=OFF` 构建验证，另有 `NormalAutoBindsPlainWindowAndCaptures` 用例 | §66 |
| "x86 后台截图没验过" | **已进常规回归**：x86 全量 `ctest` 15/15、123.69s，含 10 条注入用例 | §72.8；第 66 批 x86 `ctest -C Release` |
| "d3d12 探针编不过" | **已修**：链接清单补 `SharedFrameChannel.cpp`/`DxCaptureCommon.cpp`，`CopyImageData` 空桩删除 | §72.9；`scripts/run_d3d12_probe.ps1` 7 条断言全 `[ OK ]` |

---

## 3. 需要你拍板的（我不擅自做）

这几条都会改变**外部可观察行为或线格式**，和"不改公开 API / 不改外部行为"的约束冲突。
3.1 已评估完并做成可选开关（默认行为没变）、3.2 与 3.4 已落地、**3.3 已按拍板改回正名**；
这一章目前没有待拍板项。

### 3.1 ✅ 已评估：staging 纹理 ping-pong（做成可选开关，默认不变）

**原假设：** 1080p 每帧 1.08 ms 里 `Map`（等 GPU 就绪）占 0.407 ms = 37.8%，
两片 staging 交替应该能省掉它、再快 40%。代价是共享段滞后一帧。

**实测（第 67 批，`defect_fixes.md` §72.17）：**

| 场景 | sync（原逻辑） | ping-pong |
|---|---|---|
| 1080p，`Present(0, 0)` 无 vsync，600 帧 | **1.0540 ms** | 1.0593 ms（持平） |
| 640×360，`Present(1, 0)` 60Hz，240 帧 | 980.0 µs | **757.3 µs（快 23%）** |

**结论：** 收益完全取决于目标进程是否 vsync —— 有 vsync 时快 23%，没有时持平
（省下的等待被"CPU 整帧搬运和 GPU 拷贝抢带宽"抵掉了）。而代价是 `Capture` 拿到上一帧。

所以**默认仍是 sync**（外部行为一个字没变），需要的人设 `OP_HOOK_PINGPONG=1` 开启；
`OP_HOOK_TIMING=1` 可把钩子自身耗时打到 `%TEMP%\op_hook_timing.log` 自己复测。

两种模式都有 e2e 覆盖（`HookCaptureGfxE2ETest.*` 8 条在 `OP_HOOK_PINGPONG=1` 下也全过）。

**代码位置：** `libop/hook/D3D11Capture.cpp` 的 `Dx11StagingCache` / `capture_once()` / `dx11_capture()`。

### 3.2 ✅ 已落地：只搬请求区域（第 68 批，`defect_fixes.md` §72.18）

**做法（没有改 28 字节帧头）：** 协商区放到像素区**之后** ——
`[FrameInfo 28B][像素区 w*h*4][FrameExchange 72B]`，偏移只跟整窗尺寸有关。
旧宿主按旧尺寸分配时 magic 读不到，注入侧自动退回整窗；旧注入从不写响应，宿主也自动退回整窗。
所以三种混搭（旧+新、新+旧、新+新）都成立，回滚不需要两端同时进行。

**效果（探针 1920×1080，40 次）：** 整窗 1.0279 ms → 区域 480×270（字节 6.2%）**0.1022 ms**，
**10.1×**。GPU 侧 d3d10/d3d11 用 `CopySubresourceRegion` 只拷那块；d3d9/d3d12 是异步/无源矩形
回读，GPU 侧仍整帧、CPU 侧只搬请求行；opengl 直接 `glReadPixels` 只读那块。

**覆盖：** 新增 5 条 e2e（d3d9/d3d10/opengl 的"区域 = 整窗"逐像素一致 + d3d9/opengl 的
"区域外像素不许变"），`hook_e2e` 变 10 条（x64）/ 11 条（x86）。

**踩到的两个坑（第 69 批修掉，`defect_fixes.md` §72.19）：**

1. 注入侧原来只在"本次读到合法请求"时才回写响应。请求 TTL（250 ms）一过期，注入侧退回整窗拷贝，
   响应却停在上一次的"区域"描述 —— 宿主按区域解释整窗帧，读到的第一行就是窗口顶部的白带。
   现在响应**无条件**写，语义固定为"描述缓冲区里这一帧的实际布局"。
2. `GetScreenDataBmp` 返回的指针指向 Op 内部**复用**的缓冲区，下一次调用就会覆盖它。写 e2e 时
   要先把整窗结果拷走再发第二次调用，否则比对的是被覆盖后的数据。

### 3.3 ✅ 已落地：共享段/互斥体名字改回正名

原来是反的：`MakeOpSharedResourceName()` 生成 `op_mutex_<hwnd>`、`MakeOpMutexName()` 生成
`op_shared_mem_<hwnd>`。第 72 批按操作者拍板**改回正名**（`doc/defect_fixes.md` §72.20.1）：

- 段叫 `op_shared_mem_<hwnd>`、互斥体叫 `op_mutex_<hwnd>`（`libop/base/AutomationModes.h:83-89`）；
- 护栏用例 `DisplayInputTest.SharedResourceNamesAreLockedWireFormat` 改成钉新格式，
  并继续断言"段与互斥体不重名、不同窗口不重名、x64 上 HWND 不被截断"。

**发版注意（代价没变）**：这两个名字是主控与注入 DLL 之间的线格式，改名后**旧版注入 DLL 会失联**
（按老名字找对象，两侧永远等不到对方的帧 → 后台截图超时，而不是报错）。
所以主控与注入必须同一次发版；混装时 `HookCapture::waitForBindReady()` 超时会打
`waitForBindReady timeout ... mismatched injection DLL ...` 提示这一点。

### 3.4 ✅ 已落地：64 位注入助手 `op_inject64.exe`

原来这里是"要不要做"的选择题，第 68 批按方案 1 做完了（`defect_fixes.md` §72.16）：

- 新增 `tools/inject64/main.cpp` → `op_inject64.exe`（64 位；`OpenProcess` + 远程 stub
  调 `LoadLibraryW` / `SetDisplayHook` / `ReleaseDisplayHook`，退出码带回目标侧结论）；
- `HookCapture::BindEx` / `UnBindEx` 的跨位宽分支改成调助手，找不到助手才退化成 `-2`；
- x86 侧多一条真读帧的用例，`stage_x64_artifacts.cmake` 顺带 stage 助手。

分发注意：`op_inject64.exe` 要和 `op_x86.dll` 一起进 `bin/x86/`（或放在 `bin/x86/..\x64\`），
`op_x64.dll` 放 `bin/x64/`；助手是 64 位，32 位宿主 `CreateProcess` 它没有任何位宽障碍。

---

## 4. 需要外部条件才能推进的

### 4.4 ✅ `MouseKeyTest.DxMode*`：已拆成逐进程 ctest 项（§72.59）

历史上 `DxModeDeliversAdvancedMouseButtonsAndWheel` 在全量套件里偶发失败过两次
（其中一次 `wheel_delta_sum` 期望 240、实际 120），单跑必过、重跑全量也过。
根因不是断言太严，是**同进程内前面跑过一堆 dx 用例之后抢输入队列**。

已按文档当初给的方向落地：10 条 DX 用例全部移出主入口
（`--gtest_filter=-MouseKeyTest.DxMode*`），逐条注册成 `mouse_dx_<用例名>`，
一条一个进程。改完 10 条全过（0.88~1.58s），`mouse_dx_|^op_test$` 共 11/11 通过。
详见 `doc/defect_fixes.md` §72.59。

### 4.5 ✅ `capture_backend`：已拆成逐用例 ctest 项（§72.59）

那次红的失败文本没留下来，整组一项时连"是哪条用例"都看不出来。
已把 8 条 `CaptureBackendTest` 从"整组一项"改成 `capture_backend_<用例名>` 逐条一项，
红了直接看名字；同时避免 8 条在同进程里共用桌面截图与绑定状态。8/8 通过（3.44s）。

### 4.6 ⚠️ 两个架构的全量 `ctest` 不能并行跑（会假失败）

第 93 批（DXGI 耗时修正）跑回归时，把 x64 与 x86 的全量 `ctest` 同时起在同一台机器上：

- 两边 `hook_e2e` 都 Failed；
- 单独串行重跑 x64 `hook_e2e`：**Passed 138.99s**；
- 串行重跑两个架构全量：x86 **15/15 通过 205.52s**、x64 **15/15 通过**。

两条原因都在流程上，与代码无关：

1. **x86 构建的 POST_BUILD `stage_x64` 会读 x64 目录的产物**（`op_x64.dll`、`gfx_host_app.exe`、
   `egl_host_app.exe`、`op_inject64.exe`），而 x64 的 `hook_e2e` 正在用这些文件；
2. 一个架构全量刚结束就立刻起另一个，残留进程/系统状态会让 `ocr_contract` 偶发红（实测红过一次，
   加 8 秒间隔后同一条命令 **15/15 通过**）。

**规则：串行跑，中间隔 5~10 秒。**

```bash
cd build/vs2022-x64-Release && ctest -C Release --output-on-failure
sleep 8
cd build/vs2022-x86-Release && ctest -C Release --output-on-failure
```

### 4.7 🟡 `dx.d3d9` 注入偶发失败：已加重试兜底，仍有未解释的窗口

2026-10-24 00:27:55 的 x86 全量里 `HookCaptureGfxE2ETest.D3D9RegionCaptureMatchesFullFrame`
红过一次（257 ms 就返回，`BindWindowEx ... display=dx.d3d9 ret=0`）。排查与处置见 §72.61：

- 单跑 8 次、D3D9 组 6 轮（24 次 bind）、两架构全量各一轮 —— **全部没再复现**，
  低概率偶发，不是稳定失败；
- 失败那一刻 `%TEMP%\__op.log` 里**目标进程一行日志都没有**，而 `SetDisplayHook` 的每条
  失败/拒绝分支都会写日志 —— 说明代码很可能根本没在目标进程跑起来
  （BlackBone `MakeRemoteFunction` 在目标刚起来时的老毛病，§72.13 有同类现象）。

**已做（三条，都在 `libop/capture/backends/HookCapture.cpp`）**：

1. `setlog_persistent()`：绑定失败的诊断临时把 `m_showErrorMsg` 切到 2 写 `__op.log`。
   原来测试侧 `SetShowErrorMsg(3)` 只走 stdout，ctest 控制台日志会被下一次成功跑覆盖，
   偶发问题永远留不下证据。
2. `cret.success() && bind_ret != 1` 时 `release_remote_display_hook` + **重试一次**。
   源码级依据：`build/_deps/BlackBone/src/BlackBone/Process/RPC/RemoteFunction.hpp:66-123`
   的 `Call()` 只要 `ExecInNewThread` 返回 `STATUS_SUCCESS` 就把结果标成成功，
   而 `RemoteExec.cpp:93-101` 的 `Join()` 是 `INFINITE`、远程线程异常退出也算
   `WAIT_OBJECT_0`，读回的 `callResult` 就是 0 —— 于是"没执行"和"返回 0"在调用侧
   不可区分。重试一次实测代价 59 ms。
3. `release_remote_display_hook()` 自己的失败也不再静默（`MakeRemoteFunction` 失败、
   `cret.success() == false` 都落盘）。**不用返回值判成败** —— 目标侧本来没绑时
   `ReleaseDisplayHook` 返回 0 是正常路径，按 `ret != 1` 判会把正常情况误报成失败。

**验证**：临时加环境变量强制第一次返回 0 → 日志出现 `retrying once` + `recovered on retry`，
用例照常通过；临时开关已删（两架构 dll 里搜不到 `OP_HOOK_FORCE_RETRY`）。
最终回归 x64 28 项 / 146 s、x86 29 项 / 149 s 全绿，`__op.log` 里没有新增 `BindEx` 行
（正常路径不被诊断日志污染）。

**仍未做（第 101 批复核后判定不做）**：没在"注入后确认目标主线程就绪"这一层动手。
重试兜住的是"偶发没执行"；目标侧真因别的原因拒绝时，仍会多花一次远程调用再失败。

不做的理由（完整版在 §72.65）：

1. `libop/hook/SharedFrameExchange.h:73` 有
   `static_assert(sizeof(FrameExchange) == 72, "FrameExchange 是跨进程协商区，布局不许变")`，
   加字段要连带改断言、`kFrameExchangeVersion`、"新宿主 + 旧注入侧"的降级判定 ——
   为一个纯诊断字段破这条线不划算。
2. 本想复用现成的 `ReadFrameResponse(...).valid` 当就绪信号，但注入侧传的是**后端表面尺寸**
   （`libop/hook/D3D9Capture.cpp:117` 的 `surface_Desc.Width`、`D3D10Capture.cpp:169` 的 `textDesc.Width`），
   宿主侧传的是**整窗尺寸**（`HookCapture.cpp:645` 的 `src_width`）。两者不等时
   `FrameExchangeOffset` 算出的偏移不同，宿主读不到注入侧写的那个位置 —— 信号不可靠。
3. 换成不动 ABI 的方案：`waitForBindReady` 超时时把帧头的 `chk` / `expect` / `frame_hwnd` /
   `width`×`height` 原样打出来（§72.65），三种原因能直接分开，够用了。

### 4.8 🟡 x86 `mouse_dx_DxModeDeliversAdvancedMouseButtonsAndWheel` 偶发红

第 100 批跑 x86 全量时红过一次：单跑立刻绿、整轮重跑 34/34 绿、同批 x64 全量两次都绿。
`__op.log` 里**零条输入侧日志**（`input hook inject failed` / `attach failed` /
`remote function 'SetInputHook' not found` 都没触发），所以不是 attach 或注入失败。

- 排查盲区：`libop/hook/InputHookClient.cpp` 的远程调用**失败时既不检查 `status` 也不记日志**，
  只有 `MakeRemoteFunction` 返回空才写一行。远程调用本身失败时，调用侧只能看到"返回 0"。
- 怀疑方向：dx 输入注入后目标侧 DirectInput 状态刷新与用例发送的时序竞争
  （1.7 s 内发完 4 个按钮 + 滚轮，目标侧按帧采样）。

**已做（第 101 批）**

1. `setlog_persistent` 从 `libop/capture/backends/HookCapture.cpp` 的匿名命名空间抽到
   `libop/base/Utils.h`，两个后端共用一份。顺手补了 `#include "Environment.h"` ——
   模板要读 `RuntimeEnvironment::m_showErrorMsg`，靠 `Types.h` 的传递包含太脆。
2. `InputHookClient.cpp` 四处远程调用补 `call_ret.success()` 检查 + `setlog_persistent`：
   `SetInputHook` / `ReleaseInputHook` / `SetInputLock` / 光标形状那四个。
   光标形状是四段拼一个 64 位值，**任一段失败就直接判 false**，不再把 0 当形状拼进去。
3. **强制失败验证过**：临时把 `call_ret.status` 改成 `0xC0000001` 重编，跑
   `MouseKeyTest.DxModeDeliversAdvancedMouseButtonsAndWheel`，`__op.log` 里出现
   `input hook SetInputHook remote call failed status=0xC0000001 pid=23724 hwnd=0000000000180D58`，
   用例本身仍 PASSED（`result(0)` 拿到的还是真实值）。验证完已删除该行，`grep TEMP-VERIFY` 干净。

**仍未做（第 101 批复核后判定不做）**：真正的时序竞争没定位。

查过 `libop/hook/HookExport.cpp:87-102` 的 `SetInputHook`：它返回的 `ret` 就是
`InputHook::setup(hwnd_)` 的返回值，而 `setup` **是在目标进程里执行的** —— 返回 1 已经等于
"目标侧钩子装好了"。所以再给目标侧加一个"就绪序号"是重复确认，加完仍然分不清
"钩子装了，但目标的消息循环还没开始泵消息"。

这条路要查的是目标进程输入线程的消息泵时序，宿主控制不了。偶发率极低
（x86 一轮全量里红过一次，之后多轮全绿），现有诊断（`setlog_persistent` 记录远程调用失败）
已经能排除"注入失败"这一大类，剩下的留作已知偶发。

### 4.9 ✅ 本轮实测新发现的两项偶发红（两条都已修）

第 101 批跑了两轮 x64 全量，**每轮各红一项、且不是同一项**，单跑都绿：

| ctest 项 | 红在哪条 | 单跑结果 | 现状 |
| --- | --- | --- | --- |
| `hook_e2e` | `HookCaptureGfxE2ETest.OpenGLESBackgroundCaptureReadsFreshFrames` | 绿（144.66 s） | **已修**，见 §72.63 |
| `op_test` | `ClipboardFacadeTest.SetAndGetRoundTripRestoresOriginal` | 连跑 3 次全绿 | **已修**，见 §72.64 |

- OpenGLES 那条的现场是两次 `waitForBindReady timeout 200ms: injection side wrote no frame`，
  然后 `tests/hook_capture_e2e_test.cpp:418` 的 `GfxHostColors().count(color) == 1` 断言失败。
  `__op.log` 里没有任何 `BindEx:` 失败日志，说明 `SetDisplayHook` 是成功的 —— 是**首帧慢**，
  200 ms 的固定等待不够，紧接着的第一次 `requestCapture` 又**一次都不等**直接返回 false。
  改法（§72.63）：抽出 `waitForFrameReady`，`requestCapture` 的第一次尝试补等一轮。
  改完 `hook_e2e` 单跑 0 条超时日志。
- 剪贴板那条根因不在用例，在 `ClipboardGuard` 裸调一次 `OpenClipboard` —— 剪贴板是全局独占
  资源，别的进程持有时必然失败，而典型占用只有几毫秒。改法（§72.64）是库侧加 4 次 ×10 ms 重试，
  不是让用例重试盖住问题。用 pwsh 脚本持续占用 25 ms×120 次构造真实竞争，期间用例 5 次全绿。
- 改完双架构全量各 34/34 绿（x86 249.50 s / x64 233.66 s）。


### 4.1 ✅ OCR 25 条用例真跑 → 已装 Tesseract

**现状（第 101 批）**：本机已装好，25 条实测全绿。ctest 的 `LastTest.log` 里
`[       OK ] OcrFixture.` 25 条、`SKIPPED` 0 条 —— 不是计入通过的假覆盖。

| 组件 | 版本 / 位置 |
| --- | --- |
| Tesseract | 5.4.0.20240606，`C:\Program Files\Tesseract-OCR\tesseract.exe`（winget `UB-Mannheim.TesseractOCR`） |
| 语言包 | `eng` + `osd`（没装中文；用例只识别英文数字，够用） |
| pytesseract | 0.3.13 |
| pillow | 13.0.0 |

**怎么跑**（服务不在时用例仍 `GTEST_SKIP()` —— 那是 `OcrTest::SetUp` 的 `/health` 探测，既有设计）：

```bash
export PATH="/c/Program Files/Tesseract-OCR:$PATH"
python scripts/op_ocr_server_min.py --backend tesseract --lang eng --port 8080
# 另开一个终端
cd build/vs2022-x64-Release
export PATH="$PWD/libop/Release:$PWD/tests/Release:$PWD/tools/Release:$PWD:$PATH"
./tests/Release/op_test.exe --gtest_filter="OcrFixture.*"
```

两个坑：

- suite 名是 `OcrFixture`（`TEST_F(OcrFixture, ...)` 的宏第一个参数），不是 `OcrTest`。
  `using OcrFixture = test_support::OcrTest;` 只是类型别名，filter 写 `OcrTest.*` 会 0 条匹配。
- `op_test` 这个 ctest 项只设了 `OP_SKIP_OCR_CONTRACT_TESTS=1`，**没有** `OP_SKIP_OCR_TESTS`，
  所以服务在跑时这 25 条会被 ctest 真跑（+1.7 s 左右）。

没有 Tesseract 也能联调协议：`python scripts/op_ocr_server_min.py --backend echo`。

**没做的**：没把服务做成开机自启或 ctest fixture。OCR 是外部依赖，缺席时跳过是既有设计，
强行常驻会让没装 Tesseract 的机器变红。第 29 批的 `SkipSummaryListener` 会逐条打印被跳过的用例，
报表里看到 skip 就知道服务没起。

### 4.2 Win10 上 WGC 单进程第二次会话必失败 → 平台限制

实测：`WgcTest` 在**同一个进程**里跑，第一条过，第二条起全部
`wgc init step=0 activation_factory` + `activation factory unknown exception`；
每条**单独起进程** 11/11 全绿。

这是 Win10（19045）对 `GraphicsCaptureItem` 的进程级限制，不是本项目的 bug。
所以 `tests/CMakeLists.txt` 把 11 条 WGC 用例各注册成一个 ctest 项（`wgc_<名字>`），
主入口用 `OP_SKIP_WGC_TESTS=1` 跳过。**不要为了"合并测试省时间"把它们合回一个进程** ——
最多省 0.6s，换来 11 条假红。

### 4.3 ✅ 已缓解：`dx.d3d12` 用自己的模拟目标替代"需要真游戏"

原来这里记的是"只有真游戏能覆盖多交换链/多队列/跨线程提交"。第 72 批把
`tests/gfx_host_app.cpp` 的 d3d12 目标从"最简三条命令"改成**游戏形状**
（`doc/defect_fixes.md` §72.20.2）：

- 3 个后备缓冲（`kD3D12BufferCount = 3`）+ 2 套（分配器 + 命令列表）每帧轮转，
  不再每帧"提交完立刻等栅栏"；
- 标记带用白色纹理 `CopyTextureRegion` 画，拷贝前后完整走
  `RENDER_TARGET → COPY_DEST → PRESENT` 的状态转换；
- `HookCaptureGfxE2ETest.D3D12*` 三条用例（新帧 / 重绑 / 区域一致性）跑在这个目标上，
  x64 79.01s、x86 84.88s 全过。

第 73 批再进一步（§72.21）：目标加了 `--d3d12-extra-swap` —— 第二个窗口 + 第二条交换链，
**录制/提交/等栅栏/Present 全在独立线程**里做。这条用例第一次跑就抓到注入侧的真缺陷：
`D3D12Capture` 单例的抓取状态没有互斥，两条链互相 Reset 对方的在途命令列表，
栅栏永远等不到，共享段冻在第一帧。加递归锁后转绿（`D3D12TwoSwapChainsKeepsReadingFrames`）。

第 74 批又补上了 d3d11（§72.22）：目标加 `--extra-swap`（第二个窗口 + 独立线程的第二条链），
第一次跑就抓到更本质的缺陷 —— 多条链写同一个按 hwnd 命名的共享段，"像素区"和"响应区"可能来自
不同链，宿主按整窗去解释一块区域，颜色冻在第一帧。修法是按 `OutputWindow` 过滤交换链。

第 75 批补上了 opengl（§72.23）：`--gl-extra-swap` 起第二个 GL 窗口 + 独立线程 `SwapBuffers`，
未按窗口过滤时 3 跑 2 红；改成 `WindowFromDC(hdc)` 比对 `render_hwnd` 后全绿。

仍然没覆盖的：游戏自己也在 `Present` 里做 GPU 等待的极端时序；EGL 路径（`eglSwapBuffers`
没有 hdc）还没有目标程序可验（§72.23 残留）。

注意：探针的**退出码不可信**（调试层在进程退出时会把退出码改成 2173/0x87D），
判定一律看 stdout 的 `PASS:` / `FAIL:` 行。脚本已经这么做了。

---


- **`opengl.es` 的多窗口过滤**：ANGLE 的 `eglQuerySurface(EGL_NATIVE_WINDOW)` 返回
  `EGL_BAD_ATTRIBUTE(0x3004)`，拿不到 surface→窗口映射；hook `eglCreateWindowSurface`
  记表也解决不了（注入时目标早已建好 surface）。证据见 §72.26。这类目标改用
  `normal` / `opengl` 后端。
- **GDI 后端（`normal`）的桌面捕获地板是 16.7 ms/次（60 Hz）**，不是缺陷：`BitBlt` 桌面 DC → DIB
  被 DWM 合成节奏同步，320×240 与 1920×1080 耗时几乎相同（15.3 vs 19.1 ms）。**但 `normal.dxgi`
  只有 1.5 ms/次**（首次绑定 168 ms；同进程解绑后重绑已降到 11~23 ms，§72.45），比 GDI 快 10 倍——桌面找色/找图优先 `normal.dxgi`，
  要更快再上注入后端（~1.08 ms/帧）。早前记的“dxgi 19.5 ms”是把绑定开销摊进捕获的算错结果，
  已在 §72.44 修正并留了修正记录。

## 5. 明确不做的（写下来免得反复讨论）

| 项 | 为什么不做 |
|---|---|
| `normal.wgc` 在 Win10 失败后自动降级到 DXGI | **操作者明确禁止**。不得实现、不得启用、不得建议。DXGI 降级范围与触发条件也不动 |
| 把 11 条 WGC 用例合并成一个进程 | Win10 平台限制，合并 = 11 条假红（见 4.2） |
| 单方面改共享段/互斥体名字 | 跨版本线格式。第 72 批已按操作者拍板改回正名（见 3.3），此后**不得再单方面改**：主控与注入必须同批发版 |
| 去掉 `OP_SKIP_WGC_TESTS` 之类的跳过开关 | 那些开关就是为了让环境限制不污染报表；真覆盖靠独立 ctest 项，不是靠打开开关 |

---

## 6. 改完之后怎么验

```powershell
# 1) 构建（两种架构都过一遍，确认零警告）
python build.py
python build.py -a x86

# 2) 全量回归（约 124s，15 项；含 10 条跨进程注入用例）
cd build/vs2022-x64-Release
ctest -C Release --output-on-failure

# 3) x86 全量回归（第 66 批起进发布前检查，约 124s）
cd build/vs2022-x86-Release
ctest -C Release --output-on-failure

# 4) 只关心后台截图时（两个架构都跑）
ctest -C Release -R "hook_e2e|capture_backend"

# 5) 注入侧性能（改 D3D*Capture.cpp 或 DxCaptureCommon.cpp 之后必跑）
cd E:\AIWork\github\op-master
powershell -ExecutionPolicy Bypass -File scripts\run_hook_capture_probe.ps1 -Phase
powershell -ExecutionPolicy Bypass -File scripts\run_hook_capture_probe.ps1 -CopyBench

# 5b) d3d12 抓帧（改 D3D12Capture.* 之后必跑；两个探针都编真实现，别再加空桩）
powershell -ExecutionPolicy Bypass -File scripts\run_d3d12_probe.ps1

# 6) 环境判定（换机器后先跑这个）
powershell -ExecutionPolicy Bypass -File scripts\check_env.ps1
```

**看报表时的三个坑：**

1. **gtest 把 `GTEST_SKIP()` 算作通过。** 想知道真跑了多少条，看 `SkipSummaryListener`
   打印的 `[ ENV-SKIP ]` 清单（第 29 批加的），别只看 "100% passed"。
   跨进程注入那几条另有严格门禁兜底：`ctest -R hook_require_e2e` 在环境缺失时**必须红**
   （`OP_REQUIRE_HOOK_E2E=1`，见 §72.53）。它绿了才说明那台机器真验过注入链路。
2. **探针/自建工具的退出码不可信。** 一律看 stdout 的 `PASS:` / `FAIL:` 行（D3D12 调试层会改退出码）。
3. **`Testing/Temporary/LastTestsFailed.log` 是陈旧的。** 它只留"上一次有失败的那一轮"，
   之后全绿也不会自动清空。拿它当"当前哪些用例红"会把人带偏 —— 以当次 `ctest` 输出为准。
   同理，`ctest --rerun-failed` 在它过期时会重跑一批其实已经通过的用例。

---

## 7. 回滚

`_backup/` 目录已在一次工作区清理中删除（363 个历史快照），此前这里列出的
`fix_<时间戳>_<主题>/` 快照全部不存在了。

要重新获得回滚能力，先 `git init` 并把当前状态提交成基线，之后的改动就有据可查。
`doc/defect_fixes.md` 里每条记录的"改法"仍然有效 —— 它们写的是改了哪几行，
可以照着反向改回去，只是没有现成文件可拷。
