#include "op_c_api.h"

#include "../../include/libop.h"
#include "../memory/ProcessMemory.h"
#include "../runtime/RuntimeEnvironment.h"

#include <Windows.h>
#include <cstdint>
#include <string>
#include <utility>

#undef FindWindow
#undef FindWindowEx
#undef SetWindowText

extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        RuntimeEnvironment::setInstance(hInstance);
    }
    return TRUE;
}

#define OP_WIDEN_TEXT2(text) L##text
#define OP_WIDEN_TEXT(text) OP_WIDEN_TEXT2(text)

using op::ProcessMemory;

struct op_c_context {
    op_c_context() = default;
    ~op_c_context() = default;

    op_c_context(const op_c_context &) = delete;
    op_c_context &operator=(const op_c_context &) = delete;

    op::Op op;
    std::wstring string_result;
};

namespace {

template <typename Func>
int call_int(op_handle handle, Func &&func) {
    op::Op *op = handle ? &handle->op : nullptr;
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
    return call_int(handle, [&](op::Op &op) {
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

const wchar_t *OP_CALL OpVer(void) {
    return OP_WIDEN_TEXT(OP_VERSION);
}

int OP_CALL OpSetPath(op_handle handle, const wchar_t *path) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.SetPath(safe_text(path), ret); });
}

const wchar_t *OP_CALL OpGetPath(op_handle handle) {
    return call_string(handle, [](op::Op &op, std::wstring &ret) { op.GetPath(ret); });
}

const wchar_t *OP_CALL OpGetBasePath(op_handle handle) {
    return call_string(handle, [](op::Op &op, std::wstring &ret) { op.GetBasePath(ret); });
}

int OP_CALL OpGetID(op_handle handle) {
    return call_ret(handle, [](op::Op &op, long *ret) { op.GetID(ret); });
}

int OP_CALL OpGetLastError(op_handle handle) {
    return call_ret(handle, [](op::Op &op, long *ret) { op.GetLastError(ret); });
}

int OP_CALL OpSetShowErrorMsg(op_handle handle, int show_type) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.SetShowErrorMsg(show_type, ret); });
}

int OP_CALL OpSleep(op_handle handle, int millseconds) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.Sleep(millseconds, ret); });
}

int OP_CALL OpInjectDll(op_handle handle, const wchar_t *process_name, const wchar_t *dll_name) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.InjectDll(safe_text(process_name), safe_text(dll_name), ret);
    });
}

int OP_CALL OpEnablePicCache(op_handle handle, int enable) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.EnablePicCache(enable, ret); });
}

int OP_CALL OpCapturePre(op_handle handle, const wchar_t *file_name) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.CapturePre(safe_text(file_name), ret); });
}

int OP_CALL OpSetScreenDataMode(op_handle handle, int mode) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.SetScreenDataMode(mode, ret); });
}

// Algorithm

const wchar_t *OP_CALL OpAStarFindPath(op_handle handle, int map_width, int map_height,
                                       const wchar_t *disable_points, int begin_x, int begin_y, int end_x,
                                       int end_y) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.AStarFindPath(map_width, map_height, safe_text(disable_points), begin_x, begin_y, end_x, end_y, ret);
    });
}

const wchar_t *OP_CALL OpFindNearestPos(op_handle handle, const wchar_t *all_pos, int type, int x, int y) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.FindNearestPos(safe_text(all_pos), type, x, y, ret);
    });
}

// Window and process

const wchar_t *OP_CALL OpEnumWindow(op_handle handle, intptr_t parent, const wchar_t *title,
                                    const wchar_t *class_name, int filter) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.EnumWindow(static_cast<LONG_PTR>(parent), safe_text(title), safe_text(class_name), filter, ret);
    });
}

const wchar_t *OP_CALL OpEnumWindowByProcess(op_handle handle, const wchar_t *process_name, const wchar_t *title,
                                             const wchar_t *class_name, int filter) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.EnumWindowByProcess(safe_text(process_name), safe_text(title), safe_text(class_name), filter, ret);
    });
}

const wchar_t *OP_CALL OpEnumProcess(op_handle handle, const wchar_t *name) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) { op.EnumProcess(safe_text(name), ret); });
}

int OP_CALL OpClientToScreen(op_handle handle, intptr_t hwnd, int *x, int *y) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        long lx = in_int(x);
        long ly = in_int(y);
        op.ClientToScreen(static_cast<LONG_PTR>(hwnd), &lx, &ly, ret);
        out_int(x, lx);
        out_int(y, ly);
    });
}

intptr_t OP_CALL OpFindWindow(op_handle handle, const wchar_t *class_name, const wchar_t *title) {
    return call_intptr(handle, [&](op::Op &op, LONG_PTR *ret) { op.FindWindow(safe_text(class_name), safe_text(title), ret); });
}

intptr_t OP_CALL OpFindWindowByProcess(op_handle handle, const wchar_t *process_name, const wchar_t *class_name,
                                       const wchar_t *title) {
    return call_intptr(handle, [&](op::Op &op, LONG_PTR *ret) {
        op.FindWindowByProcess(safe_text(process_name), safe_text(class_name), safe_text(title), ret);
    });
}

intptr_t OP_CALL OpFindWindowByProcessId(op_handle handle, int process_id, const wchar_t *class_name,
                                         const wchar_t *title) {
    return call_intptr(handle, [&](op::Op &op, LONG_PTR *ret) {
        op.FindWindowByProcessId(process_id, safe_text(class_name), safe_text(title), ret);
    });
}

intptr_t OP_CALL OpFindWindowEx(op_handle handle, intptr_t parent, const wchar_t *class_name, const wchar_t *title) {
    return call_intptr(handle, [&](op::Op &op, LONG_PTR *ret) {
        op.FindWindowEx(static_cast<LONG_PTR>(parent), safe_text(class_name), safe_text(title), ret);
    });
}

