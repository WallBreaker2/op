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

func (o *Op) SetBinaryPreprocess(mode, isolatedThreshold, minComponentArea, bridgeGap int) int {
	if !o.valid() {
		return 0
	}

	ret, _, _ := procSetBinaryPreprocess.Call(o.handle, uintptr(mode), uintptr(isolatedThreshold), uintptr(minComponentArea), uintptr(bridgeGap))
	return int(ret)
}

func (o *Op) GetBinaryPreprocess() (int, int, int, int, int) {
	if !o.valid() {
		return 0, 0, 0, 0, 0
	}

	var mode, isolatedThreshold, minComponentArea, bridgeGap int32
	ret, _, _ := procGetBinaryPreprocess.Call(o.handle, int32Ptr(&mode), int32Ptr(&isolatedThreshold), int32Ptr(&minComponentArea), int32Ptr(&bridgeGap))
	return int(ret), int(mode), int(isolatedThreshold), int(minComponentArea), int(bridgeGap)
}

func (o *Op) FetchWord(x1, y1, x2, y2 int, color, word string) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procFetchWord.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(color), strArg(word))
	return wcharString(ret)
}

func (o *Op) FetchWordEx(x1, y1, x2, y2 int, color string, sim float64, word string) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procFetchWordEx.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(color), f64Arg(sim), strArg(word))
	return wcharString(ret)
}

func (o *Op) ExtractWordRects(x1, y1, x2, y2 int, color string, sim float64, minWordH int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procExtractWordRects.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(color), f64Arg(sim), uintptr(minWordH))
	return wcharString(ret)
}

func (o *Op) ExtractWordRectsEx(x1, y1, x2, y2 int, color string, sim float64, minWordW, minWordH, padding int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procExtractWordRectsEx.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(color), f64Arg(sim), uintptr(minWordW), uintptr(minWordH), uintptr(padding))
	return wcharString(ret)
}

func (o *Op) FetchWords(x1, y1, x2, y2 int, color string, sim float64, words string, minWordH int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procFetchWords.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(color), f64Arg(sim), strArg(words), uintptr(minWordH))
	return wcharString(ret)
}

func (o *Op) FetchWordsEx(x1, y1, x2, y2 int, color string, sim float64, words string, minWordW, minWordH, padding int) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procFetchWordsEx.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(color), f64Arg(sim), strArg(words), uintptr(minWordW), uintptr(minWordH), uintptr(padding))
	return wcharString(ret)
}

func (o *Op) FetchWordsByRects(x1, y1, x2, y2 int, color string, sim float64, words, rects string) string {
	if !o.valid() {
		return ""
	}

	ret, _, _ := procFetchWordsByRects.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(color), f64Arg(sim), strArg(words), strArg(rects))
	return wcharString(ret)
}

func (o *Op) GetBinaryPreview(x1, y1, x2, y2 int, color string, sim float64) (string, int) {
	if !o.valid() {
		return "", 0
	}

	var count int32
	ret, _, _ := procGetBinaryPreview.Call(o.handle, uintptr(x1), uintptr(y1), uintptr(x2), uintptr(y2), strArg(color), f64Arg(sim), int32Ptr(&count))
	return wcharString(ret), int(count)
}

func (o *Op) GetWordPreview(dictInfo string) (string, int) {
	if !o.valid() {
		return "", 0
	}

	var valid int32
	ret, _, _ := procGetWordPreview.Call(o.handle, strArg(dictInfo), int32Ptr(&valid))
	return wcharString(ret), int(valid)
}

func (o *Op) CheckWordDict(dictInfo string) (string, int) {
	if !o.valid() {
		return "", 0
	}

	var validCount int32
	ret, _, _ := procCheckWordDict.Call(o.handle, strArg(dictInfo), int32Ptr(&validCount))
	return wcharString(ret), int(validCount)
}

func (o *Op) NormalizeWordDict(dictInfo string) (string, int) {
	if !o.valid() {
		return "", 0
	}

	var validCount int32
	ret, _, _ := procNormalizeWordDict.Call(o.handle, strArg(dictInfo), int32Ptr(&validCount))
	return wcharString(ret), int(validCount)
}

func (o *Op) RenameWordDict(dictInfo, words string) (string, int) {
	if !o.valid() {
		return "", 0
	}

	var renamedCount int32
	ret, _, _ := procRenameWordDict.Call(o.handle, strArg(dictInfo), strArg(words), int32Ptr(&renamedCount))
	return wcharString(ret), int(renamedCount)
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
