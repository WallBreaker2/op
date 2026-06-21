# OP Python C API 包装

这是基于 `ctypes` 的 Python 包装层，用来调用 `op_c_api_x64.dll` 或
`op_c_api_x86.dll`。它和仓库里已有的 SWIG 模块相互独立。

Python 层已经覆盖 `include/op_c_api.h` 导出的所有函数。`OpCreate` 和
`OpDestroy` 由 `Op` 类自动管理，其余接口都以 Python 风格的 `snake_case`
方法暴露，例如 `bind_window`、`find_color`、`read_int`。

当前不提供 `BindWindow`、`ReadInt` 这类 COM 风格兼容别名。

## 本地开发安装

在仓库根目录执行：

```powershell
pip install -e .\bindings\python
```

## DLL 放置和查找

包装层按 **Python 解释器位数** 选择 DLL，不按系统位数选择。64 位 Python
加载 `op_c_api_x64.dll`，32 位 Python 加载 `op_c_api_x86.dll`。

推荐随 Python 包放置：

```text
bindings/python/op/bin/x64/op_c_api_x64.dll
bindings/python/op/bin/x86/op_c_api_x86.dll
```

上面的路径是相对仓库根目录。安装成 Python 包后，对应的是相对 `op`
包目录的 `op/bin/x64` 或 `op/bin/x86`。

如果 DLL 依赖其他运行时 DLL，请把依赖 DLL 放在同一个 `x64` 或 `x86` 目录下。
Python 3.8+ 会在加载前把该目录加入 DLL 搜索路径。

查找顺序：

1. `Op(dll_path=...)`
2. `Op(dll_dir=...)`
3. 包内目录 `op/bin/x64` 或 `op/bin/x86`
4. 仓库目录 `bin/x64` 或 `bin/x86`
5. 当前工作目录
6. 系统 `PATH`

显式指定 DLL 文件：

```python
from op import Op

with Op(dll_path=r".\bin\x64\op_c_api_x64.dll") as op:
    print(op.version)
```

显式指定 DLL 目录：

```python
from op import Op

with Op(dll_dir=r".\bin\x64") as op:
    print(op.dll_path)
```

## 快速开始

```python
from op import Op

with Op() as op:
    print("版本:", op.version)
    print("DLL:", op.dll_path)
```

`Op` 支持上下文管理器。离开 `with` 块时会自动释放底层 `op_handle`。

## 窗口查找和后台绑定

```python
from op import Op

with Op() as op:
    hwnd = op.find_window("", "窗口标题")
    if not hwnd:
        raise RuntimeError("没有找到目标窗口")

    title = op.get_window_title(hwnd)
    rect = op.get_window_rect(hwnd)
    print("窗口:", title, rect)

    op.bind_window(hwnd, display="normal", mouse="normal", keypad="normal", mode=0)
    try:
        print("已绑定窗口:", op.get_bind_window())
        op.move_to(100, 100)
        op.left_click()
    finally:
        op.unbind_window()
```

## 图色和截图

```python
from op import Op

with Op() as op:
    op.capture(0, 0, 800, 600, r".\screen.bmp")

    ret, x, y = op.find_color(
        0,
        0,
        800,
        600,
        "FFFFFF",
        0.9,
    )
    if ret:
        print("找到颜色:", x, y)
    else:
        print("没有找到颜色")
```

颜色字符串和相似度参数沿用原生 C API 的规则。方向参数可以省略，也可以使用 `constants.FindDirection` 指定。

## 常量示例

```python
from op import Op, constants

with Op() as op:
    op.set_show_error_msg(constants.ShowError.CONSOLE)
    op.set_screen_data_mode(constants.ScreenDataMode.TOP_DOWN)

    hwnds = op.enum_window(
        title="记事本",
        filter=constants.WindowEnumFilter.TITLE | constants.WindowEnumFilter.VISIBLE,
    )

    hwnd = op.find_window(title="记事本")
    if hwnd:
        op.set_window_state(hwnd, constants.WindowStateAction.ACTIVATE)
        visible = op.get_window_state(hwnd, constants.WindowState.VISIBLE)
        parent = op.get_window(hwnd, constants.WindowRelation.PARENT)
        print(hwnds, visible, parent)

    op.bind_window(
        hwnd,
        display=constants.DisplayMode.NORMAL,
        mouse=constants.MouseMode.NORMAL,
        keypad=constants.KeypadMode.NORMAL,
    )
```

