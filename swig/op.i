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
%apply size_t *OUTPUT {size_t *}
%include "../libop/libop.h"

