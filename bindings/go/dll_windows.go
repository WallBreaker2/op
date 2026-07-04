//go:build windows

package opcapi

import (
	"math"
	"os"
	"path/filepath"
	"sync"
	"unsafe"

	"golang.org/x/sys/windows"
)

var (
	dllMu sync.Mutex
	dll   *windows.LazyDLL

	procCreate                 *windows.LazyProc
	procDestroy                *windows.LazyProc
	procVer                    *windows.LazyProc
	procSetPath                *windows.LazyProc
	procGetPath                *windows.LazyProc
	procGetBasePath            *windows.LazyProc
	procGetID                  *windows.LazyProc
	procGetLastError           *windows.LazyProc
	procSetShowErrorMsg        *windows.LazyProc
	procSleep                  *windows.LazyProc
	procInjectDll              *windows.LazyProc
	procEnablePicCache         *windows.LazyProc
	procCapturePre             *windows.LazyProc
	procSetScreenDataMode      *windows.LazyProc
	procAStarFindPath          *windows.LazyProc
	procFindNearestPos         *windows.LazyProc
	procEnumWindow             *windows.LazyProc
	procEnumWindowByProcess    *windows.LazyProc
	procEnumProcess            *windows.LazyProc
	procClientToScreen         *windows.LazyProc
	procFindWindow             *windows.LazyProc
	procFindWindowByProcess    *windows.LazyProc
	procFindWindowByProcessId  *windows.LazyProc
	procFindWindowEx           *windows.LazyProc
	procGetClientRect          *windows.LazyProc
	procGetClientSize          *windows.LazyProc
	procGetForegroundFocus     *windows.LazyProc
	procGetForegroundWindow    *windows.LazyProc
	procGetMousePointWindow    *windows.LazyProc
	procGetPointWindow         *windows.LazyProc
	procGetProcessInfo         *windows.LazyProc
	procGetSpecialWindow       *windows.LazyProc
	procGetWindow              *windows.LazyProc
	procGetWindowClass         *windows.LazyProc
	procGetWindowProcessId     *windows.LazyProc
	procGetWindowProcessPath   *windows.LazyProc
	procGetWindowRect          *windows.LazyProc
	procGetWindowState         *windows.LazyProc
	procGetWindowTitle         *windows.LazyProc
	procMoveWindow             *windows.LazyProc
	procScreenToClient         *windows.LazyProc
	procSendPaste              *windows.LazyProc
	procSetClientSize          *windows.LazyProc
	procSetWindowState         *windows.LazyProc
	procSetWindowSize          *windows.LazyProc
	procLayoutWindows          *windows.LazyProc
	procSetWindowText          *windows.LazyProc
	procSetWindowTransparent   *windows.LazyProc
	procSendString             *windows.LazyProc
	procSendStringIme          *windows.LazyProc
	procRunApp                 *windows.LazyProc
	procWinExec                *windows.LazyProc
	procGetCmdStr              *windows.LazyProc
	procSetClipboard           *windows.LazyProc
	procGetClipboard           *windows.LazyProc
	procDelay                  *windows.LazyProc
	procDelays                 *windows.LazyProc
	procBindWindow             *windows.LazyProc
	procBindWindowEx           *windows.LazyProc
	procUnBindWindow           *windows.LazyProc
	procLockInput              *windows.LazyProc
	procGetBindWindow          *windows.LazyProc
	procIsBind                 *windows.LazyProc
	procGetCursorPos           *windows.LazyProc
	procGetCursorShape         *windows.LazyProc
	procMoveR                  *windows.LazyProc
	procMoveTo                 *windows.LazyProc
	procMoveToEx               *windows.LazyProc
	procMoveToSmooth           *windows.LazyProc
	procMoveToExSmooth         *windows.LazyProc
	procMovePath               *windows.LazyProc
	procDragPath               *windows.LazyProc
	procSetMouseTrajectory     *windows.LazyProc
	procLeftClick              *windows.LazyProc
	procLeftDoubleClick        *windows.LazyProc
	procLeftDown               *windows.LazyProc
	procLeftUp                 *windows.LazyProc
	procMiddleClick            *windows.LazyProc
	procMiddleDoubleClick      *windows.LazyProc
	procMiddleDown             *windows.LazyProc
	procMiddleUp               *windows.LazyProc
	procRightClick             *windows.LazyProc
	procRightDoubleClick       *windows.LazyProc
	procRightDown              *windows.LazyProc
	procRightUp                *windows.LazyProc
	procXButton1Click          *windows.LazyProc
	procXButton1DoubleClick    *windows.LazyProc
	procXButton1Down           *windows.LazyProc
	procXButton1Up             *windows.LazyProc
	procXButton2Click          *windows.LazyProc
	procXButton2DoubleClick    *windows.LazyProc
	procXButton2Down           *windows.LazyProc
	procXButton2Up             *windows.LazyProc
	procWheel                  *windows.LazyProc
	procHWheel                 *windows.LazyProc
	procWheelDown              *windows.LazyProc
	procWheelUp                *windows.LazyProc
	procSetMouseDelay          *windows.LazyProc
	procGetKeyState            *windows.LazyProc
	procKeyDown                *windows.LazyProc
	procKeyDownChar            *windows.LazyProc
	procKeyUp                  *windows.LazyProc
	procKeyUpChar              *windows.LazyProc
	procWaitKey                *windows.LazyProc
	procKeyPress               *windows.LazyProc
	procKeyPressChar           *windows.LazyProc
	procSetKeypadDelay         *windows.LazyProc
	procKeyPressStr            *windows.LazyProc
	procCapture                *windows.LazyProc
	procCmpColor               *windows.LazyProc
	procFindColor              *windows.LazyProc
	procFindColorEx            *windows.LazyProc
	procFindMultiColor         *windows.LazyProc
	procFindMultiColorEx       *windows.LazyProc
	procFindPic                *windows.LazyProc
	procFindPicEx              *windows.LazyProc
	procFindPicExS             *windows.LazyProc
	procFindColorBlock         *windows.LazyProc
	procFindColorBlockEx       *windows.LazyProc
	procGetColor               *windows.LazyProc
	procGetColorNum            *windows.LazyProc
	procSetDisplayInput        *windows.LazyProc
	procLoadPic                *windows.LazyProc
	procFreePic                *windows.LazyProc
	procLoadMemPic             *windows.LazyProc
	procGetPicSize             *windows.LazyProc
	procGetScreenData          *windows.LazyProc
	procGetScreenDataBmp       *windows.LazyProc
	procGetScreenFrameInfo     *windows.LazyProc
	procMatchPicName           *windows.LazyProc
	procCvLoadTemplate         *windows.LazyProc
	procCvLoadMaskedTemplate   *windows.LazyProc
	procCvRemoveTemplate       *windows.LazyProc
	procCvRemoveAllTemplates   *windows.LazyProc
	procCvHasTemplate          *windows.LazyProc
	procCvGetTemplateCount     *windows.LazyProc
	procCvGetAllTemplateNames  *windows.LazyProc
	procCvGetOpenCvVersion     *windows.LazyProc
	procCvLoadTemplateList     *windows.LazyProc
	procCvToGray               *windows.LazyProc
	procCvToBinary             *windows.LazyProc
	procCvToEdge               *windows.LazyProc
	procCvToOutline            *windows.LazyProc
	procCvDenoise              *windows.LazyProc
	procCvEqualize             *windows.LazyProc
	procCvCLAHE                *windows.LazyProc
	procCvBlur                 *windows.LazyProc
	procCvSharpen              *windows.LazyProc
	procCvCropValid            *windows.LazyProc
	procCvConnectedComponents  *windows.LazyProc
	procCvFindContours         *windows.LazyProc
	procCvPreprocessPipeline   *windows.LazyProc
	procCvCrop                 *windows.LazyProc
	procCvResize               *windows.LazyProc
	procCvThreshold            *windows.LazyProc
	procCvInRange              *windows.LazyProc
	procCvMorphology           *windows.LazyProc
	procCvThin                 *windows.LazyProc
	procCvMatchTemplate        *windows.LazyProc
	procCvMatchTemplateScale   *windows.LazyProc
	procCvMatchAnyTemplate     *windows.LazyProc
	procCvMatchAllTemplates    *windows.LazyProc
	procCvFeatureMatchTemplate *windows.LazyProc
	procCvEdgeMatchTemplate    *windows.LazyProc
	procCvShapeMatchTemplate   *windows.LazyProc
	procSetOcrEngine           *windows.LazyProc
	procSetYoloEngine          *windows.LazyProc
	procYoloDetect             *windows.LazyProc
	procYoloDetectFromFile     *windows.LazyProc
	procSetDict                *windows.LazyProc
	procGetDict                *windows.LazyProc
	procSetMemDict             *windows.LazyProc
	procUseDict                *windows.LazyProc
	procAddDict                *windows.LazyProc
	procSaveDict               *windows.LazyProc
	procClearDict              *windows.LazyProc
	procGetDictCount           *windows.LazyProc
	procGetNowDict             *windows.LazyProc
	procSetBinaryPreprocess    *windows.LazyProc
	procGetBinaryPreprocess    *windows.LazyProc
	procFetchWord              *windows.LazyProc
	procFetchWordEx            *windows.LazyProc
	procExtractWordRects       *windows.LazyProc
	procExtractWordRectsEx     *windows.LazyProc
	procFetchWords             *windows.LazyProc
	procFetchWordsEx           *windows.LazyProc
	procFetchWordsByRects      *windows.LazyProc
	procGetBinaryPreview       *windows.LazyProc
	procGetWordPreview         *windows.LazyProc
	procCheckWordDict          *windows.LazyProc
	procNormalizeWordDict      *windows.LazyProc
	procRenameWordDict         *windows.LazyProc
	procGetWordsNoDict         *windows.LazyProc
	procGetWordResultCount     *windows.LazyProc
	procGetWordResultPos       *windows.LazyProc
	procGetWordResultStr       *windows.LazyProc
	procOcr                    *windows.LazyProc
	procOcrEx                  *windows.LazyProc
	procFindStr                *windows.LazyProc
	procFindStrEx              *windows.LazyProc
	procOcrAuto                *windows.LazyProc
	procOcrFromFile            *windows.LazyProc
	procOcrAutoFromFile        *windows.LazyProc
	procFindLine               *windows.LazyProc
	procWriteData              *windows.LazyProc
	procReadData               *windows.LazyProc
	procReadInt                *windows.LazyProc
	procWriteInt               *windows.LazyProc
	procReadFloat              *windows.LazyProc
	procWriteFloat             *windows.LazyProc
	procReadDouble             *windows.LazyProc
	procWriteDouble            *windows.LazyProc
	procReadString             *windows.LazyProc
	procWriteString            *windows.LazyProc
)