int OP_CALL OpGetClientRect(op_handle handle, intptr_t hwnd, int *x1, int *y1, int *x2, int *y2) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        long lx1 = 0, ly1 = 0, lx2 = 0, ly2 = 0;
        op.GetClientRect(static_cast<LONG_PTR>(hwnd), &lx1, &ly1, &lx2, &ly2, ret);
        out_int(x1, lx1);
        out_int(y1, ly1);
        out_int(x2, lx2);
        out_int(y2, ly2);
    });
}

int OP_CALL OpGetClientSize(op_handle handle, intptr_t hwnd, int *width, int *height) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        long w = 0, h = 0;
        op.GetClientSize(static_cast<LONG_PTR>(hwnd), &w, &h, ret);
        out_int(width, w);
        out_int(height, h);
    });
}

intptr_t OP_CALL OpGetForegroundFocus(op_handle handle) {
    return call_intptr(handle, [](op::Op &op, LONG_PTR *ret) { op.GetForegroundFocus(ret); });
}

intptr_t OP_CALL OpGetForegroundWindow(op_handle handle) {
    return call_intptr(handle, [](op::Op &op, LONG_PTR *ret) { op.GetForegroundWindow(ret); });
}

intptr_t OP_CALL OpGetMousePointWindow(op_handle handle) {
    return call_intptr(handle, [](op::Op &op, LONG_PTR *ret) { op.GetMousePointWindow(ret); });
}

intptr_t OP_CALL OpGetPointWindow(op_handle handle, int x, int y) {
    return call_intptr(handle, [&](op::Op &op, LONG_PTR *ret) { op.GetPointWindow(x, y, ret); });
}

const wchar_t *OP_CALL OpGetProcessInfo(op_handle handle, int pid) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) { op.GetProcessInfo(pid, ret); });
}

intptr_t OP_CALL OpGetSpecialWindow(op_handle handle, int flag) {
    return call_intptr(handle, [&](op::Op &op, LONG_PTR *ret) { op.GetSpecialWindow(flag, ret); });
}

intptr_t OP_CALL OpGetWindow(op_handle handle, intptr_t hwnd, int flag) {
    return call_intptr(handle, [&](op::Op &op, LONG_PTR *ret) {
        op.GetWindow(static_cast<LONG_PTR>(hwnd), flag, ret);
    });
}

const wchar_t *OP_CALL OpGetWindowClass(op_handle handle, intptr_t hwnd) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.GetWindowClass(static_cast<LONG_PTR>(hwnd), ret);
    });
}

int OP_CALL OpGetWindowProcessId(op_handle handle, intptr_t hwnd) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.GetWindowProcessId(static_cast<LONG_PTR>(hwnd), ret); });
}

const wchar_t *OP_CALL OpGetWindowProcessPath(op_handle handle, intptr_t hwnd) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.GetWindowProcessPath(static_cast<LONG_PTR>(hwnd), ret);
    });
}

int OP_CALL OpGetWindowRect(op_handle handle, intptr_t hwnd, int *x1, int *y1, int *x2, int *y2) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        long lx1 = 0, ly1 = 0, lx2 = 0, ly2 = 0;
        op.GetWindowRect(static_cast<LONG_PTR>(hwnd), &lx1, &ly1, &lx2, &ly2, ret);
        out_int(x1, lx1);
        out_int(y1, ly1);
        out_int(x2, lx2);
        out_int(y2, ly2);
    });
}

int OP_CALL OpGetWindowState(op_handle handle, intptr_t hwnd, int flag) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.GetWindowState(static_cast<LONG_PTR>(hwnd), flag, ret); });
}

const wchar_t *OP_CALL OpGetWindowTitle(op_handle handle, intptr_t hwnd) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.GetWindowTitle(static_cast<LONG_PTR>(hwnd), ret);
    });
}

int OP_CALL OpMoveWindow(op_handle handle, intptr_t hwnd, int x, int y) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.MoveWindow(static_cast<LONG_PTR>(hwnd), x, y, ret); });
}

int OP_CALL OpScreenToClient(op_handle handle, intptr_t hwnd, int *x, int *y) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        long lx = in_int(x);
        long ly = in_int(y);
        op.ScreenToClient(static_cast<LONG_PTR>(hwnd), &lx, &ly, ret);
        out_int(x, lx);
        out_int(y, ly);
    });
}

int OP_CALL OpSendPaste(op_handle handle, intptr_t hwnd) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.SendPaste(static_cast<LONG_PTR>(hwnd), ret); });
}

int OP_CALL OpSetClientSize(op_handle handle, intptr_t hwnd, int width, int height) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.SetClientSize(static_cast<LONG_PTR>(hwnd), width, height, ret);
    });
}

int OP_CALL OpSetWindowState(op_handle handle, intptr_t hwnd, int flag) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.SetWindowState(static_cast<LONG_PTR>(hwnd), flag, ret);
    });
}

int OP_CALL OpSetWindowSize(op_handle handle, intptr_t hwnd, int width, int height) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.SetWindowSize(static_cast<LONG_PTR>(hwnd), width, height, ret);
    });
}

int OP_CALL OpLayoutWindows(op_handle handle, const wchar_t *hwnds, int layout_type, int columns, int start_x,
                            int start_y, int gap_x, int gap_y, int size_mode, int window_width, int window_height,
                            int anchor_mode) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.LayoutWindows(safe_text(hwnds), layout_type, columns, start_x, start_y, gap_x, gap_y, size_mode,
                         window_width, window_height, anchor_mode, ret);
    });
}

int OP_CALL OpSetWindowText(op_handle handle, intptr_t hwnd, const wchar_t *title) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.SetWindowText(static_cast<LONG_PTR>(hwnd), safe_text(title), ret);
    });
}

