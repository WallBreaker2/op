#define OP_API
%module opEx

%{
    #define OP_API
#include "../libop/libop.h"
%}

%include "std_string.i"
%include "std_wstring.i"
%include "typemaps.i"
%apply std::wstring &OUTPUT {std::wstring &}
%apply long *OUTPUT {long *}
%include "../libop/libop.h"

