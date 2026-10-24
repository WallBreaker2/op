#pragma once
#ifndef OP_BASE_AUTOMATION_MODES_H_
#define OP_BASE_AUTOMATION_MODES_H_
#include "Types.h"
#include <cstdint>
#define SAFE_CLOSE(h)                                                                                                  \
    if (h)                                                                                                             \
        CloseHandle(h);                                                                                                \
    h = NULL;
template <class Type> void SAFE_DELETE(Type *&ptr) {
    delete ptr;
    ptr = nullptr;
}

#define SAFE_DELETE_ARRAY(ptr)                                                                                         \
    if (ptr)                                                                                                           \
        delete[] ptr;                                                                                                  \
    ptr = nullptr

#define SAFE_RELEASE(obj)                                                                                              \
    if (obj)                                                                                                           \
        obj->Release();                                                                                                \
    obj = nullptr

// #define _sto_wstring(s) boost::locale::conv::to_utf<wchar_t>(s, "GBK")
// #define _wsto_string(s)  boost::locale::conv::from_utf(s,"GBK")

#define DLL_API extern "C" _declspec(dllexport)
// normal windows,gdi;,dx;opengl;
enum RENDER_TYPE {
    NORMAL = 0,
    GDI = 1,
    DX = 2,
    OPENGL = 3
};

#define MAKE_RENDER(type, flag) ((type << 16) | flag)

#define GET_RENDER_TYPE(t) (t >> 16)

#define GET_RENDER_FLAG(t) (t & 0xffff)

constexpr int RDT_NORMAL = MAKE_RENDER(NORMAL, 0);
constexpr int RDT_NORMAL_DXGI = MAKE_RENDER(NORMAL, 1);
constexpr int RDT_NORMAL_WGC = MAKE_RENDER(NORMAL, 2);
constexpr int RDT_GDI = MAKE_RENDER(GDI, 0);
constexpr int RDT_GDI2 = MAKE_RENDER(GDI, 1);
constexpr int RDT_GDI_DX2 = MAKE_RENDER(GDI, 2);
constexpr int RDT_DX_DEFAULT = MAKE_RENDER(DX, 0);
constexpr int RDT_DX_D3D9 = MAKE_RENDER(DX, 1);
constexpr int RDT_DX_D3D10 = MAKE_RENDER(DX, 2);
constexpr int RDT_DX_D3D11 = MAKE_RENDER(DX, 3);
constexpr int RDT_DX_D3D12 = MAKE_RENDER(DX, 4);
constexpr int RDT_GL_DEFAULT = MAKE_RENDER(OPENGL, 0);
constexpr int RDT_GL_STD = MAKE_RENDER(OPENGL, 1);
constexpr int RDT_GL_NOX = MAKE_RENDER(OPENGL, 2);
constexpr int RDT_GL_ES = MAKE_RENDER(OPENGL, 3);
constexpr int RDT_GL_FI = MAKE_RENDER(OPENGL, 4); // glFinish

enum INPUT_TYPE {
    IN_NORMAL = 0,
    IN_NORMAL2 = 1,
    IN_WINDOWS = 2,
    IN_DX = 3,
};
// define Image byte format
constexpr int IBF_R8G8B8A8 = 0;
constexpr int IBF_B8G8R8A8 = 1;
constexpr int IBF_R8G8B8 = 2;

// const size_t MAX_IMAGE_WIDTH = 1<<11;
// const size_t SHARED_MEMORY_SIZE = 1080 * 1928 * 4;

// 这两个名字是主控 DLL 与注入的 hook DLL 之间的"线格式"：
//   - 命名共享段（SharedMemory）叫 "op_shared_mem_<hwnd>"
//   - 命名互斥体（ProcessMutex）叫 "op_mutex_<hwnd>"
// 历史上这两个前缀是**反的**（段叫 op_mutex_*、互斥体叫 op_shared_mem_*），第 72 批按操作者
// 要求改回正名（见 defect_fixes.md §72.20）。改名的代价是**旧版注入 DLL 会失联**：它按老名字
// 找对象，两边永远等不到对方的帧（表现是后台截图超时，而不是报错）。所以主控端与注入端
// 必须同一次发版；`waitForBindReady()` 超时时会打一行日志提示这种混装可能。
// 这里原来还有 SHARED_RES_NAME_FORMAT / MUTEX_NAME_FORMAT 两个 "op_xxx_%d" 的宏：从未被使用，
// 而且 %d 在 x64 上会把 HWND 截断成 32 位，已删除，避免有人拿它拼出"看起来对"的错误名字。
inline std::wstring MakeOpSharedResourceName(HWND hwnd) {
    return std::wstring(L"op_shared_mem_") + std::to_wstring(reinterpret_cast<std::uintptr_t>(hwnd));
}

inline std::wstring MakeOpMutexName(HWND hwnd) {
    return std::wstring(L"op_mutex_") + std::to_wstring(reinterpret_cast<std::uintptr_t>(hwnd));
}

extern long KEYPAD_NORMAL_DELAY;
extern long KEYPAD_NORMAL2_DELAY;
extern long KEYPAD_WINDOWS_DELAY;
extern long KEYPAD_DX_DELAY;

extern long MOUSE_NORMAL_DELAY;
extern long MOUSE_WINDOWS_DELAY;
extern long MOUSE_DX_DELAY;

#ifndef _M_X64
#define OP64 0
#else
#define OP64 1
#endif

#define _TOSTRING(x) #x

#define MAKE_OP_VERSION(a, b, c, d) _TOSTRING(a##.##b##.##c##.##d)

#ifndef OP_VERSION
#define OP_VERSION MAKE_OP_VERSION(0, 4, 8, 3)
#endif // OP_VERSION
// 模块句柄
// extern HINSTANCE gInstance;
// 是否显示错误信息
// extern int gShowError;
// op 路径
// extern wstring m_opPath;

// extern wstring g_op_name;

#endif // OP_BASE_AUTOMATION_MODES_H_
