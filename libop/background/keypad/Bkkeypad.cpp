// #include "stdafx.h"
#include "Bkkeypad.h"
// #include "globalVar.h"
// #include "helpfunc.h"
//
// static uint oem_code(uint key){
//	short code[256] = { 0 };
//	code['q'] = 0x10; code['a'] = 0x1e;
//	code['w'] = 0x11; code['s'] = 0x1f;
//	code['e'] = 0x12; code['d'] = 0x20;
//	code['r'] = 0x13; code['f'] = 0x21;
//	code['t'] = 0x14; code['g'] = 0x22;
//	code['y'] = 0x15; code['h'] = 0x23;
//	code['u'] = 0x16; code['j'] = 0x24;
//	code['i'] = 0x17; code['k'] = 0x25;
//	code['o'] = 0x18; code['l'] = 0x26;
//	code['p'] = 0x19; code[':'] = 0x27; code[';'] = 0x27;
//
//	code['z'] = 0x2c;
//	code['x'] = 0x2d;
//	code['c'] = 0x2e;
//	code['v'] = 0x2f;
//	code['b'] = 0x30;
//	code['n'] = 0x31;
//	code['m'] = 0x32;
//	return code[key & 0xffu];
//
// }
//
bkkeypad::bkkeypad() : _hwnd(0), _mode(0) {
}
//
//
bkkeypad::~bkkeypad() {
    UnBind();
}
//
// long bkkeypad::Bind(HWND hwnd, long mode) {
//	if (!::IsWindow(hwnd))
//		return 0;
//	_hwnd = hwnd;
//	_mode = mode;
//	return 1;
//}
//
long bkkeypad::UnBind() {
    _hwnd = NULL;
    _mode = 0;
    return 1;
}
