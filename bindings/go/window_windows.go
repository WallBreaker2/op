//go:build windows

package opcapi

func (o *Op) EnumWindow(parent uintptr, title, className string, filter int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procEnumWindow.Call(o.handle, parent, strArg(title), strArg(className), uintptr(filter))
	return wcharString(ret)
}

func (o *Op) EnumWindowByProcess(processName, title, className string, filter int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procEnumWindowByProcess.Call(o.handle, strArg(processName), strArg(title), strArg(className), uintptr(filter))
	return wcharString(ret)
}

func (o *Op) EnumProcess(name string) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procEnumProcess.Call(o.handle, strArg(name))
	return wcharString(ret)
}

func (o *Op) ClientToScreen(hwnd uintptr, x, y int) (int, int, int) {
	if !o.valid() {
		return 0, x, y
	}

	xx, yy := int32(x), int32(y)
	ret, _, _ := procClientToScreen.Call(o.handle, hwnd, int32Ptr(&xx), int32Ptr(&yy))
	return int(ret), int(xx), int(yy)
}

func (o *Op) FindWindow(className, title string) uintptr {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procFindWindow.Call(o.handle, strArg(className), strArg(title))
	return ret
}

func (o *Op) FindWindowByProcess(processName, className, title string) uintptr {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procFindWindowByProcess.Call(o.handle, strArg(processName), strArg(className), strArg(title))
	return ret
}

func (o *Op) FindWindowByProcessId(processID int, className, title string) uintptr {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procFindWindowByProcessId.Call(o.handle, uintptr(processID), strArg(className), strArg(title))
	return ret
}

func (o *Op) FindWindowEx(parent uintptr, className, title string) uintptr {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procFindWindowEx.Call(o.handle, parent, strArg(className), strArg(title))
	return ret
}

func (o *Op) GetClientRect(hwnd uintptr) (int, int, int, int, int) {
	if !o.valid() {
		return 0, 0, 0, 0, 0
	}

	var x1, y1, x2, y2 int32
	ret, _, _ := procGetClientRect.Call(o.handle, hwnd, int32Ptr(&x1), int32Ptr(&y1), int32Ptr(&x2), int32Ptr(&y2))
	return int(ret), int(x1), int(y1), int(x2), int(y2)
}

func (o *Op) GetClientSize(hwnd uintptr) (int, int, int) {
	if !o.valid() {
		return 0, 0, 0
	}

	var width, height int32
	ret, _, _ := procGetClientSize.Call(o.handle, hwnd, int32Ptr(&width), int32Ptr(&height))
	return int(ret), int(width), int(height)
}

func (o *Op) GetForegroundFocus() uintptr {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procGetForegroundFocus.Call(o.handle)
	return ret
}

func (o *Op) GetForegroundWindow() uintptr {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procGetForegroundWindow.Call(o.handle)
	return ret
}

func (o *Op) GetMousePointWindow() uintptr {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procGetMousePointWindow.Call(o.handle)
	return ret
}

func (o *Op) GetPointWindow(x, y int) uintptr {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procGetPointWindow.Call(o.handle, uintptr(x), uintptr(y))
	return ret
}

func (o *Op) GetProcessInfo(pid int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procGetProcessInfo.Call(o.handle, uintptr(pid))
	return wcharString(ret)
}

func (o *Op) GetSpecialWindow(flag int) uintptr {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procGetSpecialWindow.Call(o.handle, uintptr(flag))
	return ret
}

func (o *Op) GetWindow(hwnd uintptr, flag int) uintptr {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procGetWindow.Call(o.handle, hwnd, uintptr(flag))
	return ret
}

func (o *Op) GetWindowClass(hwnd uintptr) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procGetWindowClass.Call(o.handle, hwnd)
	return wcharString(ret)
}

func (o *Op) GetWindowProcessId(hwnd uintptr) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procGetWindowProcessId.Call(o.handle, hwnd)
	return int(ret)
}

func (o *Op) GetWindowProcessPath(hwnd uintptr) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procGetWindowProcessPath.Call(o.handle, hwnd)
	return wcharString(ret)
}

