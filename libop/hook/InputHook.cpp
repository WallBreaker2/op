#include "InputHook.h"
#include "../runtime/RuntimeUtils.h"
#include "../runtime/RuntimeEnvironment.h"
#include "../input/mouse/CursorShape.h"
#include "../hook/ApiResolver.h"
#include "MinHookRuntime.h"
#include "MinHook.h"
#include "HookProtocol.h"

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include "dinput.h"
#include <algorithm>
#include <array>
#include <atlbase.h>
#include <cstring>
#include <deque>
#include <mutex>
#include <vector>

namespace op::hook {

const GUID OP_IID_IDirectInput8W = {0xBF798031, 0x483A, 0x4DA2, 0xAA, 0x99, 0x5D, 0x64, 0xED, 0x36, 0x97, 0x00};
const GUID OP_GUID_SysMouse = {0x6F1D2B60, 0xD5A0, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};
const GUID OP_GUID_SysKeyboard = {0x6F1D2B61, 0xD5A0, 0x11CF, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};

HWND InputHook::input_hwnd = nullptr;
MouseState InputHook::m_mouseState = {};
BYTE InputHook::m_keyboardState[256] = {};
BYTE InputHook::m_vkState[256] = {};
LONG InputHook::m_lastMouseX = 0;
LONG InputHook::m_lastMouseY = 0;
LONG InputHook::m_wheelDelta = 0;
HCURSOR InputHook::m_cursor = nullptr;
bool InputHook::m_cursorVisible = false;
unsigned long long InputHook::m_cursorHash = 0;
unsigned long long InputHook::m_cursorMeta = 0;
bool InputHook::is_hooked = false;

namespace {

WNDPROC g_rawWindowProc = nullptr;
void *g_mouseGetDeviceStateRaw = nullptr;
void *g_keyboardGetDeviceStateRaw = nullptr;
void *g_mouseGetDeviceDataRaw = nullptr;
void *g_keyboardGetDeviceDataRaw = nullptr;
void *g_getKeyStateRaw = nullptr;
void *g_getAsyncKeyStateRaw = nullptr;
void *g_getKeyboardStateRaw = nullptr;
void *g_getRawInputDataRaw = nullptr;
void *g_getRawInputBufferRaw = nullptr;
void *g_registerRawInputDevicesRaw = nullptr;
void *g_getRawInputDeviceInfoWRaw = nullptr;
void *g_getRawInputDeviceInfoARaw = nullptr;
void *g_getRawInputDeviceListRaw = nullptr;
void *g_getRegisteredRawInputDevicesRaw = nullptr;
void *g_setCursorRaw = nullptr;
std::array<void *, 32> g_mouseVtable{};
std::array<void *, 32> g_keyboardVtable{};
void **g_mouseVtablePtr = nullptr;
void **g_keyboardVtablePtr = nullptr;
std::vector<void *> g_hookTargets;
std::mutex g_eventMutex;
std::deque<DIDEVICEOBJECTDATA> g_mouseEvents;
std::deque<DIDEVICEOBJECTDATA> g_keyboardEvents;
std::deque<std::pair<HRAWINPUT, RAWINPUT>> g_rawEvents;
RAWINPUTDEVICE g_registeredMouse = {};
RAWINPUTDEVICE g_registeredKeyboard = {};
bool g_rawMouseRegistered = false;
bool g_rawKeyboardRegistered = false;
DWORD g_eventSequence = 0;
UINT_PTR g_rawSequence = 0;
constexpr int kAcquireIndex = 7;
constexpr int kGetDeviceStateIndex = 9;
constexpr int kGetDeviceDataIndex = 10;
constexpr int kPollIndex = 25;
constexpr size_t kDinputEventLimit = 128;
constexpr size_t kRawEventLimit = 128;
constexpr USHORT kHidUsagePageGeneric = 0x01;
constexpr USHORT kHidUsageMouse = 0x02;
constexpr USHORT kHidUsageKeyboard = 0x06;
#ifdef _WIN64
constexpr UINT_PTR kFakeRawBase = 0x4F50000000000000ull;
constexpr UINT_PTR kFakeRawMask = 0xFFFF000000000000ull;
constexpr UINT_PTR kFakeRawMouseDevice = 0x4F51000000000001ull;
constexpr UINT_PTR kFakeRawKeyboardDevice = 0x4F51000000000002ull;
#else
constexpr UINT_PTR kFakeRawBase = 0x4F500000u;
constexpr UINT_PTR kFakeRawMask = 0xFFFF0000u;
constexpr UINT_PTR kFakeRawMouseDevice = 0x4F510001u;
constexpr UINT_PTR kFakeRawKeyboardDevice = 0x4F510002u;
#endif
constexpr wchar_t kFakeMouseName[] =
    L"\\\\?\\HID#VID_4F50&PID_0001&MI_00#OP_RAW_MOUSE#{378de44c-56ef-11d1-bc8c-00a0c91405dd}";
constexpr wchar_t kFakeKeyboardName[] =
    L"\\\\?\\HID#VID_4F50&PID_0002&MI_00#OP_RAW_KEYBOARD#{884b96c3-56ef-11d1-bc8c-00a0c91405dd}";
constexpr char kFakeMouseNameA[] =
    "\\\\?\\HID#VID_4F50&PID_0001&MI_00#OP_RAW_MOUSE#{378de44c-56ef-11d1-bc8c-00a0c91405dd}";
constexpr char kFakeKeyboardNameA[] =
    "\\\\?\\HID#VID_4F50&PID_0002&MI_00#OP_RAW_KEYBOARD#{884b96c3-56ef-11d1-bc8c-00a0c91405dd}";

using ATL::CComPtr;

HRESULT __stdcall hkAcquire(IDirectInputDevice8W *device);
HRESULT __stdcall hkPoll(IDirectInputDevice8W *device);
HRESULT __stdcall hkGetDeviceState(IDirectInputDevice8W *device, DWORD size, LPVOID ptr);
HRESULT __stdcall hkGetDeviceData(IDirectInputDevice8W *device, DWORD object_size, LPDIDEVICEOBJECTDATA data,
                                  LPDWORD count, DWORD flags);
SHORT WINAPI hkGetKeyState(int vk);
SHORT WINAPI hkGetAsyncKeyState(int vk);
BOOL WINAPI hkGetKeyboardState(PBYTE key_state);
UINT WINAPI hkGetRawInputData(HRAWINPUT raw_input, UINT command, LPVOID data, PUINT size, UINT header_size);
UINT WINAPI hkGetRawInputBuffer(PRAWINPUT data, PUINT size, UINT header_size);
BOOL WINAPI hkRegisterRawInputDevices(PCRAWINPUTDEVICE devices, UINT count, UINT size);
UINT WINAPI hkGetRawInputDeviceInfoW(HANDLE device, UINT command, LPVOID data, PUINT size);
UINT WINAPI hkGetRawInputDeviceInfoA(HANDLE device, UINT command, LPVOID data, PUINT size);
UINT WINAPI hkGetRawInputDeviceList(PRAWINPUTDEVICELIST devices, PUINT count, UINT size);
UINT WINAPI hkGetRegisteredRawInputDevices(PRAWINPUTDEVICE devices, PUINT count, UINT size);
HCURSOR WINAPI hkSetCursor(HCURSOR cursor);
LRESULT CALLBACK opWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
bool is_extended_vk(WPARAM vk);

bool create_hook(void *target, void *detour, void **original = nullptr) {
    if (!target || !detour)
        return false;

    const MH_STATUS status = MH_CreateHook(target, detour, original);
    if (status != MH_OK && status != MH_ERROR_ALREADY_CREATED)
        return false;

    if (std::find(g_hookTargets.begin(), g_hookTargets.end(), target) == g_hookTargets.end())
        g_hookTargets.push_back(target);
    return true;
}

void enable_input_hooks() {
    for (void *target : g_hookTargets)
        MH_EnableHook(target);
}

void remove_input_hooks() {
    for (void *target : g_hookTargets) {
        MH_DisableHook(target);
        MH_RemoveHook(target);
    }
    g_hookTargets.clear();
}
WORD scan_code(WPARAM vk);

template <typename Target, typename Value> void set_out(Target *target, Value value) {
    if (target)
        *target = static_cast<Target>(value);
}

using GetDeviceStateFn = HRESULT(__stdcall *)(IDirectInputDevice8W *, DWORD, LPVOID);
using GetDeviceDataFn = HRESULT(__stdcall *)(IDirectInputDevice8W *, DWORD, LPDIDEVICEOBJECTDATA, LPDWORD, DWORD);
using GetKeyStateFn = SHORT(WINAPI *)(int);
using GetKeyboardStateFn = BOOL(WINAPI *)(PBYTE);
using GetRawInputDataFn = UINT(WINAPI *)(HRAWINPUT, UINT, LPVOID, PUINT, UINT);
using GetRawInputBufferFn = UINT(WINAPI *)(PRAWINPUT, PUINT, UINT);
using RegisterRawInputDevicesFn = BOOL(WINAPI *)(PCRAWINPUTDEVICE, UINT, UINT);
using GetRawInputDeviceInfoWFn = UINT(WINAPI *)(HANDLE, UINT, LPVOID, PUINT);
using GetRawInputDeviceInfoAFn = UINT(WINAPI *)(HANDLE, UINT, LPVOID, PUINT);
using GetRawInputDeviceListFn = UINT(WINAPI *)(PRAWINPUTDEVICELIST, PUINT, UINT);
using GetRegisteredRawInputDevicesFn = UINT(WINAPI *)(PRAWINPUTDEVICE, PUINT, UINT);
using SetCursorFn = HCURSOR(WINAPI *)(HCURSOR);

bool same_vtable(IDirectInputDevice8W *device, void **vtable) {
    return device && vtable && *(void ***)device == vtable;
}

bool fake_raw_handle(HRAWINPUT handle) {
    const UINT_PTR value = reinterpret_cast<UINT_PTR>(handle);
    return (value & kFakeRawMask) == kFakeRawBase;
}

bool fake_mouse_device(HANDLE device) {
    return reinterpret_cast<UINT_PTR>(device) == kFakeRawMouseDevice;
}

bool fake_keyboard_device(HANDLE device) {
    return reinterpret_cast<UINT_PTR>(device) == kFakeRawKeyboardDevice;
}

bool fake_raw_device(HANDLE device) {
    return fake_mouse_device(device) || fake_keyboard_device(device);
}

void push_dinput_event(std::deque<DIDEVICEOBJECTDATA> &events, DWORD offset, DWORD data) {
    if (events.size() >= kDinputEventLimit)
        events.pop_front();

    DIDEVICEOBJECTDATA event = {};
    event.dwOfs = offset;
    event.dwData = data;
    event.dwTimeStamp = ::GetTickCount();
    event.dwSequence = ++g_eventSequence;
    events.push_back(event);
}

void push_raw_event(const RAWINPUT &raw) {
    if (g_rawEvents.size() >= kRawEventLimit)
        g_rawEvents.pop_front();

    const HRAWINPUT handle = reinterpret_cast<HRAWINPUT>(kFakeRawBase | (++g_rawSequence & 0xFFFF));
    g_rawEvents.emplace_back(handle, raw);
    if (InputHook::input_hwnd)
        ::PostMessage(InputHook::input_hwnd, WM_INPUT, RIM_INPUT, reinterpret_cast<LPARAM>(handle));
}

RAWINPUT *find_raw_event(HRAWINPUT handle) {
    for (auto &event : g_rawEvents) {
        if (event.first == handle)
            return &event.second;
    }
    return nullptr;
}

DWORD mouse_button_offset(int key) {
    return static_cast<DWORD>(FIELD_OFFSET(DIMOUSESTATE2, rgbButtons) + key);
}

DWORD key_data(bool down) {
    return down ? 0x80 : 0;
}

bool raw_usage_is_mouse(const RAWINPUTDEVICE &device) {
    return device.usUsagePage == kHidUsagePageGeneric && device.usUsage == kHidUsageMouse;
}

bool raw_usage_is_keyboard(const RAWINPUTDEVICE &device) {
    return device.usUsagePage == kHidUsagePageGeneric && device.usUsage == kHidUsageKeyboard;
}

RAWINPUTDEVICE make_registered_device(USHORT usage, HWND hwnd) {
    RAWINPUTDEVICE device = {};
    device.usUsagePage = kHidUsagePageGeneric;
    device.usUsage = usage;
    device.dwFlags = 0;
    device.hwndTarget = hwnd;
    return device;
}

bool same_guid(const GUID &lhs, const GUID &rhs) {
    return std::memcmp(&lhs, &rhs, sizeof(GUID)) == 0;
}

bool dinput_device_kind(IDirectInputDevice8W *device, bool &is_mouse, bool &is_keyboard) {
    is_mouse = false;
    is_keyboard = false;
    if (!device)
        return false;

    DIDEVICEINSTANCEW info = {};
    info.dwSize = sizeof(info);
    if (FAILED(device->GetDeviceInfo(&info)))
        return false;

    const DWORD type = GET_DIDEVICE_TYPE(info.dwDevType);
    is_mouse = type == DI8DEVTYPE_MOUSE || same_guid(info.guidInstance, OP_GUID_SysMouse);
    is_keyboard = type == DI8DEVTYPE_KEYBOARD || same_guid(info.guidInstance, OP_GUID_SysKeyboard);
    return is_mouse || is_keyboard;
}

bool load_dinput_vtables() {
    using DirectInput8CreateFn = decltype(DirectInput8Create);
    auto directInput8Create = reinterpret_cast<DirectInput8CreateFn *>(ResolveApi("dinput8.dll", "DirectInput8Create"));
    if (!directInput8Create) {
        setlog("dinput8 not found");
        return false;
    }

    CComPtr<IDirectInput8W> directInput;
    if (directInput8Create(::GetModuleHandle(NULL), DIRECTINPUT_VERSION, OP_IID_IDirectInput8W,
                           reinterpret_cast<void **>(&directInput), NULL) < 0) {
        setlog("DirectInput8Create failed");
        return false;
    }

    CComPtr<IDirectInputDevice8W> mouse;
    CComPtr<IDirectInputDevice8W> keyboard;
    const bool mouse_ok = directInput->CreateDevice(OP_GUID_SysMouse, &mouse, NULL) >= 0;
    const bool keyboard_ok = directInput->CreateDevice(OP_GUID_SysKeyboard, &keyboard, NULL) >= 0;

    if (mouse_ok) {
        g_mouseVtablePtr = *(void ***)mouse.p;
        std::copy_n(g_mouseVtablePtr, g_mouseVtable.size(), g_mouseVtable.begin());
    }
    if (keyboard_ok) {
        g_keyboardVtablePtr = *(void ***)keyboard.p;
        std::copy_n(g_keyboardVtablePtr, g_keyboardVtable.size(), g_keyboardVtable.begin());
    }

    if (!mouse_ok && !keyboard_ok) {
        setlog("DirectInput device creation failed");
        return false;
    }
    return true;
}

bool hook_dinput() {
    if (!load_dinput_vtables())
        return false;

    bool hooked = false;
    if (g_mouseVtable[kGetDeviceStateIndex]) {
        hooked |= create_hook(g_mouseVtable[kGetDeviceStateIndex], reinterpret_cast<void *>(hkGetDeviceState),
                              &g_mouseGetDeviceStateRaw);
        if (g_mouseVtable[kGetDeviceDataIndex])
            hooked |= create_hook(g_mouseVtable[kGetDeviceDataIndex], reinterpret_cast<void *>(hkGetDeviceData),
                                  &g_mouseGetDeviceDataRaw);
        if (g_mouseVtable[kPollIndex])
            create_hook(g_mouseVtable[kPollIndex], reinterpret_cast<void *>(hkPoll));
    }
    if (g_keyboardVtable[kGetDeviceStateIndex]) {
        if (g_keyboardVtable[kGetDeviceStateIndex] == g_mouseVtable[kGetDeviceStateIndex]) {
            g_keyboardGetDeviceStateRaw = g_mouseGetDeviceStateRaw;
            hooked = true;
        } else {
            hooked |= create_hook(g_keyboardVtable[kGetDeviceStateIndex], reinterpret_cast<void *>(hkGetDeviceState),
                                  &g_keyboardGetDeviceStateRaw);
        }
        if (g_keyboardVtable[kGetDeviceDataIndex]) {
            if (g_keyboardVtable[kGetDeviceDataIndex] == g_mouseVtable[kGetDeviceDataIndex]) {
                g_keyboardGetDeviceDataRaw = g_mouseGetDeviceDataRaw;
                hooked = true;
            } else {
                hooked |= create_hook(g_keyboardVtable[kGetDeviceDataIndex], reinterpret_cast<void *>(hkGetDeviceData),
                                      &g_keyboardGetDeviceDataRaw);
            }
        }
        if (g_keyboardVtable[kPollIndex] && g_keyboardVtable[kPollIndex] != g_mouseVtable[kPollIndex])
            create_hook(g_keyboardVtable[kPollIndex], reinterpret_cast<void *>(hkPoll));
    }
    if (g_mouseVtable[kAcquireIndex])
        create_hook(g_mouseVtable[kAcquireIndex], reinterpret_cast<void *>(hkAcquire));
    if (g_keyboardVtable[kAcquireIndex] && g_keyboardVtable[kAcquireIndex] != g_mouseVtable[kAcquireIndex])
        create_hook(g_keyboardVtable[kAcquireIndex], reinterpret_cast<void *>(hkAcquire));

    return hooked;
}

void hook_win32_key_state() {
    auto getKeyState = reinterpret_cast<void *>(ResolveApi("user32.dll", "GetKeyState"));
    auto getAsyncKeyState = reinterpret_cast<void *>(ResolveApi("user32.dll", "GetAsyncKeyState"));
    auto getKeyboardState = reinterpret_cast<void *>(ResolveApi("user32.dll", "GetKeyboardState"));

    if (getKeyState)
        create_hook(getKeyState, reinterpret_cast<void *>(hkGetKeyState), &g_getKeyStateRaw);
    if (getAsyncKeyState)
        create_hook(getAsyncKeyState, reinterpret_cast<void *>(hkGetAsyncKeyState), &g_getAsyncKeyStateRaw);
    if (getKeyboardState)
        create_hook(getKeyboardState, reinterpret_cast<void *>(hkGetKeyboardState), &g_getKeyboardStateRaw);
}

void hook_win32_cursor() {
    auto setCursor = reinterpret_cast<void *>(ResolveApi("user32.dll", "SetCursor"));
    if (setCursor)
        create_hook(setCursor, reinterpret_cast<void *>(hkSetCursor), &g_setCursorRaw);
}

void hook_raw_input() {
    auto getRawInputData = reinterpret_cast<void *>(ResolveApi("user32.dll", "GetRawInputData"));
    auto getRawInputBuffer = reinterpret_cast<void *>(ResolveApi("user32.dll", "GetRawInputBuffer"));
    auto registerRawInputDevices = reinterpret_cast<void *>(ResolveApi("user32.dll", "RegisterRawInputDevices"));
    auto getRawInputDeviceInfoW = reinterpret_cast<void *>(ResolveApi("user32.dll", "GetRawInputDeviceInfoW"));
    auto getRawInputDeviceInfoA = reinterpret_cast<void *>(ResolveApi("user32.dll", "GetRawInputDeviceInfoA"));
    auto getRawInputDeviceList = reinterpret_cast<void *>(ResolveApi("user32.dll", "GetRawInputDeviceList"));
    auto getRegisteredRawInputDevices =
        reinterpret_cast<void *>(ResolveApi("user32.dll", "GetRegisteredRawInputDevices"));

    if (getRawInputData)
        create_hook(getRawInputData, reinterpret_cast<void *>(hkGetRawInputData), &g_getRawInputDataRaw);
    if (getRawInputBuffer)
        create_hook(getRawInputBuffer, reinterpret_cast<void *>(hkGetRawInputBuffer), &g_getRawInputBufferRaw);
    if (registerRawInputDevices)
        create_hook(registerRawInputDevices, reinterpret_cast<void *>(hkRegisterRawInputDevices),
                    &g_registerRawInputDevicesRaw);
    if (getRawInputDeviceInfoW)
        create_hook(getRawInputDeviceInfoW, reinterpret_cast<void *>(hkGetRawInputDeviceInfoW),
                    &g_getRawInputDeviceInfoWRaw);
    if (getRawInputDeviceInfoA)
        create_hook(getRawInputDeviceInfoA, reinterpret_cast<void *>(hkGetRawInputDeviceInfoA),
                    &g_getRawInputDeviceInfoARaw);
    if (getRawInputDeviceList)
        create_hook(getRawInputDeviceList, reinterpret_cast<void *>(hkGetRawInputDeviceList),
                    &g_getRawInputDeviceListRaw);
    if (getRegisteredRawInputDevices)
        create_hook(getRegisteredRawInputDevices, reinterpret_cast<void *>(hkGetRegisteredRawInputDevices),
                    &g_getRegisteredRawInputDevicesRaw);
}

void fill_mouse_state(DWORD size, LPVOID ptr) {
    if (size == sizeof(MouseState)) {
        memcpy(ptr, &InputHook::m_mouseState, sizeof(MouseState));
        return;
    }

    // DirectInput 鼠标轴是相对移动量，读取后需要消费，避免同一次移动被重复返回。
    const LONG dx = InputHook::m_mouseState.lAxisX;
    const LONG dy = InputHook::m_mouseState.lAxisY;
    InputHook::m_mouseState.lAxisX = 0;
    InputHook::m_mouseState.lAxisY = 0;

    if (size == sizeof(DIMOUSESTATE)) {
        DIMOUSESTATE state = {};
        state.lX = dx;
        state.lY = dy;
        state.lZ = InputHook::consumeWheelDelta();
        state.rgbButtons[0] = InputHook::m_mouseState.abButtons[0];
        state.rgbButtons[1] = InputHook::m_mouseState.abButtons[1];
        state.rgbButtons[2] = InputHook::m_mouseState.abButtons[2];
        memcpy(ptr, &state, sizeof(state));
    } else {
        DIMOUSESTATE2 state = {};
        state.lX = dx;
        state.lY = dy;
        state.lZ = InputHook::consumeWheelDelta();
        state.rgbButtons[0] = InputHook::m_mouseState.abButtons[0];
        state.rgbButtons[1] = InputHook::m_mouseState.abButtons[1];
        state.rgbButtons[2] = InputHook::m_mouseState.abButtons[2];
        state.rgbButtons[3] = InputHook::m_mouseState.abButtons[3];
        state.rgbButtons[4] = InputHook::m_mouseState.abButtons[4];
        memcpy(ptr, &state, sizeof(state));
    }
}

void fill_keyboard_state(LPVOID ptr) {
    // DirectInput 键盘状态是 256 字节数组，最高位表示按下。
    memcpy(ptr, InputHook::m_keyboardState, 256);
}

HRESULT read_dinput_events(std::deque<DIDEVICEOBJECTDATA> &events, DWORD object_size, LPDIDEVICEOBJECTDATA data,
                           LPDWORD count, DWORD flags) {
    if (!count || object_size < sizeof(DIDEVICEOBJECTDATA))
        return DIERR_INVALIDPARAM;

    std::lock_guard<std::mutex> lock(g_eventMutex);
    const DWORD available = static_cast<DWORD>(events.size());
    const DWORD requested = *count;

    if (!data) {
        set_out(count, available);
        return DI_OK;
    }

    const DWORD copied = (std::min)(requested, available);
    for (DWORD i = 0; i < copied; ++i) {
        data[i] = events[i];
    }
    set_out(count, copied);

    // PEEK 只查看事件，不消费队列。
    if ((flags & DIGDD_PEEK) == 0) {
        for (DWORD i = 0; i < copied; ++i) {
            events.pop_front();
        }
    }
    return copied < available ? DI_BUFFEROVERFLOW : DI_OK;
}

RAWINPUT make_raw_mouse(LONG dx, LONG dy, USHORT button_flags, USHORT button_data) {
    RAWINPUT raw = {};
    raw.header.dwType = RIM_TYPEMOUSE;
    raw.header.dwSize = sizeof(RAWINPUT);
    raw.header.hDevice = reinterpret_cast<HANDLE>(kFakeRawMouseDevice);
    raw.header.wParam = RIM_INPUT;
    raw.data.mouse.usFlags = MOUSE_MOVE_RELATIVE;
    raw.data.mouse.usButtonFlags = button_flags;
    raw.data.mouse.usButtonData = button_data;
    raw.data.mouse.lLastX = dx;
    raw.data.mouse.lLastY = dy;
    return raw;
}

RAWINPUT make_raw_keyboard(WPARAM vk, bool down) {
    RAWINPUT raw = {};
    raw.header.dwType = RIM_TYPEKEYBOARD;
    raw.header.dwSize = sizeof(RAWINPUT);
    raw.header.hDevice = reinterpret_cast<HANDLE>(kFakeRawKeyboardDevice);
    raw.header.wParam = RIM_INPUT;
    raw.data.keyboard.MakeCode = scan_code(vk);
    raw.data.keyboard.Flags = static_cast<USHORT>((down ? RI_KEY_MAKE : RI_KEY_BREAK) | (is_extended_vk(vk) ? RI_KEY_E0 : 0));
    raw.data.keyboard.VKey = static_cast<USHORT>(vk);
    raw.data.keyboard.Message = down ? WM_KEYDOWN : WM_KEYUP;
    return raw;
}

UINT write_raw_input(const RAWINPUT &raw, UINT command, LPVOID data, PUINT size) {
    const UINT bytes = command == RID_HEADER ? sizeof(RAWINPUTHEADER) : sizeof(RAWINPUT);
    if (!size)
        return static_cast<UINT>(-1);
    if (!data) {
        set_out(size, bytes);
        return 0;
    }
    if (*size < bytes) {
        set_out(size, bytes);
        return static_cast<UINT>(-1);
    }

    if (command == RID_HEADER)
        memcpy(data, &raw.header, sizeof(RAWINPUTHEADER));
    else
        memcpy(data, &raw, sizeof(RAWINPUT));
    set_out(size, bytes);
    return bytes;
}

RID_DEVICE_INFO make_raw_device_info(HANDLE device) {
    RID_DEVICE_INFO info = {};
    info.cbSize = sizeof(info);
    if (fake_mouse_device(device)) {
        info.dwType = RIM_TYPEMOUSE;
        info.mouse.dwId = 1;
        info.mouse.dwNumberOfButtons = 5;
        info.mouse.dwSampleRate = 125;
        info.mouse.fHasHorizontalWheel = FALSE;
    } else {
        info.dwType = RIM_TYPEKEYBOARD;
        info.keyboard.dwType = 4;
        info.keyboard.dwSubType = 0;
        info.keyboard.dwKeyboardMode = 1;
        info.keyboard.dwNumberOfFunctionKeys = 12;
        info.keyboard.dwNumberOfIndicators = 3;
        info.keyboard.dwNumberOfKeysTotal = 104;
    }
    return info;
}

template <class CharT>
UINT write_raw_device_name(const CharT *name, LPVOID data, PUINT size) {
    const UINT chars = static_cast<UINT>(std::char_traits<CharT>::length(name) + 1);
    if (!size)
        return static_cast<UINT>(-1);
    if (!data) {
        set_out(size, chars);
        return 0;
    }
    if (*size < chars) {
        set_out(size, chars);
        return static_cast<UINT>(-1);
    }
    memcpy(data, name, chars * sizeof(CharT));
    set_out(size, chars);
    return chars;
}

UINT write_raw_device_info(HANDLE device, LPVOID data, PUINT size) {
    if (!size)
        return static_cast<UINT>(-1);
    if (!data) {
        set_out(size, sizeof(RID_DEVICE_INFO));
        return 0;
    }
    if (*size < sizeof(RID_DEVICE_INFO)) {
        set_out(size, sizeof(RID_DEVICE_INFO));
        return static_cast<UINT>(-1);
    }

    auto *info = static_cast<RID_DEVICE_INFO *>(data);
    if (info->cbSize != sizeof(RID_DEVICE_INFO))
        info->cbSize = sizeof(RID_DEVICE_INFO);
    *info = make_raw_device_info(device);
    set_out(size, sizeof(RID_DEVICE_INFO));
    return sizeof(RID_DEVICE_INFO);
}

UINT write_fake_raw_device_info_w(HANDLE device, UINT command, LPVOID data, PUINT size) {
    if (command == RIDI_DEVICENAME)
        return write_raw_device_name(fake_mouse_device(device) ? kFakeMouseName : kFakeKeyboardName, data, size);
    if (command == RIDI_DEVICEINFO)
        return write_raw_device_info(device, data, size);
    return static_cast<UINT>(-1);
}

UINT write_fake_raw_device_info_a(HANDLE device, UINT command, LPVOID data, PUINT size) {
    if (command == RIDI_DEVICENAME)
        return write_raw_device_name(fake_mouse_device(device) ? kFakeMouseNameA : kFakeKeyboardNameA, data, size);
    if (command == RIDI_DEVICEINFO)
        return write_raw_device_info(device, data, size);
    return static_cast<UINT>(-1);
}

UINT write_registered_raw_devices(PRAWINPUTDEVICE devices, PUINT count, UINT size) {
    if (!count || size != sizeof(RAWINPUTDEVICE))
        return static_cast<UINT>(-1);

    RAWINPUTDEVICE registered[2] = {};
    UINT available = 0;
    if (g_rawMouseRegistered)
        registered[available++] = g_registeredMouse;
    if (g_rawKeyboardRegistered)
        registered[available++] = g_registeredKeyboard;

    if (!devices) {
        set_out(count, available);
        return 0;
    }
    if (*count < available) {
        set_out(count, available);
        return static_cast<UINT>(-1);
    }

    for (UINT i = 0; i < available; ++i)
        devices[i] = registered[i];
    set_out(count, available);
    return available;
}

UINT write_raw_device_list(PRAWINPUTDEVICELIST devices, PUINT count, UINT size) {
    if (!count || size != sizeof(RAWINPUTDEVICELIST))
        return static_cast<UINT>(-1);

    constexpr UINT available = 2;
    if (!devices) {
        set_out(count, available);
        return 0;
    }
    if (*count < available) {
        set_out(count, available);
        return static_cast<UINT>(-1);
    }

    devices[0].hDevice = reinterpret_cast<HANDLE>(kFakeRawMouseDevice);
    devices[0].dwType = RIM_TYPEMOUSE;
    devices[1].hDevice = reinterpret_cast<HANDLE>(kFakeRawKeyboardDevice);
    devices[1].dwType = RIM_TYPEKEYBOARD;
    set_out(count, available);
    return available;
}

UINT append_fake_raw_device_list(PRAWINPUTDEVICELIST devices, PUINT count, UINT size) {
    if (!count || size != sizeof(RAWINPUTDEVICELIST))
        return static_cast<UINT>(-1);

    if (!g_getRawInputDeviceListRaw)
        return write_raw_device_list(devices, count, size);

    UINT real_count = 0;
    const UINT query_ret =
        reinterpret_cast<GetRawInputDeviceListFn>(g_getRawInputDeviceListRaw)(nullptr, &real_count, size);
    if (query_ret == static_cast<UINT>(-1))
        return write_raw_device_list(devices, count, size);

    const UINT total = real_count + 2;
    if (!devices) {
        set_out(count, total);
        return 0;
    }
    if (*count < total) {
        set_out(count, total);
        return static_cast<UINT>(-1);
    }

    UINT written_real = real_count;
    if (real_count > 0) {
        const UINT real_ret =
            reinterpret_cast<GetRawInputDeviceListFn>(g_getRawInputDeviceListRaw)(devices, &written_real, size);
        if (real_ret == static_cast<UINT>(-1))
            written_real = 0;
    }

    devices[written_real].hDevice = reinterpret_cast<HANDLE>(kFakeRawMouseDevice);
    devices[written_real].dwType = RIM_TYPEMOUSE;
    devices[written_real + 1].hDevice = reinterpret_cast<HANDLE>(kFakeRawKeyboardDevice);
    devices[written_real + 1].dwType = RIM_TYPEKEYBOARD;
    const UINT written_total = written_real + 2;
    set_out(count, written_total);
    return written_total;
}

bool is_extended_vk(WPARAM vk) {
    switch (vk) {
    case VK_RMENU:
    case VK_RCONTROL:
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:
    case VK_NUMLOCK:
    case VK_SNAPSHOT:
    case VK_DIVIDE:
    case VK_APPS:
    case VK_LWIN:
    case VK_RWIN:
        return true;
    default:
        return false;
    }
}

BYTE dik_code(WPARAM vk) {
    switch (vk) {
    case VK_NUMLOCK:
        return 0x45;
    case VK_PAUSE:
        return 0xC5;
    case VK_SNAPSHOT:
        return 0xB7;
    default:
        break;
    }

    BYTE scan = static_cast<BYTE>(::MapVirtualKey(static_cast<UINT>(vk), MAPVK_VK_TO_VSC) & 0xff);
    if (scan == 0)
        return 0;
    return is_extended_vk(vk) ? static_cast<BYTE>(scan | 0x80) : scan;
}

WORD scan_code(WPARAM vk) {
    return static_cast<WORD>((::MapVirtualKey(static_cast<UINT>(vk), MAPVK_VK_TO_VSC)) & 0xff);
}

LPARAM make_key_lparam(WPARAM vk, bool up) {
    DWORD value = 1;
    value |= static_cast<DWORD>(scan_code(vk)) << 16;
    if (is_extended_vk(vk))
        value |= 1u << 24;
    if (up)
        value |= 3u << 30;
    return static_cast<LPARAM>(value);
}

LPARAM client_to_screen_lparam(HWND hwnd, LPARAM lparam) {
    POINT pt{static_cast<SHORT>(LOWORD(lparam)), static_cast<SHORT>(HIWORD(lparam))};
    ::ClientToScreen(hwnd, &pt);
    return MAKELPARAM(pt.x, pt.y);
}

void dispatch_window_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    // 直接交给原窗口过程，避免异步队列里的真实鼠标消息覆盖脚本坐标。
    if (g_rawWindowProc)
        ::CallWindowProc(g_rawWindowProc, hwnd, message, wparam, lparam);
}

} // namespace

