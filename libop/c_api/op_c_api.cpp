#include "op_c_api.h"

#include "../../include/libop.h"
#include "../memory/ProcessMemory.h"

#include <Windows.h>
#include <cstdint>
#include <string>
#include <utility>

#undef FindWindow
#undef FindWindowEx
#undef SetWindowText

#define OP_WIDEN_TEXT2(text) L##text
#define OP_WIDEN_TEXT(text) OP_WIDEN_TEXT2(text)

using op::ProcessMemory;

struct op_c_context {
    op_c_context() = default;
    ~op_c_context() = default;

    op_c_context(const op_c_context &) = delete;
    op_c_context &operator=(const op_c_context &) = delete;

    op::Client op;
    std::wstring string_result;
};

namespace {

template <typename Func>
int call_int(op_handle handle, Func &&func) {
    op::Client *op = handle ? &handle->op : nullptr;
    if (!op)
        return 0;

    try {
        return static_cast<int>(std::forward<Func>(func)(*op));
    } catch (...) {
        return 0;
    }
}

template <typename Func>
int call_ret(op_handle handle, Func &&func) {
    return call_int(handle, [&](op::Client &op) {
        long ret = 0;
        std::forward<Func>(func)(op, &ret);
        return ret;
    });
}

template <typename Func>
intptr_t call_intptr(op_handle handle, Func &&func) {
    if (!handle)
        return 0;

    try {
        LONG_PTR ret = 0;
        std::forward<Func>(func)(handle->op, &ret);
        return static_cast<intptr_t>(ret);
    } catch (...) {
        return 0;
    }
}

template <typename Func>
const wchar_t *call_string(op_handle handle, Func &&func) {
    if (!handle)
        return L"";

    try {
        handle->string_result.clear();
        std::forward<Func>(func)(handle->op, handle->string_result);
        return handle->string_result.c_str();
    } catch (...) {
        return L"";
    }
}

const wchar_t *safe_text(const wchar_t *text) {
    return text ? text : L"";
}

long in_int(const int *value) {
    return value ? *value : 0;
}

template <typename Target, typename Value>
void out_value(Target *target, Value value) {
    if (target)
        *target = static_cast<Target>(value);
}

void out_int(int *target, long value) {
    out_value(target, value);
}

LONG_PTR resolve_memory_hwnd(op_handle handle, intptr_t hwnd) {
    if (hwnd != 0 || !handle)
        return static_cast<LONG_PTR>(hwnd);

    LONG_PTR bind_hwnd = 0;
    handle->op.GetBindWindow(&bind_hwnd);
    return bind_hwnd;
}

template <typename Func>
int call_memory(op_handle handle, intptr_t hwnd, Func &&func) {
    if (!handle)
        return 0;

    try {
        ProcessMemory mem;
        const LONG_PTR target = resolve_memory_hwnd(handle, hwnd);
        return std::forward<Func>(func)(mem, reinterpret_cast<HWND>(target)) ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

} // namespace

// Lifecycle

op_handle OP_CALL OpCreate(void) {
    try {
        return new op_c_context();
    } catch (...) {
        return nullptr;
    }
}

void OP_CALL OpDestroy(op_handle handle) {
    delete handle;
}

// Basic

const wchar_t *OP_CALL OpRuntimeVer(void) {
    return OP_WIDEN_TEXT(OP_VERSION);
}

int OP_CALL OpRuntimeSetPath(op_handle handle, const wchar_t *path) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.SetPath(safe_text(path), ret); });
}

const wchar_t *OP_CALL OpRuntimeGetPath(op_handle handle) {
    return call_string(handle, [](op::Client &op, std::wstring &ret) { op.GetPath(ret); });
}

const wchar_t *OP_CALL OpRuntimeGetBasePath(op_handle handle) {
    return call_string(handle, [](op::Client &op, std::wstring &ret) { op.GetBasePath(ret); });
}

int OP_CALL OpRuntimeGetID(op_handle handle) {
    return call_ret(handle, [](op::Client &op, long *ret) { op.GetID(ret); });
}

int OP_CALL OpRuntimeGetLastError(op_handle handle) {
    return call_ret(handle, [](op::Client &op, long *ret) { op.GetLastError(ret); });
}

int OP_CALL OpRuntimeSetShowErrorMsg(op_handle handle, int show_type) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.SetShowErrorMsg(show_type, ret); });
}

int OP_CALL OpRuntimeSleep(op_handle handle, int millseconds) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.Sleep(millseconds, ret); });
}

int OP_CALL OpWindowInjectDll(op_handle handle, const wchar_t *process_name, const wchar_t *dll_name) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.InjectDll(safe_text(process_name), safe_text(dll_name), ret);
    });
}

int OP_CALL OpImageEnablePicCache(op_handle handle, int enable) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.EnablePicCache(enable, ret); });
}

int OP_CALL OpImageCapturePre(op_handle handle, const wchar_t *file_name) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.CapturePre(safe_text(file_name), ret); });
}

int OP_CALL OpImageSetScreenDataMode(op_handle handle, int mode) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.SetScreenDataMode(mode, ret); });
}

// Algorithm

const wchar_t *OP_CALL OpAlgorithmAStarFindPath(op_handle handle, int map_width, int map_height,
                                       const wchar_t *disable_points, int begin_x, int begin_y, int end_x,
                                       int end_y) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.AStarFindPath(map_width, map_height, safe_text(disable_points), begin_x, begin_y, end_x, end_y, ret);
    });
}

