//go:build windows

package opcapi

import (
	"fmt"
)

const (
	DisplayNormal     = "normal"
	DisplayNormalAuto = "normal.auto"
	DisplayNormalDXGI = "normal.dxgi"
	DisplayNormalWGC  = "normal.wgc"
	DisplayGDI        = "gdi"
	DisplayGDI2       = "gdi2"
	DisplayDX2        = "dx2"
	DisplayDX         = "dx"
	DisplayDXD3D9     = "dx.d3d9"
	DisplayDXD3D10    = "dx.d3d10"
	DisplayDXD3D11    = "dx.d3d11"
	DisplayDXD3D12    = "dx.d3d12"
	DisplayOpenGL     = "opengl"
	DisplayOpenGLStd  = "opengl.std"
	DisplayOpenGLNox  = "opengl.nox"
	DisplayOpenGLES   = "opengl.es"
	DisplayOpenGLFI   = "opengl.fi"
)

const (
	MouseNormal  = "normal"
	MouseWindows = "windows"
	MouseDX      = "dx"
)

const (
	KeypadNormal   = "normal"
	KeypadNormalHD = "normal.hd"
	KeypadWindows  = "windows"
	KeypadDX       = "dx"
)

const BindModeNormal = 0

const (
	IntTypeI32 = 0
	IntTypeI16 = 1
	IntTypeI8  = 2
	IntTypeI64 = 3
	IntTypeU32 = 4
	IntTypeU16 = 5
	IntTypeU8  = 6
)

const (
	StringTypeANSI  = 0
	StringTypeGBK   = 0
	StringTypeUTF16 = 1
	StringTypeUTF8  = 2
)

type Op struct {
	handle uintptr
}

type Options struct {
	// DLLPath 指定 op_c_api_x64.dll 或 op_c_api_x86.dll 的完整路径。
	// 这是包级 DLL 选择，建议在创建第一个 Op 前设置；不支持同时混用多份 DLL。
	DLLPath string
}

func New() (*Op, error) {
	return NewWithOptions(Options{})
}

func NewWithOptions(opts Options) (*Op, error) {
	if opts.DLLPath != "" {
		configureDLL(opts.DLLPath)
	}

	handle, _, _ := procCreate.Call()
	if handle == 0 {
		return nil, fmt.Errorf("OpCreate failed")
	}
	return &Op{handle: handle}, nil
}

func Ver() string {
	ret, _, _ := procVer.Call()
	return wcharString(ret)
}

func (o *Op) Destroy() {
	if o == nil || o.handle == 0 {
		return
	}
	procDestroy.Call(o.handle)
	o.handle = 0
}

func (o *Op) Ver() string {
	ret, _, _ := procVer.Call()
	return wcharString(ret)
}

func (o *Op) SetPath(path string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSetPath.Call(o.handle, strArg(path))
	return int(ret)
}

func (o *Op) GetPath() string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procGetPath.Call(o.handle)
	return wcharString(ret)
}

func (o *Op) GetBasePath() string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procGetBasePath.Call(o.handle)
	return wcharString(ret)
}

func (o *Op) GetID() int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procGetID.Call(o.handle)
	return int(ret)
}

func (o *Op) GetLastError() int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procGetLastError.Call(o.handle)
	return int(ret)
}

func (o *Op) SetShowErrorMsg(showType int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSetShowErrorMsg.Call(o.handle, uintptr(showType))
	return int(ret)
}

func (o *Op) Sleep(milliseconds int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSleep.Call(o.handle, uintptr(milliseconds))
	return int(ret)
}

func (o *Op) InjectDll(processName, dllName string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procInjectDll.Call(o.handle, strArg(processName), strArg(dllName))
	return int(ret)
}

func (o *Op) EnablePicCache(enable int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procEnablePicCache.Call(o.handle, uintptr(enable))
	return int(ret)
}

func (o *Op) CapturePre(fileName string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procCapturePre.Call(o.handle, strArg(fileName))
	return int(ret)
}

func (o *Op) SetScreenDataMode(mode int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSetScreenDataMode.Call(o.handle, uintptr(mode))
	return int(ret)
}

func (o *Op) Delay(mis int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procDelay.Call(o.handle, uintptr(mis))
	return int(ret)
}

func (o *Op) Delays(misMin, misMax int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procDelays.Call(o.handle, uintptr(misMin), uintptr(misMax))
	return int(ret)
}

func (o *Op) valid() bool {
	return o != nil && o.handle != 0
}

func (o *Op) callNoArgs(proc interface {
	Call(...uintptr) (uintptr, uintptr, error)
}) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := proc.Call(o.handle)
	return int(ret)
}