func init() {
	configureDLL(defaultDLLPath())
}

func configureDLL(path string) {
	dllMu.Lock()
	defer dllMu.Unlock()

	dll = windows.NewLazyDLL(path)
	bindProcs()
}

func bindProcs() {
	procCreate = dll.NewProc("OpCreate")
	procDestroy = dll.NewProc("OpDestroy")
	procVer = dll.NewProc("OpVer")
	procSetPath = dll.NewProc("OpSetPath")
	procGetPath = dll.NewProc("OpGetPath")
	procGetBasePath = dll.NewProc("OpGetBasePath")
	procGetID = dll.NewProc("OpGetID")
	procGetLastError = dll.NewProc("OpGetLastError")
	procSetShowErrorMsg = dll.NewProc("OpSetShowErrorMsg")
	procSleep = dll.NewProc("OpSleep")
	procInjectDll = dll.NewProc("OpInjectDll")
	procEnablePicCache = dll.NewProc("OpEnablePicCache")
	procCapturePre = dll.NewProc("OpCapturePre")
	procSetScreenDataMode = dll.NewProc("OpSetScreenDataMode")
	procAStarFindPath = dll.NewProc("OpAStarFindPath")
	procFindNearestPos = dll.NewProc("OpFindNearestPos")
	procEnumWindow = dll.NewProc("OpEnumWindow")
	procEnumWindowByProcess = dll.NewProc("OpEnumWindowByProcess")
	procEnumProcess = dll.NewProc("OpEnumProcess")
	procClientToScreen = dll.NewProc("OpClientToScreen")
	procFindWindow = dll.NewProc("OpFindWindow")
	procFindWindowByProcess = dll.NewProc("OpFindWindowByProcess")
	procFindWindowByProcessId = dll.NewProc("OpFindWindowByProcessId")
	procFindWindowEx = dll.NewProc("OpFindWindowEx")
	procGetClientRect = dll.NewProc("OpGetClientRect")
	procGetClientSize = dll.NewProc("OpGetClientSize")
	procGetForegroundFocus = dll.NewProc("OpGetForegroundFocus")
	procGetForegroundWindow = dll.NewProc("OpGetForegroundWindow")
	procGetMousePointWindow = dll.NewProc("OpGetMousePointWindow")
	procGetPointWindow = dll.NewProc("OpGetPointWindow")
	procGetProcessInfo = dll.NewProc("OpGetProcessInfo")
	procGetSpecialWindow = dll.NewProc("OpGetSpecialWindow")
	procGetWindow = dll.NewProc("OpGetWindow")
	procGetWindowClass = dll.NewProc("OpGetWindowClass")
	procGetWindowProcessId = dll.NewProc("OpGetWindowProcessId")
	procGetWindowProcessPath = dll.NewProc("OpGetWindowProcessPath")
	procGetWindowRect = dll.NewProc("OpGetWindowRect")
	procGetWindowState = dll.NewProc("OpGetWindowState")
	procGetWindowTitle = dll.NewProc("OpGetWindowTitle")
	procMoveWindow = dll.NewProc("OpMoveWindow")
	procScreenToClient = dll.NewProc("OpScreenToClient")
	procSendPaste = dll.NewProc("OpSendPaste")
	procSetClientSize = dll.NewProc("OpSetClientSize")
	procSetWindowState = dll.NewProc("OpSetWindowState")
	procSetWindowSize = dll.NewProc("OpSetWindowSize")
	procLayoutWindows = dll.NewProc("OpLayoutWindows")
	procSetWindowText = dll.NewProc("OpSetWindowText")
	procSetWindowTransparent = dll.NewProc("OpSetWindowTransparent")
	procSendString = dll.NewProc("OpSendString")
	procSendStringIme = dll.NewProc("OpSendStringIme")
	procRunApp = dll.NewProc("OpRunApp")
	procWinExec = dll.NewProc("OpWinExec")
	procGetCmdStr = dll.NewProc("OpGetCmdStr")
	procSetClipboard = dll.NewProc("OpSetClipboard")
	procGetClipboard = dll.NewProc("OpGetClipboard")
	procDelay = dll.NewProc("OpDelay")
	procDelays = dll.NewProc("OpDelays")
	procBindWindow = dll.NewProc("OpBindWindow")
	procBindWindowEx = dll.NewProc("OpBindWindowEx")
	procUnBindWindow = dll.NewProc("OpUnBindWindow")
	procLockInput = dll.NewProc("OpLockInput")
	procGetBindWindow = dll.NewProc("OpGetBindWindow")
	procIsBind = dll.NewProc("OpIsBind")
	procGetCursorPos = dll.NewProc("OpGetCursorPos")
	procGetCursorShape = dll.NewProc("OpGetCursorShape")
	procMoveR = dll.NewProc("OpMoveR")
	procMoveTo = dll.NewProc("OpMoveTo")
	procMoveToEx = dll.NewProc("OpMoveToEx")
	procMoveToSmooth = dll.NewProc("OpMoveToSmooth")
	procMoveToExSmooth = dll.NewProc("OpMoveToExSmooth")
	procMovePath = dll.NewProc("OpMovePath")
	procDragPath = dll.NewProc("OpDragPath")
	procSetMouseTrajectory = dll.NewProc("OpSetMouseTrajectory")
	procLeftClick = dll.NewProc("OpLeftClick")
	procLeftDoubleClick = dll.NewProc("OpLeftDoubleClick")
	procLeftDown = dll.NewProc("OpLeftDown")
	procLeftUp = dll.NewProc("OpLeftUp")
	procMiddleClick = dll.NewProc("OpMiddleClick")
	procMiddleDoubleClick = dll.NewProc("OpMiddleDoubleClick")
	procMiddleDown = dll.NewProc("OpMiddleDown")
	procMiddleUp = dll.NewProc("OpMiddleUp")
	procRightClick = dll.NewProc("OpRightClick")
	procRightDoubleClick = dll.NewProc("OpRightDoubleClick")
	procRightDown = dll.NewProc("OpRightDown")
	procRightUp = dll.NewProc("OpRightUp")
	procXButton1Click = dll.NewProc("OpXButton1Click")
	procXButton1DoubleClick = dll.NewProc("OpXButton1DoubleClick")
	procXButton1Down = dll.NewProc("OpXButton1Down")
	procXButton1Up = dll.NewProc("OpXButton1Up")
	procXButton2Click = dll.NewProc("OpXButton2Click")
	procXButton2DoubleClick = dll.NewProc("OpXButton2DoubleClick")
	procXButton2Down = dll.NewProc("OpXButton2Down")
	procXButton2Up = dll.NewProc("OpXButton2Up")
	procWheel = dll.NewProc("OpWheel")
	procHWheel = dll.NewProc("OpHWheel")
	procWheelDown = dll.NewProc("OpWheelDown")
	procWheelUp = dll.NewProc("OpWheelUp")
	procSetMouseDelay = dll.NewProc("OpSetMouseDelay")
	procGetKeyState = dll.NewProc("OpGetKeyState")
	procKeyDown = dll.NewProc("OpKeyDown")
	procKeyDownChar = dll.NewProc("OpKeyDownChar")
	procKeyUp = dll.NewProc("OpKeyUp")
	procKeyUpChar = dll.NewProc("OpKeyUpChar")
	procWaitKey = dll.NewProc("OpWaitKey")
	procKeyPress = dll.NewProc("OpKeyPress")
	procKeyPressChar = dll.NewProc("OpKeyPressChar")
	procSetKeypadDelay = dll.NewProc("OpSetKeypadDelay")
	procKeyPressStr = dll.NewProc("OpKeyPressStr")
	procCapture = dll.NewProc("OpCapture")
	procCmpColor = dll.NewProc("OpCmpColor")
	procFindColor = dll.NewProc("OpFindColor")
	procFindColorEx = dll.NewProc("OpFindColorEx")
	procFindMultiColor = dll.NewProc("OpFindMultiColor")
	procFindMultiColorEx = dll.NewProc("OpFindMultiColorEx")
	procFindPic = dll.NewProc("OpFindPic")
	procFindPicEx = dll.NewProc("OpFindPicEx")
	procFindPicExS = dll.NewProc("OpFindPicExS")
	procFindColorBlock = dll.NewProc("OpFindColorBlock")
	procFindColorBlockEx = dll.NewProc("OpFindColorBlockEx")
	procGetColor = dll.NewProc("OpGetColor")
	procGetColorNum = dll.NewProc("OpGetColorNum")
	procSetDisplayInput = dll.NewProc("OpSetDisplayInput")
	procLoadPic = dll.NewProc("OpLoadPic")
	procFreePic = dll.NewProc("OpFreePic")
	procLoadMemPic = dll.NewProc("OpLoadMemPic")
	procGetPicSize = dll.NewProc("OpGetPicSize")
	procGetScreenData = dll.NewProc("OpGetScreenData")
	procGetScreenDataBmp = dll.NewProc("OpGetScreenDataBmp")
	procGetScreenFrameInfo = dll.NewProc("OpGetScreenFrameInfo")
	procMatchPicName = dll.NewProc("OpMatchPicName")
	procCvLoadTemplate = dll.NewProc("OpCvLoadTemplate")
	procCvLoadMaskedTemplate = dll.NewProc("OpCvLoadMaskedTemplate")
	procCvRemoveTemplate = dll.NewProc("OpCvRemoveTemplate")
	procCvRemoveAllTemplates = dll.NewProc("OpCvRemoveAllTemplates")
	procCvHasTemplate = dll.NewProc("OpCvHasTemplate")
	procCvGetTemplateCount = dll.NewProc("OpCvGetTemplateCount")
	procCvGetAllTemplateNames = dll.NewProc("OpCvGetAllTemplateNames")
	procCvGetOpenCvVersion = dll.NewProc("OpCvGetOpenCvVersion")
	procCvLoadTemplateList = dll.NewProc("OpCvLoadTemplateList")
	procCvToGray = dll.NewProc("OpCvToGray")
	procCvToBinary = dll.NewProc("OpCvToBinary")
	procCvToEdge = dll.NewProc("OpCvToEdge")
	procCvToOutline = dll.NewProc("OpCvToOutline")
	procCvDenoise = dll.NewProc("OpCvDenoise")
	procCvEqualize = dll.NewProc("OpCvEqualize")
	procCvCLAHE = dll.NewProc("OpCvCLAHE")
	procCvBlur = dll.NewProc("OpCvBlur")
	procCvSharpen = dll.NewProc("OpCvSharpen")
	procCvCropValid = dll.NewProc("OpCvCropValid")
	procCvConnectedComponents = dll.NewProc("OpCvConnectedComponents")
	procCvFindContours = dll.NewProc("OpCvFindContours")
	procCvPreprocessPipeline = dll.NewProc("OpCvPreprocessPipeline")
	procCvCrop = dll.NewProc("OpCvCrop")
	procCvResize = dll.NewProc("OpCvResize")
	procCvThreshold = dll.NewProc("OpCvThreshold")
	procCvInRange = dll.NewProc("OpCvInRange")
	procCvMorphology = dll.NewProc("OpCvMorphology")
	procCvThin = dll.NewProc("OpCvThin")
	procCvMatchTemplate = dll.NewProc("OpCvMatchTemplate")
	procCvMatchTemplateScale = dll.NewProc("OpCvMatchTemplateScale")
	procCvMatchAnyTemplate = dll.NewProc("OpCvMatchAnyTemplate")
	procCvMatchAllTemplates = dll.NewProc("OpCvMatchAllTemplates")
	procCvFeatureMatchTemplate = dll.NewProc("OpCvFeatureMatchTemplate")
	procCvEdgeMatchTemplate = dll.NewProc("OpCvEdgeMatchTemplate")
	procCvShapeMatchTemplate = dll.NewProc("OpCvShapeMatchTemplate")
	procSetOcrEngine = dll.NewProc("OpSetOcrEngine")
	procSetYoloEngine = dll.NewProc("OpSetYoloEngine")
	procYoloDetect = dll.NewProc("OpYoloDetect")
	procYoloDetectFromFile = dll.NewProc("OpYoloDetectFromFile")
	procSetDict = dll.NewProc("OpSetDict")
	procGetDict = dll.NewProc("OpGetDict")
	procSetMemDict = dll.NewProc("OpSetMemDict")
	procUseDict = dll.NewProc("OpUseDict")
	procAddDict = dll.NewProc("OpAddDict")
	procSaveDict = dll.NewProc("OpSaveDict")
	procClearDict = dll.NewProc("OpClearDict")
	procGetDictCount = dll.NewProc("OpGetDictCount")
	procGetNowDict = dll.NewProc("OpGetNowDict")
	procSetBinaryPreprocess = dll.NewProc("OpSetBinaryPreprocess")
	procGetBinaryPreprocess = dll.NewProc("OpGetBinaryPreprocess")
	procFetchWord = dll.NewProc("OpFetchWord")
	procFetchWordEx = dll.NewProc("OpFetchWordEx")
	procExtractWordRects = dll.NewProc("OpExtractWordRects")
	procExtractWordRectsEx = dll.NewProc("OpExtractWordRectsEx")
	procFetchWords = dll.NewProc("OpFetchWords")
	procFetchWordsEx = dll.NewProc("OpFetchWordsEx")
	procFetchWordsByRects = dll.NewProc("OpFetchWordsByRects")
	procGetBinaryPreview = dll.NewProc("OpGetBinaryPreview")
	procGetWordPreview = dll.NewProc("OpGetWordPreview")
	procCheckWordDict = dll.NewProc("OpCheckWordDict")
	procNormalizeWordDict = dll.NewProc("OpNormalizeWordDict")
	procRenameWordDict = dll.NewProc("OpRenameWordDict")
	procGetWordsNoDict = dll.NewProc("OpGetWordsNoDict")
	procGetWordResultCount = dll.NewProc("OpGetWordResultCount")
	procGetWordResultPos = dll.NewProc("OpGetWordResultPos")
	procGetWordResultStr = dll.NewProc("OpGetWordResultStr")
	procOcr = dll.NewProc("OpOcr")
	procOcrEx = dll.NewProc("OpOcrEx")
	procFindStr = dll.NewProc("OpFindStr")
	procFindStrEx = dll.NewProc("OpFindStrEx")
	procOcrAuto = dll.NewProc("OpOcrAuto")
	procOcrFromFile = dll.NewProc("OpOcrFromFile")
	procOcrAutoFromFile = dll.NewProc("OpOcrAutoFromFile")
	procFindLine = dll.NewProc("OpFindLine")
	procWriteData = dll.NewProc("OpWriteData")
	procReadData = dll.NewProc("OpReadData")
	procReadInt = dll.NewProc("OpReadInt")
	procWriteInt = dll.NewProc("OpWriteInt")
	procReadFloat = dll.NewProc("OpReadFloat")
	procWriteFloat = dll.NewProc("OpWriteFloat")
	procReadDouble = dll.NewProc("OpReadDouble")
	procWriteDouble = dll.NewProc("OpWriteDouble")
	procReadString = dll.NewProc("OpReadString")
	procWriteString = dll.NewProc("OpWriteString")
}

