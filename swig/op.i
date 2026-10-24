#define OP_API
#define _In_
#define _In_reads_bytes_(x)
#define _Out_
#define _Out_opt_
#define _Inout_
%module pyop

%{
    #define OP_API
#include "../include/libop.h"
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

// '|' 分隔的批量内存接口要用：把文本切成地址/数值列表。
static void op_split_pipe(const std::wstring &text, std::vector<std::wstring> &out) {
    out.clear();
    if (text.empty()) {
        return;
    }
    std::wstring current;
    for (wchar_t c : text) {
        if (c == L'|') {
            out.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    out.push_back(current);
}
%}

%include "std_string.i"
%include "std_wstring.i"
%include "typemaps.i"
%apply long long { LONG_PTR };
%apply std::wstring &OUTPUT {std::wstring &}
%apply long *OUTPUT {long *}
%apply long long *OUTPUT {LONG_PTR *}
%apply size_t *OUTPUT {size_t *}

%typemap(in) const void * (Py_buffer view) {
    if (PyObject_GetBuffer($input, &view, PyBUF_SIMPLE) != 0) {
        SWIG_exception_fail(SWIG_TypeError, "expected a bytes-like object");
    }
    $1 = view.buf;
}

%typemap(freearg) const void * {
    PyBuffer_Release(&view$argnum);
}

%extend op::Op {
    void RunApp(const wchar_t *cmdline, long mode, long *ret) {
        unsigned long pid = 0;
        $self->RunApp(cmdline, mode, &pid, ret);
    }

    // 批量读：addresses 是 '|' 分隔的地址表达式；out 是 '|' 分隔的十进制值，
    // 读失败的项留空字段（与 COM 层 ReadIntBatch 的输出形状一致）。返回成功条数。
    // C++ 接口收的是 wchar_t** 数组，Python 侧传列表很别扭，所以在绑定层补这层薄封装。
    long ReadIntBatchText(LONG_PTR hwnd, std::wstring addresses, long type, std::wstring &out) {
        std::vector<std::wstring> parts;
        op_split_pipe(addresses, parts);
        std::vector<const wchar_t *> ptrs;
        ptrs.reserve(parts.size());
        for (size_t i = 0; i < parts.size(); ++i) {
            ptrs.push_back(parts[i].c_str());
        }

        std::vector<int64_t> values(parts.size(), 0);
        std::vector<unsigned char> ok_buf(parts.size(), 0);
        long ret = 0;
        if (!ptrs.empty()) {
            $self->ReadIntBatch(hwnd, ptrs.data(), static_cast<long>(ptrs.size()), type, values.data(), &ret,
                                reinterpret_cast<bool *>(ok_buf.data()));
        }

        out.clear();
        for (size_t i = 0; i < values.size(); ++i) {
            if (i != 0) {
                out += L'|';
            }
            if (ok_buf[i] != 0) {
                out += std::to_wstring(values[i]);
            }
        }
        return ret;
    }

    // 批量写：addresses / values 都是 '|' 分隔，条数必须一致（不一致返回 -1，不猜）。
    long WriteIntBatchText(LONG_PTR hwnd, std::wstring addresses, long type, std::wstring values_text) {
        std::vector<std::wstring> parts;
        op_split_pipe(addresses, parts);
        std::vector<std::wstring> value_parts;
        op_split_pipe(values_text, value_parts);
        if (parts.size() != value_parts.size()) {
            return -1;
        }

        std::vector<const wchar_t *> ptrs;
        std::vector<int64_t> values;
        ptrs.reserve(parts.size());
        values.reserve(parts.size());
        for (size_t i = 0; i < parts.size(); ++i) {
            ptrs.push_back(parts[i].c_str());
            values.push_back(static_cast<int64_t>(_wtoi64(value_parts[i].c_str())));
        }

        std::vector<unsigned char> ok_buf(parts.size(), 0);
        long ret = 0;
        if (!ptrs.empty()) {
            $self->WriteIntBatch(hwnd, ptrs.data(), static_cast<long>(ptrs.size()), type, values.data(), &ret,
                                 reinterpret_cast<bool *>(ok_buf.data()));
        }
        return ret;
    }
}

%include "../include/libop.h"