const wchar_t *OP_CALL OpAlgorithmFindNearestPos(op_handle handle, const wchar_t *all_pos, int type, int x, int y) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.FindNearestPos(safe_text(all_pos), type, x, y, ret);
    });
}

// Window and process

const wchar_t *OP_CALL OpWindowEnumWindow(op_handle handle, intptr_t parent, const wchar_t *title,
                                    const wchar_t *class_name, int filter) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.EnumWindow(static_cast<LONG_PTR>(parent), safe_text(title), safe_text(class_name), filter, ret);
    });
}

const wchar_t *OP_CALL OpWindowEnumWindowByProcess(op_handle handle, const wchar_t *process_name, const wchar_t *title,
                                             const wchar_t *class_name, int filter) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.EnumWindowByProcess(safe_text(process_name), safe_text(title), safe_text(class_name), filter, ret);
    });
}

const wchar_t *OP_CALL OpWindowEnumProcess(op_handle handle, const wchar_t *name) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) { op.EnumProcess(safe_text(name), ret); });
}

int OP_CALL OpWindowClientToScreen(op_handle handle, intptr_t hwnd, int *x, int *y) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        long lx = in_int(x);
        long ly = in_int(y);
        op.ClientToScreen(static_cast<LONG_PTR>(hwnd), &lx, &ly, ret);
        out_int(x, lx);
        out_int(y, ly);
    });
}

intptr_t OP_CALL OpWindowFindWindow(op_handle handle, const wchar_t *class_name, const wchar_t *title) {
    return call_intptr(handle, [&](op::Client &op, LONG_PTR *ret) { op.FindWindow(safe_text(class_name), safe_text(title), ret); });
}

intptr_t OP_CALL OpWindowFindWindowByProcess(op_handle handle, const wchar_t *process_name, const wchar_t *class_name,
                                       const wchar_t *title) {
    return call_intptr(handle, [&](op::Client &op, LONG_PTR *ret) {
        op.FindWindowByProcess(safe_text(process_name), safe_text(class_name), safe_text(title), ret);
    });
}

intptr_t OP_CALL OpWindowFindWindowByProcessId(op_handle handle, int process_id, const wchar_t *class_name,
                                         const wchar_t *title) {
    return call_intptr(handle, [&](op::Client &op, LONG_PTR *ret) {
        op.FindWindowByProcessId(process_id, safe_text(class_name), safe_text(title), ret);
    });
}

intptr_t OP_CALL OpWindowFindWindowEx(op_handle handle, intptr_t parent, const wchar_t *class_name, const wchar_t *title) {
    return call_intptr(handle, [&](op::Client &op, LONG_PTR *ret) {
        op.FindWindowEx(static_cast<LONG_PTR>(parent), safe_text(class_name), safe_text(title), ret);
    });
}

int OP_CALL OpWindowGetClientRect(op_handle handle, intptr_t hwnd, int *x1, int *y1, int *x2, int *y2) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        long lx1 = 0, ly1 = 0, lx2 = 0, ly2 = 0;
        op.GetClientRect(static_cast<LONG_PTR>(hwnd), &lx1, &ly1, &lx2, &ly2, ret);
        out_int(x1, lx1);
        out_int(y1, ly1);
        out_int(x2, lx2);
        out_int(y2, ly2);
    });
}

int OP_CALL OpWindowGetClientSize(op_handle handle, intptr_t hwnd, int *width, int *height) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        long w = 0, h = 0;
        op.GetClientSize(static_cast<LONG_PTR>(hwnd), &w, &h, ret);
        out_int(width, w);
        out_int(height, h);
    });
}

intptr_t OP_CALL OpWindowGetForegroundFocus(op_handle handle) {
    return call_intptr(handle, [](op::Client &op, LONG_PTR *ret) { op.GetForegroundFocus(ret); });
}

intptr_t OP_CALL OpWindowGetForegroundWindow(op_handle handle) {
    return call_intptr(handle, [](op::Client &op, LONG_PTR *ret) { op.GetForegroundWindow(ret); });
}

intptr_t OP_CALL OpWindowGetMousePointWindow(op_handle handle) {
    return call_intptr(handle, [](op::Client &op, LONG_PTR *ret) { op.GetMousePointWindow(ret); });
}

intptr_t OP_CALL OpWindowGetPointWindow(op_handle handle, int x, int y) {
    return call_intptr(handle, [&](op::Client &op, LONG_PTR *ret) { op.GetPointWindow(x, y, ret); });
}

const wchar_t *OP_CALL OpWindowGetProcessInfo(op_handle handle, int pid) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) { op.GetProcessInfo(pid, ret); });
}

intptr_t OP_CALL OpWindowGetSpecialWindow(op_handle handle, int flag) {
    return call_intptr(handle, [&](op::Client &op, LONG_PTR *ret) { op.GetSpecialWindow(flag, ret); });
}

intptr_t OP_CALL OpWindowGetWindow(op_handle handle, intptr_t hwnd, int flag) {
    return call_intptr(handle, [&](op::Client &op, LONG_PTR *ret) {
        op.GetWindow(static_cast<LONG_PTR>(hwnd), flag, ret);
    });
}

const wchar_t *OP_CALL OpWindowGetWindowClass(op_handle handle, intptr_t hwnd) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.GetWindowClass(static_cast<LONG_PTR>(hwnd), ret);
    });
}

int OP_CALL OpWindowGetWindowProcessId(op_handle handle, intptr_t hwnd) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.GetWindowProcessId(static_cast<LONG_PTR>(hwnd), ret); });
}