func defaultDLLPath() string {
	name := "op_c_api_" + opArch() + ".dll"
	if cwd, err := os.Getwd(); err == nil {
		if path := filepath.Join(cwd, name); fileExists(path) {
			return path
		}
	}
	// 找不到当前目录 DLL 时，交给 Windows 按程序目录和 PATH 搜索。
	return name
}

func opArch() string {
	if unsafe.Sizeof(uintptr(0)) == 4 {
		return "x86"
	}
	return "x64"
}

func fileExists(path string) bool {
	_, err := os.Stat(path)
	return err == nil
}

func int32Ptr(v *int32) uintptr {
	return uintptr(unsafe.Pointer(v))
}

func uint32Ptr(v *uint32) uintptr {
	return uintptr(unsafe.Pointer(v))
}

func int64Ptr(v *int64) uintptr {
	return uintptr(unsafe.Pointer(v))
}

func float32Ptr(v *float32) uintptr {
	return uintptr(unsafe.Pointer(v))
}

func float64Ptr(v *float64) uintptr {
	return uintptr(unsafe.Pointer(v))
}

func bytesPtr(data []byte) uintptr {
	if len(data) == 0 {
		return 0
	}
	return uintptr(unsafe.Pointer(&data[0]))
}

func strArg(s string) uintptr {
	return uintptr(unsafe.Pointer(utf16Ptr(s)))
}

func f32Arg(v float32) uintptr {
	return uintptr(math.Float32bits(v))
}

func f64Arg(v float64) uintptr {
	return uintptr(math.Float64bits(v))
}

func utf16Ptr(s string) *uint16 {
	ptr, err := windows.UTF16PtrFromString(s)
	if err != nil {
		return nil
	}
	return ptr
}

func wcharString(ptr uintptr) string {
	if ptr == 0 {
		return ""
	}
	return windows.UTF16PtrToString((*uint16)(unsafe.Pointer(ptr)))
}
