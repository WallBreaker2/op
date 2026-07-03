#include "OpContext.h"
#include "OpResult.h"

#include "runtime/RuntimeEnvironment.h"
#include "runtime/RuntimeUtils.h"

#include <libop.h>
#include <Shlwapi.h>
#include <tchar.h>

std::wstring op::Op::Ver() {
    return _T(OP_VERSION);
}

void op::Op::SetPath(const wchar_t *path, long *ret) {
    std::wstring fpath = path;
    replacew(fpath, L"/", L"\\");
    if (fpath.find(L'\\') != std::wstring::npos && ::PathFileExistsW(fpath.data())) {
        m_context->curr_path = fpath;
        m_context->image_proc._curr_path = m_context->curr_path;
        m_context->bkproc._curr_path = m_context->curr_path;
        internal::set_result(ret, 1L);
    } else {

        if (!fpath.empty() && fpath[0] != L'\\')
            fpath = m_context->curr_path + L'\\' + fpath;
        else
            fpath = m_context->curr_path + fpath;
        if (::PathFileExistsW(fpath.data())) {
            m_context->curr_path = path;
            m_context->image_proc._curr_path = m_context->curr_path;
            m_context->bkproc._curr_path = m_context->curr_path;
            internal::set_result(ret, 1L);
        } else {
            setlog("path '%s' not exists", fpath.data());
            internal::set_result(ret, 0L);
        }
    }
}

void op::Op::GetPath(std::wstring &path) {
    path = m_context->curr_path;
}

void op::Op::GetBasePath(std::wstring &path) {
    path = RuntimeEnvironment::getBasePath();
}

void op::Op::GetID(long *ret) {
    internal::set_result(ret, m_context->id);
}

void op::Op::GetLastError(long *ret) {
    internal::set_result(ret, ::GetLastError());
}

void op::Op::SetShowErrorMsg(long show_type, long *ret) {
    RuntimeEnvironment::m_showErrorMsg = show_type;
    internal::set_result(ret, 1L);
}

void op::Op::Sleep(long millseconds, long *ret) {
    ::Sleep(millseconds);
    internal::set_result(ret, 1L);
}

void op::Op::Delay(long mis, long *ret) {
    internal::set_result(ret, ::Delay(mis));
}

void op::Op::Delays(long mis_min, long mis_max, long *ret) {
    internal::set_result(ret, ::Delays(mis_min, mis_max));
}
