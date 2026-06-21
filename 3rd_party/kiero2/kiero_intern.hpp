// Should be included only in implementation files
// After including other headers

#pragma once
#ifndef KIERO_INTERN_HPP
#define KIERO_INTERN_HPP

#if defined(WIN32) || defined(WIN64)
# define KIERO_ON_WINDOWS
# include <windows.h>
#elif defined(__linux__)
# define KIERO_ON_LINUX
#elif defined(__APPLE__)
# define KIERO_ON_MACOS
#endif

#define KIERO_TOKEN_CONCAT(a, b) a##b
#define KIERO_TOKEN_CONCAT2(a, b) KIERO_TOKEN_CONCAT(a, b)


namespace kiero {
namespace _ {

template<typename F>
struct Defer {
  F f;
  Defer(F f) : f(f) {}
  Defer(const Defer&) = delete;
  ~Defer() { f(); }
};

} // namespace _
} // namespace kiero

#define KIERO_DEFER(x) kiero::_::Defer KIERO_TOKEN_CONCAT2(_defer_, __LINE__) = (x);


namespace kiero {
namespace _ {
#ifdef KIERO_ON_WINDOWS

struct DummyWin32Window {
  WNDCLASSEXA wc;
  HWND hwnd;
};

void create_dummy_win32_window(DummyWin32Window* window);
void destroy_dummy_win32_window(DummyWin32Window* window);

#endif // KIERO_ON_WINDOWS
} // namespace _
} // namespace kiero

#endif // KIERO_INTERN_HPP