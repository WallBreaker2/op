# Open Issues Scan

这份说明用于归纳当前公开 GitHub issue 的主要问题簇，并给出和代码实现相对应的 triage 结论，方便后续维护与分诊。

## 高优先级结论

### 1. `MoveToEx` 返回值不一致属于文档漂移

- 对应 issue：`#181`
- 当前实现返回的是数值状态，不是字符串。
- 代码依据：
  - `libop/libop.h`：`void MoveToEx(long x, long y, long w, long h, long *ret);`
  - `libop/com/op.idl`：`HRESULT MoveToEx(..., [out, retval] LONG* ret);`
  - `libop/libop.cpp`：将鼠标后端返回值写入 `LONG`。

结论：实现本身一致，若外部 wiki 写成字符串返回值，应视为旧文档。

### 2. `OcrFromFile` 之前确实忽略了 `color_format`

- 关联问题：`#145` 相关 OCR 参数困惑
- 在本次扫描前，`ImageProc::OcrFromFile()` 虽然接收了颜色参数，但内部固定调用 `OCR(L"", ...)`，导致颜色过滤被静默忽略。
- 现已修正为调用 `OCR(color, ...)`。

结论：这不是 `#145` 中完全相同的报错路径，但它确实是一个真实的 OCR 接口契约问题，已经值得修复。

### 3. 跨语言崩溃问题仍需要最小复现

- 对应 issue：`#149`、`#138`
- 表现：`C#` 中调用 `CmpColor` 或 `FindPic` 时出现 `AccessViolationException` / 受保护内存访问错误。
- 本次扫描没有在这两个方法里发现单行可定位的明显内存破坏点。
- 更可能的风险点：
  - 宿主进程与 COM DLL 的 x86/x64 位数不匹配
  - 未绑定窗口、绑定状态异常、截图源无效
  - 宿主应用自身的 COM 生命周期或线程模型问题

结论：这类问题目前更适合标记为 `needs-repro`，先收集最小 C# 复现样例。

### 4. 强制终止线程不是受支持的生命周期

- 对应 issue：`#140`
- 报告中使用了 `QThread.terminate()` 强制终止并重启线程。
- issue 作者自己也确认了改用 `deleteLater()` 后可以规避问题。

结论：这类问题应先作为线程生命周期约束写入文档，再决定是否继续追查更深层的线程安全问题。

## 后台输入问题簇

当前最集中的 issue 是后台输入/游戏兼容性：`#176`、`#136`、`#124`、`#114`、`#103`。

### 当前代码明确支持的能力

- 鼠标后端：
  - `normal`
  - `windows`
  - `dx`
- 键盘后端：
  - `normal`
  - `normal.hd`
  - `windows`
- 当前没有公开提供 `keypad=dx`。

### 为什么仍然会有大量相关 issue

- `windows` 键盘路径依赖 `WM_KEYDOWN` / `WM_KEYUP`，不少游戏并不把它当作真实设备输入。
- `dx` 鼠标路径依赖注入 Hook 与自定义消息，在目标进程重启、渲染链路变化、输入过滤更严格时仍可能失效。
- 组合键和滚轮在后台模式下尤其脆弱，因为很多程序读取的是实时键盘状态、低层 Hook，或更贴近真实设备的输入链路，而不是单纯窗口消息。

结论：这类 issue 中有相当一部分更像“能力边界”或“目标程序兼容性限制”，不一定是简单 bug。

## 文档缺口

当前 README 已经覆盖了 OCR 服务模式与 `SetDisplayInput`，但从 issue 看仍有几个明显缺口：

- Python 用户依然不清楚如何用内存图像走 `SetDisplayInput`
- C# 用户频繁遇到位数/宿主集成问题
- 后台键鼠限制虽然有写，但对游戏场景还不够直白
- 外部 wiki 可能比仓库源码老，容易出现签名漂移

## 建议使用的 triage 标签

- `bug`：已确认的实现缺陷
- `docs`：文档错误或缺失
- `feature`：能力诉求
- `question`：使用问题，无需改代码
- `needs-repro`：缺少稳定复现
- `known-limitation`：受 Windows / 游戏 / 输入模型限制

## 建议的后续动作

1. 为 `#149`、`#138` 收集最小复现：宿主位数、DLL 位数、是否免注册、是否先 `BindWindow`、失败是否可稳定重现。
2. 审核外部 wiki 中容易漂移的接口，优先检查鼠标类方法，例如 `MoveToEx`。
3. 继续在 README 中补充 Python / C# 的最小可运行示例，尤其是 OCR、内存图像输入、免注册或位数相关说明。