int OP_CALL OpSetWindowTransparent(op_handle handle, intptr_t hwnd, int trans) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.SetWindowTransparent(static_cast<LONG_PTR>(hwnd), trans, ret);
    });
}

int OP_CALL OpSendString(op_handle handle, intptr_t hwnd, const wchar_t *str) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.SendString(static_cast<LONG_PTR>(hwnd), safe_text(str), ret);
    });
}

int OP_CALL OpSendStringIme(op_handle handle, intptr_t hwnd, const wchar_t *str) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.SendStringIme(static_cast<LONG_PTR>(hwnd), safe_text(str), ret);
    });
}

int OP_CALL OpRunApp(op_handle handle, const wchar_t *cmdline, int mode, uint32_t *pid) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        unsigned long local_pid = 0;
        op.RunApp(safe_text(cmdline), mode, &local_pid, ret);
        out_value(pid, local_pid);
    });
}

int OP_CALL OpWinExec(op_handle handle, const wchar_t *cmdline, int cmdshow) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.WinExec(safe_text(cmdline), cmdshow, ret); });
}

const wchar_t *OP_CALL OpGetCmdStr(op_handle handle, const wchar_t *cmd, int millseconds) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) { op.GetCmdStr(safe_text(cmd), millseconds, ret); });
}

int OP_CALL OpSetClipboard(op_handle handle, const wchar_t *str) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.SetClipboard(safe_text(str), ret); });
}

const wchar_t *OP_CALL OpGetClipboard(op_handle handle) {
    return call_string(handle, [](op::Op &op, std::wstring &ret) { op.GetClipboard(ret); });
}

int OP_CALL OpDelay(op_handle handle, int mis) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.Delay(mis, ret); });
}

int OP_CALL OpDelays(op_handle handle, int mis_min, int mis_max) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.Delays(mis_min, mis_max, ret); });
}

// Background binding

int OP_CALL OpBindWindow(op_handle handle, intptr_t hwnd, const wchar_t *display, const wchar_t *mouse,
                         const wchar_t *keypad, int mode) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.BindWindow(static_cast<LONG_PTR>(hwnd), safe_text(display), safe_text(mouse), safe_text(keypad), mode, ret);
    });
}

int OP_CALL OpBindWindowEx(op_handle handle, intptr_t display_hwnd, intptr_t input_hwnd, const wchar_t *display,
                           const wchar_t *mouse, const wchar_t *keypad, int mode) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.BindWindowEx(static_cast<LONG_PTR>(display_hwnd), static_cast<LONG_PTR>(input_hwnd), safe_text(display),
                        safe_text(mouse), safe_text(keypad), mode, ret);
    });
}

int OP_CALL OpUnBindWindow(op_handle handle) {
    return call_ret(handle, [](op::Op &op, long *ret) { op.UnBindWindow(ret); });
}

int OP_CALL OpUnbindWindow(op_handle handle) {
    return OpUnBindWindow(handle);
}

intptr_t OP_CALL OpGetBindWindow(op_handle handle) {
    return call_intptr(handle, [](op::Op &op, LONG_PTR *ret) { op.GetBindWindow(ret); });
}

int OP_CALL OpIsBind(op_handle handle) {
    return call_ret(handle, [](op::Op &op, long *ret) { op.IsBind(ret); });
}

// Mouse

int OP_CALL OpGetCursorPos(op_handle handle, int *x, int *y) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        long lx = 0, ly = 0;
        op.GetCursorPos(&lx, &ly, ret);
        out_int(x, lx);
        out_int(y, ly);
    });
}

const wchar_t *OP_CALL OpGetCursorShape(op_handle handle) {
    return call_string(handle, [](op::Op &op, std::wstring &ret) { op.GetCursorShape(ret); });
}

int OP_CALL OpMoveR(op_handle handle, int x, int y) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.MoveR(x, y, ret); });
}

int OP_CALL OpMoveTo(op_handle handle, int x, int y) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.MoveTo(x, y, ret); });
}

const wchar_t *OP_CALL OpMoveToEx(op_handle handle, int x, int y, int w, int h) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) { op.MoveToEx(x, y, w, h, ret); });
}

int OP_CALL OpMoveToSmooth(op_handle handle, int x, int y, int duration) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.MoveToSmooth(x, y, duration, ret); });
}

const wchar_t *OP_CALL OpMoveToExSmooth(op_handle handle, int x, int y, int w, int h, int duration) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) { op.MoveToExSmooth(x, y, w, h, duration, ret); });
}

int OP_CALL OpMovePath(op_handle handle, const wchar_t *path, int duration) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.MovePath(safe_text(path), duration, ret); });
}

int OP_CALL OpDragPath(op_handle handle, const wchar_t *path, int duration) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.DragPath(safe_text(path), duration, ret); });
}

int OP_CALL OpSetMouseTrajectory(op_handle handle, int mode, int min_duration, int max_duration, int jitter,
                                 int start_delay, int end_delay) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.SetMouseTrajectory(mode, min_duration, max_duration, jitter, start_delay, end_delay, ret);
    });
}

#define OP_MOUSE_RET(name, method) \
    int OP_CALL name(op_handle handle) { return call_ret(handle, [](op::Op &op, long *ret) { op.method(ret); }); }