func (o *Op) GetWindowRect(hwnd uintptr) (int, int, int, int, int) {
	if !o.valid() {
		return 0, 0, 0, 0, 0
	}

	var x1, y1, x2, y2 int32
	ret, _, _ := procGetWindowRect.Call(o.handle, hwnd, int32Ptr(&x1), int32Ptr(&y1), int32Ptr(&x2), int32Ptr(&y2))
	return int(ret), int(x1), int(y1), int(x2), int(y2)
}

func (o *Op) GetWindowState(hwnd uintptr, flag int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procGetWindowState.Call(o.handle, hwnd, uintptr(flag))
	return int(ret)
}

func (o *Op) GetWindowTitle(hwnd uintptr) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procGetWindowTitle.Call(o.handle, hwnd)
	return wcharString(ret)
}

func (o *Op) MoveWindow(hwnd uintptr, x, y int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procMoveWindow.Call(o.handle, hwnd, uintptr(x), uintptr(y))
	return int(ret)
}

func (o *Op) ScreenToClient(hwnd uintptr, x, y int) (int, int, int) {
	if !o.valid() {
		return 0, x, y
	}

	xx, yy := int32(x), int32(y)
	ret, _, _ := procScreenToClient.Call(o.handle, hwnd, int32Ptr(&xx), int32Ptr(&yy))
	return int(ret), int(xx), int(yy)
}

func (o *Op) SendPaste(hwnd uintptr) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSendPaste.Call(o.handle, hwnd)
	return int(ret)
}

func (o *Op) SetClientSize(hwnd uintptr, width, height int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSetClientSize.Call(o.handle, hwnd, uintptr(width), uintptr(height))
	return int(ret)
}

func (o *Op) SetWindowState(hwnd uintptr, flag int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSetWindowState.Call(o.handle, hwnd, uintptr(flag))
	return int(ret)
}

func (o *Op) SetWindowSize(hwnd uintptr, width, height int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSetWindowSize.Call(o.handle, hwnd, uintptr(width), uintptr(height))
	return int(ret)
}

func (o *Op) LayoutWindows(hwnds string, layoutType, columns, startX, startY, gapX, gapY, sizeMode, windowWidth, windowHeight, anchorMode int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procLayoutWindows.Call(
		o.handle,
		strArg(hwnds),
		uintptr(layoutType),
		uintptr(columns),
		uintptr(startX),
		uintptr(startY),
		uintptr(gapX),
		uintptr(gapY),
		uintptr(sizeMode),
		uintptr(windowWidth),
		uintptr(windowHeight),
		uintptr(anchorMode),
	)
	return int(ret)
}

func (o *Op) SetWindowText(hwnd uintptr, title string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSetWindowText.Call(o.handle, hwnd, strArg(title))
	return int(ret)
}

func (o *Op) SetWindowTransparent(hwnd uintptr, trans int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSetWindowTransparent.Call(o.handle, hwnd, uintptr(trans))
	return int(ret)
}

func (o *Op) SendString(hwnd uintptr, str string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSendString.Call(o.handle, hwnd, strArg(str))
	return int(ret)
}

func (o *Op) SendStringIme(hwnd uintptr, str string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSendStringIme.Call(o.handle, hwnd, strArg(str))
	return int(ret)
}

func (o *Op) RunApp(cmdline string, mode int) (int, uint32) {
	if !o.valid() {
		return 0, 0
	}

	var pid uint32
	ret, _, _ := procRunApp.Call(o.handle, strArg(cmdline), uintptr(mode), uint32Ptr(&pid))
	return int(ret), pid
}

func (o *Op) WinExec(cmdline string, cmdshow int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procWinExec.Call(o.handle, strArg(cmdline), uintptr(cmdshow))
	return int(ret)
}

func (o *Op) GetCmdStr(cmd string, milliseconds int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procGetCmdStr.Call(o.handle, strArg(cmd), uintptr(milliseconds))
	return wcharString(ret)
}

func (o *Op) SetClipboard(str string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSetClipboard.Call(o.handle, strArg(str))
	return int(ret)
}

func (o *Op) GetClipboard() string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procGetClipboard.Call(o.handle)
	return wcharString(ret)
}