int InputHook::setup(HWND hwnd) {
    if (!::IsWindow(hwnd))
        return 0;

    RuntimeEnvironment::m_showErrorMsg = 0;
    input_hwnd = hwnd;
    memset(&m_mouseState, 0, sizeof(m_mouseState));
    memset(m_vkState, 0, sizeof(m_vkState));
    memset(m_keyboardState, 0, sizeof(m_keyboardState));
    m_lastMouseX = 0;
    m_lastMouseY = 0;
    m_wheelDelta = 0;

    if (!AcquireMinHook())
        return 0;

    if (!hook_dinput()) {
        setlog("input hook dinput setup failed");
    }
    hook_win32_key_state();
    hook_win32_cursor();
    hook_raw_input();
    enable_input_hooks();

    g_rawWindowProc =
        reinterpret_cast<WNDPROC>(::SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(opWndProc)));
    if (!g_rawWindowProc) {
        release();
        return 0;
    }
    return 1;
}

int InputHook::release() {
    LONG_PTR restored = 1;
    if (g_rawWindowProc && input_hwnd) {
        restored = ::SetWindowLongPtr(input_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_rawWindowProc));
    }

    remove_input_hooks();
    ReleaseMinHook();

    g_rawWindowProc = nullptr;
    input_hwnd = nullptr;
    g_mouseGetDeviceStateRaw = nullptr;
    g_keyboardGetDeviceStateRaw = nullptr;
    g_mouseGetDeviceDataRaw = nullptr;
    g_keyboardGetDeviceDataRaw = nullptr;
    g_getKeyStateRaw = nullptr;
    g_getAsyncKeyStateRaw = nullptr;
    g_getKeyboardStateRaw = nullptr;
    g_getRawInputDataRaw = nullptr;
    g_getRawInputBufferRaw = nullptr;
    g_registerRawInputDevicesRaw = nullptr;
    g_getRawInputDeviceInfoWRaw = nullptr;
    g_getRawInputDeviceInfoARaw = nullptr;
    g_getRawInputDeviceListRaw = nullptr;
    g_getRegisteredRawInputDevicesRaw = nullptr;
    g_setCursorRaw = nullptr;
    g_mouseVtable = {};
    g_keyboardVtable = {};
    g_mouseVtablePtr = nullptr;
    g_keyboardVtablePtr = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_eventMutex);
        g_mouseEvents.clear();
        g_keyboardEvents.clear();
        g_rawEvents.clear();
        g_registeredMouse = {};
        g_registeredKeyboard = {};
        g_rawMouseRegistered = false;
        g_rawKeyboardRegistered = false;
    }
    g_eventSequence = 0;
    g_rawSequence = 0;
    memset(&m_mouseState, 0, sizeof(m_mouseState));
    memset(m_vkState, 0, sizeof(m_vkState));
    memset(m_keyboardState, 0, sizeof(m_keyboardState));
    m_lastMouseX = 0;
    m_lastMouseY = 0;
    m_wheelDelta = 0;
    m_cursor = nullptr;
    m_cursorVisible = false;
    m_cursorHash = 0;
    m_cursorMeta = 0;
    return restored ? 1 : 0;
}