OP_MOUSE_RET(OpLeftClick, LeftClick)
OP_MOUSE_RET(OpLeftDoubleClick, LeftDoubleClick)
OP_MOUSE_RET(OpLeftDown, LeftDown)
OP_MOUSE_RET(OpLeftUp, LeftUp)
OP_MOUSE_RET(OpMiddleClick, MiddleClick)
OP_MOUSE_RET(OpMiddleDoubleClick, MiddleDoubleClick)
OP_MOUSE_RET(OpMiddleDown, MiddleDown)
OP_MOUSE_RET(OpMiddleUp, MiddleUp)
OP_MOUSE_RET(OpRightClick, RightClick)
OP_MOUSE_RET(OpRightDoubleClick, RightDoubleClick)
OP_MOUSE_RET(OpRightDown, RightDown)
OP_MOUSE_RET(OpRightUp, RightUp)
OP_MOUSE_RET(OpXButton1Click, XButton1Click)
OP_MOUSE_RET(OpXButton1DoubleClick, XButton1DoubleClick)
OP_MOUSE_RET(OpXButton1Down, XButton1Down)
OP_MOUSE_RET(OpXButton1Up, XButton1Up)
OP_MOUSE_RET(OpXButton2Click, XButton2Click)
OP_MOUSE_RET(OpXButton2DoubleClick, XButton2DoubleClick)
OP_MOUSE_RET(OpXButton2Down, XButton2Down)
OP_MOUSE_RET(OpXButton2Up, XButton2Up)
OP_MOUSE_RET(OpWheelDown, WheelDown)
OP_MOUSE_RET(OpWheelUp, WheelUp)

#undef OP_MOUSE_RET

int OP_CALL OpWheel(op_handle handle, int delta) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.Wheel(delta, ret); });
}

int OP_CALL OpHWheel(op_handle handle, int delta) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.HWheel(delta, ret); });
}

int OP_CALL OpSetMouseDelay(op_handle handle, const wchar_t *type, int delay) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.SetMouseDelay(safe_text(type), delay, ret); });
}

// Keyboard

int OP_CALL OpGetKeyState(op_handle handle, int vk_code) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.GetKeyState(vk_code, ret); });
}

int OP_CALL OpKeyDown(op_handle handle, int vk_code) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.KeyDown(vk_code, ret); });
}

int OP_CALL OpKeyDownChar(op_handle handle, const wchar_t *vk_code) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.KeyDownChar(safe_text(vk_code), ret); });
}

int OP_CALL OpKeyUp(op_handle handle, int vk_code) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.KeyUp(vk_code, ret); });
}

int OP_CALL OpKeyUpChar(op_handle handle, const wchar_t *vk_code) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.KeyUpChar(safe_text(vk_code), ret); });
}

int OP_CALL OpWaitKey(op_handle handle, int vk_code, int time_out) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.WaitKey(vk_code, time_out, ret); });
}

int OP_CALL OpKeyPress(op_handle handle, int vk_code) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.KeyPress(vk_code, ret); });
}

int OP_CALL OpKeyPressChar(op_handle handle, const wchar_t *vk_code) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.KeyPressChar(safe_text(vk_code), ret); });
}

int OP_CALL OpSetKeypadDelay(op_handle handle, const wchar_t *type, int delay) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.SetKeypadDelay(safe_text(type), delay, ret); });
}

int OP_CALL OpKeyPressStr(op_handle handle, const wchar_t *key_str, int delay) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.KeyPressStr(safe_text(key_str), delay, ret); });
}

// Image and color

int OP_CALL OpCapture(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *file_name) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.Capture(x1, y1, x2, y2, safe_text(file_name), ret); });
}

int OP_CALL OpCmpColor(op_handle handle, int x, int y, const wchar_t *color, double sim) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.CmpColor(x, y, safe_text(color), sim, ret); });
}

int OP_CALL OpFindColor(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color, double sim, int dir,
                        int *x, int *y) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        long lx = 0, ly = 0;
        op.FindColor(x1, y1, x2, y2, safe_text(color), sim, dir, &lx, &ly, ret);
        out_int(x, lx);
        out_int(y, ly);
    });
}

const wchar_t *OP_CALL OpFindColorEx(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                     double sim, int dir) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.FindColorEx(x1, y1, x2, y2, safe_text(color), sim, dir, ret);
    });
}

int OP_CALL OpFindMultiColor(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *first_color,
                             const wchar_t *offset_color, double sim, int dir, int *x, int *y) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        long lx = 0, ly = 0;
        op.FindMultiColor(x1, y1, x2, y2, safe_text(first_color), safe_text(offset_color), sim, dir, &lx, &ly, ret);
        out_int(x, lx);
        out_int(y, ly);
    });
}

const wchar_t *OP_CALL OpFindMultiColorEx(op_handle handle, int x1, int y1, int x2, int y2,
                                          const wchar_t *first_color, const wchar_t *offset_color, double sim,
                                          int dir) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.FindMultiColorEx(x1, y1, x2, y2, safe_text(first_color), safe_text(offset_color), sim, dir, ret);
    });
}

int OP_CALL OpFindPic(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *files,
                      const wchar_t *delta_color, double sim, int dir, int *x, int *y) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        long lx = 0, ly = 0;
        op.FindPic(x1, y1, x2, y2, safe_text(files), safe_text(delta_color), sim, dir, &lx, &ly, ret);
        out_int(x, lx);
        out_int(y, ly);
    });
}

const wchar_t *OP_CALL OpFindPicEx(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *files,
                                   const wchar_t *delta_color, double sim, int dir) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.FindPicEx(x1, y1, x2, y2, safe_text(files), safe_text(delta_color), sim, dir, ret);
    });
}

const wchar_t *OP_CALL OpFindPicExS(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *files,
                                    const wchar_t *delta_color, double sim, int dir) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.FindPicExS(x1, y1, x2, y2, safe_text(files), safe_text(delta_color), sim, dir, ret);
    });
}

int OP_CALL OpFindColorBlock(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color, double sim,
                             int count, int height, int width, int *x, int *y) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        long lx = 0, ly = 0;
        op.FindColorBlock(x1, y1, x2, y2, safe_text(color), sim, count, height, width, &lx, &ly, ret);
        out_int(x, lx);
        out_int(y, ly);
    });
}

const wchar_t *OP_CALL OpFindColorBlockEx(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                          double sim, int count, int height, int width) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.FindColorBlockEx(x1, y1, x2, y2, safe_text(color), sim, count, height, width, ret);
    });
}