const wchar_t *OP_CALL OpWindowGetWindowProcessPath(op_handle handle, intptr_t hwnd) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.GetWindowProcessPath(static_cast<LONG_PTR>(hwnd), ret);
    });
}

int OP_CALL OpWindowGetWindowRect(op_handle handle, intptr_t hwnd, int *x1, int *y1, int *x2, int *y2) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        long lx1 = 0, ly1 = 0, lx2 = 0, ly2 = 0;
        op.GetWindowRect(static_cast<LONG_PTR>(hwnd), &lx1, &ly1, &lx2, &ly2, ret);
        out_int(x1, lx1);
        out_int(y1, ly1);
        out_int(x2, lx2);
        out_int(y2, ly2);
    });
}

int OP_CALL OpWindowGetWindowState(op_handle handle, intptr_t hwnd, int flag) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.GetWindowState(static_cast<LONG_PTR>(hwnd), flag, ret); });
}

const wchar_t *OP_CALL OpWindowGetWindowTitle(op_handle handle, intptr_t hwnd) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.GetWindowTitle(static_cast<LONG_PTR>(hwnd), ret);
    });
}

int OP_CALL OpWindowMoveWindow(op_handle handle, intptr_t hwnd, int x, int y) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.MoveWindow(static_cast<LONG_PTR>(hwnd), x, y, ret); });
}

int OP_CALL OpWindowScreenToClient(op_handle handle, intptr_t hwnd, int *x, int *y) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        long lx = in_int(x);
        long ly = in_int(y);
        op.ScreenToClient(static_cast<LONG_PTR>(hwnd), &lx, &ly, ret);
        out_int(x, lx);
        out_int(y, ly);
    });
}

int OP_CALL OpWindowSendPaste(op_handle handle, intptr_t hwnd) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.SendPaste(static_cast<LONG_PTR>(hwnd), ret); });
}

int OP_CALL OpWindowSetClientSize(op_handle handle, intptr_t hwnd, int width, int height) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.SetClientSize(static_cast<LONG_PTR>(hwnd), width, height, ret);
    });
}

int OP_CALL OpWindowSetWindowState(op_handle handle, intptr_t hwnd, int flag) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.SetWindowState(static_cast<LONG_PTR>(hwnd), flag, ret);
    });
}

int OP_CALL OpWindowSetWindowSize(op_handle handle, intptr_t hwnd, int width, int height) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.SetWindowSize(static_cast<LONG_PTR>(hwnd), width, height, ret);
    });
}

int OP_CALL OpWindowLayoutWindows(op_handle handle, const wchar_t *hwnds, int layout_type, int columns, int start_x,
                            int start_y, int gap_x, int gap_y, int size_mode, int window_width, int window_height,
                            int anchor_mode) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.LayoutWindows(safe_text(hwnds), layout_type, columns, start_x, start_y, gap_x, gap_y, size_mode,
                         window_width, window_height, anchor_mode, ret);
    });
}

int OP_CALL OpWindowSetWindowText(op_handle handle, intptr_t hwnd, const wchar_t *title) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.SetWindowText(static_cast<LONG_PTR>(hwnd), safe_text(title), ret);
    });
}

int OP_CALL OpWindowSetWindowTransparent(op_handle handle, intptr_t hwnd, int trans) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.SetWindowTransparent(static_cast<LONG_PTR>(hwnd), trans, ret);
    });
}

int OP_CALL OpWindowSendString(op_handle handle, intptr_t hwnd, const wchar_t *str) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.SendString(static_cast<LONG_PTR>(hwnd), safe_text(str), ret);
    });
}

int OP_CALL OpWindowSendStringIme(op_handle handle, intptr_t hwnd, const wchar_t *str) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.SendStringIme(static_cast<LONG_PTR>(hwnd), safe_text(str), ret);
    });
}

int OP_CALL OpWindowRunApp(op_handle handle, const wchar_t *cmdline, int mode, uint32_t *pid) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        unsigned long local_pid = 0;
        op.RunApp(safe_text(cmdline), mode, &local_pid, ret);
        out_value(pid, local_pid);
    });
}

int OP_CALL OpWindowWinExec(op_handle handle, const wchar_t *cmdline, int cmdshow) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.WinExec(safe_text(cmdline), cmdshow, ret); });
}

const wchar_t *OP_CALL OpWindowGetCmdStr(op_handle handle, const wchar_t *cmd, int millseconds) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) { op.GetCmdStr(safe_text(cmd), millseconds, ret); });
}

int OP_CALL OpWindowSetClipboard(op_handle handle, const wchar_t *str) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.SetClipboard(safe_text(str), ret); });
}

const wchar_t *OP_CALL OpWindowGetClipboard(op_handle handle) {
    return call_string(handle, [](op::Client &op, std::wstring &ret) { op.GetClipboard(ret); });
}

int OP_CALL OpRuntimeDelay(op_handle handle, int mis) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.Delay(mis, ret); });
}

int OP_CALL OpRuntimeDelays(op_handle handle, int mis_min, int mis_max) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.Delays(mis_min, mis_max, ret); });
}

// Background binding

int OP_CALL OpBindingBindWindow(op_handle handle, intptr_t hwnd, const wchar_t *display, const wchar_t *mouse,
                         const wchar_t *keypad, int mode) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.BindWindow(static_cast<LONG_PTR>(hwnd), safe_text(display), safe_text(mouse), safe_text(keypad), mode, ret);
    });
}

