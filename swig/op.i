#define OP_API
%module pyop

%{
    #define OP_API
#include "../libop/libop.h"
%}

%include "std_string.i"
%include "std_wstring.i"
%include "typemaps.i"
%apply std::wstring &OUTPUT {std::wstring &}
%apply long *OUTPUT {long *}
%apply long long *OUTPUT {LONG_PTR *}
%apply size_t *OUTPUT {size_t *}

%extend libop {
    void RunApp(const wchar_t *cmdline, long mode, long *ret) {
        unsigned long pid = 0;
        $self->RunApp(cmdline, mode, &pid, ret);
    }
}

%include "../libop/libop.h"

