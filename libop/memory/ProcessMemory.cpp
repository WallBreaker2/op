#include "ProcessMemory.h"
#include <Windows.h>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <sstream>

namespace op {

namespace {

template <typename T> T pop_back_value(vector<T> &items) {
    T value = items.back();
    items.pop_back();
    return value;
}

wstring bytesToWide(const vector<uchar> &bin, UINT cp) {
    if (bin.empty())
        return L"";
    const int len =
        ::MultiByteToWideChar(cp, 0, reinterpret_cast<LPCCH>(bin.data()), static_cast<int>(bin.size()), nullptr, 0);
    if (len <= 0)
        return L"";
    wstring out(len, L'\0');
    ::MultiByteToWideChar(cp, 0, reinterpret_cast<LPCCH>(bin.data()), static_cast<int>(bin.size()), &out[0], len);
    return out;
}

vector<uchar> wideToBytes(const wstring &text, UINT cp, bool nullTerminated) {
    const int len = ::WideCharToMultiByte(cp, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0)
        return {};
    vector<uchar> bin(static_cast<size_t>(len));
    ::WideCharToMultiByte(cp, 0, text.c_str(), -1, reinterpret_cast<LPSTR>(bin.data()), len, nullptr, nullptr);
    if (!nullTerminated && !bin.empty() && bin.back() == 0)
        bin.pop_back();
    return bin;
}

wstring decodeUnicodeBytes(const vector<uchar> &bin) {
    if (bin.size() < sizeof(wchar_t))
        return L"";
    const size_t chars = bin.size() / sizeof(wchar_t);
    wstring out(chars, L'\0');
    memcpy(&out[0], bin.data(), chars * sizeof(wchar_t));
    const size_t z = out.find(L'\0');
    if (z != wstring::npos)
        out.resize(z);
    return out;
}

vector<uchar> encodeUnicodeBytes(const wstring &text) {
    vector<uchar> bin((text.size() + 1) * sizeof(wchar_t));
    if (!text.empty())
        memcpy(bin.data(), text.c_str(), text.size() * sizeof(wchar_t));
    return bin;
}

size_t trimNullByteLength(const vector<uchar> &bin, size_t charWidth) {
    if (charWidth == 0 || bin.empty())
        return 0;
    for (size_t i = 0; i + charWidth <= bin.size(); i += charWidth) {
        bool zero = true;
        for (size_t j = 0; j < charWidth; ++j) {
            if (bin[i + j] != 0) {
                zero = false;
                break;
            }
        }
        if (zero)
            return i;
    }
    return bin.size();
}

bool address_has_operator(const wstring &address) {
    for (wchar_t c : address) {
        if (c == L'+' || c == L'-' || c == L'*' || c == L'/')
            return true;
    }
    return false;
}

size_t parse_hex_word(const wstring &num) {
    if (num.empty())
        return 0;
    wchar_t *end = nullptr;
    const unsigned long long v = wcstoull(num.c_str(), &end, 16);
    if (end == num.c_str())
        return 0;
#if defined(_WIN64)
    return static_cast<size_t>(v);
#else
    return static_cast<size_t>(static_cast<unsigned long>(v));
#endif
}

size_t do_op_ptr(size_t a, size_t b, wchar_t op) {
    switch (op) {
    case L'+':
        return a + b;
    case L'-':
        return a - b;
    case L'*':
        return a * b;
    case L'/':
        return b ? a / b : 0;
    default:
        return 0;
    }
}

wstring normalize_hex(const wstring &hex) {
    wstring out;
    out.reserve(hex.size());
    for (wchar_t c : hex) {
        if ((c >= L'0' && c <= L'9') || (c >= L'A' && c <= L'F') || (c >= L'a' && c <= L'f'))
            out.push_back(static_cast<wchar_t>(towupper(c)));
    }
    return out;
}

wstring toUpperHex(uintptr_t value) {
    std::wstringstream out;
    out << std::uppercase << std::hex << value;
    return out.str();
}

} // namespace

namespace {

int get_op_prior(wchar_t op) {
    if (op == L'+' || op == L'-')
        return 0;
    if (op == L'*' || op == L'/')
        return 1;
    return 2;
}

bool is_op(wchar_t op) {
    return op == L'+' || op == L'-' || op == L'*' || op == L'/';
}

// like AA+BB+DD-CC*cc. Use size_t so x64 absolute addresses parse correctly.
size_t stringcompute(const wchar_t *s) {
    if (!s || !*s)
        return 0;
    wstring num;
    vector<size_t> ns;
    vector<wchar_t> os;
    while (true) {
        if (*s && !is_op(*s)) {
            num.push_back(*s++);
            continue;
        }

        if (!num.empty()) {
            ns.push_back(parse_hex_word(num));
            num.clear();
        }

        if (*s == 0)
            break;

        if (ns.empty()) {
            os.push_back(*s++);
            continue;
        }

        if (!os.empty()) {
            const wchar_t pending = os.back();
            if (get_op_prior(pending) >= get_op_prior(*s)) {
                const size_t num2 = pop_back_value(ns);
                const size_t num1 = pop_back_value(ns);
                const wchar_t popped = pop_back_value(os);
                ns.push_back(do_op_ptr(num1, num2, popped));
                continue;
            }
        }

        os.push_back(*s++);
    }

    while (!os.empty()) {
        if (ns.size() < 2)
            break;
        const size_t num2 = pop_back_value(ns);
        const size_t num1 = pop_back_value(ns);
        const wchar_t popped = pop_back_value(os);
        ns.push_back(do_op_ptr(num1, num2, popped));
    }

    if (ns.empty())
        return 0;
    return ns.back();
}

} // namespace

ProcessMemory::ProcessMemory() {
}

ProcessMemory::~ProcessMemory() {
}

long ProcessMemory::WriteData(HWND hwnd, const wstring &address, const wstring &data, LONG size) {
    if (size <= 0 || !checkaddress(address))
        return 0;
    vector<uchar> bin;
    hex2bins(bin, data, size);
    return WriteRaw(hwnd, address, bin.data(), bin.size()) ? 1 : 0;
}

wstring ProcessMemory::ReadData(HWND hwnd, const wstring &address, LONG size) {
    // size 是外部传进来的 LONG：不设上限时一次 0x7FFFFFFF 就能让 vector 申请 2GB。
    constexpr LONG kMaxReadDataBytes = 64 * 1024 * 1024;
    if (size <= 0 || size > kMaxReadDataBytes || !checkaddress(address))
        return L"";
    vector<uchar> bin(static_cast<size_t>(size));
    wstring hex;
    if (!ReadRaw(hwnd, address, bin.data(), bin.size()))
        return L"";
    bin2hexs(bin, hex);
    return hex;
}

bool ProcessMemory::ReadRaw(HWND hwnd, const wstring &address, void *buf, size_t size) {
    if (!buf || size == 0 || !checkaddress(address))
        return false;
    if (!prepare_process(hwnd))
        return false;
    const size_t addr = resolve_address(address);
    if (addr == 0)
        return false;
    return mem_read(buf, addr, size);
}

bool ProcessMemory::WriteRaw(HWND hwnd, const wstring &address, const void *buf, size_t size) {
    if (!buf || size == 0 || !checkaddress(address))
        return false;
    if (!prepare_process(hwnd))
        return false;
    const size_t addr = resolve_address(address);
    if (addr == 0)
        return false;
    return mem_write(addr, const_cast<void *>(buf), size);
}

size_t ProcessMemory::IntTypeSize(long type) {
    switch (type) {
    case 1:
    case 5:
        return 2;
    case 2:
    case 6:
        return 1;
    case 3:
        return 8;
    case 0:
    case 4:
    default:
        return 4;
    }
}

namespace {

// 目标进程内存里的原始字节 -> 整数。编号沿用大漠：0=i32 1=i16 2=i8 3=i64 4=u32 5=u16 6=u8。
int64_t decode_int_bytes(const uchar *bin, long type) {
    switch (type) {
    case 2:
        return static_cast<int64_t>(static_cast<int8_t>(bin[0]));
    case 6:
        return static_cast<int64_t>(bin[0]);
    case 1: {
        int16_t v = 0;
        memcpy(&v, bin, sizeof(v));
        return v;
    }
    case 5: {
        uint16_t v = 0;
        memcpy(&v, bin, sizeof(v));
        return static_cast<int64_t>(v);
    }
    case 3: {
        int64_t v = 0;
        memcpy(&v, bin, sizeof(v));
        return v;
    }
    case 4: {
        uint32_t v = 0;
        memcpy(&v, bin, sizeof(v));
        return static_cast<int64_t>(v);
    }
    case 0:
    default: {
        int32_t v = 0;
        memcpy(&v, bin, sizeof(v));
        return static_cast<int64_t>(v);
    }
    }
}

// 整数 -> 目标进程内存里的原始字节。
void encode_int_bytes(int64_t value, long type, uchar *bin) {
    switch (type) {
    case 2:
        bin[0] = static_cast<uchar>(static_cast<int8_t>(value));
        break;
    case 6:
        bin[0] = static_cast<uchar>(value);
        break;
    case 1: {
        const int16_t v = static_cast<int16_t>(value);
        memcpy(bin, &v, sizeof(v));
        break;
    }
    case 5: {
        const uint16_t v = static_cast<uint16_t>(value);
        memcpy(bin, &v, sizeof(v));
        break;
    }
    case 3:
        memcpy(bin, &value, sizeof(value));
        break;
    case 4: {
        const uint32_t v = static_cast<uint32_t>(value);
        memcpy(bin, &v, sizeof(v));
        break;
    }
    case 0:
    default: {
        const int32_t v = static_cast<int32_t>(value);
        memcpy(bin, &v, sizeof(v));
        break;
    }
    }
}

} // namespace

bool ProcessMemory::ReadInt(HWND hwnd, const wstring &address, long type, int64_t *value) {
    if (value)
        *value = 0;
    if (!value)
        return false;

    const size_t sz = IntTypeSize(type);
    uchar bin[8] = {};
    if (sz > sizeof(bin))
        return false;
    if (!ReadRaw(hwnd, address, bin, sz))
        return false;

    *value = decode_int_bytes(bin, type);
    return true;
}

int64_t ProcessMemory::ReadInt(HWND hwnd, const wstring &address, long type) {
    int64_t value = 0;
    ReadInt(hwnd, address, type, &value);
    return value;
}

long ProcessMemory::WriteInt(HWND hwnd, const wstring &address, long type, int64_t value) {
    const size_t sz = IntTypeSize(type);
    uchar bin[8] = {};
    if (sz > sizeof(bin))
        return 0;
    encode_int_bytes(value, type, bin);
    return WriteRaw(hwnd, address, bin, sz) ? 1 : 0;
}

bool ProcessMemory::ReadFloat(HWND hwnd, const wstring &address, float *value) {
    if (value)
        *value = 0.0f;
    if (!value)
        return false;
    return ReadRaw(hwnd, address, value, sizeof(*value));
}

float ProcessMemory::ReadFloat(HWND hwnd, const wstring &address) {
    float value = 0.0f;
    ReadFloat(hwnd, address, &value);
    return value;
}

long ProcessMemory::WriteFloat(HWND hwnd, const wstring &address, float value) {
    return WriteRaw(hwnd, address, &value, sizeof(value)) ? 1 : 0;
}

bool ProcessMemory::ReadDouble(HWND hwnd, const wstring &address, double *value) {
    if (value)
        *value = 0.0;
    if (!value)
        return false;
    return ReadRaw(hwnd, address, value, sizeof(*value));
}

double ProcessMemory::ReadDouble(HWND hwnd, const wstring &address) {
    double value = 0.0;
    ReadDouble(hwnd, address, &value);
    return value;
}

long ProcessMemory::WriteDouble(HWND hwnd, const wstring &address, double value) {
    return WriteRaw(hwnd, address, &value, sizeof(value)) ? 1 : 0;
}

wstring ProcessMemory::ReadString(HWND hwnd, const wstring &address, long type, long len) {
    const long maxAuto = 4096;
    // §72.51：len 是调用方给的（COM 的 LONG / C API 的 int），不夹取的话
    // len=INT_MAX 会当场 vector<uchar>(2GB) —— 一个脚本笔误就把调用方进程打爆。
    // 和 ReadData 用同一个上限。
    constexpr long kMaxReadStringBytes = 64 * 1024 * 1024;
    const long readLen = len > 0 ? len : maxAuto;
    if (readLen <= 0 || readLen > kMaxReadStringBytes)
        return L"";
    vector<uchar> bin(static_cast<size_t>(readLen));
    if (!ReadRaw(hwnd, address, bin.data(), bin.size()))
        return L"";

    if (len <= 0) {
        // Auto-length reads stop at the first null character in the selected encoding.
        const size_t byteWidth = (type == 1) ? sizeof(wchar_t) : 1;
        const size_t used = trimNullByteLength(bin, byteWidth);
        bin.resize(used);
    }

    switch (type) {
    case 1:
        return decodeUnicodeBytes(bin);
    case 2:
        return bytesToWide(bin, CP_UTF8);
    case 0:
    default:
        return bytesToWide(bin, CP_ACP);
    }
}

long ProcessMemory::WriteString(HWND hwnd, const wstring &address, long type, const wstring &value) {
    vector<uchar> bin;
    // type follows the historical DM convention: 0=ACP/GBK, 1=UTF-16, 2=UTF-8.
    switch (type) {
    case 1:
        bin = encodeUnicodeBytes(value);
        break;
    case 2:
        bin = wideToBytes(value, CP_UTF8, true);
        break;
    case 0:
    default:
        bin = wideToBytes(value, CP_ACP, true);
        break;
    }
    if (bin.empty())
        return 0;
    return WriteRaw(hwnd, address, bin.data(), bin.size()) ? 1 : 0;
}

void ProcessMemory::invalidate_attach() {
    if (_attached_pid != 0) {
        _proc.Detach();
        _attached_pid = 0;
    }
    // 注意：不要在这里清地址缓存。本进程路径每次都会走一遍 invalidate_attach（_attached_pid 一直是 0），
    // 清掉的话 self 路径永远命中不了缓存。目标切换由 resolve_address 里的 pid 比对负责失效。
}

size_t ProcessMemory::resolve_address(const wstring &address) {
    // 命中条件：同一个目标进程 + 同一份表达式原文。模块基址按进程不同，所以 pid 必须参与比较。
    if (_addr_cache_pid != _attached_pid) {
        _addr_cache.clear();
        _addr_cache_pid = _attached_pid;
    }
    for (const auto &entry : _addr_cache) {
        if (entry.address == address)
            return entry.resolved;
    }

    const size_t resolved = str2address(address);
    if (resolved != 0) {
        // 容量上限压住：地址表达式通常只有几个，超了就整体清空重来，避免无限增长。
        constexpr size_t kMaxCachedAddresses = 16;
        if (_addr_cache.size() >= kMaxCachedAddresses)
            _addr_cache.clear();
        _addr_cache.push_back({address, resolved});
    }
    return resolved;
}

bool ProcessMemory::prepare_process(HWND hwnd) {
    _hwnd = hwnd;
    if (!_hwnd) {
        // 从跨进程切回本进程：把句柄放掉，别一直攥着别人进程的 handle。
        invalidate_attach();
        return true;
    }
    if (!::IsWindow(hwnd))
        return false;

    DWORD pid = 0;
    ::GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0)
        return false;

