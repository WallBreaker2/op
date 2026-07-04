//go:build windows

package opcapi

func (o *Op) Capture(x1, y1, x2, y2 int, fileName string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procCapture.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(fileName))
	return int(ret)
}

func (o *Op) CmpColor(x, y int, color string, sim float64) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procCmpColor.Call(o.handle, uintptr(x), uintptr(y), strArg(color), f64Arg(sim))
	return int(ret)
}

func (o *Op) FindColor(x1, y1, x2, y2 int, color string, sim float64, dir int) (int, int, int) {
	if !o.valid() {
		return 0, 0, 0
	}

	var x, y int32
	ret, _, _ := procFindColor.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(color), f64Arg(sim), uintptr(dir), int32Ptr(&x), int32Ptr(&y))
	return int(ret), int(x), int(y)
}

func (o *Op) FindColorEx(x1, y1, x2, y2 int, color string, sim float64, dir int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procFindColorEx.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(color), f64Arg(sim), uintptr(dir))
	return wcharString(ret)
}

func (o *Op) FindMultiColor(x1, y1, x2, y2 int, firstColor, offsetColor string, sim float64, dir int) (int, int, int) {
	if !o.valid() {
		return 0, 0, 0
	}

	var x, y int32
	ret, _, _ := procFindMultiColor.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(firstColor), strArg(offsetColor), f64Arg(sim), uintptr(dir), int32Ptr(&x), int32Ptr(&y))
	return int(ret), int(x), int(y)
}

func (o *Op) FindMultiColorEx(x1, y1, x2, y2 int, firstColor, offsetColor string, sim float64, dir int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procFindMultiColorEx.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(firstColor), strArg(offsetColor), f64Arg(sim), uintptr(dir))
	return wcharString(ret)
}

func (o *Op) FindPic(x1, y1, x2, y2 int, files, deltaColor string, sim float64, dir int) (int, int, int) {
	if !o.valid() {
		return -1, -1, -1
	}

	var x, y int32
	ret, _, _ := procFindPic.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(files), strArg(deltaColor), f64Arg(sim), uintptr(dir), int32Ptr(&x), int32Ptr(&y))
	return int(ret), int(x), int(y)
}

func (o *Op) FindPicEx(x1, y1, x2, y2 int, files, deltaColor string, sim float64, dir int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procFindPicEx.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(files), strArg(deltaColor), f64Arg(sim), uintptr(dir))
	return wcharString(ret)
}

func (o *Op) FindPicExS(x1, y1, x2, y2 int, files, deltaColor string, sim float64, dir int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procFindPicExS.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(files), strArg(deltaColor), f64Arg(sim), uintptr(dir))
	return wcharString(ret)
}

func (o *Op) FindColorBlock(x1, y1, x2, y2 int, color string, sim float64, count, height, width int) (int, int, int) {
	if !o.valid() {
		return 0, -1, -1
	}

	var x, y int32
	ret, _, _ := procFindColorBlock.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(color), f64Arg(sim), uintptr(count), uintptr(height), uintptr(width), int32Ptr(&x), int32Ptr(&y))
	return int(ret), int(x), int(y)
}

func (o *Op) FindColorBlockEx(x1, y1, x2, y2 int, color string, sim float64, count, height, width int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procFindColorBlockEx.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(color), f64Arg(sim), uintptr(count), uintptr(height), uintptr(width))
	return wcharString(ret)
}

func (o *Op) GetColor(x, y int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procGetColor.Call(o.handle, uintptr(x), uintptr(y))
	return wcharString(ret)
}

func (o *Op) GetColorNum(x1, y1, x2, y2 int, color string, sim float64) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procGetColorNum.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(color), f64Arg(sim))
	return int(ret)
}

func (o *Op) SetDisplayInput(mode string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSetDisplayInput.Call(o.handle, strArg(mode))
	return int(ret)
}

func (o *Op) LoadPic(fileName string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procLoadPic.Call(o.handle, strArg(fileName))
	return int(ret)
}

func (o *Op) FreePic(fileName string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procFreePic.Call(o.handle, strArg(fileName))
	return int(ret)
}

func (o *Op) LoadMemPic(fileName string, data []byte) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procLoadMemPic.Call(o.handle, strArg(fileName), bytesPtr(data), uintptr(len(data)))
	return int(ret)
}

func (o *Op) GetPicSize(picName string) (int, int, int) {
	if !o.valid() {
		return 0, 0, 0
	}

	var width, height int32
	ret, _, _ := procGetPicSize.Call(o.handle, strArg(picName), int32Ptr(&width), int32Ptr(&height))
	return int(ret), int(width), int(height)
}

func (o *Op) GetScreenData(x1, y1, x2, y2 int) (uintptr, int) {
	if !o.valid() {
		return 0, 0
	}

	var retCode int32
	ptr, _, _ := procGetScreenData.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), int32Ptr(&retCode))
	return ptr, int(retCode)
}

func (o *Op) GetScreenDataBmp(x1, y1, x2, y2 int) (uintptr, int, int) {
	if !o.valid() {
		return 0, 0, 0
	}

	var size, retCode int32
	ptr, _, _ := procGetScreenDataBmp.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), int32Ptr(&size), int32Ptr(&retCode))
	return ptr, int(size), int(retCode)
}

func (o *Op) GetScreenFrameInfo() (int, int) {
	if !o.valid() {
		return 0, 0
	}

	var frameID, time int32
	procGetScreenFrameInfo.Call(o.handle, int32Ptr(&frameID), int32Ptr(&time))
	return int(frameID), int(time)
}

func (o *Op) MatchPicName(picName string) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procMatchPicName.Call(o.handle, strArg(picName))
	return wcharString(ret)
}
