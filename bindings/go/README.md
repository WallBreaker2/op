# op Go binding

这是 `op_c_api_x64.dll` / `op_c_api_x86.dll` 的 Windows Go 包装。

## 使用

```go
package main

import "opcapi"

func main() {
	op, err := opcapi.New()
	if err != nil {
		panic(err)
	}
	defer op.Destroy()

	hwnd := client.GetForegroundWindow()
	op.BindWindow(hwnd, opcapi.DisplayNormalAuto, opcapi.MouseNormal, opcapi.KeypadNormal, opcapi.BindModeNormal)
	op.Capture(0, 0, 800, 600, "capture.png")
}
```

如果 DLL 不在当前工作目录或 Windows 默认搜索路径里，可以指定完整路径：

```go
op, err := opcapi.NewWithOptions(op.Options{
	DLLPath: `D:\op\bin\x64\op_c_api_x64.dll`,
})
```

DLL 查找顺序：

1. 当前工作目录下的 `op_c_api_x64.dll` 或 `op_c_api_x86.dll`
2. Windows 默认 DLL 搜索路径

## 说明

- 包名：`opcapi`
- 模块名：`opcapi`
- 平台：Windows
- 依赖：`golang.org/x/sys/windows`