const wchar_t *OP_CALL OpGetColor(op_handle handle, int x, int y) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) { op.GetColor(x, y, ret); });
}

int OP_CALL OpGetColorNum(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color, double sim) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.GetColorNum(x1, y1, x2, y2, safe_text(color), sim, ret); });
}

int OP_CALL OpSetDisplayInput(op_handle handle, const wchar_t *mode) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.SetDisplayInput(safe_text(mode), ret); });
}

int OP_CALL OpLoadPic(op_handle handle, const wchar_t *file_name) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.LoadPic(safe_text(file_name), ret); });
}

int OP_CALL OpFreePic(op_handle handle, const wchar_t *file_name) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.FreePic(safe_text(file_name), ret); });
}

int OP_CALL OpLoadMemPic(op_handle handle, const wchar_t *file_name, void *data, int size) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.LoadMemPic(safe_text(file_name), data, size, ret); });
}

int OP_CALL OpGetPicSize(op_handle handle, const wchar_t *pic_name, int *width, int *height) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        long w = 0, h = 0;
        op.GetPicSize(safe_text(pic_name), &w, &h, ret);
        out_int(width, w);
        out_int(height, h);
    });
}

uintptr_t OP_CALL OpGetScreenData(op_handle handle, int x1, int y1, int x2, int y2, int *out_ret) {
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

uintptr_t OP_CALL OpGetScreenDataBmp(op_handle handle, int x1, int y1, int x2, int y2, int *size, int *out_ret) {
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

void OP_CALL OpGetScreenFrameInfo(op_handle handle, int *frame_id, int *time) {
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

const wchar_t *OP_CALL OpMatchPicName(op_handle handle, const wchar_t *pic_name) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) { op.MatchPicName(safe_text(pic_name), ret); });
}

// OpenCV

int OP_CALL OpCvLoadTemplate(op_handle handle, const wchar_t *name, const wchar_t *file_path) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.CvLoadTemplate(safe_text(name), safe_text(file_path), ret); });
}

int OP_CALL OpCvLoadMaskedTemplate(op_handle handle, const wchar_t *name, const wchar_t *template_path,
                                   const wchar_t *mask_path) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.CvLoadMaskedTemplate(safe_text(name), safe_text(template_path), safe_text(mask_path), ret);
    });
}

int OP_CALL OpCvRemoveTemplate(op_handle handle, const wchar_t *name) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.CvRemoveTemplate(safe_text(name), ret); });
}

int OP_CALL OpCvRemoveAllTemplates(op_handle handle) {
    return call_ret(handle, [](op::Op &op, long *ret) { op.CvRemoveAllTemplates(ret); });
}

int OP_CALL OpCvHasTemplate(op_handle handle, const wchar_t *name) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.CvHasTemplate(safe_text(name), ret); });
}

int OP_CALL OpCvGetTemplateCount(op_handle handle) {
    return call_ret(handle, [](op::Op &op, long *ret) { op.CvGetTemplateCount(ret); });
}

const wchar_t *OP_CALL OpCvGetAllTemplateNames(op_handle handle) {
    return call_string(handle, [](op::Op &op, std::wstring &ret) { op.CvGetAllTemplateNames(ret); });
}

const wchar_t *OP_CALL OpCvGetOpenCvVersion(op_handle handle) {
    return call_string(handle, [](op::Op &op, std::wstring &ret) { op.CvGetOpenCvVersion(ret); });
}

int OP_CALL OpCvLoadTemplateList(op_handle handle, const wchar_t *template_list) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.CvLoadTemplateList(safe_text(template_list), ret); });
}

#define OP_CV_FILE_RET(name, method) \
    int OP_CALL name(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file) { \
        return call_ret(handle, [&](op::Op &op, long *ret) { op.method(safe_text(src_file), safe_text(dst_file), ret); }); \
    }

OP_CV_FILE_RET(OpCvToGray, CvToGray)
OP_CV_FILE_RET(OpCvToBinary, CvToBinary)
OP_CV_FILE_RET(OpCvToEdge, CvToEdge)
OP_CV_FILE_RET(OpCvToOutline, CvToOutline)
OP_CV_FILE_RET(OpCvDenoise, CvDenoise)
OP_CV_FILE_RET(OpCvEqualize, CvEqualize)
OP_CV_FILE_RET(OpCvCropValid, CvCropValid)

#undef OP_CV_FILE_RET

int OP_CALL OpCvCLAHE(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file, double clip_limit,
                      int tile_grid_size) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.CvCLAHE(safe_text(src_file), safe_text(dst_file), clip_limit, tile_grid_size, ret);
    });
}

int OP_CALL OpCvBlur(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file, const wchar_t *mode,
                     int kernel_size) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.CvBlur(safe_text(src_file), safe_text(dst_file), safe_text(mode), kernel_size, ret);
    });
}

int OP_CALL OpCvSharpen(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file, double strength) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.CvSharpen(safe_text(src_file), safe_text(dst_file), strength, ret);
    });
}

const wchar_t *OP_CALL OpCvConnectedComponents(op_handle handle, const wchar_t *src_file, double min_area) {
    return call_string(handle, [&](op::Op &op, std::wstring &json) {
        long ret = 0;
        op.CvConnectedComponents(safe_text(src_file), min_area, json, &ret);
    });
}

const wchar_t *OP_CALL OpCvFindContours(op_handle handle, const wchar_t *src_file, double min_area) {
    return call_string(handle, [&](op::Op &op, std::wstring &json) {
        long ret = 0;
        op.CvFindContours(safe_text(src_file), min_area, json, &ret);
    });
}

int OP_CALL OpCvPreprocessPipeline(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                                   const wchar_t *pipeline) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.CvPreprocessPipeline(safe_text(src_file), safe_text(dst_file), safe_text(pipeline), ret);
    });
}