int OP_CALL OpBindingBindWindowEx(op_handle handle, intptr_t display_hwnd, intptr_t input_hwnd, const wchar_t *display,
                           const wchar_t *mouse, const wchar_t *keypad, int mode) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.BindWindowEx(static_cast<LONG_PTR>(display_hwnd), static_cast<LONG_PTR>(input_hwnd), safe_text(display),
                        safe_text(mouse), safe_text(keypad), mode, ret);
    });
}

int OP_CALL OpUnBindWindow(op_handle handle) {
    return call_ret(handle, [](op::Client &op, long *ret) { op.UnBindWindow(ret); });
}

int OP_CALL OpBindingUnbindWindow(op_handle handle) {
    return OpUnBindWindow(handle);
}

intptr_t OP_CALL OpBindingGetBindWindow(op_handle handle) {
    return call_intptr(handle, [](op::Client &op, LONG_PTR *ret) { op.GetBindWindow(ret); });
}

int OP_CALL OpBindingIsBind(op_handle handle) {
    return call_ret(handle, [](op::Client &op, long *ret) { op.IsBind(ret); });
}

// Mouse

int OP_CALL OpMouseGetCursorPos(op_handle handle, int *x, int *y) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        long lx = 0, ly = 0;
        op.GetCursorPos(&lx, &ly, ret);
        out_int(x, lx);
        out_int(y, ly);
    });
}

const wchar_t *OP_CALL OpMouseGetCursorShape(op_handle handle) {
    return call_string(handle, [](op::Client &op, std::wstring &ret) { op.GetCursorShape(ret); });
}

int OP_CALL OpMouseMoveR(op_handle handle, int x, int y) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.MoveR(x, y, ret); });
}

int OP_CALL OpMouseMoveTo(op_handle handle, int x, int y) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.MoveTo(x, y, ret); });
}

const wchar_t *OP_CALL OpMouseMoveToEx(op_handle handle, int x, int y, int w, int h) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) { op.MoveToEx(x, y, w, h, ret); });
}

#define OP_MOUSE_RET(name, method) \
    int OP_CALL name(op_handle handle) { return call_ret(handle, [](op::Client &op, long *ret) { op.method(ret); }); }

OP_MOUSE_RET(OpMouseLeftClick, LeftClick)
OP_MOUSE_RET(OpMouseLeftDoubleClick, LeftDoubleClick)
OP_MOUSE_RET(OpMouseLeftDown, LeftDown)
OP_MOUSE_RET(OpMouseLeftUp, LeftUp)
OP_MOUSE_RET(OpMouseMiddleClick, MiddleClick)
OP_MOUSE_RET(OpMouseMiddleDown, MiddleDown)
OP_MOUSE_RET(OpMouseMiddleUp, MiddleUp)
OP_MOUSE_RET(OpMouseRightClick, RightClick)
OP_MOUSE_RET(OpMouseRightDown, RightDown)
OP_MOUSE_RET(OpMouseRightUp, RightUp)
OP_MOUSE_RET(OpMouseWheelDown, WheelDown)
OP_MOUSE_RET(OpMouseWheelUp, WheelUp)

#undef OP_MOUSE_RET

int OP_CALL OpMouseSetMouseDelay(op_handle handle, const wchar_t *type, int delay) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.SetMouseDelay(safe_text(type), delay, ret); });
}

// Keyboard

int OP_CALL OpKeyboardGetKeyState(op_handle handle, int vk_code) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.GetKeyState(vk_code, ret); });
}

int OP_CALL OpKeyboardKeyDown(op_handle handle, int vk_code) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.KeyDown(vk_code, ret); });
}

int OP_CALL OpKeyboardKeyDownChar(op_handle handle, const wchar_t *vk_code) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.KeyDownChar(safe_text(vk_code), ret); });
}

int OP_CALL OpKeyboardKeyUp(op_handle handle, int vk_code) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.KeyUp(vk_code, ret); });
}

int OP_CALL OpKeyboardKeyUpChar(op_handle handle, const wchar_t *vk_code) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.KeyUpChar(safe_text(vk_code), ret); });
}

int OP_CALL OpKeyboardWaitKey(op_handle handle, int vk_code, int time_out) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.WaitKey(vk_code, time_out, ret); });
}

int OP_CALL OpKeyboardKeyPress(op_handle handle, int vk_code) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.KeyPress(vk_code, ret); });
}

int OP_CALL OpKeyboardKeyPressChar(op_handle handle, const wchar_t *vk_code) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.KeyPressChar(safe_text(vk_code), ret); });
}

int OP_CALL OpKeyboardSetKeypadDelay(op_handle handle, const wchar_t *type, int delay) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.SetKeypadDelay(safe_text(type), delay, ret); });
}

int OP_CALL OpKeyboardKeyPressStr(op_handle handle, const wchar_t *key_str, int delay) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.KeyPressStr(safe_text(key_str), delay, ret); });
}

// Image and color

int OP_CALL OpImageCapture(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *file_name) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.Capture(x1, y1, x2, y2, safe_text(file_name), ret); });
}

int OP_CALL OpImageCmpColor(op_handle handle, int x, int y, const wchar_t *color, double sim) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.CmpColor(x, y, safe_text(color), sim, ret); });
}

int OP_CALL OpImageFindColor(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color, double sim, int dir,
                        int *x, int *y) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        long lx = 0, ly = 0;
        op.FindColor(x1, y1, x2, y2, safe_text(color), sim, dir, &lx, &ly, ret);
        out_int(x, lx);
        out_int(y, ly);
    });
}

const wchar_t *OP_CALL OpImageFindColorEx(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                     double sim, int dir) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.FindColorEx(x1, y1, x2, y2, safe_text(color), sim, dir, ret);
    });
}

