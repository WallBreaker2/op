//go:build windows

package opcapi

func (o *Op) GetCursorPos() (int, int, int) {
	if !o.valid() {
		return 0, 0, 0
	}

	var x, y int32
	ret, _, _ := procGetCursorPos.Call(o.handle, int32Ptr(&x), int32Ptr(&y))
	return int(ret), int(x), int(y)
}

func (o *Op) GetCursorShape() string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procGetCursorShape.Call(o.handle)
	return wcharString(ret)
}

func (o *Op) MoveR(x, y int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procMoveR.Call(o.handle, uintptr(x), uintptr(y))
	return int(ret)
}

func (o *Op) MoveTo(x, y int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procMoveTo.Call(o.handle, uintptr(x), uintptr(y))
	return int(ret)
}

func (o *Op) MoveToEx(x, y, w, h int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procMoveToEx.Call(o.handle, uintptr(x), uintptr(y), uintptr(w), uintptr(h))
	return wcharString(ret)
}

func (o *Op) LeftClick() int {
	return o.callNoArgs(procLeftClick)
}

func (o *Op) LeftDoubleClick() int {
	return o.callNoArgs(procLeftDoubleClick)
}

func (o *Op) LeftDown() int {
	return o.callNoArgs(procLeftDown)
}

func (o *Op) LeftUp() int {
	return o.callNoArgs(procLeftUp)
}

func (o *Op) MiddleClick() int {
	return o.callNoArgs(procMiddleClick)
}

func (o *Op) MiddleDoubleClick() int {
	return o.callNoArgs(procMiddleDoubleClick)
}

func (o *Op) MiddleDown() int {
	return o.callNoArgs(procMiddleDown)
}

func (o *Op) MiddleUp() int {
	return o.callNoArgs(procMiddleUp)
}

func (o *Op) RightClick() int {
	return o.callNoArgs(procRightClick)
}

func (o *Op) RightDoubleClick() int {
	return o.callNoArgs(procRightDoubleClick)
}

func (o *Op) RightDown() int {
	return o.callNoArgs(procRightDown)
}

func (o *Op) RightUp() int {
	return o.callNoArgs(procRightUp)
}

func (o *Op) XButton1Click() int {
	return o.callNoArgs(procXButton1Click)
}

func (o *Op) XButton1DoubleClick() int {
	return o.callNoArgs(procXButton1DoubleClick)
}

func (o *Op) XButton1Down() int {
	return o.callNoArgs(procXButton1Down)
}

func (o *Op) XButton1Up() int {
	return o.callNoArgs(procXButton1Up)
}

func (o *Op) XButton2Click() int {
	return o.callNoArgs(procXButton2Click)
}

func (o *Op) XButton2DoubleClick() int {
	return o.callNoArgs(procXButton2DoubleClick)
}

func (o *Op) XButton2Down() int {
	return o.callNoArgs(procXButton2Down)
}

func (o *Op) XButton2Up() int {
	return o.callNoArgs(procXButton2Up)
}

func (o *Op) Wheel(delta int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procWheel.Call(o.handle, uintptr(delta))
	return int(ret)
}

func (o *Op) HWheel(delta int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procHWheel.Call(o.handle, uintptr(delta))
	return int(ret)
}

func (o *Op) WheelDown() int {
	return o.callNoArgs(procWheelDown)
}

func (o *Op) WheelUp() int {
	return o.callNoArgs(procWheelUp)
}

func (o *Op) SetMouseDelay(typ string, delay int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSetMouseDelay.Call(o.handle, strArg(typ), uintptr(delay))
	return int(ret)
}

func (o *Op) GetKeyState(vkCode int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procGetKeyState.Call(o.handle, uintptr(vkCode))
	return int(ret)
}

func (o *Op) KeyDown(vkCode int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procKeyDown.Call(o.handle, uintptr(vkCode))
	return int(ret)
}

func (o *Op) KeyDownChar(vkCode string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procKeyDownChar.Call(o.handle, strArg(vkCode))
	return int(ret)
}

func (o *Op) KeyUp(vkCode int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procKeyUp.Call(o.handle, uintptr(vkCode))
	return int(ret)
}

func (o *Op) KeyUpChar(vkCode string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procKeyUpChar.Call(o.handle, strArg(vkCode))
	return int(ret)
}

func (o *Op) WaitKey(vkCode, timeout int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procWaitKey.Call(o.handle, uintptr(vkCode), uintptr(timeout))
	return int(ret)
}

func (o *Op) KeyPress(vkCode int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procKeyPress.Call(o.handle, uintptr(vkCode))
	return int(ret)
}

func (o *Op) KeyPressChar(vkCode string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procKeyPressChar.Call(o.handle, strArg(vkCode))
	return int(ret)
}

func (o *Op) SetKeypadDelay(typ string, delay int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSetKeypadDelay.Call(o.handle, strArg(typ), uintptr(delay))
	return int(ret)
}

func (o *Op) KeyPressStr(keyStr string, delay int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procKeyPressStr.Call(o.handle, strArg(keyStr), uintptr(delay))
	return int(ret)
}
