#define OP_API
%module opEx
%{
    #define OP_API
#include "../libop/libop.h"
%}
%include "std_string.i"
%include "std_wstring.i"
%include "../libop/libop.h"