int OP_CALL OpCvCrop(op_handle handle, const wchar_t *src_file, int x, int y, int width, int height,
                     const wchar_t *dst_file) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.CvCrop(safe_text(src_file), x, y, width, height, safe_text(dst_file), ret);
    });
}

int OP_CALL OpCvResize(op_handle handle, const wchar_t *src_file, int width, int height, const wchar_t *dst_file) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.CvResize(safe_text(src_file), width, height, safe_text(dst_file), ret);
    });
}

int OP_CALL OpCvThreshold(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file, double threshold,
                          double max_value, const wchar_t *mode) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.CvThreshold(safe_text(src_file), safe_text(dst_file), threshold, max_value, safe_text(mode), ret);
    });
}

int OP_CALL OpCvInRange(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file,
                        const wchar_t *color_space, const wchar_t *lower, const wchar_t *upper) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.CvInRange(safe_text(src_file), safe_text(dst_file), safe_text(color_space), safe_text(lower),
                     safe_text(upper), ret);
    });
}

int OP_CALL OpCvMorphology(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file, const wchar_t *mode,
                           int kernel_size, int iterations) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.CvMorphology(safe_text(src_file), safe_text(dst_file), safe_text(mode), kernel_size, iterations, ret);
    });
}

int OP_CALL OpCvThin(op_handle handle, const wchar_t *src_file, const wchar_t *dst_file, const wchar_t *mode) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.CvThin(safe_text(src_file), safe_text(dst_file), safe_text(mode), ret);
    });
}

const wchar_t *OP_CALL OpCvMatchTemplate(op_handle handle, int x, int y, int width, int height,
                                         const wchar_t *template_name, double threshold, int dir, int strip_mode,
                                         int method, int color_mode) {
    return call_string(handle, [&](op::Op &op, std::wstring &json) {
        long ret = 0;
        op.CvMatchTemplate(x, y, width, height, safe_text(template_name), threshold, dir, strip_mode, method,
                           color_mode, json, &ret);
    });
}

const wchar_t *OP_CALL OpCvMatchTemplateScale(op_handle handle, int x, int y, int width, int height,
                                              const wchar_t *template_name, const wchar_t *scales, double threshold,
                                              int method, int color_mode) {
    return call_string(handle, [&](op::Op &op, std::wstring &json) {
        long ret = 0;
        op.CvMatchTemplateScale(x, y, width, height, safe_text(template_name), safe_text(scales), threshold, method,
                                color_mode, json, &ret);
    });
}

const wchar_t *OP_CALL OpCvMatchAnyTemplate(op_handle handle, int x, int y, int width, int height,
                                            const wchar_t *template_names, double threshold, int dir,
                                            int strip_mode, int method, int color_mode) {
    return call_string(handle, [&](op::Op &op, std::wstring &json) {
        long ret = 0;
        op.CvMatchAnyTemplate(x, y, width, height, safe_text(template_names), threshold, dir, strip_mode, method,
                              color_mode, json, &ret);
    });
}

const wchar_t *OP_CALL OpCvMatchAllTemplates(op_handle handle, int x, int y, int width, int height,
                                             const wchar_t *template_names, double threshold, int dir,
                                             int strip_mode, int method, int color_mode) {
    return call_string(handle, [&](op::Op &op, std::wstring &json) {
        long ret = 0;
        op.CvMatchAllTemplates(x, y, width, height, safe_text(template_names), threshold, dir, strip_mode, method,
                               color_mode, json, &ret);
    });
}

const wchar_t *OP_CALL OpCvFeatureMatchTemplate(op_handle handle, int x, int y, int width, int height,
                                                const wchar_t *template_name, double threshold) {
    return call_string(handle, [&](op::Op &op, std::wstring &json) {
        long ret = 0;
        op.CvFeatureMatchTemplate(x, y, width, height, safe_text(template_name), threshold, json, &ret);
    });
}

const wchar_t *OP_CALL OpCvEdgeMatchTemplate(op_handle handle, int x, int y, int width, int height,
                                             const wchar_t *template_name, double threshold) {
    return call_string(handle, [&](op::Op &op, std::wstring &json) {
        long ret = 0;
        op.CvEdgeMatchTemplate(x, y, width, height, safe_text(template_name), threshold, json, &ret);
    });
}

const wchar_t *OP_CALL OpCvShapeMatchTemplate(op_handle handle, int x, int y, int width, int height,
                                              const wchar_t *template_name, double threshold) {
    return call_string(handle, [&](op::Op &op, std::wstring &json) {
        long ret = 0;
        op.CvShapeMatchTemplate(x, y, width, height, safe_text(template_name), threshold, json, &ret);
    });
}

// OCR

int OP_CALL OpSetOcrEngine(op_handle handle, const wchar_t *path_of_engine, const wchar_t *dll_name,
                           const wchar_t *argv) {
    return call_int(handle, [&](op::Op &op) {
        return op.SetOcrEngine(safe_text(path_of_engine), safe_text(dll_name), safe_text(argv));
    });
}

int OP_CALL OpSetYoloEngine(op_handle handle, const wchar_t *path_of_engine, const wchar_t *dll_name,
                            const wchar_t *argv) {
    return call_int(handle, [&](op::Op &op) {
        return op.SetYoloEngine(safe_text(path_of_engine), safe_text(dll_name), safe_text(argv));
    });
}

const wchar_t *OP_CALL OpYoloDetect(op_handle handle, int x1, int y1, int x2, int y2, double conf, double iou) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        long status = 0;
        op.YoloDetect(x1, y1, x2, y2, conf, iou, ret, &status);
    });
}

const wchar_t *OP_CALL OpYoloDetectFromFile(op_handle handle, const wchar_t *file_name, double conf, double iou) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        long status = 0;
        op.YoloDetectFromFile(safe_text(file_name), conf, iou, ret, &status);
    });
}

