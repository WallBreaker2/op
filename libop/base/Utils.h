#pragma once
#ifndef OP_BASE_UTILS_H_
#define OP_BASE_UTILS_H_
#include "Types.h"
#include "Environment.h"
#include <windows.h>
std::wstring _s2wstring(const std::string &s);
std::string _ws2string(const std::wstring &s);

std::string utf8_to_ansi(std::string strUTF8);
// UTF-8 -> UTF-16。服务端返回的文本一律走这条，别再绕 GBK/ACP（会丢非 GBK 字符）。
inline std::wstring utf8_to_wstring(const std::string &utf8) {
    if (utf8.empty())
        return L"";

    const int length = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (length <= 0)
        return L"";

    std::wstring out(static_cast<size_t>(length), static_cast<wchar_t>(0));
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), out.data(), length);
    return out;
}
// 将路径转化为全局路径
long Path2GlobalPath(const std::wstring &file, const std::wstring &curr_path, std::wstring &out);

void split(const std::wstring &s, std::vector<std::wstring> &v, const std::wstring &c);
void split(const std::string &s, std::vector<std::string> &v, const std::string &c);

void wstring2lower(std::wstring &s);

void replacew(std::wstring &str, const std::wstring &oldval, const std::wstring &newval);

// for debug
long setlog(const wchar_t *format, ...);
//
long setlog(const char *format, ...);

// 诊断日志必须落盘。宿主/测试常用 SetShowErrorMsg(3)（只走 stdout），而 ctest 的控制台
// 日志会被下一次成功跑覆盖 —— 偶发问题就永远留不下证据。这里临时切到 2 写 __op.log。
// 只在失败分支用，正常路径别调（每次都要开关一次全局状态，没必要）。
template <typename... Args> void setlog_persistent(const wchar_t *format, Args... args) {
    const int saved = RuntimeEnvironment::m_showErrorMsg;
    RuntimeEnvironment::m_showErrorMsg = 2;
    setlog(format, args...);
    RuntimeEnvironment::m_showErrorMsg = saved;
}

// Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetLastErrorAsString();

int inline hex2bin(int c) {
    return c <= L'9' ? c - L'0' : c - L'A' + 10;
};

int inline bin2hex(int c) {
    int ans = 0;
    int c1 = c >> 4 & 0xf;
    int c2 = c & 0xf;
    ans |= (c1 <= 9 ? c1 + L'0' : c1 + 'A' - 10) << 8;
    ans |= c2 <= 9 ? c2 + L'0' : c2 + 'A' - 10;
    return ans;
};

constexpr int PTY(op::uint pt) {
    return pt >> 16;
}

constexpr int PTX(op::uint pt) {
    return pt & 0xffff;
}

namespace op {
std::ostream &operator<<(std::ostream &o, point_t const &rhs);
std::wostream &operator<<(std::wostream &o, point_t const &rhs);

// 可空输出参数：target 为空时跳过写入，返回是否写成功。
template <typename Target, typename Value> inline bool set_out(Target *target, Value value) {
    if (!target)
        return false;
    *target = static_cast<Target>(value);
    return true;
}
} // namespace op

bool Delay(long mis);
bool Delays(long mis_min, long mis_max);

namespace op {
// §72.48：OP 自己的失败码。
// 以前 Op::GetLastError 直接返回 ::GetLastError()，而全库只有 InputHook 里给被 hook
// 的 API 传假错误时调过一次 SetLastError —— 于是 OP 自身失败（非法模式、窗口不存在、
// 共享段建不起来）之后，调用方读到的是别的 Win32 API 留下的残留值，查文档必然查错方向。
// 现在 OP 的失败路径统一用下面这个入口写码；码在私有段 0x4F0000xx，不会和 Win32 撞。
constexpr int kOpErrBadArgument = 0x4F000001;        // 参数非法（hwnd 为空、宽高 <= 0、模式串不认识）
constexpr int kOpErrWindowNotFound = 0x4F000002;     // 窗口不存在 / 已销毁
constexpr int kOpErrBackendUnavailable = 0x4F000003; // 该后端在本机不可用（版本门、缺依赖）
constexpr int kOpErrBindFailed = 0x4F000004;         // 后端 BindEx 返回失败
constexpr int kOpErrSharedMemoryFailed = 0x4F000005; // 命名共享段 / 互斥体创建失败
constexpr int kOpErrNotBound = 0x4F000006;           // 还没绑定就调用需要绑定的接口
constexpr int kOpErrInjectionFailed = 0x4F000007;    // 注入 DLL 失败
constexpr int kOpErrSizeLimit = 0x4F000008;          // 超出本接口允许的长度上限

void op_set_last_error(int code);

} // namespace op

#endif // OP_BASE_UTILS_H_
