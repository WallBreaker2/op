#include "OpContext.h"
#include "OpResult.h"

#include <libop.h>

void op::Op::SendPaste(LONG_PTR hwnd, long *nret) {
    internal::set_result(nret, m_context->window_service.SendPaste(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd))));
}

void op::Op::SetClipboard(const wchar_t *str, long *ret) {
    internal::set_result(ret, m_context->window_service.SetClipboard(str));
}

void op::Op::GetClipboard(std::wstring &ret) {
    m_context->window_service.GetClipboard(ret);
}