void InputHook::moveTo(LPARAM lp) {
    const SHORT x = static_cast<SHORT>(LOWORD(lp));
    const SHORT y = static_cast<SHORT>(HIWORD(lp));
    const LONG dx = x - m_lastMouseX;
    const LONG dy = y - m_lastMouseY;
    m_mouseState.lAxisX += dx;
    m_mouseState.lAxisY += dy;
    m_lastMouseX = x;
    m_lastMouseY = y;

    if (dx == 0 && dy == 0)
        return;

    std::lock_guard<std::mutex> lock(g_eventMutex);
    // Buffered DirectInput 读取的是事件队列，Raw Input 读取的是 WM_INPUT 后的原始包。
    if (dx != 0)
        push_dinput_event(g_mouseEvents, DIMOFS_X, static_cast<DWORD>(dx));
    if (dy != 0)
        push_dinput_event(g_mouseEvents, DIMOFS_Y, static_cast<DWORD>(dy));
    push_raw_event(make_raw_mouse(dx, dy, 0, 0));
}

void InputHook::button(LPARAM lp, int key, bool down) {
    moveTo(lp);
    if (0 <= key && key < 5) {
        m_mouseState.abButtons[key] = down ? 0x80 : 0;
        static constexpr int vk_buttons[] = {VK_LBUTTON, VK_MBUTTON, VK_RBUTTON, VK_XBUTTON1, VK_XBUTTON2};
        m_vkState[vk_buttons[key]] = down ? 0x80 : 0;
        std::lock_guard<std::mutex> lock(g_eventMutex);
        push_dinput_event(g_mouseEvents, mouse_button_offset(key), down ? 0x80 : 0);

        static constexpr USHORT down_flags[] = {RI_MOUSE_LEFT_BUTTON_DOWN, RI_MOUSE_MIDDLE_BUTTON_DOWN,
                                                RI_MOUSE_RIGHT_BUTTON_DOWN, RI_MOUSE_BUTTON_4_DOWN,
                                                RI_MOUSE_BUTTON_5_DOWN};
        static constexpr USHORT up_flags[] = {RI_MOUSE_LEFT_BUTTON_UP, RI_MOUSE_MIDDLE_BUTTON_UP,
                                              RI_MOUSE_RIGHT_BUTTON_UP, RI_MOUSE_BUTTON_4_UP, RI_MOUSE_BUTTON_5_UP};
        push_raw_event(make_raw_mouse(0, 0, down ? down_flags[key] : up_flags[key], 0));
    }
}

