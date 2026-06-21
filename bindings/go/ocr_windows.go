//go:build windows

package opcapi

func (o *Op) SetOcrEngine(pathOfEngine, dllName, argv string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSetOcrEngine.Call(o.handle, strArg(pathOfEngine), strArg(dllName), strArg(argv))
	return int(ret)
}

func (o *Op) SetYoloEngine(pathOfEngine, dllName, argv string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSetYoloEngine.Call(o.handle, strArg(pathOfEngine), strArg(dllName), strArg(argv))
	return int(ret)
}

func (o *Op) YoloDetect(x1, y1, x2, y2 int, conf, iou float64) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procYoloDetect.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), f64Arg(conf), f64Arg(iou))
	return wcharString(ret)
}

func (o *Op) YoloDetectFromFile(fileName string, conf, iou float64) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procYoloDetectFromFile.Call(o.handle, strArg(fileName), f64Arg(conf), f64Arg(iou))
	return wcharString(ret)
}

func (o *Op) SetDict(idx int, fileName string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSetDict.Call(o.handle, uintptr(idx), strArg(fileName))
	return int(ret)
}

func (o *Op) GetDict(idx, fontIndex int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procGetDict.Call(o.handle, uintptr(idx), uintptr(fontIndex))
	return wcharString(ret)
}

func (o *Op) SetMemDict(idx int, data string, size int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSetMemDict.Call(o.handle, uintptr(idx), strArg(data), uintptr(size))
	return int(ret)
}

func (o *Op) UseDict(idx int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procUseDict.Call(o.handle, uintptr(idx))
	return int(ret)
}

func (o *Op) AddDict(idx int, dictInfo string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procAddDict.Call(o.handle, uintptr(idx), strArg(dictInfo))
	return int(ret)
}

func (o *Op) SaveDict(idx int, fileName string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSaveDict.Call(o.handle, uintptr(idx), strArg(fileName))
	return int(ret)
}

func (o *Op) ClearDict(idx int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procClearDict.Call(o.handle, uintptr(idx))
	return int(ret)
}

func (o *Op) GetDictCount(idx int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procGetDictCount.Call(o.handle, uintptr(idx))
	return int(ret)
}

func (o *Op) GetNowDict() int {
	return o.callNoArgs(procGetNowDict)
}

func (o *Op) FetchWord(x1, y1, x2, y2 int, color, word string) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procFetchWord.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(color), strArg(word))
	return wcharString(ret)
}

func (o *Op) GetWordsNoDict(x1, y1, x2, y2 int, color string) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procGetWordsNoDict.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(color))
	return wcharString(ret)
}

func (o *Op) GetWordResultCount(result string) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procGetWordResultCount.Call(o.handle, strArg(result))
	return int(ret)
}

func (o *Op) GetWordResultPos(result string, index int) (int, int, int) {
	if !o.valid() {
		return 0, 0, 0
	}

	var x, y int32
	ret, _, _ := procGetWordResultPos.Call(o.handle, strArg(result), uintptr(index), int32Ptr(&x), int32Ptr(&y))
	return int(ret), int(x), int(y)
}

func (o *Op) GetWordResultStr(result string, index int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procGetWordResultStr.Call(o.handle, strArg(result), uintptr(index))
	return wcharString(ret)
}

func (o *Op) Ocr(x1, y1, x2, y2 int, color string, sim float64) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procOcr.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(color), f64Arg(sim))
	return wcharString(ret)
}

func (o *Op) OcrEx(x1, y1, x2, y2 int, color string, sim float64) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procOcrEx.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(color), f64Arg(sim))
	return wcharString(ret)
}

func (o *Op) FindStr(x1, y1, x2, y2 int, strs, color string, sim float64) (int, int, int) {
	if !o.valid() {
		return 0, 0, 0
	}

	var x, y int32
	ret, _, _ := procFindStr.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(strs), strArg(color), f64Arg(sim), int32Ptr(&x), int32Ptr(&y))
	return int(ret), int(x), int(y)
}

func (o *Op) FindStrEx(x1, y1, x2, y2 int, strs, color string, sim float64) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procFindStrEx.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(strs), strArg(color), f64Arg(sim))
	return wcharString(ret)
}

func (o *Op) OcrAuto(x1, y1, x2, y2 int, sim float64) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procOcrAuto.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), f64Arg(sim))
	return wcharString(ret)
}

func (o *Op) OcrFromFile(fileName, colorFormat string, sim float64) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procOcrFromFile.Call(o.handle, strArg(fileName), strArg(colorFormat), f64Arg(sim))
	return wcharString(ret)
}

func (o *Op) OcrAutoFromFile(fileName string, sim float64) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procOcrAutoFromFile.Call(o.handle, strArg(fileName), f64Arg(sim))
	return wcharString(ret)
}

func (o *Op) FindLine(x1, y1, x2, y2 int, color string, sim float64) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procFindLine.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(color), f64Arg(sim))
	return wcharString(ret)
}