int OP_CALL OpImageFindMultiColor(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *first_color,
                             const wchar_t *offset_color, double sim, int dir, int *x, int *y) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        long lx = 0, ly = 0;
        op.FindMultiColor(x1, y1, x2, y2, safe_text(first_color), safe_text(offset_color), sim, dir, &lx, &ly, ret);
        out_int(x, lx);
        out_int(y, ly);
    });
}

const wchar_t *OP_CALL OpImageFindMultiColorEx(op_handle handle, int x1, int y1, int x2, int y2,
                                          const wchar_t *first_color, const wchar_t *offset_color, double sim,
                                          int dir) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.FindMultiColorEx(x1, y1, x2, y2, safe_text(first_color), safe_text(offset_color), sim, dir, ret);
    });
}

int OP_CALL OpImageFindPic(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *files,
                      const wchar_t *delta_color, double sim, int dir, int *x, int *y) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        long lx = 0, ly = 0;
        op.FindPic(x1, y1, x2, y2, safe_text(files), safe_text(delta_color), sim, dir, &lx, &ly, ret);
        out_int(x, lx);
        out_int(y, ly);
    });
}

const wchar_t *OP_CALL OpImageFindPicEx(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *files,
                                   const wchar_t *delta_color, double sim, int dir) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.FindPicEx(x1, y1, x2, y2, safe_text(files), safe_text(delta_color), sim, dir, ret);
    });
}

const wchar_t *OP_CALL OpImageFindPicExS(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *files,
                                    const wchar_t *delta_color, double sim, int dir) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.FindPicExS(x1, y1, x2, y2, safe_text(files), safe_text(delta_color), sim, dir, ret);
    });
}

int OP_CALL OpImageFindColorBlock(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color, double sim,
                             int count, int height, int width, int *x, int *y) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        long lx = 0, ly = 0;
        op.FindColorBlock(x1, y1, x2, y2, safe_text(color), sim, count, height, width, &lx, &ly, ret);
        out_int(x, lx);
        out_int(y, ly);
    });
}

const wchar_t *OP_CALL OpImageFindColorBlockEx(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                          double sim, int count, int height, int width) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.FindColorBlockEx(x1, y1, x2, y2, safe_text(color), sim, count, height, width, ret);
    });
}

const wchar_t *OP_CALL OpImageGetColor(op_handle handle, int x, int y) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) { op.GetColor(x, y, ret); });
}

int OP_CALL OpImageGetColorNum(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color, double sim) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.GetColorNum(x1, y1, x2, y2, safe_text(color), sim, ret); });
}

int OP_CALL OpImageSetDisplayInput(op_handle handle, const wchar_t *mode) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.SetDisplayInput(safe_text(mode), ret); });
}

int OP_CALL OpImageLoadPic(op_handle handle, const wchar_t *file_name) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.LoadPic(safe_text(file_name), ret); });
}

int OP_CALL OpImageFreePic(op_handle handle, const wchar_t *file_name) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.FreePic(safe_text(file_name), ret); });
}

int OP_CALL OpImageLoadMemPic(op_handle handle, const wchar_t *file_name, void *data, int size) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.LoadMemPic(safe_text(file_name), data, size, ret); });
}

int OP_CALL OpImageGetPicSize(op_handle handle, const wchar_t *pic_name, int *width, int *height) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        long w = 0, h = 0;
        op.GetPicSize(safe_text(pic_name), &w, &h, ret);
        out_int(width, w);
        out_int(height, h);
    });
}

uintptr_t OP_CALL OpImageGetScreenData(op_handle handle, int x1, int y1, int x2, int y2, int *out_ret) {
    out_int(out_ret, 0);
    if (!handle)
        return 0;

    try {
        size_t data = 0;
        long ret = 0;
        handle->op.GetScreenData(x1, y1, x2, y2, &data, &ret);
        out_int(out_ret, ret);
        return static_cast<uintptr_t>(data);
    } catch (...) {
        out_int(out_ret, 0);
        return 0;
    }
}

uintptr_t OP_CALL OpImageGetScreenDataBmp(op_handle handle, int x1, int y1, int x2, int y2, int *size, int *out_ret) {
    out_int(size, 0);
    out_int(out_ret, 0);
    if (!handle)
        return 0;

    try {
        size_t data = 0;
        long data_size = 0;
        long ret = 0;
        handle->op.GetScreenDataBmp(x1, y1, x2, y2, &data, &data_size, &ret);
        out_int(size, data_size);
        out_int(out_ret, ret);
        return static_cast<uintptr_t>(data);
    } catch (...) {
        out_int(size, 0);
        out_int(out_ret, 0);
        return 0;
    }
}

void OP_CALL OpImageGetScreenFrameInfo(op_handle handle, int *frame_id, int *time) {
    out_int(frame_id, 0);
    out_int(time, 0);
    if (!handle)
        return;

    try {
        long frame = 0, frame_time = 0;
        handle->op.GetScreenFrameInfo(&frame, &frame_time);
        out_int(frame_id, frame);
        out_int(time, frame_time);
    } catch (...) {
        out_int(frame_id, 0);
        out_int(time, 0);
    }
}

const wchar_t *OP_CALL OpImageMatchPicName(op_handle handle, const wchar_t *pic_name) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) { op.MatchPicName(safe_text(pic_name), ret); });
}

// OpenCV

int OP_CALL OpOpenCvLoadTemplate(op_handle handle, const wchar_t *name, const wchar_t *file_path) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.CvLoadTemplate(safe_text(name), safe_text(file_path), ret); });
}

