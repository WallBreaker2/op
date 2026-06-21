//go:build windows

package opcapi

func (o *Op) BindWindow(hwnd uintptr, display, mouse, keypad string, mode int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procBindWindow.Call(
		o.handle,
		hwnd,
		strArg(display),
		strArg(mouse),
		strArg(keypad),
		uintptr(mode),
	)
	return int(ret)
}

func (o *Op) BindWindowEx(displayHWND, inputHWND uintptr, display, mouse, keypad string, mode int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procBindWindowEx.Call(
		o.handle,
		displayHWND,
		inputHWND,
		strArg(display),
		strArg(mouse),
		strArg(keypad),
		uintptr(mode),
	)
	return int(ret)
}

func (o *Op) UnBindWindow() int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procUnBindWindow.Call(o.handle)
	return int(ret)
}

func (o *Op) GetBindWindow() uintptr {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procGetBindWindow.Call(o.handle)
	return ret
}

func (o *Op) IsBind() int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procIsBind.Call(o.handle)
	return int(ret)
}
