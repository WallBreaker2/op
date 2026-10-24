# op::Op 函数调用文档

本文件由 `scripts/gen_api_doc.py` 从 `include/libop.h` 生成，签名与注释永远和代码同源。
改完头文件重跑一次脚本即可。

## 通用约定

### `long *ret` 的含义

绝大多数接口的最后一个参数是 `long *ret`，含义只有一个：**这次调用是否成功**。

| 值 | 含义 |
|---|---|
| `1` | 成功 |
| `0` | 失败（参数非法 / 目标不存在 / 超时 / 后端不支持） |

**它不承载业务数据。** 业务数据走各自的出参（`rect_t &`、`wstring &`、`bytearray &`、`point_t &`）。

```cpp
op::Op op;
long ret = 0;
rect_t rect;
op.FindPic(0, 0, 2000, 2000, L"target.bmp", L"101010", 0.9, 0, &rect, &ret);
if (ret != 1) {
    // 没找到；此时 rect 的内容不保证有意义
}
```

### 坐标与尺寸

- 所有坐标都是**绑定目标窗口的客户区坐标**（`BindWindow` 之后），不是屏幕坐标。
- 需要屏幕坐标时用 `ClientToScreen` / `ScreenToClient` 显式换算。
- `rect_t` 的字段是 `x` / `y` / `width` / `height`（`libop/runtime/Types.h`）。

### 错误信息怎么出来

`SetShowErrorMsg(mode)`：`0` 关闭、`1` 信息框、`2` 写文件（`__op.log`）、`3` 标准输出，默认 `1`。
服务端或无桌面程序里，第一件事就该设成 `0` 或 `2`——否则失败时弹框会把调用线程卡住。

### 线程安全

同一个 `op::Op` 实例**不是线程安全的**：OCR 与找图系列共用“当前捕获图” `_src`，并发调用会互相覆盖。
一个线程一个实例，或者自己加锁。

例外：HTTP session 池、截图设备的共享缓存是进程级加锁的，跨实例并发安全。

### 从 C++ 到 C API

`libop/c_api/op_c_api.cpp` 的导出是 C++ 接口的扁平化版本，命名规则是 `Op` + 方法名，
参数顺序不变，引用出参换成指针。

| C++ | C API |
|---|---|
| `op.Version(str)` | `OpVersion(wchar_t **str)` |
| `op.FindPic(x1,y1,x2,y2,name,delta,sim,dir,rect,ret)` | `OpFindPic(hwnd, x1,y1,x2,y2,name,delta,sim,dir,long *x,long *y,long *ret)` |

返回 `wchar_t *` 的接口，指针指向**句柄内部**的环形缓冲池（4 槽轮转）：

- 最近 4 次调用返回的指针**同时有效**，可以放心地拿第一个返回值当第二次调用的入参
  （`OpGetWordResultStr(h, OpFetchWords(...), 0)` 这种链式用法就是这样）。
- 第 5 次调用会回收最早那一槽，指向它的旧指针**立刻失效**，读到的是新内容。
- 要长期保留就自己拷出来（`Marshal.PtrToStringUni` / `std::wstring(...)`）。

例外：这些指针属于**单个句柄**，不同 `op_handle` 之间互不干扰，多线程各持一个句柄是安全的。

这一段约定踩过两次坑：§72.48 修的是入参自别名（`OpGetWordResultStr` 的第一个参数就是上一次
返回值，清理缓冲会把入参抹成空串），§72.57 修的是短字符串（≤7 个 wchar，如 `GetColor` 的
`RRGGBB`）落在 `std::wstring` 的 SSO 里、`swap` 换不掉 buffer 地址，于是「上一轮」那份缓冲
照样被改写 —— 换成槽位池之后才真正成立。

### 典型调用序列

```cpp
op::Op op;
long ret = 0;

op.SetShowErrorMsg(0, &ret);                       // 后台程序：别弹框
op.BindWindow(hwnd, L"normal", L"normal", L"normal", 0, &ret);  // 绑窗口

rect_t found;
op.FindPic(0, 0, 800, 600, L"button.bmp", L"101010", 0.85, 0, &found, &ret);
if (ret == 1) {
    op.MoveTo(found.x + found.width / 2, found.y + found.height / 2, &ret);
    op.LeftClick(&ret);
}

op.UnBindWindow(&ret);
```

## 接口索引

| 文档 | 覆盖的分组 | 函数数 |
|---|---|---|
| [运行时与算法](api/01-runtime.md) | 基本设置/属性、algorithm | 14 |
| [窗口与进程](api/02-window.md) | windows api、Background | 47 |
| [鼠标与键盘](api/03-input.md) | mouse & keyboard | 45 |
| [图像与颜色](api/04-image.md) | image and color | 22 |
| [OpenCV](api/05-opencv.md) | opcv | 35 |
| [OCR 与 YOLO](api/06-ocr.md) | ocr | 51 |
| **合计** | | **214** |

每个方法都带签名和一句说明；说明来自头文件注释，头文件没写注释的由 `scripts/gen_api_doc.py` 的 `EXTRA_DOC` 对着实现补。

## 按任务查

[常见任务配方](api/recipes.md)：截图存盘、找图后点击、OCR 取文字、跨进程读内存、多窗口批量绑定、窗口位置整理。
每个配方都是完整可编译的一段代码，外加这类任务最容易踩的坑。

这个文件是手写的，生成器不会覆盖它。

