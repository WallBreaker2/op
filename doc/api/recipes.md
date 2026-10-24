# 常见任务配方

按「要做一件事」而不是「要查一个函数」组织的示例。每个配方都是完整可编译的一段代码，
末尾列出这类任务最容易踩的坑。

签名级细节（每个参数的含义、`ret` 的取值）见各组文档；本页只讲怎么串起来用。
返回索引：[doc/api.md](../api.md)。

## 通用前提

```cpp
#include <windows.h>

#include "libop.h"

// 链接 op_x64.lib（64 位）或 op_x86.lib（32 位），两者必须与加载的 DLL 位数一致。
```

所有配方的坐标都是**绑定目标的客户区坐标**：绑桌面（`hwnd = 0`）时等于屏幕坐标，
绑某个窗口时 (0,0) 就是那个窗口客户区的左上角。这一点对 `Capture`、`FindPic`、
`MoveTo` 全都成立，所以「找图返回的坐标」可以直接喂给「移动鼠标」。

`SetShowErrorMsg(0, &ret)` 关掉弹框。默认模式下出错会弹出对话框，脚本跑批时会被卡住。

## 1. 截一张图存盘

```cpp
op::Op op;
long ret = 0;
op.SetShowErrorMsg(0, &ret);

// hwnd = 0 → 绑桌面。display / mouse / keypad 都用 normal（前台后台都取前台画面）。
op.BindWindow(0, L"normal", L"normal", L"normal", 0, &ret);
if (ret != 1) {
    // 绑定失败：多半是没有交互式桌面（服务里跑、Session 0）
    return 1;
}

// 注意：后两个参数是右下角坐标，不是宽高。要 640x480 就传 (0,0,639,479)。
op.Capture(0, 0, 639, 479, L"D:\\shot.bmp", &ret);
if (ret != 1) {
    return 1;  // 路径写错、磁盘满、或目标窗口被最小化
}

op.UnBindWindow(&ret);
```

**坑**

- `Capture(x1, y1, x2, y2)` 的第 3、4 个参数是**右下角坐标**，不是宽高。传宽高会静默截错区域。
- 目标窗口最小化时，`normal` 截到的是空白；要截后台窗口得用 `dx.d3d11` 之类的 display 模式，
  且目标窗口的渲染 API 要和模式对上（D3D11 程序用 `dx.d3d11`，OpenGL 程序用 `opengl`）。
- 绑桌面不需要目标有画面，但要求当前会话有可见桌面；在 `Session 0` 或锁屏状态下会失败。

相关：[`BindWindow`](02-window.md)、[`Capture`](04-image.md)、[`UnBindWindow`](02-window.md)

## 2. 找图后点击

```cpp
long client_w = 800, client_h = 600;  // 目标的客户区尺寸，用 GetClientRect 取
op.BindWindow(hwnd, L"normal", L"normal", L"normal", 0, &ret);
if (ret != 1) {
    return 1;
}

long x = -1, y = -1;
op.FindPic(0, 0, client_w - 1, client_h - 1,
           L"button.bmp", L"000000", 0.9, 0, &x, &y, &ret);
if (ret != 1) {
    op.UnBindWindow(&ret);
    return 2;  // 没找到
}

op.MoveTo(x + 5, y + 5, &ret);   // 往图中心挪几个像素，避免点在边缘像素上
op.LeftClick(&ret);

op.UnBindWindow(&ret);
```

**坑**

- `sim` 是相似度（0.1~1.0），`delta_color` 是颜色偏移容差（`"000000"` 表示不额外放宽）。
  模板图和实际画面有色差时调 `delta_color` 比调 `sim` 更有效。
- 同一张图在屏幕上有多个实例时，`FindPic` 只回第一个（受 `dir` 扫描方向影响）；
  要全部实例用 `FindPicEx`，返回 `"x|y"` 形式的多组结果，按 `|` 切开。