int OP_CALL OpSetDict(op_handle handle, int idx, const wchar_t *file_name) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.SetDict(idx, safe_text(file_name), ret); });
}

const wchar_t *OP_CALL OpGetDict(op_handle handle, int idx, int font_index) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) { op.GetDict(idx, font_index, ret); });
}

int OP_CALL OpSetMemDict(op_handle handle, int idx, const void *data, int size) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.SetMemDict(idx, data, size, ret); });
}

int OP_CALL OpUseDict(op_handle handle, int idx) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.UseDict(idx, ret); });
}

int OP_CALL OpAddDict(op_handle handle, int idx, const wchar_t *dict_info) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.AddDict(idx, safe_text(dict_info), ret); });
}

int OP_CALL OpSaveDict(op_handle handle, int idx, const wchar_t *file_name) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.SaveDict(idx, safe_text(file_name), ret); });
}

int OP_CALL OpClearDict(op_handle handle, int idx) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.ClearDict(idx, ret); });
}

int OP_CALL OpGetDictCount(op_handle handle, int idx) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.GetDictCount(idx, ret); });
}

int OP_CALL OpGetNowDict(op_handle handle) {
    return call_ret(handle, [](op::Op &op, long *ret) { op.GetNowDict(ret); });
}

int OP_CALL OpSetBinaryPreprocess(op_handle handle, int mode, int isolated_threshold, int min_component_area,
                                  int bridge_gap) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.SetBinaryPreprocess(mode, isolated_threshold, min_component_area, bridge_gap, ret);
    });
}

int OP_CALL OpGetBinaryPreprocess(op_handle handle, int *mode, int *isolated_threshold, int *min_component_area,
                                  int *bridge_gap) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        long local_mode = 0;
        long local_isolated_threshold = 0;
        long local_min_component_area = 0;
        long local_bridge_gap = 0;
        op.GetBinaryPreprocess(&local_mode, &local_isolated_threshold, &local_min_component_area, &local_bridge_gap,
                               ret);
        out_int(mode, local_mode);
        out_int(isolated_threshold, local_isolated_threshold);
        out_int(min_component_area, local_min_component_area);
        out_int(bridge_gap, local_bridge_gap);
    });
}

const wchar_t *OP_CALL OpFetchWord(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                   const wchar_t *word) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.FetchWord(x1, y1, x2, y2, safe_text(color), safe_text(word), ret);
    });
}

const wchar_t *OP_CALL OpFetchWordEx(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                     double sim, const wchar_t *word) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.FetchWordEx(x1, y1, x2, y2, safe_text(color), sim, safe_text(word), ret);
    });
}

const wchar_t *OP_CALL OpExtractWordRects(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                          double sim, int min_word_h) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.ExtractWordRects(x1, y1, x2, y2, safe_text(color), sim, min_word_h, ret);
    });
}

const wchar_t *OP_CALL OpExtractWordRectsEx(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                            double sim, int min_word_w, int min_word_h, int padding) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.ExtractWordRectsEx(x1, y1, x2, y2, safe_text(color), sim, min_word_w, min_word_h, padding, ret);
    });
}

const wchar_t *OP_CALL OpFetchWords(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                    double sim, const wchar_t *words, int min_word_h) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.FetchWords(x1, y1, x2, y2, safe_text(color), sim, safe_text(words), min_word_h, ret);
    });
}

const wchar_t *OP_CALL OpFetchWordsEx(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                      double sim, const wchar_t *words, int min_word_w, int min_word_h, int padding) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.FetchWordsEx(x1, y1, x2, y2, safe_text(color), sim, safe_text(words), min_word_w, min_word_h, padding, ret);
    });
}

const wchar_t *OP_CALL OpFetchWordsByRects(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                           double sim, const wchar_t *words, const wchar_t *rects) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.FetchWordsByRects(x1, y1, x2, y2, safe_text(color), sim, safe_text(words), safe_text(rects), ret);
    });
}

const wchar_t *OP_CALL OpGetBinaryPreview(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                          double sim, int *ret) {
    return call_string(handle, [&](op::Op &op, std::wstring &out) {
        long point_count = 0;
        op.GetBinaryPreview(x1, y1, x2, y2, safe_text(color), sim, out, &point_count);
        out_int(ret, point_count);
    });
}

const wchar_t *OP_CALL OpGetWordPreview(op_handle handle, const wchar_t *dict_info, int *ret) {
    return call_string(handle, [&](op::Op &op, std::wstring &out) {
        long valid = 0;
        op.GetWordPreview(safe_text(dict_info), out, &valid);
        out_int(ret, valid);
    });
}

const wchar_t *OP_CALL OpCheckWordDict(op_handle handle, const wchar_t *dict_info, int *ret) {
    return call_string(handle, [&](op::Op &op, std::wstring &out) {
        long valid_count = 0;
        op.CheckWordDict(safe_text(dict_info), out, &valid_count);
        out_int(ret, valid_count);
    });
}

const wchar_t *OP_CALL OpNormalizeWordDict(op_handle handle, const wchar_t *dict_info, int *ret) {
    return call_string(handle, [&](op::Op &op, std::wstring &out) {
        long valid_count = 0;
        op.NormalizeWordDict(safe_text(dict_info), out, &valid_count);
        out_int(ret, valid_count);
    });
}

const wchar_t *OP_CALL OpRenameWordDict(op_handle handle, const wchar_t *dict_info, const wchar_t *words, int *ret) {
    return call_string(handle, [&](op::Op &op, std::wstring &out) {
        long renamed_count = 0;
        op.RenameWordDict(safe_text(dict_info), safe_text(words), out, &renamed_count);
        out_int(ret, renamed_count);
    });
}

