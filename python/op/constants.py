from enum import Enum, IntEnum, IntFlag

__all__ = [
    "Blur",
    "ColorSpace",
    "DisplayInputMode",
    "DisplayInputPrefix",
    "DisplayMode",
    "FindDirection",
    "KeypadDelayType",
    "KeypadMode",
    "LayoutAnchor",
    "LayoutSize",
    "LayoutType",
    "MatchColor",
    "MatchMethod",
    "Morphology",
    "MouseDelayType",
    "MouseMode",
    "NearestPosType",
    "ScreenDataMode",
    "SearchDirection",
    "ShowError",
    "SpecialWindow",
    "Strip",
    "Thin",
    "Threshold",
    "WindowEnumFilter",
    "WindowRelation",
    "WindowState",
    "WindowStateAction",
]


class ShowError(IntEnum):
    NONE = 0
    MESSAGE_BOX = 1
    LOG_FILE = 2
    CONSOLE = 3


class ScreenDataMode(IntEnum):
    TOP_DOWN = 0
    BOTTOM_UP = 1


class NearestPosType(IntEnum):
    NAME_X_Y = 0
    X_Y = 1


class WindowEnumFilter(IntFlag):
    ALL = 0
    TITLE = 1
    CLASS_NAME = 2
    DIRECT_CHILD = 4
    TOP_LEVEL = 8
    VISIBLE = 16
    Z_ORDER = 32


class SpecialWindow(IntEnum):
    DESKTOP = 0
    TASKBAR = 1


class WindowRelation(IntEnum):
    PARENT = 0
    CHILD = 1
    FIRST = 2
    LAST = 3
    NEXT = 4
    PREVIOUS = 5
    OWNER = 6
    TOP = 7


class WindowState(IntEnum):
    EXISTS = 0
    ACTIVE = 1
    VISIBLE = 2
    MINIMIZED = 3
    MAXIMIZED = 4
    FOREGROUND = 5
    HUNG = 6
    ENABLED = 7


class WindowStateAction(IntEnum):
    CLOSE = 0
    ACTIVATE = 1
    MINIMIZE_NO_ACTIVATE = 2
    MINIMIZE = 3
    MAXIMIZE = 4
    RESTORE_NO_ACTIVATE = 5
    HIDE = 6
    SHOW = 7
    TOPMOST = 8
    NOT_TOPMOST = 9
    DISABLE = 10
    ENABLE = 11
    RESTORE = 12
    TERMINATE_PROCESS = 13
    FLASH = 14
    FOCUS = 15


class LayoutType(IntEnum):
    GRID = 0
    DIAGONAL = 1


class LayoutSize(IntEnum):
    KEEP = 0
    UNIFORM = 1


class LayoutAnchor(IntEnum):
    WINDOW = 0
    CLIENT = 1


class DisplayMode(str, Enum):
    NORMAL = "normal"
    NORMAL_DXGI = "normal.dxgi"
    NORMAL_WGC = "normal.wgc"
    GDI = "gdi"
    GDI2 = "gdi2"
    DX2 = "dx2"
    DX = "dx"
    DX_D3D9 = "dx.d3d9"
    DX_D3D10 = "dx.d3d10"
    DX_D3D11 = "dx.d3d11"
    DX_D3D12 = "dx.d3d12"
    OPENGL = "opengl"
    OPENGL_STD = "opengl.std"
    OPENGL_NOX = "opengl.nox"
    OPENGL_ES = "opengl.es"
    OPENGL_FI = "opengl.fi"


class MouseMode(str, Enum):
    NORMAL = "normal"
    WINDOWS = "windows"
    DX = "dx"


class KeypadMode(str, Enum):
    NORMAL = "normal"
    NORMAL_HD = "normal.hd"
    WINDOWS = "windows"
    DX = "dx"


class MouseDelayType(str, Enum):
    NORMAL = "normal"
    WINDOWS = "windows"
    DX = "dx"


class KeypadDelayType(str, Enum):
    NORMAL = "normal"
    NORMAL_HD = "normal.hd"
    WINDOWS = "windows"
    DX = "dx"


class DisplayInputMode(str, Enum):
    SCREEN = "screen"


class DisplayInputPrefix(str, Enum):
    PIC = "pic:"
    MEM = "mem:"


class FindDirection(IntEnum):
    LEFT_TOP_TO_RIGHT_BOTTOM = 0
    LEFT_BOTTOM_TO_RIGHT_TOP = 1
    RIGHT_TOP_TO_LEFT_BOTTOM = 2
    RIGHT_BOTTOM_TO_LEFT_TOP = 3
    CENTER = 4
    TOP_LEFT_TO_BOTTOM_RIGHT = 5
    TOP_RIGHT_TO_BOTTOM_LEFT = 6
    BOTTOM_LEFT_TO_TOP_RIGHT = 7
    BOTTOM_RIGHT_TO_TOP_LEFT = 8


class MatchMethod(IntEnum):
    SQDIFF = 0
    SQDIFF_NORMED = 1
    CCORR = 2
    CCORR_NORMED = 3
    CCOEFF = 4
    CCOEFF_NORMED = 5


class MatchColor(IntEnum):
    COLOR = 0
    GRAY = 1


class SearchDirection(IntEnum):
    LEFT_TO_RIGHT = 0
    RIGHT_TO_LEFT = 1
    TOP_TO_BOTTOM = 2
    BOTTOM_TO_TOP = 3


class Strip(IntEnum):
    NONE = 0
    HORIZONTAL = 1
    VERTICAL = 2


class Threshold(str, Enum):
    BINARY = "binary"
    BINARY_INV = "binary_inv"
    INV = "inv"
    OTSU = "otsu"
    OTSU_INV = "otsu_inv"
    ADAPTIVE = "adaptive"
    ADAPTIVE_INV = "adaptive_inv"


class ColorSpace(str, Enum):
    BGR = "bgr"
    HSV = "hsv"
    GRAY = "gray"
    GREY = "grey"


class Morphology(str, Enum):
    ERODE = "erode"
    DILATE = "dilate"
    OPEN = "open"
    CLOSE = "close"


class Thin(str, Enum):
    ZHANG_SUEN = "zhang_suen"
    ZHANGSUEN = "zhangsuen"
    GUO_HALL = "guo_hall"
    GUOHALL = "guohall"
    MORPH = "morph"


class Blur(str, Enum):
    GAUSSIAN = "gaussian"
    MEDIAN = "median"
    BILATERAL = "bilateral"
    BOX = "box"
    MEAN = "mean"