    // 同一个目标连续读写是常态（脚本一次循环里几十次 ReadInt）：pid 没变就复用已 attach 的上下文。
    // BlackBone 的 Attach(pid) 内部是 Detach() + OpenProcess()，每调用一次就白付一次句柄 churn。
    if (pid == _attached_pid)
        return true;

    invalidate_attach();
    if (_proc.Attach(pid) < 0)
        return false;
    _attached_pid = pid;
    return true;
}

namespace {

// BlackBone 在不可读地址上会抛异常（工程用 /EHa）。这里单独兜住，避免一条坏地址顺着
// 调用栈把整批读掉，也避免抛出路径跳过调用方的 invalidate_attach（§72.36）。
bool blackbone_read(blackbone::Process &proc, size_t src, size_t size, void *dst) {
    try {
        return proc.memory().Read(src, size, dst) >= 0;
    } catch (...) {
        return false;
    }
}

bool blackbone_write(blackbone::Process &proc, size_t dst_addr, size_t size, void *src) {
    try {
        return proc.memory().Write(dst_addr, size, src) >= 0;
    } catch (...) {
        return false;
    }
}

} // namespace

bool ProcessMemory::mem_read(void *dst, size_t src, size_t size) {
    if (!dst || size == 0 || src == 0)
        return false;
    if (_hwnd) {
        const bool ok = blackbone_read(_proc, src, size, dst);
        // 目标可能已经退出：丢掉缓存，下一次调用重新 Open，别让缓存把失败固化下来。
        if (!ok)
            invalidate_attach();
        return ok;
    }
    // 本进程直接 memcpy：ReadProcessMemory 即使读自己也要走一次系统调用 + 内核拷贝，
    // 单次 4 字节读实测 1.25us，memcpy 只要几十纳秒。非法地址交给 SEH 兜住（工程用 /EHa）。
    __try {
        memcpy(dst, reinterpret_cast<const void *>(src), size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool ProcessMemory::mem_write(size_t dst, void *src, size_t size) {
    if (!src || size == 0 || dst == 0)
        return false;
    if (_hwnd) {
        const bool ok = blackbone_write(_proc, dst, size, src);
        if (!ok)
            invalidate_attach();
        return ok;
    }
    __try {
        memcpy(reinterpret_cast<void *>(dst), src, size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool ProcessMemory::checkaddress(const wstring &address) {
    // 只做括号配对检查：用计数器，别每次调用都分配一个 vector。
    int depth = 0;
    for (const wchar_t ch : address) {
        if (ch == L'[') {
            ++depth;
        } else if (ch == L']') {
            if (--depth < 0)
                return false;
        }
    }
    return depth == 0;
}

size_t ProcessMemory::str2address(const wstring &caddress) {
    wstring address = caddress;
    if (!checkaddress(address))
        return 0;

    vector<size_t> sk;
    size_t idx1 = address.find(L'<');
    size_t idx2 = address.find(L'>');
    if (idx1 != wstring::npos && idx2 != wstring::npos && idx1 < idx2) {
        auto mod_name = address.substr(idx1 + 1, idx2 - idx1 - 1);
        HMODULE hmod = NULL;
        if (_hwnd == 0)
            hmod = ::GetModuleHandleW(mod_name.data());
        else {
            auto mptr = _proc.modules().GetModule(mod_name);
            if (mptr)
                hmod = (HMODULE)mptr->baseAddress;
        }
        if (hmod == NULL)
            return 0;
        address.replace(idx1, idx2 - idx1 + 1, toUpperHex(reinterpret_cast<uintptr_t>(hmod)));
    }
    for (size_t i = 0; i < address.size();) {
        if (address[i] == L'[')
            sk.push_back(i);
        if (address[i] == L']') {
            idx1 = pop_back_value(sk);
            idx2 = i;
            auto sad = address.substr(idx1 + 1, idx2 - idx1 - 1);
            size_t src = stringcompute(sad.data());
            size_t next;
            if (!mem_read(&next, src, sizeof(size_t)) || next == 0)
                return 0;
            address.replace(idx1, idx2 - idx1 + 1, toUpperHex(next));
            i = idx1;
        }
        ++i;
    }
    if (address.find(L'[') == wstring::npos && !address_has_operator(address))
        return parse_hex_word(address);
    return stringcompute(address.data());
}

void ProcessMemory::hex2bins(vector<uchar> &bin, const wstring &hex, size_t size) {
    const wstring clean = normalize_hex(hex);
    const size_t available = clean.size() / 2;
    const size_t write_bytes = size > 0 ? size : available;
    bin.resize(write_bytes);
    ZeroMemory(bin.data(), bin.size());
    const size_t copy_bytes = available < write_bytes ? available : write_bytes;
    for (size_t i = 0; i < copy_bytes; ++i) {
        const int hi = hex2bin(clean[i * 2]);
        const int lo = hex2bin(clean[i * 2 + 1]);
        bin[i] = static_cast<uchar>((hi << 4) | lo);
    }
}

void ProcessMemory::bin2hexs(const vector<uchar> &bin, wstring &hex) {
    hex.reserve(bin.size() * 2);
    hex.clear();
    for (size_t i = 0; i < bin.size(); ++i) {
        int ans = bin2hex(bin[i]);
        hex.push_back(ans >> 8);
        hex.push_back(ans & 0xff);
    }
}


long ProcessMemory::ReadIntBatch(HWND hwnd, const wstring *addresses, long count, long type, int64_t *values,
                                 bool *ok) {
    if (!addresses || count <= 0 || !values)
        return 0;
    if (!prepare_process(hwnd))
        return 0;

    const size_t sz = IntTypeSize(type);
    if (sz > sizeof(int64_t))
        return 0;

    uchar bin[8] = {};
    long done = 0;
    for (long i = 0; i < count; ++i) {
        bool good = false;
        // 每项单独 try：工程用 /EHa，mem_read 撞到不可读地址会以 C++ 异常形式抛出来，
        // 不拦的话一条坏地址就把整批（包括后面本来能读的）全废掉，返回值还会变成 0。
        // 单条形状有 facade 层的 catch 兜着，批量形状必须逐项兜（§72.36）。
        try {
            if (!addresses[i].empty() && checkaddress(addresses[i])) {
                const size_t addr = resolve_address(addresses[i]);
                if (addr != 0 && mem_read(bin, addr, sz)) {
                    values[i] = decode_int_bytes(bin, type);
                    good = true;
                }
            }
        } catch (...) {
            good = false;
        }
        if (!good)
            prepare_process(hwnd); // 句柄可能被上一条失败 invalidate 掉了，重新挂上再读下一项
        if (!good)
            values[i] = 0;
        else
            ++done;
        if (ok)
            ok[i] = good;
    }
    return done;
}

long ProcessMemory::WriteIntBatch(HWND hwnd, const wstring *addresses, long count, long type,
                                  const int64_t *values, bool *ok) {
    if (!addresses || !values || count <= 0)
        return 0;
    if (!prepare_process(hwnd))
        return 0;

    const size_t sz = IntTypeSize(type);
    if (sz > sizeof(int64_t))
        return 0;

    uchar bin[8] = {};
    long done = 0;
    for (long i = 0; i < count; ++i) {
        bool good = false;
        try { // 逐项兜异常，理由同 ReadIntBatch
            if (!addresses[i].empty() && checkaddress(addresses[i])) {
                const size_t addr = resolve_address(addresses[i]);
                if (addr != 0) {
                    encode_int_bytes(values[i], type, bin);
                    good = mem_write(addr, bin, sz);
                }
            }
        } catch (...) {
            good = false;
        }
        if (!good)
            prepare_process(hwnd); // 理由同 ReadIntBatch
        if (good)
            ++done;
        if (ok)
            ok[i] = good;
    }
    return done;
}

long ProcessMemory::ReadRawBatch(HWND hwnd, const wstring *addresses, long count, size_t item_size, void *buffer,
                                 bool *ok) {
    if (!addresses || !buffer || count <= 0 || item_size == 0)
        return 0;
    if (!prepare_process(hwnd))
        return 0;

    auto *out = static_cast<uchar *>(buffer);
    long done = 0;
    for (long i = 0; i < count; ++i) {
        bool good = false;
        try { // 逐项兜异常，理由同 ReadIntBatch
            if (!addresses[i].empty() && checkaddress(addresses[i])) {
                const size_t addr = resolve_address(addresses[i]);
                good = addr != 0 && mem_read(out + static_cast<size_t>(i) * item_size, addr, item_size);
            }
        } catch (...) {
            good = false;
        }
        if (!good)
            prepare_process(hwnd); // 理由同 ReadIntBatch
        if (!good)
            memset(out + static_cast<size_t>(i) * item_size, 0, item_size);
        else
            ++done;
        if (ok)
            ok[i] = good;
    }
    return done;
}

} // namespace op
