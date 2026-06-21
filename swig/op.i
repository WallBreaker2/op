#define OP_API
%module pyop

%{
    #define OP_API
#include "../include/libop.h"
%}

%include "std_string.i"
%include "std_wstring.i"
%include "typemaps.i"
%apply long long { LONG_PTR };
%apply std::wstring &OUTPUT {std::wstring &}
%apply long *OUTPUT {long *}
%apply long long *OUTPUT {LONG_PTR *}
%apply size_t *OUTPUT {size_t *}

%extend op::Client {
    void RunApp(const wchar_t *cmdline, long mode, long *ret) {
        unsigned long pid = 0;
        $self->RunApp(cmdline, mode, &pid, ret);
    }
}

%include "../include/libop.h"