int OP_CALL OpOpenCvLoadMaskedTemplate(op_handle handle, const wchar_t *name, const wchar_t *template_path,
                                   const wchar_t *mask_path) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.CvLoadMaskedTemplate(safe_text(name), safe_text(template_path), safe_text(mask_path), ret);
    });
}

int OP_CALL OpOpenCvRemoveTemplate(op_handle handle, const wchar_t *name) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.CvRemoveTemplate(safe_text(name), ret); });
}

int OP_CALL OpOpenCvRemoveAllTemplates(op_handle handle) {
    return call_ret(handle, [](op::Client &op, long *ret) { op.CvRemoveAllTemplates(ret); });
}

int OP_CALL OpOpenCvHasTemplate(op_handle handle, const wchar_t *name) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.CvHasTemplate(safe_text(name), ret); });
}

int OP_CALL OpOpenCvGetTemplateCount(op_handle handle) {
    return call_ret(handle, [](op::Client &op, long *ret) { op.CvGetTemplateCount(ret); });
}

const wchar_t *OP_CALL OpOpenCvGetAllTemplateNames(op_handle handle) {
    return call_string(handle, [](op::Client &op, std::wstring &ret) { op.CvGetAllTemplateNames(ret); });
}

const wchar_t *OP_CALL OpOpenCvGetOpenCvVersion(op_handle handle) {
    return call_string(handle, [](op::Client &op, std::wstring &ret) { op.CvGetOpenCvVersion(ret); });
}

int OP_CALL OpOpenCvLoadTemplateList(op_handle handle, const wchar_t *template_list) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.CvLoadTemplateList(safe_text(template_list), ret); });
}

#define OP_CV_FILE_RET(name, method) \
    int OP_CALL name(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file) { \
        return call_ret(handle, [&](op::Client &op, long *ret) { op.method(safe_text(src_file), safe_text(dst_file), ret); }); \
    }

OP_CV_FILE_RET(OpOpenCvToGray, CvToGray)
OP_CV_FILE_RET(OpOpenCvToBinary, CvToBinary)
OP_CV_FILE_RET(OpOpenCvToEdge, CvToEdge)
OP_CV_FILE_RET(OpOpenCvToOutline, CvToOutline)
OP_CV_FILE_RET(OpOpenCvDenoise, CvDenoise)
OP_CV_FILE_RET(OpOpenCvEqualize, CvEqualize)
OP_CV_FILE_RET(OpOpenCvCropValid, CvCropValid)

#undef OP_CV_FILE_RET

int OP_CALL OpOpenCvCLAHE(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file, double clip_limit,
                      int tile_grid_size) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.CvCLAHE(safe_text(src_file), safe_text(dst_file), clip_limit, tile_grid_size, ret);
    });
}

int OP_CALL OpOpenCvBlur(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file, const wchar_t *mode,
                     int kernel_size) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.CvBlur(safe_text(src_file), safe_text(dst_file), safe_text(mode), kernel_size, ret);
    });
}

int OP_CALL OpOpenCvSharpen(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file, double strength) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.CvSharpen(safe_text(src_file), safe_text(dst_file), strength, ret);
    });
}

const wchar_t *OP_CALL OpOpenCvConnectedComponents(op_handle handle, const wchar_t *src_file, double min_area) {
    return call_string(handle, [&](op::Client &op, std::wstring &json) {
        long ret = 0;
        op.CvConnectedComponents(safe_text(src_file), min_area, json, &ret);
    });
}

const wchar_t *OP_CALL OpOpenCvFindContours(op_handle handle, const wchar_t *src_file, double min_area) {
    return call_string(handle, [&](op::Client &op, std::wstring &json) {
        long ret = 0;
        op.CvFindContours(safe_text(src_file), min_area, json, &ret);
    });
}

int OP_CALL OpOpenCvPreprocessPipeline(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                                   const wchar_t *pipeline) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.CvPreprocessPipeline(safe_text(src_file), safe_text(dst_file), safe_text(pipeline), ret);
    });
}

int OP_CALL OpOpenCvCrop(op_handle handle, const wchar_t *src_file, int x, int y, int width, int height,
                     const wchar_t *dst_file) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.CvCrop(safe_text(src_file), x, y, width, height, safe_text(dst_file), ret);
    });
}

int OP_CALL OpOpenCvResize(op_handle handle, const wchar_t *src_file, int width, int height, const wchar_t *dst_file) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.CvResize(safe_text(src_file), width, height, safe_text(dst_file), ret);
    });
}

int OP_CALL OpOpenCvThreshold(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file, double threshold,
                          double max_value, const wchar_t *mode) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.CvThreshold(safe_text(src_file), safe_text(dst_file), threshold, max_value, safe_text(mode), ret);
    });
}

int OP_CALL OpOpenCvInRange(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                        const wchar_t *color_space, const wchar_t *lower, const wchar_t *upper) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.CvInRange(safe_text(src_file), safe_text(dst_file), safe_text(color_space), safe_text(lower),
                     safe_text(upper), ret);
    });
}

int OP_CALL OpOpenCvMorphology(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file, const wchar_t *mode,
                           int kernel_size, int iterations) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.CvMorphology(safe_text(src_file), safe_text(dst_file), safe_text(mode), kernel_size, iterations, ret);
    });
}

int OP_CALL OpOpenCvThin(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file, const wchar_t *mode) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.CvThin(safe_text(src_file), safe_text(dst_file), safe_text(mode), ret);
    });
}

