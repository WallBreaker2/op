#include "ClientContext.h"
#include "ClientResult.h"

#include "memory/ProcessMemory.h"

#include <libop.h>

#include <Windows.h>
#include <cstdint>
#include <string>

namespace {

static LONG_PTR resolve_memory_hwnd(op::Client *self, LONG_PTR hwnd) {
    if (hwnd != 0)
        return hwnd;
    LONG_PTR bind_hwnd = 0;
    self->GetBindWindow(&bind_hwnd);
    return bind_hwnd;
}

} // namespace

void op::Client::WriteData(LONG_PTR hwnd, const wchar_t *address, const wchar_t *data, long size, long *ret) {
    internal::set_result(ret, 0L);
    if (!ret || !address || !data || size < 0)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory mem;
        internal::set_result(ret, mem.WriteData(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address, data, size));
    } catch (...) {
        internal::set_result(ret, 0L);
    }
}
// 读取数据
void op::Client::ReadData(LONG_PTR hwnd, const wchar_t *address, long size, std::wstring &retstr) {
    retstr.clear();
    if (!address || size < 0)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory mem;
        retstr = mem.ReadData(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address, size);
    } catch (...) {
        retstr.clear();
    }
}

void op::Client::ReadInt(LONG_PTR hwnd, const wchar_t *address, long type, int64_t *ret) {
    internal::set_result(ret, 0);
    if (!address || !ret)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory mem;
        internal::set_result(ret, mem.ReadInt(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address, type));
    } catch (...) {
        internal::set_result(ret, 0);
    }
}

void op::Client::WriteInt(LONG_PTR hwnd, const wchar_t *address, long type, int64_t value, long *ret) {
    internal::set_result(ret, 0L);
    if (!address || !ret)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory mem;
        internal::set_result(ret, mem.WriteInt(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address, type, value));
    } catch (...) {
        internal::set_result(ret, 0L);
    }
}

void op::Client::ReadFloat(LONG_PTR hwnd, const wchar_t *address, float *ret) {
    internal::set_result(ret, 0.0f);
    if (!address || !ret)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory mem;
        internal::set_result(ret, mem.ReadFloat(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address));
    } catch (...) {
        internal::set_result(ret, 0.0f);
    }
}

void op::Client::WriteFloat(LONG_PTR hwnd, const wchar_t *address, float value, long *ret) {
    internal::set_result(ret, 0L);
    if (!address || !ret)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory mem;
        internal::set_result(ret, mem.WriteFloat(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address, value));
    } catch (...) {
        internal::set_result(ret, 0L);
    }
}

void op::Client::ReadDouble(LONG_PTR hwnd, const wchar_t *address, double *ret) {
    internal::set_result(ret, 0.0);
    if (!address || !ret)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory mem;
        internal::set_result(ret, mem.ReadDouble(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address));
    } catch (...) {
        internal::set_result(ret, 0.0);
    }
}

void op::Client::WriteDouble(LONG_PTR hwnd, const wchar_t *address, double value, long *ret) {
    internal::set_result(ret, 0L);
    if (!address || !ret)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory mem;
        internal::set_result(ret, mem.WriteDouble(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address, value));
    } catch (...) {
        internal::set_result(ret, 0L);
    }
}

void op::Client::ReadString(LONG_PTR hwnd, const wchar_t *address, long type, long len, std::wstring &retstr) {
    retstr.clear();
    if (!address)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory mem;
        retstr = mem.ReadString(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address, type, len);
    } catch (...) {
        retstr.clear();
    }
}

void op::Client::WriteString(LONG_PTR hwnd, const wchar_t *address, long type, const wchar_t *value, long *ret) {
    internal::set_result(ret, 0L);
    if (!address || !value || !ret)
        return;
    hwnd = resolve_memory_hwnd(this, hwnd);
    try {
        ProcessMemory mem;
        internal::set_result(ret,
                             mem.WriteString(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd)), address, type, value));
    } catch (...) {
        internal::set_result(ret, 0L);
    }
}
