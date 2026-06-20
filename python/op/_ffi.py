from __future__ import annotations

import ctypes
import os
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

from .errors import DllLoadError


op_handle = ctypes.c_void_p
intptr_t = ctypes.c_ssize_t

_DLL_DIR_HANDLES: list[object] = []
_PACKAGE_DIR = Path(__file__).resolve().parent


@dataclass(frozen=True)
class NativeApi:
    dll: ctypes.WinDLL
    path: Path | None


def python_bits() -> int:
    return struct.calcsize("P") * 8


def arch_dir(bits: int | None = None) -> str:
    bits = bits or python_bits()
    return "x64" if bits == 64 else "x86"


def dll_name(bits: int | None = None) -> str:
    return f"op_c_api_{arch_dir(bits)}.dll"


def _repo_root() -> Path:
    return _PACKAGE_DIR.parents[1]


def _candidate_paths(dll_path: str | os.PathLike[str] | None, dll_dir: str | os.PathLike[str] | None) -> Iterable[Path]:
    name = dll_name()

    if dll_path:
        yield Path(dll_path)
    if dll_dir:
        yield Path(dll_dir) / name

    yield _PACKAGE_DIR / "bin" / arch_dir() / name
    yield _repo_root() / "bin" / arch_dir() / name
    yield Path.cwd() / name
    yield Path.cwd() / "bin" / arch_dir() / name


def _add_dll_directory(path: Path) -> None:
    if hasattr(os, "add_dll_directory"):
        _DLL_DIR_HANDLES.append(os.add_dll_directory(str(path)))


def _load_from_path(path: Path) -> NativeApi:
    path = path.resolve()
    _add_dll_directory(path.parent)
    try:
        return NativeApi(_bind(ctypes.WinDLL(str(path))), path)
    except AttributeError as exc:
        raise DllLoadError(f"{path} is missing a required C API export") from exc


def load_dll(
    dll_path: str | os.PathLike[str] | None = None,
    dll_dir: str | os.PathLike[str] | None = None,
) -> NativeApi:
    if os.name != "nt":
        raise DllLoadError("op C API is only available on Windows")

    searched: list[str] = []
    for candidate in _candidate_paths(dll_path, dll_dir):
        candidate = candidate.expanduser()
        searched.append(str(candidate))
        if candidate.is_file():
            return _load_from_path(candidate)

    name = dll_name()
    try:
        return NativeApi(_bind(ctypes.WinDLL(name)), None)
    except AttributeError as exc:
        raise DllLoadError(f"{name} from PATH is missing a required C API export") from exc
    except OSError as exc:
        searched.append(f"PATH:{name}")
        joined = "\n  ".join(searched)
        raise DllLoadError(f"cannot load {name}; searched:\n  {joined}") from exc


def _signature(dll: ctypes.WinDLL, name: str, restype: object, argtypes: list[object]) -> None:
    func = getattr(dll, name)
    func.restype = restype
    func.argtypes = argtypes


