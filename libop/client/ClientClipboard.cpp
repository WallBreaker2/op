#include "ClientContext.h"
#include "ClientResult.h"

#include <libop.h>

void op::Client::SendPaste(LONG_PTR hwnd, long *nret) {
    internal::set_result(nret, m_context->window_service.SendPaste(reinterpret_cast<HWND>(static_cast<LONG_PTR>(hwnd))));
}

void op::Client::SetClipboard(const wchar_t *str, long *ret) {
    internal::set_result(ret, m_context->window_service.SetClipboard(str));
}

void op::Client::GetClipboard(std::wstring &ret) {
    m_context->window_service.GetClipboard(ret);
}
