#include "OpContext.h"
#include "OpResult.h"

#include "memory/ProcessMemory.h"

#include <libop.h>

#include <Windows.h>
#include <cstdint>
#include <string>
#include <vector>

namespace {

static LONG_PTR resolve_memory_hwnd(op::Op *self, LONG_PTR hwnd) {
    if (hwnd != 0)
        return hwnd;
    LONG_PTR bind_hwnd = 0;
    self->GetBindWindow(&bind_hwnd);
    return bind_hwnd;
}

// §72.51（审计 F）：原来每个入口都 `ProcessMemory mem;` —— 构造/析构一次就要
// attach 一次目标进程（枚举进程 + 开句柄），读一个 4 字节的值也走全套。
// 改成每线程常驻：同一线程反复调 ReadInt/WriteInt/ReadData 时，pid 没变就命中
// ProcessMemory 内部的 attach 缓存（§72.30）与地址缓存，只花一次句柄查询。
//
// 为什么是 thread_local 而不是 Op 的成员：ProcessMemory 的地址缓存没有锁，
// 多线程共用一个对象会撕裂。
// 为什么 new 了不 delete：thread_local 对象在线程退出 / DLL 卸载时析构，那时
// 可能正处在 loader lock 里，而 ~ProcessMemory 要 Detach。泄漏几十字节换进程
// 退出期的确定性。
op::ProcessMemory &thread_memory() {
    thread_local op::ProcessMemory *mem = new op::ProcessMemory();
    return *mem;
}

} // namespace

void op::Op::WriteData(LONG_PTR hwnd, const wchar_t *address, const wchar_t *data, long size, long *ret) {
    internal::set_result(ret, 0L);
    if (!ret || !address || !data || size < 0)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory &mem = thread_memory();
        internal::set_result(ret, mem.WriteData(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address, data, size));
    } catch (...) {
        internal::set_result(ret, 0L);
    }
}
// 读取数据
void op::Op::ReadData(LONG_PTR hwnd, const wchar_t *address, long size, std::wstring &retstr) {
    retstr.clear();
    if (!address || size < 0)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory &mem = thread_memory();
        retstr = mem.ReadData(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address, size);
    } catch (...) {
        retstr.clear();
    }
}

void op::Op::ReadInt(LONG_PTR hwnd, const wchar_t *address, long type, int64_t *ret) {
    internal::set_result(ret, 0);
    if (!address || !ret)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory &mem = thread_memory();
        internal::set_result(ret, mem.ReadInt(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address, type));
    } catch (...) {
        internal::set_result(ret, 0);
    }
}

void op::Op::WriteInt(LONG_PTR hwnd, const wchar_t *address, long type, int64_t value, long *ret) {
    internal::set_result(ret, 0L);
    if (!address || !ret)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory &mem = thread_memory();
        internal::set_result(ret, mem.WriteInt(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address, type, value));
    } catch (...) {
        internal::set_result(ret, 0L);
    }
}

void op::Op::ReadIntBatch(LONG_PTR hwnd, const wchar_t *const *addresses, long count, long type, int64_t *values,
                          long *ret, bool *ok) {
    internal::set_result(ret, 0L);
    if (!addresses || count <= 0 || !values)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        std::vector<std::wstring> list;
        list.reserve(static_cast<size_t>(count));
        for (long i = 0; i < count; ++i)
            list.emplace_back(addresses[i] ? addresses[i] : L"");
        ProcessMemory &mem = thread_memory();
        internal::set_result(ret, mem.ReadIntBatch(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), list.data(),
                                                   count, type, values, ok));
    } catch (...) {
        internal::set_result(ret, 0L);
    }
}

void op::Op::WriteIntBatch(LONG_PTR hwnd, const wchar_t *const *addresses, long count, long type,
                           const int64_t *values, long *ret, bool *ok) {
    internal::set_result(ret, 0L);
    if (!addresses || !values || count <= 0)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        std::vector<std::wstring> list;
        list.reserve(static_cast<size_t>(count));
        for (long i = 0; i < count; ++i)
            list.emplace_back(addresses[i] ? addresses[i] : L"");
        ProcessMemory &mem = thread_memory();
        internal::set_result(ret, mem.WriteIntBatch(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), list.data(),
                                                    count, type, values, ok));
    } catch (...) {
        internal::set_result(ret, 0L);
    }
}

void op::Op::ReadFloat(LONG_PTR hwnd, const wchar_t *address, float *ret) {
    internal::set_result(ret, 0.0f);
    if (!address || !ret)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory &mem = thread_memory();
        internal::set_result(ret, mem.ReadFloat(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address));
    } catch (...) {
        internal::set_result(ret, 0.0f);
    }
}

void op::Op::WriteFloat(LONG_PTR hwnd, const wchar_t *address, float value, long *ret) {
    internal::set_result(ret, 0L);
    if (!address || !ret)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory &mem = thread_memory();
        internal::set_result(ret, mem.WriteFloat(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address, value));
    } catch (...) {
        internal::set_result(ret, 0L);
    }
}

void op::Op::ReadDouble(LONG_PTR hwnd, const wchar_t *address, double *ret) {
    internal::set_result(ret, 0.0);
    if (!address || !ret)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory &mem = thread_memory();
        internal::set_result(ret, mem.ReadDouble(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address));
    } catch (...) {
        internal::set_result(ret, 0.0);
    }
}

void op::Op::WriteDouble(LONG_PTR hwnd, const wchar_t *address, double value, long *ret) {
    internal::set_result(ret, 0L);
    if (!address || !ret)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory &mem = thread_memory();
        internal::set_result(ret, mem.WriteDouble(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address, value));
    } catch (...) {
        internal::set_result(ret, 0L);
    }
}

void op::Op::ReadString(LONG_PTR hwnd, const wchar_t *address, long type, long len, std::wstring &retstr) {
    retstr.clear();
    if (!address)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory &mem = thread_memory();
        retstr = mem.ReadString(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address, type, len);
    } catch (...) {
        retstr.clear();
    }
}

void op::Op::WriteString(LONG_PTR hwnd, const wchar_t *address, long type, const wchar_t *value, long *ret) {
    internal::set_result(ret, 0L);
    if (!address || !value || !ret)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory &mem = thread_memory();
        internal::set_result(ret,
                             mem.WriteString(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address, type, value));
    } catch (...) {
        internal::set_result(ret, 0L);
    }
}
