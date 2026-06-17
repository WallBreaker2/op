#include "MemoryEx.h"
#include <Windows.h>
#include <cstring>
#include <cwchar>
#include <cwctype>

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

MemoryEx::MemoryEx() {
}

MemoryEx::~MemoryEx() {
}

long MemoryEx::WriteData(HWND hwnd, const wstring &address, const wstring &data, LONG size) {
    if (size <= 0 || !checkaddress(address))
        return 0;
    vector<uchar> bin;
    hex2bins(bin, data, size);
    return WriteRaw(hwnd, address, bin.data(), bin.size()) ? 1 : 0;
}

wstring MemoryEx::ReadData(HWND hwnd, const wstring &address, LONG size) {
    if (size <= 0 || !checkaddress(address))
        return L"";
    vector<uchar> bin(static_cast<size_t>(size));
    wstring hex;
    if (!ReadRaw(hwnd, address, bin.data(), bin.size()))
        return L"";
    bin2hexs(bin, hex);
    return hex;
}

bool MemoryEx::ReadRaw(HWND hwnd, const wstring &address, void *buf, size_t size) {
    if (!buf || size == 0 || !checkaddress(address))
        return false;
    if (!prepare_process(hwnd))
        return false;
    const size_t addr = str2address(address);
    if (addr == 0)
        return false;
    return mem_read(buf, addr, size);
}

bool MemoryEx::WriteRaw(HWND hwnd, const wstring &address, const void *buf, size_t size) {
    if (!buf || size == 0 || !checkaddress(address))
        return false;
    if (!prepare_process(hwnd))
        return false;
    const size_t addr = str2address(address);
    if (addr == 0)
        return false;
    return mem_write(addr, const_cast<void *>(buf), size);
}