void InputHook::updateWheel(WPARAM wp, LPARAM lp, bool horizontal) {
    moveTo(lp);
    const SHORT delta = static_cast<SHORT>(HIWORD(wp));
    m_wheelDelta += delta;
    std::lock_guard<std::mutex> lock(g_eventMutex);
    push_dinput_event(g_mouseEvents, DIMOFS_Z, static_cast<DWORD>(static_cast<LONG>(delta)));
    push_raw_event(make_raw_mouse(0, 0, horizontal ? RI_MOUSE_HWHEEL : RI_MOUSE_WHEEL, static_cast<USHORT>(delta)));
}

LONG InputHook::consumeWheelDelta() {
    const LONG delta = m_wheelDelta;
    m_wheelDelta = 0;
    return delta;
}

void InputHook::updateKey(WPARAM vk, bool down) {
    if (vk >= 256)
        return;

    const BYTE value = down ? 0x80 : 0;
    m_vkState[vk] = value;

    // DirectInput 键盘数组使用 DIK/扫描码下标，不能直接拿 VK 当下标。
    const BYTE dik = dik_code(vk);
    if (dik != 0) {
        m_keyboardState[dik] = value;
        std::lock_guard<std::mutex> lock(g_eventMutex);
        push_dinput_event(g_keyboardEvents, dik, key_data(down));
        push_raw_event(make_raw_keyboard(vk, down));
    }
}