- 模板图不要用截图里的整块画面，只截目标控件本身；模板越大越容易被无关的遮挡打断。
- 后台绑定后 `MoveTo` / `LeftClick` 走的是消息注入，某些用 DirectInput 读输入的程序收不到，
  这时要改用 `windows` 或 `dx` 这两档输入模式（`BindWindow` 的第 3、4 个参数）。

相关：[`FindPic`](04-image.md)、[`FindPicEx`](04-image.md)、[`MoveTo`](03-input.md)、[`LeftClick`](03-input.md)

## 3. OCR 取文字

两条路：本地字库（快、只认固定字体）和 HTTP 服务（认得多、要起服务）。

### 3a. HTTP 服务（推荐，默认走 tesseract 端点）

```cpp
op.BindWindow(0, L"normal", L"normal", L"normal", 0, &ret);

std::wstring text;
op.OcrAuto(0, 0, 639, 479, 0.9, text);   // 自动灰度 + 自动二值 + 识别
if (!text.empty()) {
    // text 是识别到的整段文字
}
```

要每个字的位置用 `OcrEx`，它返回带坐标的结果串，再按索引取：

```cpp
std::wstring result;
op.OcrEx(0, 0, 639, 479, L"", 0.9, result);

long count = 0;
op.GetWordResultCount(result.c_str(), &count);
for (long i = 0; i < count; ++i) {
    long px = 0, py = 0;
    std::wstring word;
    op.GetWordResultPos(result.c_str(), i, &px, &py, &ret);
    op.GetWordResultStr(result.c_str(), i, word);
}
```

### 3b. 本地字库

```cpp
op.SetDict(0, L"D:\\dict.txt", &ret);   // idx 0~9，最多挂 10 个字典
op.UseDict(0, &ret);                     // 0 = 只查 0 号；更大的值可以叠加多个

std::wstring text;
op.Ocr(0, 0, 300, 60, L"", 0.9, text);
```

**坑**

- `OcrAuto` 需要 HTTP 服务在跑（默认 `http://127.0.0.1:8080/api/v1/ocr`）。
  服务没起时返回空串而不是报错，容易误判成「图上没字」。
- `Ocr` 的第 5 个参数是 `color`（只识别这个颜色的字），`""` 表示不限颜色；
  传错格式会一个都认不出来。
- 本地字库只认它学过的字体和字号，换字体要重新做字典；跨字体场景直接用 HTTP 服务。
- 大区域直接丢给 OCR 很慢。先把区域缩到文字所在的那一条（用 `FindMultiColor` 定位边框），
  再送 OCR，通常快一个数量级。

相关：[`OcrAuto`](06-ocr.md)、[`OcrEx`](06-ocr.md)、[`Ocr`](06-ocr.md)、
[`SetDict`](06-ocr.md)、[`GetWordResultPos`](06-ocr.md)

## 4. 跨进程读内存

```cpp
op.BindWindow(hwnd, L"normal", L"normal", L"normal", 0, &ret);

// 读一个 4 字节整数。地址必须带 0x 前缀（内部按 16 进制解析）。
int64_t hp = 0;
op.ReadInt(hwnd, L"0x00401000", 4, &hp);

// type：0/4 → 4 字节，1/5 → 2 字节，2/6 → 1 字节，3 → 8 字节。
```

批量读比逐个快很多（一次进程附加、一次地址解析缓存）：

```cpp
// 偶数下标是地址，奇数是 type
const wchar_t *batch = L"0x00401000|4|0x00401004|4|0x00401008|4";

std::wstring values;                                 // "100|200|300"
op.ReadIntBatch(hwnd, batch, values);

int64_t ok = 0;                                      // 成功条数
op.ReadIntBatchOk(hwnd, batch, ok);
```

读字符串：

```cpp
// type：0 = ACP/GBK，1 = UTF-16，2 = UTF-8；len 是**字节数**，不是字符数。
std::wstring name;
op.ReadString(hwnd, L"0x00402000", 1, 64, name);
```

**坑**

