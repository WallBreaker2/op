from __future__ import annotations

import ctypes
from enum import Enum
from pathlib import Path
from typing import Any

from . import constants
from ._ffi import NativeApi, load_dll
from .errors import ClosedHandleError, OpCallError
from .types import IntType, StringType


class Op:
    def __init__(
        self,
        dll_path: str | Path | None = None,
        dll_dir: str | Path | None = None,
        raise_on_error: bool = True,
    ) -> None:
        self._api: NativeApi = load_dll(dll_path=dll_path, dll_dir=dll_dir)
        self._dll = self._api.dll
        self._raise_on_error = raise_on_error
        self._handle = self._dll.OpCreate()
        if not self._handle:
            raise OpCallError("OpCreate")

    @property
    def dll_path(self) -> Path | None:
        return self._api.path

    @property
    def handle(self) -> int:
        self._check_open()
        return int(self._handle)

    @property
    def version(self) -> str:
        return self._dll.OpVer() or ""

    def close(self) -> None:
        if self._handle:
            self._dll.OpDestroy(self._handle)
            self._handle = None

    def __enter__(self) -> "Op":
        self._check_open()
        return self

    def __exit__(self, exc_type: object, exc: object, tb: object) -> None:
        self.close()

    def __del__(self) -> None:
        try:
            self.close()
        except Exception:
            pass

    def _check_open(self) -> None:
        if not self._handle:
            raise ClosedHandleError("Op handle is closed")

    def _ok(self, value: int, name: str) -> bool:
        if value:
            return True
        if self._raise_on_error:
            raise OpCallError(name)
        return False

    def _call_ok(self, name: str, *args: Any) -> bool:
        self._check_open()
        return self._ok(getattr(self._dll, name)(self._handle, *args), name)

    def _call_int(self, name: str, *args: Any) -> int:
        self._check_open()
        return int(getattr(self._dll, name)(self._handle, *args))

    def _call_intptr(self, name: str, *args: Any) -> int:
        self._check_open()
        return int(getattr(self._dll, name)(self._handle, *args))

    def _call_string(self, name: str, *args: Any) -> str:
        self._check_open()
        value = getattr(self._dll, name)(self._handle, *args)
        return value or ""

    def _call_string_with_int(self, name: str, *args: Any) -> tuple[str, int]:
        self._check_open()
        ret = ctypes.c_int()
        value = getattr(self._dll, name)(self._handle, *args, ctypes.byref(ret))
        return value or "", int(ret.value)

    @staticmethod
    def _enum_text(value: str | Enum) -> str:
        return str(value.value) if isinstance(value, Enum) else str(value)

    def _call_checked_point(self, name: str, *args: Any) -> tuple[int, int]:
        x = ctypes.c_int()
        y = ctypes.c_int()
        ok = getattr(self._dll, name)(self._handle, *args, ctypes.byref(x), ctypes.byref(y))
        self._ok(ok, name)
        return int(x.value), int(y.value)

    def _call_checked_inout_point(self, name: str, hwnd: int, x: int, y: int) -> tuple[int, int]:
        cx = ctypes.c_int(int(x))
        cy = ctypes.c_int(int(y))
        ok = getattr(self._dll, name)(self._handle, int(hwnd), ctypes.byref(cx), ctypes.byref(cy))
        self._ok(ok, name)
        return int(cx.value), int(cy.value)

    def _call_result_point(self, name: str, *args: Any) -> tuple[int, int, int]:
        x = ctypes.c_int()
        y = ctypes.c_int()
        ret = getattr(self._dll, name)(self._handle, *args, ctypes.byref(x), ctypes.byref(y))
        return int(ret), int(x.value), int(y.value)

    def _call_checked_rect(self, name: str, *args: Any) -> tuple[int, int, int, int]:
        x1 = ctypes.c_int()
        y1 = ctypes.c_int()
        x2 = ctypes.c_int()
        y2 = ctypes.c_int()
        ok = getattr(self._dll, name)(
            self._handle,
            *args,
            ctypes.byref(x1),
            ctypes.byref(y1),
            ctypes.byref(x2),
            ctypes.byref(y2),
        )
        self._ok(ok, name)
        return int(x1.value), int(y1.value), int(x2.value), int(y2.value)

    def _call_checked_size(self, name: str, *args: Any) -> tuple[int, int]:
        width = ctypes.c_int()
        height = ctypes.c_int()
        ok = getattr(self._dll, name)(self._handle, *args, ctypes.byref(width), ctypes.byref(height))
        self._ok(ok, name)
        return int(width.value), int(height.value)

    # Basic
    def set_path(self, path: str | Path) -> bool:
        return self._call_ok("OpSetPath", str(path))

    def get_path(self) -> str:
        return self._call_string("OpGetPath")

    def get_base_path(self) -> str:
        return self._call_string("OpGetBasePath")

    def get_id(self) -> int:
        return self._call_int("OpGetID")

    def get_last_error(self) -> int:
        return self._call_int("OpGetLastError")

    def set_show_error_msg(self, show_type: constants.ShowError | int) -> bool:
        return self._call_ok("OpSetShowErrorMsg", int(show_type))

    def sleep(self, milliseconds: int) -> bool:
        return self._call_ok("OpSleep", int(milliseconds))

    def inject_dll(self, process_name: str, dll_name: str) -> bool:
        return self._call_ok("OpInjectDll", process_name, dll_name)

    def enable_pic_cache(self, enable: bool | int) -> bool:
        return self._call_ok("OpEnablePicCache", int(enable))

    def capture_pre(self, file_name: str | Path) -> bool:
        return self._call_ok("OpCapturePre", str(file_name))

    def set_screen_data_mode(self, mode: constants.ScreenDataMode | int) -> bool:
        return self._call_ok("OpSetScreenDataMode", int(mode))

    # Algorithm
    def a_star_find_path(
        self,
        map_width: int,
        map_height: int,
        disable_points: str,
        begin_x: int,
        begin_y: int,
        end_x: int,
        end_y: int,
    ) -> str:
        return self._call_string(
            "OpAStarFindPath",
            int(map_width),
            int(map_height),
            disable_points,
            int(begin_x),
            int(begin_y),
            int(end_x),
            int(end_y),
        )

    def find_nearest_pos(self, all_pos: str, pos_type: constants.NearestPosType | int, x: int, y: int) -> str:
        return self._call_string("OpFindNearestPos", all_pos, int(pos_type), int(x), int(y))

    # Window and process
    def enum_window(
        self,
        parent: int = 0,
        title: str = "",
        class_name: str = "",
        filter: constants.WindowEnumFilter | int = constants.WindowEnumFilter.ALL,
    ) -> str:
        return self._call_string("OpEnumWindow", int(parent), title, class_name, int(filter))

    def enum_window_by_process(
        self,
        process_name: str,
        title: str = "",
        class_name: str = "",
        filter: constants.WindowEnumFilter | int = constants.WindowEnumFilter.ALL,
    ) -> str:
        return self._call_string("OpEnumWindowByProcess", process_name, title, class_name, int(filter))

    def enum_process(self, name: str = "") -> str:
        return self._call_string("OpEnumProcess", name)

    def client_to_screen(self, hwnd: int, x: int, y: int) -> tuple[int, int]:
        return self._call_checked_inout_point("OpClientToScreen", hwnd, x, y)

    def find_window(self, class_name: str = "", title: str = "") -> int:
        return self._call_intptr("OpFindWindow", class_name, title)

    def find_window_by_process(self, process_name: str, class_name: str = "", title: str = "") -> int:
        return self._call_intptr("OpFindWindowByProcess", process_name, class_name, title)

    def find_window_by_process_id(self, process_id: int, class_name: str = "", title: str = "") -> int:
        return self._call_intptr("OpFindWindowByProcessId", int(process_id), class_name, title)

    def find_window_ex(self, parent: int, class_name: str = "", title: str = "") -> int:
        return self._call_intptr("OpFindWindowEx", int(parent), class_name, title)

    def get_client_rect(self, hwnd: int) -> tuple[int, int, int, int]:
        return self._call_checked_rect("OpGetClientRect", int(hwnd))

    def get_client_size(self, hwnd: int) -> tuple[int, int]:
        return self._call_checked_size("OpGetClientSize", int(hwnd))

    def get_foreground_focus(self) -> int:
        return self._call_intptr("OpGetForegroundFocus")

    def get_foreground_window(self) -> int:
        return self._call_intptr("OpGetForegroundWindow")

    def get_mouse_point_window(self) -> int:
        return self._call_intptr("OpGetMousePointWindow")

    def get_point_window(self, x: int, y: int) -> int:
        return self._call_intptr("OpGetPointWindow", int(x), int(y))

    def get_process_info(self, pid: int) -> str:
        return self._call_string("OpGetProcessInfo", int(pid))

    def get_special_window(self, flag: constants.SpecialWindow | int) -> int:
        return self._call_intptr("OpGetSpecialWindow", int(flag))

    def get_window(self, hwnd: int, flag: constants.WindowRelation | int) -> int:
        return self._call_intptr("OpGetWindow", int(hwnd), int(flag))

    def get_window_class(self, hwnd: int) -> str:
        return self._call_string("OpGetWindowClass", int(hwnd))

    def get_window_process_id(self, hwnd: int) -> int:
        return self._call_int("OpGetWindowProcessId", int(hwnd))

    def get_window_process_path(self, hwnd: int) -> str:
        return self._call_string("OpGetWindowProcessPath", int(hwnd))

    def get_window_rect(self, hwnd: int) -> tuple[int, int, int, int]:
        return self._call_checked_rect("OpGetWindowRect", int(hwnd))

    def get_window_state(self, hwnd: int, flag: constants.WindowState | int) -> int:
        return self._call_int("OpGetWindowState", int(hwnd), int(flag))

    def get_window_title(self, hwnd: int) -> str:
        return self._call_string("OpGetWindowTitle", int(hwnd))

    def move_window(self, hwnd: int, x: int, y: int) -> bool:
        return self._call_ok("OpMoveWindow", int(hwnd), int(x), int(y))

    def screen_to_client(self, hwnd: int, x: int, y: int) -> tuple[int, int]:
        return self._call_checked_inout_point("OpScreenToClient", hwnd, x, y)

    def send_paste(self, hwnd: int) -> bool:
        return self._call_ok("OpSendPaste", int(hwnd))

    def set_client_size(self, hwnd: int, width: int, height: int) -> bool:
        return self._call_ok("OpSetClientSize", int(hwnd), int(width), int(height))

    def set_window_state(self, hwnd: int, flag: constants.WindowStateAction | int) -> bool:
        return self._call_ok("OpSetWindowState", int(hwnd), int(flag))

    def set_window_size(self, hwnd: int, width: int, height: int) -> bool:
        return self._call_ok("OpSetWindowSize", int(hwnd), int(width), int(height))

    def layout_windows(
        self,
        hwnds: str,
        layout_type: constants.LayoutType | int,
        columns: int,
        start_x: int,
        start_y: int,
        gap_x: int,
        gap_y: int,
        size_mode: constants.LayoutSize | int,
        window_width: int,
        window_height: int,
        anchor_mode: constants.LayoutAnchor | int,
    ) -> bool:
        return self._call_ok(
            "OpLayoutWindows",
            hwnds,
            int(layout_type),
            int(columns),
            int(start_x),
            int(start_y),
            int(gap_x),
            int(gap_y),
            int(size_mode),
            int(window_width),
            int(window_height),
            int(anchor_mode),
        )

    def set_window_text(self, hwnd: int, title: str) -> bool:
        return self._call_ok("OpSetWindowText", int(hwnd), title)

    def set_window_transparent(self, hwnd: int, trans: int) -> bool:
        return self._call_ok("OpSetWindowTransparent", int(hwnd), int(trans))

    def send_string(self, hwnd: int, value: str) -> bool:
        return self._call_ok("OpSendString", int(hwnd), value)

    def send_string_ime(self, hwnd: int, value: str) -> bool:
        return self._call_ok("OpSendStringIme", int(hwnd), value)

    def run_app(self, cmdline: str, mode: int = 1) -> int:
        self._check_open()
        pid = ctypes.c_uint32()
        ok = self._dll.OpRunApp(self._handle, cmdline, int(mode), ctypes.byref(pid))
        self._ok(ok, "OpRunApp")
        return int(pid.value)

    def win_exec(self, cmdline: str, cmdshow: int = 1) -> bool:
        return self._call_ok("OpWinExec", cmdline, int(cmdshow))

    def get_cmd_str(self, cmd: str, milliseconds: int) -> str:
        return self._call_string("OpGetCmdStr", cmd, int(milliseconds))

    def set_clipboard(self, value: str) -> bool:
        return self._call_ok("OpSetClipboard", value)

    def get_clipboard(self) -> str:
        return self._call_string("OpGetClipboard")

    def delay(self, milliseconds: int) -> bool:
        return self._call_ok("OpDelay", int(milliseconds))

    def delays(self, min_milliseconds: int, max_milliseconds: int) -> bool:
        return self._call_ok("OpDelays", int(min_milliseconds), int(max_milliseconds))

    # Background binding
    def bind_window(
        self,
        hwnd: int,
        display: constants.DisplayMode | str = constants.DisplayMode.NORMAL,
        mouse: constants.MouseMode | str = constants.MouseMode.NORMAL,
        keypad: constants.KeypadMode | str = constants.KeypadMode.NORMAL,
        mode: int = 0,
    ) -> bool:
        return self._call_ok(
            "OpBindWindow",
            int(hwnd),
            self._enum_text(display),
            self._enum_text(mouse),
            self._enum_text(keypad),
            int(mode),
        )

    def bind_window_ex(
        self,
        display_hwnd: int,
        input_hwnd: int,
        display: constants.DisplayMode | str = constants.DisplayMode.NORMAL,
        mouse: constants.MouseMode | str = constants.MouseMode.NORMAL,
        keypad: constants.KeypadMode | str = constants.KeypadMode.NORMAL,
        mode: int = 0,
    ) -> bool:
        return self._call_ok(
            "OpBindWindowEx",
            int(display_hwnd),
            int(input_hwnd),
            self._enum_text(display),
            self._enum_text(mouse),
            self._enum_text(keypad),
            int(mode),
        )

    def unbind_window(self) -> bool:
        return self._call_ok("OpUnBindWindow")

    def get_bind_window(self) -> int:
        return self._call_intptr("OpGetBindWindow")

    def is_bind(self) -> bool:
        return bool(self._call_int("OpIsBind"))

    # Mouse
    def get_cursor_pos(self) -> tuple[int, int]:
        return self._call_checked_point("OpGetCursorPos")

    def get_cursor_shape(self) -> str:
        return self._call_string("OpGetCursorShape")

    def move_r(self, x: int, y: int) -> bool:
        return self._call_ok("OpMoveR", int(x), int(y))

    def move_to(self, x: int, y: int) -> bool:
        return self._call_ok("OpMoveTo", int(x), int(y))

    def move_to_ex(self, x: int, y: int, width: int, height: int) -> str:
        return self._call_string("OpMoveToEx", int(x), int(y), int(width), int(height))

    def left_click(self) -> bool:
        return self._call_ok("OpLeftClick")

    def left_double_click(self) -> bool:
        return self._call_ok("OpLeftDoubleClick")

    def left_down(self) -> bool:
        return self._call_ok("OpLeftDown")

    def left_up(self) -> bool:
        return self._call_ok("OpLeftUp")

    def middle_click(self) -> bool:
        return self._call_ok("OpMiddleClick")

    def middle_double_click(self) -> bool:
        return self._call_ok("OpMiddleDoubleClick")

    def middle_down(self) -> bool:
        return self._call_ok("OpMiddleDown")

    def middle_up(self) -> bool:
        return self._call_ok("OpMiddleUp")

    def right_click(self) -> bool:
        return self._call_ok("OpRightClick")

    def right_double_click(self) -> bool:
        return self._call_ok("OpRightDoubleClick")

    def right_down(self) -> bool:
        return self._call_ok("OpRightDown")

    def right_up(self) -> bool:
        return self._call_ok("OpRightUp")

    def xbutton1_click(self) -> bool:
        return self._call_ok("OpXButton1Click")

    def xbutton1_double_click(self) -> bool:
        return self._call_ok("OpXButton1DoubleClick")

    def xbutton1_down(self) -> bool:
        return self._call_ok("OpXButton1Down")

    def xbutton1_up(self) -> bool:
        return self._call_ok("OpXButton1Up")

    def xbutton2_click(self) -> bool:
        return self._call_ok("OpXButton2Click")

    def xbutton2_double_click(self) -> bool:
        return self._call_ok("OpXButton2DoubleClick")

    def xbutton2_down(self) -> bool:
        return self._call_ok("OpXButton2Down")

    def xbutton2_up(self) -> bool:
        return self._call_ok("OpXButton2Up")

    def wheel(self, delta: int) -> bool:
        return self._call_ok("OpWheel", int(delta))

    def hwheel(self, delta: int) -> bool:
        return self._call_ok("OpHWheel", int(delta))

    def wheel_down(self) -> bool:
        return self._call_ok("OpWheelDown")

    def wheel_up(self) -> bool:
        return self._call_ok("OpWheelUp")

    def set_mouse_delay(self, delay_type: constants.MouseDelayType | str, delay: int) -> bool:
        return self._call_ok("OpSetMouseDelay", self._enum_text(delay_type), int(delay))

    # Keyboard
    def get_key_state(self, vk_code: int) -> int:
        return self._call_int("OpGetKeyState", int(vk_code))

    def key_down(self, vk_code: int) -> bool:
        return self._call_ok("OpKeyDown", int(vk_code))

    def key_down_char(self, vk_code: str) -> bool:
        return self._call_ok("OpKeyDownChar", vk_code)

    def key_up(self, vk_code: int) -> bool:
        return self._call_ok("OpKeyUp", int(vk_code))

    def key_up_char(self, vk_code: str) -> bool:
        return self._call_ok("OpKeyUpChar", vk_code)

    def wait_key(self, vk_code: int, time_out: int) -> int:
        return self._call_int("OpWaitKey", int(vk_code), int(time_out))

    def key_press(self, vk_code: int) -> bool:
        return self._call_ok("OpKeyPress", int(vk_code))

    def key_press_char(self, vk_code: str) -> bool:
        return self._call_ok("OpKeyPressChar", vk_code)

    def set_keypad_delay(self, delay_type: constants.KeypadDelayType | str, delay: int) -> bool:
        return self._call_ok("OpSetKeypadDelay", self._enum_text(delay_type), int(delay))

    def key_press_str(self, key_str: str, delay: int) -> bool:
        return self._call_ok("OpKeyPressStr", key_str, int(delay))

    # Image and color
    def capture(self, x1: int, y1: int, x2: int, y2: int, file_name: str | Path) -> bool:
        return self._call_ok("OpCapture", int(x1), int(y1), int(x2), int(y2), str(file_name))

    def cmp_color(self, x: int, y: int, color: str, sim: float) -> bool:
        return bool(self._call_int("OpCmpColor", int(x), int(y), color, float(sim)))

    def find_color(
        self,
        x1: int,
        y1: int,
        x2: int,
        y2: int,
        color: str,
        sim: float,
        direction: constants.FindDirection | int = constants.FindDirection.LEFT_TOP_TO_RIGHT_BOTTOM,
    ) -> tuple[int, int, int]:
        return self._call_result_point(
            "OpFindColor",
            int(x1),
            int(y1),
            int(x2),
            int(y2),
            color,
            float(sim),
            int(direction),
        )

    def find_color_ex(
        self,
        x1: int,
        y1: int,
        x2: int,
        y2: int,
        color: str,
        sim: float,
        direction: constants.FindDirection | int = constants.FindDirection.LEFT_TOP_TO_RIGHT_BOTTOM,
    ) -> str:
        return self._call_string("OpFindColorEx", int(x1), int(y1), int(x2), int(y2), color, float(sim), int(direction))

    def find_multi_color(
        self,
        x1: int,
        y1: int,
        x2: int,
        y2: int,
        first_color: str,
        offset_color: str,
        sim: float,
        direction: constants.FindDirection | int = constants.FindDirection.LEFT_TOP_TO_RIGHT_BOTTOM,
    ) -> tuple[int, int, int]:
        return self._call_result_point(
            "OpFindMultiColor",
            int(x1),
            int(y1),
            int(x2),
            int(y2),
            first_color,
            offset_color,
            float(sim),
            int(direction),
        )

    def find_multi_color_ex(
        self,
        x1: int,
        y1: int,
        x2: int,
        y2: int,
        first_color: str,
        offset_color: str,
        sim: float,
        direction: constants.FindDirection | int = constants.FindDirection.LEFT_TOP_TO_RIGHT_BOTTOM,
    ) -> str:
        return self._call_string(
            "OpFindMultiColorEx",
            int(x1),
            int(y1),
            int(x2),
            int(y2),
            first_color,
            offset_color,
            float(sim),
            int(direction),
        )

    def find_pic(
        self,
        x1: int,
        y1: int,
        x2: int,
        y2: int,
        files: str,
        delta_color: str,
        sim: float,
        direction: constants.FindDirection | int = constants.FindDirection.LEFT_TOP_TO_RIGHT_BOTTOM,
    ) -> tuple[int, int, int]:
        return self._call_result_point(
            "OpFindPic",
            int(x1),
            int(y1),
            int(x2),
            int(y2),
            files,
            delta_color,
            float(sim),
            int(direction),
        )

    def find_pic_ex(
        self,
        x1: int,
        y1: int,
        x2: int,
        y2: int,
        files: str,
        delta_color: str,
        sim: float,
        direction: constants.FindDirection | int = constants.FindDirection.LEFT_TOP_TO_RIGHT_BOTTOM,
    ) -> str:
        return self._call_string(
            "OpFindPicEx",
            int(x1),
            int(y1),
            int(x2),
            int(y2),
            files,
            delta_color,
            float(sim),
            int(direction),
        )

    def find_pic_ex_s(
        self,
        x1: int,
        y1: int,
        x2: int,
        y2: int,
        files: str,
        delta_color: str,
        sim: float,
        direction: constants.FindDirection | int = constants.FindDirection.LEFT_TOP_TO_RIGHT_BOTTOM,
    ) -> str:
        return self._call_string(
            "OpFindPicExS",
            int(x1),
            int(y1),
            int(x2),
            int(y2),
            files,
            delta_color,
            float(sim),
            int(direction),
        )

    def find_color_block(
        self,
        x1: int,
        y1: int,
        x2: int,
        y2: int,
        color: str,
        sim: float,
        count: int,
        height: int,
        width: int,
    ) -> tuple[int, int, int]:
        return self._call_result_point(
            "OpFindColorBlock",
            int(x1),
            int(y1),
            int(x2),
            int(y2),
            color,
            float(sim),
            int(count),
            int(height),
            int(width),
        )

    def find_color_block_ex(
        self,
        x1: int,
        y1: int,
        x2: int,
        y2: int,
        color: str,
        sim: float,
        count: int,
        height: int,
        width: int,
    ) -> str:
        return self._call_string(
            "OpFindColorBlockEx",
            int(x1),
            int(y1),
            int(x2),
            int(y2),
            color,
            float(sim),
            int(count),
            int(height),
            int(width),
        )

    def get_color(self, x: int, y: int) -> str:
        return self._call_string("OpGetColor", int(x), int(y))

    def get_color_num(self, x1: int, y1: int, x2: int, y2: int, color: str, sim: float) -> int:
        return self._call_int("OpGetColorNum", int(x1), int(y1), int(x2), int(y2), color, float(sim))

    def set_display_input(self, mode: constants.DisplayInputMode | str) -> bool:
        return self._call_ok("OpSetDisplayInput", self._enum_text(mode))

    def load_pic(self, file_name: str | Path) -> bool:
        return self._call_ok("OpLoadPic", str(file_name))

    def free_pic(self, file_name: str | Path) -> bool:
        return self._call_ok("OpFreePic", str(file_name))

    def load_mem_pic(self, file_name: str, data: bytes | bytearray | memoryview | int, size: int | None = None) -> bool:
        self._check_open()
        if isinstance(data, int):
            if size is None:
                raise ValueError("size is required when data is a pointer")
            ptr = ctypes.c_void_p(data)
            data_size = int(size)
        else:
            raw = bytes(data)
            buffer = ctypes.create_string_buffer(raw)
            ptr = ctypes.cast(buffer, ctypes.c_void_p)
            data_size = len(raw) if size is None else int(size)
        ok = self._dll.OpLoadMemPic(self._handle, file_name, ptr, data_size)
        return self._ok(ok, "OpLoadMemPic")

    def get_pic_size(self, pic_name: str) -> tuple[int, int]:
        return self._call_checked_size("OpGetPicSize", pic_name)

    def get_screen_data(self, x1: int, y1: int, x2: int, y2: int) -> int:
        self._check_open()
        ret = ctypes.c_int()
        ptr = self._dll.OpGetScreenData(self._handle, int(x1), int(y1), int(x2), int(y2), ctypes.byref(ret))
        self._ok(ret.value, "OpGetScreenData")
        return int(ptr)

    def get_screen_data_bmp(self, x1: int, y1: int, x2: int, y2: int) -> tuple[int, int]:
        self._check_open()
        size = ctypes.c_int()
        ret = ctypes.c_int()
        ptr = self._dll.OpGetScreenDataBmp(
            self._handle,
            int(x1),
            int(y1),
            int(x2),
            int(y2),
            ctypes.byref(size),
            ctypes.byref(ret),
        )
        self._ok(ret.value, "OpGetScreenDataBmp")
        return int(ptr), int(size.value)

    def get_screen_frame_info(self) -> tuple[int, int]:
        self._check_open()
        frame_id = ctypes.c_int()
        frame_time = ctypes.c_int()
        self._dll.OpGetScreenFrameInfo(self._handle, ctypes.byref(frame_id), ctypes.byref(frame_time))
        return int(frame_id.value), int(frame_time.value)

    def match_pic_name(self, pic_name: str) -> str:
        return self._call_string("OpMatchPicName", pic_name)

    # OpenCV
    def cv_load_template(self, name: str, file_path: str | Path) -> bool:
        return self._call_ok("OpCvLoadTemplate", name, str(file_path))

    def cv_load_masked_template(self, name: str, template_path: str | Path, mask_path: str | Path) -> bool:
        return self._call_ok("OpCvLoadMaskedTemplate", name, str(template_path), str(mask_path))

    def cv_remove_template(self, name: str) -> bool:
        return self._call_ok("OpCvRemoveTemplate", name)

    def cv_remove_all_templates(self) -> bool:
        return self._call_ok("OpCvRemoveAllTemplates")

    def cv_has_template(self, name: str) -> bool:
        return bool(self._call_int("OpCvHasTemplate", name))

    def cv_get_template_count(self) -> int:
        return self._call_int("OpCvGetTemplateCount")

    def cv_get_all_template_names(self) -> str:
        return self._call_string("OpCvGetAllTemplateNames")

    def cv_get_open_cv_version(self) -> str:
        return self._call_string("OpCvGetOpenCvVersion")

    def cv_load_template_list(self, template_list: str) -> bool:
        return self._call_ok("OpCvLoadTemplateList", template_list)

    def cv_to_gray(self, src_file: str | Path, dst_file: str | Path) -> bool:
        return self._call_ok("OpCvToGray", str(src_file), str(dst_file))

    def cv_to_binary(self, src_file: str | Path, dst_file: str | Path) -> bool:
        return self._call_ok("OpCvToBinary", str(src_file), str(dst_file))

    def cv_to_edge(self, src_file: str | Path, dst_file: str | Path) -> bool:
        return self._call_ok("OpCvToEdge", str(src_file), str(dst_file))

    def cv_to_outline(self, src_file: str | Path, dst_file: str | Path) -> bool:
        return self._call_ok("OpCvToOutline", str(src_file), str(dst_file))

    def cv_denoise(self, src_file: str | Path, dst_file: str | Path) -> bool:
        return self._call_ok("OpCvDenoise", str(src_file), str(dst_file))

    def cv_equalize(self, src_file: str | Path, dst_file: str | Path) -> bool:
        return self._call_ok("OpCvEqualize", str(src_file), str(dst_file))

    def cv_clahe(self, src_file: str | Path, dst_file: str | Path, clip_limit: float, tile_grid_size: int) -> bool:
        return self._call_ok("OpCvCLAHE", str(src_file), str(dst_file), float(clip_limit), int(tile_grid_size))

    def cv_blur(
        self,
        src_file: str | Path,
        dst_file: str | Path,
        mode: constants.Blur | str = constants.Blur.GAUSSIAN,
        kernel_size: int = 3,
    ) -> bool:
        return self._call_ok("OpCvBlur", str(src_file), str(dst_file), self._enum_text(mode), int(kernel_size))

    def cv_sharpen(self, src_file: str | Path, dst_file: str | Path, strength: float) -> bool:
        return self._call_ok("OpCvSharpen", str(src_file), str(dst_file), float(strength))

    def cv_crop_valid(self, src_file: str | Path, dst_file: str | Path) -> bool:
        return self._call_ok("OpCvCropValid", str(src_file), str(dst_file))

    def cv_connected_components(self, src_file: str | Path, min_area: float) -> str:
        return self._call_string("OpCvConnectedComponents", str(src_file), float(min_area))

    def cv_find_contours(self, src_file: str | Path, min_area: float) -> str:
        return self._call_string("OpCvFindContours", str(src_file), float(min_area))

    def cv_preprocess_pipeline(self, src_file: str | Path, dst_file: str | Path, pipeline: str) -> bool:
        return self._call_ok("OpCvPreprocessPipeline", str(src_file), str(dst_file), pipeline)

    def cv_crop(self, src_file: str | Path, x: int, y: int, width: int, height: int, dst_file: str | Path) -> bool:
        return self._call_ok("OpCvCrop", str(src_file), int(x), int(y), int(width), int(height), str(dst_file))

    def cv_resize(self, src_file: str | Path, width: int, height: int, dst_file: str | Path) -> bool:
        return self._call_ok("OpCvResize", str(src_file), int(width), int(height), str(dst_file))

    def cv_threshold(
        self,
        src_file: str | Path,
        dst_file: str | Path,
        threshold: float,
        max_value: float,
        mode: constants.Threshold | str = constants.Threshold.BINARY,
    ) -> bool:
        return self._call_ok(
            "OpCvThreshold",
            str(src_file),
            str(dst_file),
            float(threshold),
            float(max_value),
            self._enum_text(mode),
        )

    def cv_in_range(
        self,
        src_file: str | Path,
        dst_file: str | Path,
        color_space: constants.ColorSpace | str,
        lower: str,
        upper: str,
    ) -> bool:
        return self._call_ok("OpCvInRange", str(src_file), str(dst_file), self._enum_text(color_space), lower, upper)

    def cv_morphology(
        self,
        src_file: str | Path,
        dst_file: str | Path,
        mode: constants.Morphology | str = constants.Morphology.OPEN,
        kernel_size: int = 3,
        iterations: int = 1,
    ) -> bool:
        return self._call_ok(
            "OpCvMorphology",
            str(src_file),
            str(dst_file),
            self._enum_text(mode),
            int(kernel_size),
            int(iterations),
        )

    def cv_thin(
        self,
        src_file: str | Path,
        dst_file: str | Path,
        mode: constants.Thin | str = constants.Thin.ZHANG_SUEN,
    ) -> bool:
        return self._call_ok("OpCvThin", str(src_file), str(dst_file), self._enum_text(mode))

    def cv_match_template(
        self,
        x: int,
        y: int,
        width: int,
        height: int,
        template_name: str,
        threshold: float,
        direction: constants.SearchDirection | int = constants.SearchDirection.LEFT_TO_RIGHT,
        strip_mode: constants.Strip | int = constants.Strip.NONE,
        method: constants.MatchMethod | int = constants.MatchMethod.SQDIFF_NORMED,
        color_mode: constants.MatchColor | int = constants.MatchColor.GRAY,
    ) -> str:
        return self._call_string(
            "OpCvMatchTemplate",
            int(x),
            int(y),
            int(width),
            int(height),
            template_name,
            float(threshold),
            int(direction),
            int(strip_mode),
            int(method),
            int(color_mode),
        )

    def cv_match_template_scale(
        self,
        x: int,
        y: int,
        width: int,
        height: int,
        template_name: str,
        scales: str,
        threshold: float,
        method: constants.MatchMethod | int = constants.MatchMethod.SQDIFF_NORMED,
        color_mode: constants.MatchColor | int = constants.MatchColor.GRAY,
    ) -> str:
        return self._call_string(
            "OpCvMatchTemplateScale",
            int(x),
            int(y),
            int(width),
            int(height),
            template_name,
            scales,
            float(threshold),
            int(method),
            int(color_mode),
        )

    def cv_match_any_template(
        self,
        x: int,
        y: int,
        width: int,
        height: int,
        template_names: str,
        threshold: float,
        direction: constants.SearchDirection | int = constants.SearchDirection.LEFT_TO_RIGHT,
        strip_mode: constants.Strip | int = constants.Strip.NONE,
        method: constants.MatchMethod | int = constants.MatchMethod.SQDIFF_NORMED,
        color_mode: constants.MatchColor | int = constants.MatchColor.GRAY,
    ) -> str:
        return self._call_string(
            "OpCvMatchAnyTemplate",
            int(x),
            int(y),
            int(width),
            int(height),
            template_names,
            float(threshold),
            int(direction),
            int(strip_mode),
            int(method),
            int(color_mode),
        )

    def cv_match_all_templates(
        self,
        x: int,
        y: int,
        width: int,
        height: int,
        template_names: str,
        threshold: float,
        direction: constants.SearchDirection | int = constants.SearchDirection.LEFT_TO_RIGHT,
        strip_mode: constants.Strip | int = constants.Strip.NONE,
        method: constants.MatchMethod | int = constants.MatchMethod.SQDIFF_NORMED,
        color_mode: constants.MatchColor | int = constants.MatchColor.GRAY,
    ) -> str:
        return self._call_string(
            "OpCvMatchAllTemplates",
            int(x),
            int(y),
            int(width),
            int(height),
            template_names,
            float(threshold),
            int(direction),
            int(strip_mode),
            int(method),
            int(color_mode),
        )

    def cv_feature_match_template(self, x: int, y: int, width: int, height: int, template_name: str, threshold: float) -> str:
        return self._call_string(
            "OpCvFeatureMatchTemplate",
            int(x),
            int(y),
            int(width),
            int(height),
            template_name,
            float(threshold),
        )

    def cv_edge_match_template(self, x: int, y: int, width: int, height: int, template_name: str, threshold: float) -> str:
        return self._call_string(
            "OpCvEdgeMatchTemplate",
            int(x),
            int(y),
            int(width),
            int(height),
            template_name,
            float(threshold),
        )

    def cv_shape_match_template(self, x: int, y: int, width: int, height: int, template_name: str, threshold: float) -> str:
        return self._call_string(
            "OpCvShapeMatchTemplate",
            int(x),
            int(y),
            int(width),
            int(height),
            template_name,
            float(threshold),
        )

    # OCR
    def set_ocr_engine(self, path_of_engine: str | Path, dll_name: str, argv: str) -> bool:
        return self._call_ok("OpSetOcrEngine", str(path_of_engine), dll_name, argv)

    def set_yolo_engine(self, path_of_engine: str | Path, dll_name: str, argv: str) -> bool:
        return self._call_ok("OpSetYoloEngine", str(path_of_engine), dll_name, argv)

    def yolo_detect(self, x1: int, y1: int, x2: int, y2: int, conf: float, iou: float) -> str:
        return self._call_string("OpYoloDetect", int(x1), int(y1), int(x2), int(y2), float(conf), float(iou))

    def yolo_detect_from_file(self, file_name: str | Path, conf: float, iou: float) -> str:
        return self._call_string("OpYoloDetectFromFile", str(file_name), float(conf), float(iou))

    def set_dict(self, idx: int, file_name: str | Path) -> bool:
        return self._call_ok("OpSetDict", int(idx), str(file_name))

    def get_dict(self, idx: int, font_index: int) -> str:
        return self._call_string("OpGetDict", int(idx), int(font_index))

    def set_mem_dict(self, idx: int, data: str, size: int | None = None) -> bool:
        data_size = len(data) if size is None else int(size)
        return self._call_ok("OpSetMemDict", int(idx), data, data_size)

    def use_dict(self, idx: int) -> bool:
        return self._call_ok("OpUseDict", int(idx))

    def add_dict(self, idx: int, dict_info: str) -> bool:
        return self._call_ok("OpAddDict", int(idx), dict_info)

    def save_dict(self, idx: int, file_name: str | Path) -> bool:
        return self._call_ok("OpSaveDict", int(idx), str(file_name))

    def clear_dict(self, idx: int) -> bool:
        return self._call_ok("OpClearDict", int(idx))

    def get_dict_count(self, idx: int) -> int:
        return self._call_int("OpGetDictCount", int(idx))

    def get_now_dict(self) -> int:
        return self._call_int("OpGetNowDict")

    def set_binary_preprocess(
        self,
        mode: int,
        isolated_threshold: int = 1,
        min_component_area: int = 2,
        bridge_gap: int = 0,
    ) -> bool:
        return self._call_ok(
            "OpSetBinaryPreprocess",
            int(mode),
            int(isolated_threshold),
            int(min_component_area),
            int(bridge_gap),
        )

    def get_binary_preprocess(self) -> tuple[int, int, int, int]:
        self._check_open()
        mode = ctypes.c_int()
        isolated_threshold = ctypes.c_int()
        min_component_area = ctypes.c_int()
        bridge_gap = ctypes.c_int()
        ok = self._dll.OpGetBinaryPreprocess(
            self._handle,
            ctypes.byref(mode),
            ctypes.byref(isolated_threshold),
            ctypes.byref(min_component_area),
            ctypes.byref(bridge_gap),
        )
        self._ok(ok, "OpGetBinaryPreprocess")
        return int(mode.value), int(isolated_threshold.value), int(min_component_area.value), int(bridge_gap.value)

    def fetch_word(self, x1: int, y1: int, x2: int, y2: int, color: str, word: str) -> str:
        return self._call_string("OpFetchWord", int(x1), int(y1), int(x2), int(y2), color, word)

    def fetch_word_ex(self, x1: int, y1: int, x2: int, y2: int, color: str, sim: float, word: str) -> str:
        return self._call_string("OpFetchWordEx", int(x1), int(y1), int(x2), int(y2), color, float(sim), word)

    def extract_word_rects(self, x1: int, y1: int, x2: int, y2: int, color: str, sim: float, min_word_h: int) -> str:
        return self._call_string(
            "OpExtractWordRects",
            int(x1),
            int(y1),
            int(x2),
            int(y2),
            color,
            float(sim),
            int(min_word_h),
        )

    def extract_word_rects_ex(
        self,
        x1: int,
        y1: int,
        x2: int,
        y2: int,
        color: str,
        sim: float,
        min_word_w: int,
        min_word_h: int,
        padding: int,
    ) -> str:
        return self._call_string(
            "OpExtractWordRectsEx",
            int(x1),
            int(y1),
            int(x2),
            int(y2),
            color,
            float(sim),
            int(min_word_w),
            int(min_word_h),
            int(padding),
        )

    def fetch_words(
        self,
        x1: int,
        y1: int,
        x2: int,
        y2: int,
        color: str,
        sim: float,
        words: str,
        min_word_h: int,
    ) -> str:
        return self._call_string(
            "OpFetchWords",
            int(x1),
            int(y1),
            int(x2),
            int(y2),
            color,
            float(sim),
            words,
            int(min_word_h),
        )

    def fetch_words_ex(
        self,
        x1: int,
        y1: int,
        x2: int,
        y2: int,
        color: str,
        sim: float,
        words: str,
        min_word_w: int,
        min_word_h: int,
        padding: int,
    ) -> str:
        return self._call_string(
            "OpFetchWordsEx",
            int(x1),
            int(y1),
            int(x2),
            int(y2),
            color,
            float(sim),
            words,
            int(min_word_w),
            int(min_word_h),
            int(padding),
        )

    def fetch_words_by_rects(
        self,
        x1: int,
        y1: int,
        x2: int,
        y2: int,
        color: str,
        sim: float,
        words: str,
        rects: str,
    ) -> str:
        return self._call_string(
            "OpFetchWordsByRects",
            int(x1),
            int(y1),
            int(x2),
            int(y2),
            color,
            float(sim),
            words,
            rects,
        )

    def get_binary_preview(
        self,
        x1: int,
        y1: int,
        x2: int,
        y2: int,
        color: str,
        sim: float,
    ) -> tuple[str, int]:
        return self._call_string_with_int(
            "OpGetBinaryPreview",
            int(x1),
            int(y1),
            int(x2),
            int(y2),
            color,
            float(sim),
        )

    def get_word_preview(self, dict_info: str) -> tuple[str, int]:
        return self._call_string_with_int("OpGetWordPreview", dict_info)

    def check_word_dict(self, dict_info: str) -> tuple[str, int]:
        return self._call_string_with_int("OpCheckWordDict", dict_info)

    def normalize_word_dict(self, dict_info: str) -> tuple[str, int]:
        return self._call_string_with_int("OpNormalizeWordDict", dict_info)

    def rename_word_dict(self, dict_info: str, words: str) -> tuple[str, int]:
        return self._call_string_with_int("OpRenameWordDict", dict_info, words)

    def get_words_no_dict(self, x1: int, y1: int, x2: int, y2: int, color: str) -> str:
        return self._call_string("OpGetWordsNoDict", int(x1), int(y1), int(x2), int(y2), color)

    def get_word_result_count(self, result: str) -> int:
        return self._call_int("OpGetWordResultCount", result)

    def get_word_result_pos(self, result: str, index: int) -> tuple[int, int, int]:
        return self._call_result_point("OpGetWordResultPos", result, int(index))

    def get_word_result_str(self, result: str, index: int) -> str:
        return self._call_string("OpGetWordResultStr", result, int(index))

    def ocr(self, x1: int, y1: int, x2: int, y2: int, color: str, sim: float) -> str:
        return self._call_string("OpOcr", int(x1), int(y1), int(x2), int(y2), color, float(sim))

    def ocr_ex(self, x1: int, y1: int, x2: int, y2: int, color: str, sim: float) -> str:
        return self._call_string("OpOcrEx", int(x1), int(y1), int(x2), int(y2), color, float(sim))

    def find_str(self, x1: int, y1: int, x2: int, y2: int, strs: str, color: str, sim: float) -> tuple[int, int, int]:
        return self._call_result_point("OpFindStr", int(x1), int(y1), int(x2), int(y2), strs, color, float(sim))

    def find_str_ex(self, x1: int, y1: int, x2: int, y2: int, strs: str, color: str, sim: float) -> str:
        return self._call_string("OpFindStrEx", int(x1), int(y1), int(x2), int(y2), strs, color, float(sim))

    def ocr_auto(self, x1: int, y1: int, x2: int, y2: int, sim: float) -> str:
        return self._call_string("OpOcrAuto", int(x1), int(y1), int(x2), int(y2), float(sim))

    def ocr_from_file(self, file_name: str | Path, color_format: str, sim: float) -> str:
        return self._call_string("OpOcrFromFile", str(file_name), color_format, float(sim))

    def ocr_auto_from_file(self, file_name: str | Path, sim: float) -> str:
        return self._call_string("OpOcrAutoFromFile", str(file_name), float(sim))

    def find_line(self, x1: int, y1: int, x2: int, y2: int, color: str, sim: float) -> str:
        return self._call_string("OpFindLine", int(x1), int(y1), int(x2), int(y2), color, float(sim))

    # Memory
    def read_data(self, address: str, size: int, hwnd: int = 0) -> str:
        return self._call_string("OpReadData", int(hwnd), address, int(size))

    def write_data(self, address: str, data: str | bytes | bytearray | memoryview, size: int | None = None, hwnd: int = 0) -> bool:
        hex_data, data_size = self._normalize_hex_data(data, size)
        return self._call_ok("OpWriteData", int(hwnd), address, hex_data, data_size)

    def read_int(self, address: str, int_type: IntType | int = IntType.I32, hwnd: int = 0) -> int:
        self._check_open()
        value = ctypes.c_int64()
        ok = self._dll.OpReadInt(self._handle, int(hwnd), address, int(int_type), ctypes.byref(value))
        self._ok(ok, "OpReadInt")
        return int(value.value)

    def write_int(self, address: str, value: int, int_type: IntType | int = IntType.I32, hwnd: int = 0) -> bool:
        return self._call_ok("OpWriteInt", int(hwnd), address, int(int_type), int(value))

    def read_float(self, address: str, hwnd: int = 0) -> float:
        self._check_open()
        value = ctypes.c_float()
        ok = self._dll.OpReadFloat(self._handle, int(hwnd), address, ctypes.byref(value))
        self._ok(ok, "OpReadFloat")
        return float(value.value)

    def write_float(self, address: str, value: float, hwnd: int = 0) -> bool:
        return self._call_ok("OpWriteFloat", int(hwnd), address, float(value))

    def read_double(self, address: str, hwnd: int = 0) -> float:
        self._check_open()
        value = ctypes.c_double()
        ok = self._dll.OpReadDouble(self._handle, int(hwnd), address, ctypes.byref(value))
        self._ok(ok, "OpReadDouble")
        return float(value.value)

    def write_double(self, address: str, value: float, hwnd: int = 0) -> bool:
        return self._call_ok("OpWriteDouble", int(hwnd), address, float(value))

    def read_string(
        self,
        address: str,
        string_type: StringType | int = StringType.ANSI,
        length: int = 0,
        hwnd: int = 0,
    ) -> str:
        return self._call_string("OpReadString", int(hwnd), address, int(string_type), int(length))

    def write_string(
        self,
        address: str,
        value: str,
        string_type: StringType | int = StringType.ANSI,
        hwnd: int = 0,
    ) -> bool:
        return self._call_ok("OpWriteString", int(hwnd), address, int(string_type), str(value))

    @staticmethod
    def _normalize_hex_data(data: str | bytes | bytearray | memoryview, size: int | None) -> tuple[str, int]:
        if isinstance(data, (bytes, bytearray, memoryview)):
            raw = bytes(data)
            return raw.hex().upper(), len(raw) if size is None else int(size)

        hex_data = "".join(str(data).split())
        if size is None:
            size = len(hex_data) // 2
        return hex_data, int(size)