bool InputHook::isKeyDown(int vk) {
    return 0 <= vk && vk < 256 && (m_vkState[vk] & 0x80);
}

void InputHook::updateCursor(HCURSOR cursor, bool visible) {
    m_cursor = cursor;
    m_cursorVisible = visible;

    CursorShapeInfo info;
    if (cursor && cursor_shape::FromCursor(cursor, visible, info)) {
        m_cursorHash = info.hash;
        m_cursorMeta = cursor_shape::PackMeta(info, true);
        return;
    }

    // SetCursor(NULL) 表示目标隐藏系统光标，这是一个有效状态。
    info.visible = false;
    m_cursorHash = 0;
    m_cursorMeta = cursor_shape::PackMeta(info, cursor == nullptr);
}

unsigned long long InputHook::cursorShapeHash() {
    return m_cursorHash;
}

unsigned long long InputHook::cursorShapeMeta() {
    return m_cursorMeta;
}

namespace {

HRESULT __stdcall hkAcquire(IDirectInputDevice8W *) {
    return DI_OK;
}

HRESULT __stdcall hkPoll(IDirectInputDevice8W *) {
    return DI_OK;
}

HRESULT __stdcall hkGetDeviceState(IDirectInputDevice8W *device, DWORD size, LPVOID ptr) {
    bool is_mouse = false;
    bool is_keyboard = false;
    const bool known_device = dinput_device_kind(device, is_mouse, is_keyboard);

    if (ptr && (is_keyboard || (!known_device && size == 256)) && size == 256) {
        fill_keyboard_state(ptr);
        return DI_OK;
    }
    if (ptr && (is_mouse || (!known_device && same_vtable(device, g_mouseVtablePtr))) &&
        (size == sizeof(MouseState) || size == sizeof(DIMOUSESTATE) || size == sizeof(DIMOUSESTATE2))) {
        fill_mouse_state(size, ptr);
        return DI_OK;
    }

    void *raw = g_mouseGetDeviceStateRaw;
    if (same_vtable(device, g_keyboardVtablePtr) && g_keyboardGetDeviceStateRaw)
        raw = g_keyboardGetDeviceStateRaw;
    if (raw)
        return reinterpret_cast<GetDeviceStateFn>(raw)(device, size, ptr);
    return DIERR_NOTINITIALIZED;
}

HRESULT __stdcall hkGetDeviceData(IDirectInputDevice8W *device, DWORD object_size, LPDIDEVICEOBJECTDATA data,
                                  LPDWORD count, DWORD flags) {
    bool is_mouse = false;
    bool is_keyboard = false;
    const bool known_device = dinput_device_kind(device, is_mouse, is_keyboard);
    if (is_keyboard)
        return read_dinput_events(g_keyboardEvents, object_size, data, count, flags);
    if (is_mouse)
        return read_dinput_events(g_mouseEvents, object_size, data, count, flags);
    if (!known_device && same_vtable(device, g_keyboardVtablePtr))
        return read_dinput_events(g_keyboardEvents, object_size, data, count, flags);
    if (!known_device && same_vtable(device, g_mouseVtablePtr))
        return read_dinput_events(g_mouseEvents, object_size, data, count, flags);

    void *raw = g_mouseGetDeviceDataRaw;
    if (same_vtable(device, g_keyboardVtablePtr) && g_keyboardGetDeviceDataRaw)
        raw = g_keyboardGetDeviceDataRaw;
    if (raw)
        return reinterpret_cast<GetDeviceDataFn>(raw)(device, object_size, data, count, flags);
    return DIERR_NOTINITIALIZED;
}

SHORT WINAPI hkGetKeyState(int vk) {
    SHORT original = 0;
    if (g_getKeyStateRaw)
        original = reinterpret_cast<GetKeyStateFn>(g_getKeyStateRaw)(vk);
    if (InputHook::isKeyDown(vk))
        return static_cast<SHORT>(original | 0x8000);
    return original;
}

SHORT WINAPI hkGetAsyncKeyState(int vk) {
    SHORT original = 0;
    if (g_getAsyncKeyStateRaw)
        original = reinterpret_cast<GetKeyStateFn>(g_getAsyncKeyStateRaw)(vk);
    if (InputHook::isKeyDown(vk))
        return static_cast<SHORT>(original | 0x8000);
    return original;
}

BOOL WINAPI hkGetKeyboardState(PBYTE key_state) {
    if (!key_state)
        return FALSE;

    BOOL ret = TRUE;
    if (g_getKeyboardStateRaw)
        ret = reinterpret_cast<GetKeyboardStateFn>(g_getKeyboardStateRaw)(key_state);
    else
        memset(key_state, 0, 256);

    for (int i = 0; i < 256; ++i) {
        if (InputHook::m_vkState[i] & 0x80)
            key_state[i] |= 0x80;
    }
    return ret;
}

HCURSOR WINAPI hkSetCursor(HCURSOR cursor) {
    HCURSOR ret = nullptr;
    if (g_setCursorRaw)
        ret = reinterpret_cast<SetCursorFn>(g_setCursorRaw)(cursor);

    InputHook::updateCursor(cursor, cursor != nullptr);
    return ret;
}

UINT WINAPI hkGetRawInputData(HRAWINPUT raw_input, UINT command, LPVOID data, PUINT size, UINT header_size) {
    if (fake_raw_handle(raw_input)) {
        if (header_size != sizeof(RAWINPUTHEADER))
            return static_cast<UINT>(-1);

        std::lock_guard<std::mutex> lock(g_eventMutex);
        RAWINPUT *raw = find_raw_event(raw_input);
        return raw ? write_raw_input(*raw, command, data, size) : static_cast<UINT>(-1);
    }

    if (g_getRawInputDataRaw)
        return reinterpret_cast<GetRawInputDataFn>(g_getRawInputDataRaw)(raw_input, command, data, size, header_size);
    return static_cast<UINT>(-1);
}

UINT WINAPI hkGetRawInputBuffer(PRAWINPUT data, PUINT size, UINT header_size) {
    if (!size || header_size != sizeof(RAWINPUTHEADER))
        return static_cast<UINT>(-1);

    std::lock_guard<std::mutex> lock(g_eventMutex);
    const UINT available = static_cast<UINT>(g_rawEvents.size());
    if (!data) {
        if (available > 0) {
            set_out(size, sizeof(RAWINPUT));
            return 0;
        }
        if (g_getRawInputBufferRaw)
            return reinterpret_cast<GetRawInputBufferFn>(g_getRawInputBufferRaw)(data, size, header_size);
        set_out(size, 0);
        return 0;
    }

    const UINT capacity = *size / sizeof(RAWINPUT);
    const UINT copied = (std::min)(capacity, available);
    for (UINT i = 0; i < copied; ++i) {
        data[i] = g_rawEvents[i].second;
    }
    for (UINT i = 0; i < copied; ++i) {
        g_rawEvents.pop_front();
    }
    set_out(size, copied * sizeof(RAWINPUT));

    if (copied > 0 || !g_getRawInputBufferRaw)
        return copied;
    return reinterpret_cast<GetRawInputBufferFn>(g_getRawInputBufferRaw)(data, size, header_size);
}

BOOL WINAPI hkRegisterRawInputDevices(PCRAWINPUTDEVICE devices, UINT count, UINT size) {
    if (!devices || size != sizeof(RAWINPUTDEVICE))
        return FALSE;

    bool handled = false;
    {
        std::lock_guard<std::mutex> lock(g_eventMutex);
        for (UINT i = 0; i < count; ++i) {
            const RAWINPUTDEVICE &device = devices[i];
            if (raw_usage_is_mouse(device)) {
                handled = true;
                g_rawMouseRegistered = (device.dwFlags & RIDEV_REMOVE) == 0;
                g_registeredMouse = make_registered_device(kHidUsageMouse, device.hwndTarget);
                g_registeredMouse.dwFlags = device.dwFlags;
            } else if (raw_usage_is_keyboard(device)) {
                handled = true;
                g_rawKeyboardRegistered = (device.dwFlags & RIDEV_REMOVE) == 0;
                g_registeredKeyboard = make_registered_device(kHidUsageKeyboard, device.hwndTarget);
                g_registeredKeyboard.dwFlags = device.dwFlags;
            }
        }
    }

    BOOL ret = FALSE;
    if (g_registerRawInputDevicesRaw)
        ret = reinterpret_cast<RegisterRawInputDevicesFn>(g_registerRawInputDevicesRaw)(devices, count, size);
    return ret || handled;
}

UINT WINAPI hkGetRawInputDeviceInfoW(HANDLE device, UINT command, LPVOID data, PUINT size) {
    if (fake_raw_device(device))
        return write_fake_raw_device_info_w(device, command, data, size);
    if (g_getRawInputDeviceInfoWRaw)
        return reinterpret_cast<GetRawInputDeviceInfoWFn>(g_getRawInputDeviceInfoWRaw)(device, command, data, size);
    return static_cast<UINT>(-1);
}

UINT WINAPI hkGetRawInputDeviceInfoA(HANDLE device, UINT command, LPVOID data, PUINT size) {
    if (fake_raw_device(device))
        return write_fake_raw_device_info_a(device, command, data, size);
    if (g_getRawInputDeviceInfoARaw)
        return reinterpret_cast<GetRawInputDeviceInfoAFn>(g_getRawInputDeviceInfoARaw)(device, command, data, size);
    return static_cast<UINT>(-1);
}

UINT WINAPI hkGetRawInputDeviceList(PRAWINPUTDEVICELIST devices, PUINT count, UINT size) {
    if (count && size == sizeof(RAWINPUTDEVICELIST))
        return append_fake_raw_device_list(devices, count, size);

    if (g_getRawInputDeviceListRaw)
        return reinterpret_cast<GetRawInputDeviceListFn>(g_getRawInputDeviceListRaw)(devices, count, size);
    return static_cast<UINT>(-1);
}

UINT WINAPI hkGetRegisteredRawInputDevices(PRAWINPUTDEVICE devices, PUINT count, UINT size) {
    {
        std::lock_guard<std::mutex> lock(g_eventMutex);
        if (write_registered_raw_devices(devices, count, size) != static_cast<UINT>(-1))
            return devices ? *count : 0;
    }
    if (g_getRegisteredRawInputDevicesRaw)
        return reinterpret_cast<GetRegisteredRawInputDevicesFn>(g_getRegisteredRawInputDevicesRaw)(devices, count, size);
    return static_cast<UINT>(-1);
}

LRESULT CALLBACK opWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case OP_WM_MOUSEMOVE:
        InputHook::moveTo(lparam);
        dispatch_window_message(hwnd, WM_MOUSEMOVE, wparam, lparam);
        return 1;
    case OP_WM_LBUTTONDOWN:
        InputHook::button(lparam, 0, true);
        dispatch_window_message(hwnd, WM_LBUTTONDOWN, wparam ? wparam : MK_LBUTTON, lparam);
        return 1;
    case OP_WM_LBUTTONDBLCLK:
        InputHook::button(lparam, 0, true);
        dispatch_window_message(hwnd, WM_LBUTTONDBLCLK, wparam ? wparam : MK_LBUTTON, lparam);
        return 1;
    case OP_WM_LBUTTONUP:
        InputHook::button(lparam, 0, false);
        dispatch_window_message(hwnd, WM_LBUTTONUP, wparam, lparam);
        return 1;
    case OP_WM_MBUTTONDOWN:
        InputHook::button(lparam, 1, true);
        dispatch_window_message(hwnd, WM_MBUTTONDOWN, wparam ? wparam : MK_MBUTTON, lparam);
        return 1;
    case OP_WM_MBUTTONDBLCLK:
        InputHook::button(lparam, 1, true);
        dispatch_window_message(hwnd, WM_MBUTTONDBLCLK, wparam ? wparam : MK_MBUTTON, lparam);
        return 1;
    case OP_WM_MBUTTONUP:
        InputHook::button(lparam, 1, false);
        dispatch_window_message(hwnd, WM_MBUTTONUP, wparam, lparam);
        return 1;
    case OP_WM_RBUTTONDOWN:
        InputHook::button(lparam, 2, true);
        dispatch_window_message(hwnd, WM_RBUTTONDOWN, wparam ? wparam : MK_RBUTTON, lparam);
        return 1;
    case OP_WM_RBUTTONDBLCLK:
        InputHook::button(lparam, 2, true);
        dispatch_window_message(hwnd, WM_RBUTTONDBLCLK, wparam ? wparam : MK_RBUTTON, lparam);
        return 1;
    case OP_WM_RBUTTONUP:
        InputHook::button(lparam, 2, false);
        dispatch_window_message(hwnd, WM_RBUTTONUP, wparam, lparam);
        return 1;
    case OP_WM_XBUTTONDOWN: {
        const WORD xbutton = HIWORD(wparam);
        const int key = xbutton == XBUTTON1 ? 3 : 4;
        InputHook::button(lparam, key, true);
        dispatch_window_message(hwnd, WM_XBUTTONDOWN, wparam, lparam);
        return 1;
    }
    case OP_WM_XBUTTONDBLCLK: {
        const WORD xbutton = HIWORD(wparam);
        const int key = xbutton == XBUTTON1 ? 3 : 4;
        InputHook::button(lparam, key, true);
        dispatch_window_message(hwnd, WM_XBUTTONDBLCLK, wparam, lparam);
        return 1;
    }
    case OP_WM_XBUTTONUP: {
        const WORD xbutton = HIWORD(wparam);
        const int key = xbutton == XBUTTON1 ? 3 : 4;
        InputHook::button(lparam, key, false);
        dispatch_window_message(hwnd, WM_XBUTTONUP, wparam, lparam);
        return 1;
    }
    case OP_WM_MOUSEWHEEL:
        InputHook::updateWheel(wparam, lparam, false);
        dispatch_window_message(hwnd, WM_MOUSEWHEEL, wparam, client_to_screen_lparam(hwnd, lparam));
        return 1;
    case OP_WM_MOUSEHWHEEL:
        InputHook::updateWheel(wparam, lparam, true);
        dispatch_window_message(hwnd, WM_MOUSEHWHEEL, wparam, client_to_screen_lparam(hwnd, lparam));
        return 1;
    case OP_WM_KEYDOWN:
        InputHook::updateKey(wparam, true);
        dispatch_window_message(hwnd, WM_KEYDOWN, wparam, lparam ? lparam : make_key_lparam(wparam, false));
        return 1;
    case OP_WM_KEYUP:
        InputHook::updateKey(wparam, false);
        dispatch_window_message(hwnd, WM_KEYUP, wparam, lparam ? lparam : make_key_lparam(wparam, true));
        return 1;
    case OP_WM_CHAR:
        dispatch_window_message(hwnd, WM_CHAR, wparam, lparam ? lparam : 1);
        return 1;
    }

    return ::CallWindowProc(g_rawWindowProc, hwnd, message, wparam, lparam);
}

} // namespace

} // namespace op::hook
