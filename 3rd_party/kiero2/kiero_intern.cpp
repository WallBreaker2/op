#include "kiero_intern.hpp"

#ifdef KIERO_ON_WINDOWS
void kiero::_::create_dummy_win32_window(DummyWin32Window* window)
{
  window->wc = {};
  window->wc.cbSize = sizeof(window->wc);
  window->wc.lpfnWndProc = DefWindowProcA;
  window->wc.hInstance = GetModuleHandleA(nullptr);
  window->wc.lpszClassName = "kiero_dummy_window";

  RegisterClassExA(&window->wc);

  window->hwnd = CreateWindowExA(
    0,
    window->wc.lpszClassName,
    "Kiero Dummy Window",
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    100,
    100,
    nullptr,
    nullptr,
    window->wc.hInstance,
    nullptr
  );
}

void kiero::_::destroy_dummy_win32_window(DummyWin32Window* window)
{
  DestroyWindow(window->hwnd);
  UnregisterClassA(window->wc.lpszClassName, window->wc.hInstance);
}
#endif // KIERO_ON_WINDOWS