const wchar_t *OP_CALL OpOpenCvMatchTemplate(op_handle handle, int x, int y, int width, int height,
                                         const wchar_t *template_name, double threshold, int dir, int strip_mode,
                                         int method, int color_mode) {
    return call_string(handle, [&](op::Client &op, std::wstring &json) {
        long ret = 0;
        op.CvMatchTemplate(x, y, width, height, safe_text(template_name), threshold, dir, strip_mode, method,
                           color_mode, json, &ret);
    });
}

const wchar_t *OP_CALL OpOpenCvMatchTemplateScale(op_handle handle, int x, int y, int width, int height,
                                              const wchar_t *template_name, const wchar_t *scales, double threshold,
                                              int method, int color_mode) {
    return call_string(handle, [&](op::Client &op, std::wstring &json) {
        long ret = 0;
        op.CvMatchTemplateScale(x, y, width, height, safe_text(template_name), safe_text(scales), threshold, method,
                                color_mode, json, &ret);
    });
}

const wchar_t *OP_CALL OpOpenCvMatchAnyTemplate(op_handle handle, int x, int y, int width, int height,
                                            const wchar_t *template_names, double threshold, int dir,
                                            int strip_mode, int method, int color_mode) {
    return call_string(handle, [&](op::Client &op, std::wstring &json) {
        long ret = 0;
        op.CvMatchAnyTemplate(x, y, width, height, safe_text(template_names), threshold, dir, strip_mode, method,
                              color_mode, json, &ret);
    });
}

const wchar_t *OP_CALL OpOpenCvMatchAllTemplates(op_handle handle, int x, int y, int width, int height,
                                             const wchar_t *template_names, double threshold, int dir,
                                             int strip_mode, int method, int color_mode) {
    return call_string(handle, [&](op::Client &op, std::wstring &json) {
        long ret = 0;
        op.CvMatchAllTemplates(x, y, width, height, safe_text(template_names), threshold, dir, strip_mode, method,
                               color_mode, json, &ret);
    });
}

const wchar_t *OP_CALL OpOpenCvFeatureMatchTemplate(op_handle handle, int x, int y, int width, int height,
                                                const wchar_t *template_name, double threshold) {
    return call_string(handle, [&](op::Client &op, std::wstring &json) {
        long ret = 0;
        op.CvFeatureMatchTemplate(x, y, width, height, safe_text(template_name), threshold, json, &ret);
    });
}

const wchar_t *OP_CALL OpOpenCvEdgeMatchTemplate(op_handle handle, int x, int y, int width, int height,
                                             const wchar_t *template_name, double threshold) {
    return call_string(handle, [&](op::Client &op, std::wstring &json) {
        long ret = 0;
        op.CvEdgeMatchTemplate(x, y, width, height, safe_text(template_name), threshold, json, &ret);
    });
}

const wchar_t *OP_CALL OpOpenCvShapeMatchTemplate(op_handle handle, int x, int y, int width, int height,
                                              const wchar_t *template_name, double threshold) {
    return call_string(handle, [&](op::Client &op, std::wstring &json) {
        long ret = 0;
        op.CvShapeMatchTemplate(x, y, width, height, safe_text(template_name), threshold, json, &ret);
    });
}

// OCR

int OP_CALL OpOcrSetEngine(op_handle handle, const wchar_t *path_of_engine, const wchar_t *dll_name,
                           const wchar_t *argv) {
    return call_int(handle, [&](op::Client &op) {
        return op.SetOcrEngine(safe_text(path_of_engine), safe_text(dll_name), safe_text(argv));
    });
}

int OP_CALL OpOcrSetDict(op_handle handle, int idx, const wchar_t *file_name) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.SetDict(idx, safe_text(file_name), ret); });
}

const wchar_t *OP_CALL OpOcrGetDict(op_handle handle, int idx, int font_index) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) { op.GetDict(idx, font_index, ret); });
}

int OP_CALL OpOcrSetMemDict(op_handle handle, int idx, const wchar_t *data, int size) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.SetMemDict(idx, safe_text(data), size, ret); });
}

int OP_CALL OpOcrUseDict(op_handle handle, int idx) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.UseDict(idx, ret); });
}

int OP_CALL OpOcrAddDict(op_handle handle, int idx, const wchar_t *dict_info) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.AddDict(idx, safe_text(dict_info), ret); });
}

int OP_CALL OpOcrSaveDict(op_handle handle, int idx, const wchar_t *file_name) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.SaveDict(idx, safe_text(file_name), ret); });
}

int OP_CALL OpOcrClearDict(op_handle handle, int idx) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.ClearDict(idx, ret); });
}

int OP_CALL OpOcrGetDictCount(op_handle handle, int idx) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.GetDictCount(idx, ret); });
}

int OP_CALL OpOcrGetNowDict(op_handle handle) {
    return call_ret(handle, [](op::Client &op, long *ret) { op.GetNowDict(ret); });
}

const wchar_t *OP_CALL OpOcrFetchWord(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                   const wchar_t *word) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.FetchWord(x1, y1, x2, y2, safe_text(color), safe_text(word), ret);
    });
}

const wchar_t *OP_CALL OpOcrGetWordsNoDict(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.GetWordsNoDict(x1, y1, x2, y2, safe_text(color), ret);
    });
}

int OP_CALL OpOcrGetWordResultCount(op_handle handle, const wchar_t *result) {
    return call_ret(handle, [&](op::Client &op, long *ret) { op.GetWordResultCount(safe_text(result), ret); });
}