- 地址不带 `0x` 会被当成十进制解析，读到的位置完全不对。
- 目标进程的位数必须和插件一致：32 位 DLL 读不了 64 位进程。跨位数请在 64 位宿主里跑。
- 批量接口里某一条地址无效时，只有那一条返回空，其余照常返回；
  要用「第几条成功」判断有没有踩到坏地址，用 `ReadIntBatchOk` 拿条数。
- `hwnd` 传 0 时走本进程 `memcpy` 快路径，常用于读自己的配置结构验证偏移。

相关：[`ReadInt`](06-ocr.md)、[`ReadIntBatch`](06-ocr.md)、
[`ReadString`](06-ocr.md)、[`WriteInt`](06-ocr.md)

## 5. 多窗口批量绑定

```cpp
std::wstring windows;   // 返回 "hwnd1|hwnd2|hwnd3"
op.EnumWindow(0, L"", L"Notepad", 1, windows);

std::vector<LONG_PTR> handles;
size_t begin = 0;
while (begin < windows.size()) {
    const size_t bar = windows.find(L'|', begin);
    const size_t stop = (bar == std::wstring::npos) ? windows.size() : bar;
    handles.push_back(static_cast<LONG_PTR>(wcstoull(windows.substr(begin, stop - begin).c_str(),
                                                     nullptr, 10)));
    begin = (bar == std::wstring::npos) ? windows.size() : bar + 1;
}
```

一个实例绑一个窗口 —— **不要用一个 `Op` 来回绑**：

```cpp
std::vector<std::unique_ptr<op::Op>> ops;
for (LONG_PTR handle : handles) {
    auto instance = std::make_unique<op::Op>();
    long bind_ret = 0;
    instance->SetShowErrorMsg(0, &bind_ret);
    instance->BindWindow(handle, L"dx.d3d11", L"windows", L"windows", 0, &bind_ret);
    if (bind_ret == 1) {
        ops.push_back(std::move(instance));
    }
}

// 每帧对每个窗口各抓一次
for (auto &instance : ops) {
    long cap_ret = 0;
    instance->Capture(0, 0, 319, 179, L"D:\\frame.bmp", &cap_ret);
}

for (auto &instance : ops) {
    long unbind_ret = 0;
    instance->UnBindWindow(&unbind_ret);
}
```

**坑**

- 一个 `Op` 实例同一时刻只绑一个窗口。虽然换绑会正确释放上一个窗口的注入和共享段，
  但交替 `BindWindow(A)`/`BindWindow(B)` 每次都要重新注入（几十毫秒），批量场景应该一窗口一实例。
- `EnumWindow` 的第 4 个参数 `filter` 控制要不要可见/最小化窗口，按需调；
  `class_name` 传 `""` 表示不限类名，`title` 同理。
- 同一个进程有多个窗口时，`dx.d3d11` 之类的显示模式按窗口过滤，不会串到别的窗口上。
- 窗口被关掉后再 `Capture` 会一直失败。批量循环里要把 `ret != 1` 的实例剔掉，
  否则每帧白等一次超时。
- 句柄用完要 `UnBindWindow`；进程退出时 DLL 会自动清理，但长期跑的脚本不释放会累积注入引用。

相关：[`EnumWindow`](02-window.md)、[`BindWindow`](02-window.md)、
[`SetWindowState`](02-window.md)、[`UnBindWindow`](02-window.md)

## 6. 窗口位置与状态批量整理

```cpp
// 把所有记事本窗口排成 3 列
std::wstring windows;
op.EnumWindow(0, L"", L"Notepad", 1, windows);

// 逐个 SetWindowPos；OP 只暴露状态控制，位置调整用 Win32 的 SetWindowPos 即可
```

**坑**

- `SetWindowState(hwnd, flag, &ret)` 的 `flag` 是窗口状态位（0 = 正常、3 = 最小化等，
  完整定义见 [`SetWindowState`](02-window.md)），不是「第几种操作」。
- 目标窗口属于更高权限的进程时，`SetWindowState` 会失败但不报错，只能靠 `ret` 判断。