def _bind(dll: ctypes.WinDLL) -> ctypes.WinDLL:
    c_int = ctypes.c_int
    c_double = ctypes.c_double
    c_float = ctypes.c_float
    c_size_t = ctypes.c_size_t
    c_uint32 = ctypes.c_uint32
    c_void_p = ctypes.c_void_p
    c_wchar_p = ctypes.c_wchar_p
    c_int_p = ctypes.POINTER(ctypes.c_int)
    c_int64_p = ctypes.POINTER(ctypes.c_int64)
    c_float_p = ctypes.POINTER(ctypes.c_float)
    c_double_p = ctypes.POINTER(ctypes.c_double)

    signatures = [
        ("OpCreate", op_handle, []),
        ("OpDestroy", None, [op_handle]),
        ("OpVer", c_wchar_p, []),
        ("OpSetPath", c_int, [op_handle, c_wchar_p]),
        ("OpGetPath", c_wchar_p, [op_handle]),
        ("OpGetBasePath", c_wchar_p, [op_handle]),
        ("OpGetID", c_int, [op_handle]),
        ("OpGetLastError", c_int, [op_handle]),
        ("OpSetShowErrorMsg", c_int, [op_handle, c_int]),
        ("OpSleep", c_int, [op_handle, c_int]),
        ("OpInjectDll", c_int, [op_handle, c_wchar_p, c_wchar_p]),
        ("OpEnablePicCache", c_int, [op_handle, c_int]),
        ("OpCapturePre", c_int, [op_handle, c_wchar_p]),
        ("OpSetScreenDataMode", c_int, [op_handle, c_int]),
        ("OpAStarFindPath", c_wchar_p, [op_handle, c_int, c_int, c_wchar_p, c_int, c_int, c_int, c_int]),
        ("OpFindNearestPos", c_wchar_p, [op_handle, c_wchar_p, c_int, c_int, c_int]),
        ("OpEnumWindow", c_wchar_p, [op_handle, intptr_t, c_wchar_p, c_wchar_p, c_int]),
        ("OpEnumWindowByProcess", c_wchar_p, [op_handle, c_wchar_p, c_wchar_p, c_wchar_p, c_int]),
        ("OpEnumProcess", c_wchar_p, [op_handle, c_wchar_p]),
        ("OpClientToScreen", c_int, [op_handle, intptr_t, c_int_p, c_int_p]),
        ("OpFindWindow", intptr_t, [op_handle, c_wchar_p, c_wchar_p]),
        ("OpFindWindowByProcess", intptr_t, [op_handle, c_wchar_p, c_wchar_p, c_wchar_p]),
        ("OpFindWindowByProcessId", intptr_t, [op_handle, c_int, c_wchar_p, c_wchar_p]),
        ("OpFindWindowEx", intptr_t, [op_handle, intptr_t, c_wchar_p, c_wchar_p]),
        ("OpGetClientRect", c_int, [op_handle, intptr_t, c_int_p, c_int_p, c_int_p, c_int_p]),
        ("OpGetClientSize", c_int, [op_handle, intptr_t, c_int_p, c_int_p]),
        ("OpGetForegroundFocus", intptr_t, [op_handle]),
        ("OpGetForegroundWindow", intptr_t, [op_handle]),
        ("OpGetMousePointWindow", intptr_t, [op_handle]),
        ("OpGetPointWindow", intptr_t, [op_handle, c_int, c_int]),
        ("OpGetProcessInfo", c_wchar_p, [op_handle, c_int]),
        ("OpGetSpecialWindow", intptr_t, [op_handle, c_int]),
        ("OpGetWindow", intptr_t, [op_handle, intptr_t, c_int]),
        ("OpGetWindowClass", c_wchar_p, [op_handle, intptr_t]),
        ("OpGetWindowProcessId", c_int, [op_handle, intptr_t]),
        ("OpGetWindowProcessPath", c_wchar_p, [op_handle, intptr_t]),
        ("OpGetWindowRect", c_int, [op_handle, intptr_t, c_int_p, c_int_p, c_int_p, c_int_p]),
        ("OpGetWindowState", c_int, [op_handle, intptr_t, c_int]),
        ("OpGetWindowTitle", c_wchar_p, [op_handle, intptr_t]),
        ("OpMoveWindow", c_int, [op_handle, intptr_t, c_int, c_int]),
        ("OpScreenToClient", c_int, [op_handle, intptr_t, c_int_p, c_int_p]),
        ("OpSendPaste", c_int, [op_handle, intptr_t]),
        ("OpSetClientSize", c_int, [op_handle, intptr_t, c_int, c_int]),
        ("OpSetWindowState", c_int, [op_handle, intptr_t, c_int]),
        ("OpSetWindowSize", c_int, [op_handle, intptr_t, c_int, c_int]),
        ("OpLayoutWindows", c_int, [op_handle, c_wchar_p, c_int, c_int, c_int, c_int, c_int, c_int, c_int, c_int, c_int, c_int]),
        ("OpSetWindowText", c_int, [op_handle, intptr_t, c_wchar_p]),
        ("OpSetWindowTransparent", c_int, [op_handle, intptr_t, c_int]),
        ("OpSendString", c_int, [op_handle, intptr_t, c_wchar_p]),
        ("OpSendStringIme", c_int, [op_handle, intptr_t, c_wchar_p]),
        ("OpRunApp", c_int, [op_handle, c_wchar_p, c_int, ctypes.POINTER(c_uint32)]),
        ("OpWinExec", c_int, [op_handle, c_wchar_p, c_int]),
        ("OpGetCmdStr", c_wchar_p, [op_handle, c_wchar_p, c_int]),
        ("OpSetClipboard", c_int, [op_handle, c_wchar_p]),
        ("OpGetClipboard", c_wchar_p, [op_handle]),
        ("OpDelay", c_int, [op_handle, c_int]),
        ("OpDelays", c_int, [op_handle, c_int, c_int]),
        ("OpBindWindow", c_int, [op_handle, intptr_t, c_wchar_p, c_wchar_p, c_wchar_p, c_int]),
        ("OpBindWindowEx", c_int, [op_handle, intptr_t, intptr_t, c_wchar_p, c_wchar_p, c_wchar_p, c_int]),
        ("OpUnBindWindow", c_int, [op_handle]),
        ("OpGetBindWindow", intptr_t, [op_handle]),
        ("OpIsBind", c_int, [op_handle]),
        ("OpGetCursorPos", c_int, [op_handle, c_int_p, c_int_p]),
        ("OpGetCursorShape", c_wchar_p, [op_handle]),
        ("OpMoveR", c_int, [op_handle, c_int, c_int]),
        ("OpMoveTo", c_int, [op_handle, c_int, c_int]),
        ("OpMoveToEx", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int]),
        ("OpLeftClick", c_int, [op_handle]),
        ("OpLeftDoubleClick", c_int, [op_handle]),
        ("OpLeftDown", c_int, [op_handle]),
        ("OpLeftUp", c_int, [op_handle]),
        ("OpMiddleClick", c_int, [op_handle]),
        ("OpMiddleDown", c_int, [op_handle]),
        ("OpMiddleUp", c_int, [op_handle]),
        ("OpRightClick", c_int, [op_handle]),
        ("OpRightDown", c_int, [op_handle]),
        ("OpRightUp", c_int, [op_handle]),
        ("OpWheelDown", c_int, [op_handle]),
        ("OpWheelUp", c_int, [op_handle]),
        ("OpSetMouseDelay", c_int, [op_handle, c_wchar_p, c_int]),
        ("OpGetKeyState", c_int, [op_handle, c_int]),
        ("OpKeyDown", c_int, [op_handle, c_int]),
        ("OpKeyDownChar", c_int, [op_handle, c_wchar_p]),
        ("OpKeyUp", c_int, [op_handle, c_int]),
        ("OpKeyUpChar", c_int, [op_handle, c_wchar_p]),
        ("OpWaitKey", c_int, [op_handle, c_int, c_int]),
        ("OpKeyPress", c_int, [op_handle, c_int]),
        ("OpKeyPressChar", c_int, [op_handle, c_wchar_p]),
        ("OpSetKeypadDelay", c_int, [op_handle, c_wchar_p, c_int]),
        ("OpKeyPressStr", c_int, [op_handle, c_wchar_p, c_int]),
        ("OpCapture", c_int, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p]),
        ("OpCmpColor", c_int, [op_handle, c_int, c_int, c_wchar_p, c_double]),
        ("OpFindColor", c_int, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_double, c_int, c_int_p, c_int_p]),
        ("OpFindColorEx", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_double, c_int]),
        ("OpFindMultiColor", c_int, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_wchar_p, c_double, c_int, c_int_p, c_int_p]),
        ("OpFindMultiColorEx", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_wchar_p, c_double, c_int]),
        ("OpFindPic", c_int, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_wchar_p, c_double, c_int, c_int_p, c_int_p]),
        ("OpFindPicEx", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_wchar_p, c_double, c_int]),
        ("OpFindPicExS", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_wchar_p, c_double, c_int]),
        ("OpFindColorBlock", c_int, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_double, c_int, c_int, c_int, c_int_p, c_int_p]),
        ("OpFindColorBlockEx", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_double, c_int, c_int, c_int]),
        ("OpGetColor", c_wchar_p, [op_handle, c_int, c_int]),
        ("OpGetColorNum", c_int, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_double]),
        ("OpSetDisplayInput", c_int, [op_handle, c_wchar_p]),
        ("OpLoadPic", c_int, [op_handle, c_wchar_p]),
        ("OpFreePic", c_int, [op_handle, c_wchar_p]),
        ("OpLoadMemPic", c_int, [op_handle, c_wchar_p, c_void_p, c_int]),
        ("OpGetPicSize", c_int, [op_handle, c_wchar_p, c_int_p, c_int_p]),
        ("OpGetScreenData", c_size_t, [op_handle, c_int, c_int, c_int, c_int, c_int_p]),
        ("OpGetScreenDataBmp", c_size_t, [op_handle, c_int, c_int, c_int, c_int, c_int_p, c_int_p]),
        ("OpGetScreenFrameInfo", None, [op_handle, c_int_p, c_int_p]),
        ("OpMatchPicName", c_wchar_p, [op_handle, c_wchar_p]),
        ("OpCvLoadTemplate", c_int, [op_handle, c_wchar_p, c_wchar_p]),
        ("OpCvLoadMaskedTemplate", c_int, [op_handle, c_wchar_p, c_wchar_p, c_wchar_p]),
        ("OpCvRemoveTemplate", c_int, [op_handle, c_wchar_p]),
        ("OpCvRemoveAllTemplates", c_int, [op_handle]),
        ("OpCvHasTemplate", c_int, [op_handle, c_wchar_p]),
        ("OpCvGetTemplateCount", c_int, [op_handle]),
        ("OpCvGetAllTemplateNames", c_wchar_p, [op_handle]),
        ("OpCvGetOpenCvVersion", c_wchar_p, [op_handle]),
        ("OpCvLoadTemplateList", c_int, [op_handle, c_wchar_p]),
        ("OpCvToGray", c_int, [op_handle, c_wchar_p, c_wchar_p]),
        ("OpCvToBinary", c_int, [op_handle, c_wchar_p, c_wchar_p]),
        ("OpCvToEdge", c_int, [op_handle, c_wchar_p, c_wchar_p]),
        ("OpCvToOutline", c_int, [op_handle, c_wchar_p, c_wchar_p]),
        ("OpCvDenoise", c_int, [op_handle, c_wchar_p, c_wchar_p]),
        ("OpCvEqualize", c_int, [op_handle, c_wchar_p, c_wchar_p]),
        ("OpCvCLAHE", c_int, [op_handle, c_wchar_p, c_wchar_p, c_double, c_int]),
        ("OpCvBlur", c_int, [op_handle, c_wchar_p, c_wchar_p, c_wchar_p, c_int]),
        ("OpCvSharpen", c_int, [op_handle, c_wchar_p, c_wchar_p, c_double]),
        ("OpCvCropValid", c_int, [op_handle, c_wchar_p, c_wchar_p]),
        ("OpCvConnectedComponents", c_wchar_p, [op_handle, c_wchar_p, c_double]),
        ("OpCvFindContours", c_wchar_p, [op_handle, c_wchar_p, c_double]),
        ("OpCvPreprocessPipeline", c_int, [op_handle, c_wchar_p, c_wchar_p, c_wchar_p]),
        ("OpCvCrop", c_int, [op_handle, c_wchar_p, c_int, c_int, c_int, c_int, c_wchar_p]),
        ("OpCvResize", c_int, [op_handle, c_wchar_p, c_int, c_int, c_wchar_p]),
        ("OpCvThreshold", c_int, [op_handle, c_wchar_p, c_wchar_p, c_double, c_double, c_wchar_p]),
        ("OpCvInRange", c_int, [op_handle, c_wchar_p, c_wchar_p, c_wchar_p, c_wchar_p, c_wchar_p]),
        ("OpCvMorphology", c_int, [op_handle, c_wchar_p, c_wchar_p, c_wchar_p, c_int, c_int]),
        ("OpCvThin", c_int, [op_handle, c_wchar_p, c_wchar_p, c_wchar_p]),
        ("OpCvMatchTemplate", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_double, c_int, c_int, c_int, c_int]),
        ("OpCvMatchTemplateScale", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_wchar_p, c_double, c_int, c_int]),
        ("OpCvMatchAnyTemplate", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_double, c_int, c_int, c_int, c_int]),
        ("OpCvMatchAllTemplates", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_double, c_int, c_int, c_int, c_int]),
        ("OpCvFeatureMatchTemplate", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_double]),
        ("OpCvEdgeMatchTemplate", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_double]),
        ("OpCvShapeMatchTemplate", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_double]),
        ("OpSetOcrEngine", c_int, [op_handle, c_wchar_p, c_wchar_p, c_wchar_p]),
        ("OpSetYoloEngine", c_int, [op_handle, c_wchar_p, c_wchar_p, c_wchar_p]),
        ("OpYoloDetect", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_double, c_double]),
        ("OpYoloDetectFromFile", c_wchar_p, [op_handle, c_wchar_p, c_double, c_double]),
        ("OpSetDict", c_int, [op_handle, c_int, c_wchar_p]),
        ("OpGetDict", c_wchar_p, [op_handle, c_int, c_int]),
        ("OpSetMemDict", c_int, [op_handle, c_int, c_wchar_p, c_int]),
        ("OpUseDict", c_int, [op_handle, c_int]),
        ("OpAddDict", c_int, [op_handle, c_int, c_wchar_p]),
        ("OpSaveDict", c_int, [op_handle, c_int, c_wchar_p]),
        ("OpClearDict", c_int, [op_handle, c_int]),
        ("OpGetDictCount", c_int, [op_handle, c_int]),
        ("OpGetNowDict", c_int, [op_handle]),
        ("OpFetchWord", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_wchar_p]),
        ("OpGetWordsNoDict", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p]),
        ("OpGetWordResultCount", c_int, [op_handle, c_wchar_p]),
        ("OpGetWordResultPos", c_int, [op_handle, c_wchar_p, c_int, c_int_p, c_int_p]),
        ("OpGetWordResultStr", c_wchar_p, [op_handle, c_wchar_p, c_int]),
        ("OpOcr", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_double]),
        ("OpOcrEx", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_double]),
        ("OpFindStr", c_int, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_wchar_p, c_double, c_int_p, c_int_p]),
        ("OpFindStrEx", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_wchar_p, c_double]),
        ("OpOcrAuto", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_double]),
        ("OpOcrFromFile", c_wchar_p, [op_handle, c_wchar_p, c_wchar_p, c_double]),
        ("OpOcrAutoFromFile", c_wchar_p, [op_handle, c_wchar_p, c_double]),
        ("OpFindLine", c_wchar_p, [op_handle, c_int, c_int, c_int, c_int, c_wchar_p, c_double]),
        ("OpWriteData", c_int, [op_handle, intptr_t, c_wchar_p, c_wchar_p, c_int]),
        ("OpReadData", c_wchar_p, [op_handle, intptr_t, c_wchar_p, c_int]),
        ("OpReadInt", c_int, [op_handle, intptr_t, c_wchar_p, c_int, c_int64_p]),
        ("OpWriteInt", c_int, [op_handle, intptr_t, c_wchar_p, c_int, ctypes.c_int64]),
        ("OpReadFloat", c_int, [op_handle, intptr_t, c_wchar_p, c_float_p]),
        ("OpWriteFloat", c_int, [op_handle, intptr_t, c_wchar_p, c_float]),
        ("OpReadDouble", c_int, [op_handle, intptr_t, c_wchar_p, c_double_p]),
        ("OpWriteDouble", c_int, [op_handle, intptr_t, c_wchar_p, c_double]),
        ("OpReadString", c_wchar_p, [op_handle, intptr_t, c_wchar_p, c_int, c_int]),
        ("OpWriteString", c_int, [op_handle, intptr_t, c_wchar_p, c_int, c_wchar_p]),
    ]

    for name, restype, argtypes in signatures:
        _signature(dll, name, restype, argtypes)
    return dll