size_t MemoryEx::IntTypeSize(long type) {
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

bool MemoryEx::ReadInt(HWND hwnd, const wstring &address, long type, int64_t *value) {
    if (value)
        *value = 0;
    if (!value)
        return false;

    const size_t sz = IntTypeSize(type);
    vector<uchar> bin(sz);
    if (!ReadRaw(hwnd, address, bin.data(), sz))
        return false;

    switch (type) {
    case 2:
        *value = static_cast<int64_t>(static_cast<int8_t>(bin[0]));
        break;
    case 6:
        *value = static_cast<int64_t>(bin[0]);
        break;
    case 1: {
        int16_t v = 0;
        memcpy(&v, bin.data(), sizeof(v));
        *value = v;
        break;
    }
    case 5: {
        uint16_t v = 0;
        memcpy(&v, bin.data(), sizeof(v));
        *value = static_cast<int64_t>(v);
        break;
    }
    case 3: {
        int64_t v = 0;
        memcpy(&v, bin.data(), sizeof(v));
        *value = v;
        break;
    }
    case 4: {
        uint32_t v = 0;
        memcpy(&v, bin.data(), sizeof(v));
        *value = static_cast<int64_t>(v);
        break;
    }
    case 0:
    default: {
        int32_t v = 0;
        memcpy(&v, bin.data(), sizeof(v));
        *value = static_cast<int64_t>(v);
        break;
    }
    }
    return true;
}

int64_t MemoryEx::ReadInt(HWND hwnd, const wstring &address, long type) {
    int64_t value = 0;
    ReadInt(hwnd, address, type, &value);
    return value;
}

long MemoryEx::WriteInt(HWND hwnd, const wstring &address, long type, int64_t value) {
    const size_t sz = IntTypeSize(type);
    vector<uchar> bin(sz);
    switch (type) {
    case 2:
        bin[0] = static_cast<uchar>(static_cast<int8_t>(value));
        break;
    case 6:
        bin[0] = static_cast<uchar>(value);
        break;
    case 1: {
        const int16_t v = static_cast<int16_t>(value);
        memcpy(bin.data(), &v, sizeof(v));
        break;
    }
    case 5: {
        const uint16_t v = static_cast<uint16_t>(value);
        memcpy(bin.data(), &v, sizeof(v));
        break;
    }
    case 3:
        memcpy(bin.data(), &value, sizeof(value));
        break;
    case 4: {
        const uint32_t v = static_cast<uint32_t>(value);
        memcpy(bin.data(), &v, sizeof(v));
        break;
    }
    case 0:
    default: {
        const int32_t v = static_cast<int32_t>(value);
        memcpy(bin.data(), &v, sizeof(v));
        break;
    }
    }
    return WriteRaw(hwnd, address, bin.data(), sz) ? 1 : 0;
}

bool MemoryEx::ReadFloat(HWND hwnd, const wstring &address, float *value) {
    if (value)
        *value = 0.0f;
    if (!value)
        return false;
    return ReadRaw(hwnd, address, value, sizeof(*value));
}

float MemoryEx::ReadFloat(HWND hwnd, const wstring &address) {
    float value = 0.0f;
    ReadFloat(hwnd, address, &value);
    return value;
}

long MemoryEx::WriteFloat(HWND hwnd, const wstring &address, float value) {
    return WriteRaw(hwnd, address, &value, sizeof(value)) ? 1 : 0;
}

bool MemoryEx::ReadDouble(HWND hwnd, const wstring &address, double *value) {
    if (value)
        *value = 0.0;
    if (!value)
        return false;
    return ReadRaw(hwnd, address, value, sizeof(*value));
}

double MemoryEx::ReadDouble(HWND hwnd, const wstring &address) {
    double value = 0.0;
    ReadDouble(hwnd, address, &value);
    return value;
}

long MemoryEx::WriteDouble(HWND hwnd, const wstring &address, double value) {
    return WriteRaw(hwnd, address, &value, sizeof(value)) ? 1 : 0;
}

wstring MemoryEx::ReadString(HWND hwnd, const wstring &address, long type, long len) {
    const long maxAuto = 4096;
    const long readLen = len > 0 ? len : maxAuto;
    if (readLen <= 0)
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

long MemoryEx::WriteString(HWND hwnd, const wstring &address, long type, const wstring &value) {
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

bool MemoryEx::prepare_process(HWND hwnd) {
    _hwnd = hwnd;
    if (!_hwnd)
        return true;
    if (!::IsWindow(hwnd))
        return false;

    DWORD pid = 0;
    ::GetWindowThreadProcessId(hwnd, &pid);
    return _proc.Attach(pid) >= 0;
}

bool MemoryEx::mem_read(void *dst, size_t src, size_t size) {
    if (!dst || size == 0)
        return false;
    if (_hwnd) {
        return _proc.memory().Read(src, size, dst) >= 0;
    }
    SIZE_T read = 0;
    return ::ReadProcessMemory(::GetCurrentProcess(), reinterpret_cast<LPCVOID>(src), dst, size, &read) && read == size;
}

bool MemoryEx::mem_write(size_t dst, void *src, size_t size) {
    if (!src || size == 0)
        return false;
    if (_hwnd) {
        return _proc.memory().Write(dst, size, src) >= 0;
    }
    SIZE_T written = 0;
    return ::WriteProcessMemory(::GetCurrentProcess(), reinterpret_cast<LPVOID>(dst), src, size, &written) &&
           written == size;
}

bool MemoryEx::checkaddress(const wstring &address) {
    vector<wchar_t> sk;
    auto p = address.data();
    while (*p) {
        if (*p == L'[') {
            sk.push_back(*p);
        } else if (*p == L']') {
            if (sk.empty())
                return false;
            sk.pop_back();
        }
        p++;
    }
    return sk.empty();
}

size_t MemoryEx::str2address(const wstring &caddress) {
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
        wchar_t buff[32];
        swprintf_s(buff, L"%llX", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(hmod)));
        address.replace(idx1, idx2 - idx1 + 1, buff);
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
            wchar_t buff[32];
            swprintf_s(buff, L"%llX", static_cast<unsigned long long>(next));
            address.replace(idx1, idx2 - idx1 + 1, buff);
            i = idx1;
        }
        ++i;
    }
    if (address.find(L'[') == wstring::npos && !address_has_operator(address))
        return parse_hex_word(address);
    return stringcompute(address.data());
}

void MemoryEx::hex2bins(vector<uchar> &bin, const wstring &hex, size_t size) {
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

void MemoryEx::bin2hexs(const vector<uchar> &bin, wstring &hex) {
    hex.reserve(bin.size() * 2);
    hex.clear();
    for (size_t i = 0; i < bin.size(); ++i) {
        int ans = bin2hex(bin[i]);
        hex.push_back(ans >> 8);
        hex.push_back(ans & 0xff);
    }
}
