#ifndef OP_HOOK_INPUT_HOOK_H_
#define OP_HOOK_INPUT_HOOK_H_

#include "../runtime/AutomationModes.h"

namespace op::hook {

struct MouseState {
    LONG lAxisX;
    LONG lAxisY;
    // 内部累计状态，不对应 DirectInput 的 DIMOUSESTATE 内存布局。
    BYTE abButtons[5];
    BYTE bPadding[3]; // 保持结构体 4 字节对齐。
};

class InputHook {
  public:
    // 远端目标窗口，只在被注入进程内使用。
    static HWND input_hwnd;
    static bool is_hooked;

    static int setup(HWND hwnd_);
    static int release();
    static int lockInput(int lock);
    static bool mouseLocked();
    static bool keyboardLocked();
    static void moveTo(LPARAM lp);
    static void button(LPARAM lp, int key, bool down);
    static void updateWheel(WPARAM, LPARAM, bool horizontal);
    static LONG consumeWheelDelta();
    static void updateKey(WPARAM vk, bool down);
    static bool isKeyDown(int vk);
    static void updateCursor(HCURSOR cursor, bool visible);
    static unsigned long long cursorShapeHash();
    static unsigned long long cursorShapeMeta();
    static MouseState m_mouseState;
    static BYTE m_vkState[256];
    static BYTE m_keyboardState[256];
    static LONG m_lastMouseX;
    static LONG m_lastMouseY;
    static LONG m_wheelDelta;
    static HCURSOR m_cursor;
    static bool m_cursorVisible;
    static unsigned long long m_cursorHash;
    static unsigned long long m_cursorMeta;
    static LONG m_inputLock;
};

} // namespace op::hook

#endif // OP_HOOK_INPUT_HOOK_H_