int OP_CALL OpOcrGetWordResultPos(op_handle handle, const wchar_t *result, int index, int *x, int *y) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        long lx = 0, ly = 0;
        op.GetWordResultPos(safe_text(result), index, &lx, &ly, ret);
        out_int(x, lx);
        out_int(y, ly);
    });
}

const wchar_t *OP_CALL OpOcrGetWordResultStr(op_handle handle, const wchar_t *result, int index) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.GetWordResultStr(safe_text(result), index, ret);
    });
}

const wchar_t *OP_CALL OpOcrRecognize(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color, double sim) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) { op.Ocr(x1, y1, x2, y2, safe_text(color), sim, ret); });
}

const wchar_t *OP_CALL OpOcrRecognizeEx(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color, double sim) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) { op.OcrEx(x1, y1, x2, y2, safe_text(color), sim, ret); });
}

int OP_CALL OpOcrFindStr(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *strs, const wchar_t *color,
                      double sim, int *x, int *y) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        long lx = 0, ly = 0;
        op.FindStr(x1, y1, x2, y2, safe_text(strs), safe_text(color), sim, &lx, &ly, ret);
        out_int(x, lx);
        out_int(y, ly);
    });
}

const wchar_t *OP_CALL OpOcrFindStrEx(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *strs,
                                   const wchar_t *color, double sim) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.FindStrEx(x1, y1, x2, y2, safe_text(strs), safe_text(color), sim, ret);
    });
}

const wchar_t *OP_CALL OpOcrRecognizeAuto(op_handle handle, int x1, int y1, int x2, int y2, double sim) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) { op.OcrAuto(x1, y1, x2, y2, sim, ret); });
}

const wchar_t *OP_CALL OpOcrRecognizeFromFile(op_handle handle, const wchar_t *file_name, const wchar_t *color_format,
                                     double sim) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.OcrFromFile(safe_text(file_name), safe_text(color_format), sim, ret);
    });
}

const wchar_t *OP_CALL OpOcrRecognizeAutoFromFile(op_handle handle, const wchar_t *file_name, double sim) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) { op.OcrAutoFromFile(safe_text(file_name), sim, ret); });
}

const wchar_t *OP_CALL OpOcrFindLine(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                  double sim) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.FindLine(x1, y1, x2, y2, safe_text(color), sim, ret);
    });
}

// Memory

int OP_CALL OpMemoryWriteData(op_handle handle, intptr_t hwnd, const wchar_t *address, const wchar_t *data, int size) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.WriteData(static_cast<LONG_PTR>(hwnd), safe_text(address), safe_text(data), size, ret);
    });
}

const wchar_t *OP_CALL OpMemoryReadData(op_handle handle, intptr_t hwnd, const wchar_t *address, int size) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.ReadData(static_cast<LONG_PTR>(hwnd), safe_text(address), size, ret);
    });
}

int OP_CALL OpMemoryReadInt(op_handle handle, intptr_t hwnd, const wchar_t *address, int type, int64_t *value) {
    out_value(value, 0);
    if (!value)
        return 0;

    return call_memory(handle, hwnd, [&](ProcessMemory &mem, HWND target) {
        return mem.ReadInt(target, safe_text(address), type, value);
    });
}

int OP_CALL OpMemoryWriteInt(op_handle handle, intptr_t hwnd, const wchar_t *address, int type, int64_t value) {
    return call_memory(handle, hwnd, [&](ProcessMemory &mem, HWND target) {
        return mem.WriteInt(target, safe_text(address), type, value) != 0;
    });
}

int OP_CALL OpMemoryReadFloat(op_handle handle, intptr_t hwnd, const wchar_t *address, float *value) {
    out_value(value, 0.0f);
    if (!value)
        return 0;

    return call_memory(handle, hwnd, [&](ProcessMemory &mem, HWND target) {
        return mem.ReadFloat(target, safe_text(address), value);
    });
}

int OP_CALL OpMemoryWriteFloat(op_handle handle, intptr_t hwnd, const wchar_t *address, float value) {
    return call_memory(handle, hwnd, [&](ProcessMemory &mem, HWND target) {
        return mem.WriteFloat(target, safe_text(address), value) != 0;
    });
}

int OP_CALL OpMemoryReadDouble(op_handle handle, intptr_t hwnd, const wchar_t *address, double *value) {
    out_value(value, 0.0);
    if (!value)
        return 0;

    return call_memory(handle, hwnd, [&](ProcessMemory &mem, HWND target) {
        return mem.ReadDouble(target, safe_text(address), value);
    });
}

int OP_CALL OpMemoryWriteDouble(op_handle handle, intptr_t hwnd, const wchar_t *address, double value) {
    return call_memory(handle, hwnd, [&](ProcessMemory &mem, HWND target) {
        return mem.WriteDouble(target, safe_text(address), value) != 0;
    });
}

const wchar_t *OP_CALL OpMemoryReadString(op_handle handle, intptr_t hwnd, const wchar_t *address, int type, int len) {
    return call_string(handle, [&](op::Client &op, std::wstring &ret) {
        op.ReadString(static_cast<LONG_PTR>(hwnd), safe_text(address), type, len, ret);
    });
}

int OP_CALL OpMemoryWriteString(op_handle handle, intptr_t hwnd, const wchar_t *address, int type, const wchar_t *value) {
    return call_ret(handle, [&](op::Client &op, long *ret) {
        op.WriteString(static_cast<LONG_PTR>(hwnd), safe_text(address), type, safe_text(value), ret);
    });
}

#undef OP_WIDEN_TEXT
#undef OP_WIDEN_TEXT2