## OpenCV 模板匹配

```python
from op import Op, constants

with Op() as op:
    op.cv_load_template("ok_button", r".\assets\ok_button.png")

    result = op.cv_match_template(
        0,
        0,
        1280,
        720,
        "ok_button",
        0.85,
        constants.SearchDirection.LEFT_TO_RIGHT,
        constants.Strip.NONE,
        constants.MatchMethod.CCOEFF_NORMED,
        constants.MatchColor.COLOR,
    )
    print(result)

    op.cv_remove_template("ok_button")
```

常用 OpenCV 预处理常量：

```python
from op import Op, constants

with Op() as op:
    op.cv_threshold(r".\src.png", r".\binary.png", 128, 255, constants.Threshold.BINARY)
    op.cv_in_range(r".\src.png", r".\mask.png", constants.ColorSpace.HSV, "35,50,50", "85,255,255")
    op.cv_morphology(r".\mask.png", r".\open.png", constants.Morphology.OPEN, 3, 1)
    op.cv_blur(r".\src.png", r".\blur.png", constants.Blur.GAUSSIAN, 3)
    op.cv_thin(r".\binary.png", r".\thin.png", constants.Thin.ZHANG_SUEN)
```

## OCR 示例

```python
from op import Op

with Op() as op:
    text = op.ocr(0, 0, 800, 600, "FFFFFF-000000", 0.9)
    print(text)

    result = op.find_str(0, 0, 800, 600, "开始|确定", "FFFFFF-000000", 0.9)
    ret, x, y = result
    if ret:
        print("找到文字:", x, y)
```

OCR 字典、颜色格式和相似度参数沿用原生接口规则。

## 内存读写

```python
from op import IntType, Op, StringType

with Op() as op:
    hwnd = op.find_window("", "窗口标题")
    if not hwnd:
        raise RuntimeError("没有找到目标窗口")

    value = op.read_int("0012FF80", IntType.I32, hwnd=hwnd)
    print("原始值:", value)

    op.write_int("0012FF80", 123, IntType.I32, hwnd=hwnd)
    op.write_float("0012FF84", 3.14, hwnd=hwnd)
    op.write_string("0012FF90", "hello", StringType.UTF8, hwnd=hwnd)
```

如果已经调用 `bind_window`，内存接口的 `hwnd` 可以省略或传 `0`，C API 会优先使用当前绑定窗口：

```python
from op import IntType, Op

with Op() as op:
    hwnd = op.find_window("", "窗口标题")
    op.bind_window(hwnd)
    try:
        value = op.read_int("0012FF80", IntType.I32)
        op.write_int("0012FF80", value + 1, IntType.I32)
    finally:
        op.unbind_window()
```

支持的整数类型：

```python
from op import IntType

IntType.I32
IntType.I16
IntType.I8
IntType.I64
IntType.U32
IntType.U16
IntType.U8
```

支持的字符串类型：

```python
from op import StringType

StringType.ANSI
StringType.GBK
StringType.UTF16
StringType.UTF8
```

## 返回值约定

- 普通操作方法成功返回 `True`。
- 普通操作方法失败时默认抛出 `OpCallError`。
- `find_color`、`find_str` 等查找方法返回 `(ret, x, y)`，没有找到时通常是
  正常结果，不会抛异常。
- 坐标转换方法返回 `(x, y)`。
- 窗口矩形方法返回 `(x1, y1, x2, y2)`。
- 尺寸方法返回 `(width, height)`。
- 原生字符串返回值会转换成 Python `str`。
- 屏幕数据指针返回整数地址；BMP 屏幕数据返回 `(address, size)`。

如果希望失败时返回 `False` 而不是抛异常：

```python
from op import Op

with Op(raise_on_error=False) as op:
    ok = op.bind_window(0)
    if not ok:
        print("绑定失败:", op.get_last_error())
```

## 常见排错

确认 Python 位数：

```python
import struct

print(struct.calcsize("P") * 8)
```

确认实际加载的 DLL：

```python
from op import Op

with Op() as op:
    print(op.dll_path)
```

如果加载失败，优先检查：

- Python 位数是否和 DLL 位数一致。
- `op_c_api_x64.dll` 或 `op_c_api_x86.dll` 是否存在。
- DLL 依赖文件是否和主 DLL 放在同一目录。
- 是否通过 `dll_path` 或 `dll_dir` 指到了正确位置。