const wchar_t *OP_CALL OpGetWordsNoDict(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.GetWordsNoDict(x1, y1, x2, y2, safe_text(color), ret);
    });
}

int OP_CALL OpGetWordResultCount(op_handle handle, const wchar_t *result) {
    return call_ret(handle, [&](op::Op &op, long *ret) { op.GetWordResultCount(safe_text(result), ret); });
}

int OP_CALL OpGetWordResultPos(op_handle handle, const wchar_t *result, int index, int *x, int *y) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        long lx = 0, ly = 0;
        op.GetWordResultPos(safe_text(result), index, &lx, &ly, ret);
        out_int(x, lx);
        out_int(y, ly);
    });
}

const wchar_t *OP_CALL OpGetWordResultStr(op_handle handle, const wchar_t *result, int index) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.GetWordResultStr(safe_text(result), index, ret);
    });
}

const wchar_t *OP_CALL OpOcr(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color, double sim) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) { op.Ocr(x1, y1, x2, y2, safe_text(color), sim, ret); });
}

const wchar_t *OP_CALL OpOcrEx(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color, double sim) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) { op.OcrEx(x1, y1, x2, y2, safe_text(color), sim, ret); });
}

int OP_CALL OpFindStr(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *strs, const wchar_t *color,
                      double sim, int *x, int *y) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        long lx = 0, ly = 0;
        op.FindStr(x1, y1, x2, y2, safe_text(strs), safe_text(color), sim, &lx, &ly, ret);
        out_int(x, lx);
        out_int(y, ly);
    });
}

const wchar_t *OP_CALL OpFindStrEx(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *strs,
                                   const wchar_t *color, double sim) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.FindStrEx(x1, y1, x2, y2, safe_text(strs), safe_text(color), sim, ret);
    });
}

const wchar_t *OP_CALL OpOcrAuto(op_handle handle, int x1, int y1, int x2, int y2, double sim) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) { op.OcrAuto(x1, y1, x2, y2, sim, ret); });
}

const wchar_t *OP_CALL OpOcrFromFile(op_handle handle, const wchar_t *file_name, const wchar_t *color_format,
                                     double sim) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.OcrFromFile(safe_text(file_name), safe_text(color_format), sim, ret);
    });
}

const wchar_t *OP_CALL OpOcrAutoFromFile(op_handle handle, const wchar_t *file_name, double sim) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) { op.OcrAutoFromFile(safe_text(file_name), sim, ret); });
}

const wchar_t *OP_CALL OpFindLine(op_handle handle, int x1, int y1, int x2, int y2, const wchar_t *color,
                                  double sim) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.FindLine(x1, y1, x2, y2, safe_text(color), sim, ret);
    });
}

// Memory

int OP_CALL OpWriteData(op_handle handle, intptr_t hwnd, const wchar_t *address, const wchar_t *data, int size) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.WriteData(static_cast<LONG_PTR>(hwnd), safe_text(address), safe_text(data), size, ret);
    });
}

const wchar_t *OP_CALL OpReadData(op_handle handle, intptr_t hwnd, const wchar_t *address, int size) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.ReadData(static_cast<LONG_PTR>(hwnd), safe_text(address), size, ret);
    });
}

int OP_CALL OpReadInt(op_handle handle, intptr_t hwnd, const wchar_t *address, int type, int64_t *value) {
    out_value(value, 0);
    if (!value)
        return 0;

    return call_memory(handle, hwnd, [&](ProcessMemory &mem, HWND target) {
        return mem.ReadInt(target, safe_text(address), type, value);
    });
}

int OP_CALL OpWriteInt(op_handle handle, intptr_t hwnd, const wchar_t *address, int type, int64_t value) {
    return call_memory(handle, hwnd, [&](ProcessMemory &mem, HWND target) {
        return mem.WriteInt(target, safe_text(address), type, value) != 0;
    });
}

int OP_CALL OpReadFloat(op_handle handle, intptr_t hwnd, const wchar_t *address, float *value) {
    out_value(value, 0.0f);
    if (!value)
        return 0;

    return call_memory(handle, hwnd, [&](ProcessMemory &mem, HWND target) {
        return mem.ReadFloat(target, safe_text(address), value);
    });
}

int OP_CALL OpWriteFloat(op_handle handle, intptr_t hwnd, const wchar_t *address, float value) {
    return call_memory(handle, hwnd, [&](ProcessMemory &mem, HWND target) {
        return mem.WriteFloat(target, safe_text(address), value) != 0;
    });
}

int OP_CALL OpReadDouble(op_handle handle, intptr_t hwnd, const wchar_t *address, double *value) {
    out_value(value, 0.0);
    if (!value)
        return 0;

    return call_memory(handle, hwnd, [&](ProcessMemory &mem, HWND target) {
        return mem.ReadDouble(target, safe_text(address), value);
    });
}

int OP_CALL OpWriteDouble(op_handle handle, intptr_t hwnd, const wchar_t *address, double value) {
    return call_memory(handle, hwnd, [&](ProcessMemory &mem, HWND target) {
        return mem.WriteDouble(target, safe_text(address), value) != 0;
    });
}

const wchar_t *OP_CALL OpReadString(op_handle handle, intptr_t hwnd, const wchar_t *address, int type, int len) {
    return call_string(handle, [&](op::Op &op, std::wstring &ret) {
        op.ReadString(static_cast<LONG_PTR>(hwnd), safe_text(address), type, len, ret);
    });
}

int OP_CALL OpWriteString(op_handle handle, intptr_t hwnd, const wchar_t *address, int type, const wchar_t *value) {
    return call_ret(handle, [&](op::Op &op, long *ret) {
        op.WriteString(static_cast<LONG_PTR>(hwnd), safe_text(address), type, safe_text(value), ret);
    });
}

#undef OP_WIDEN_TEXT
#undef OP_WIDEN_TEXT2
