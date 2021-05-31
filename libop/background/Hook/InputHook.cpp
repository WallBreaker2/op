
#include "InputHook.h"
/*target window hwnd*/
HWND InputHook::input_hwnd;
int InputHook::input_type;
/*name of ...*/
wchar_t InputHook::shared_res_name[256];
wchar_t InputHook::mutex_name[256];
void *InputHook::old_address;
//

int InputHook::x;
int InputHook::y;


int InputHook::setup(HWND hwnd_, int input_type_){
    return 0;
}
int InputHook::release(){
    return 0;
}

long __stdcall SetInputHook(HWND hwnd_, int input_type_){
    return 0;
}

long __stdcall ReleaseInputHook(){
    return 0;
}
