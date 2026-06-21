#pragma once

#include <Windows.h>
#include <gl\gl.h>

namespace op::hook {

void __stdcall gl_hkglBegin(GLenum mode);
void __stdcall gl_hkwglSwapBuffers(HDC hdc);
unsigned int __stdcall gl_hkeglSwapBuffers(void *dpy, void *surface);
void __stdcall gl_hkglFinish(void);

} // namespace op::hook
