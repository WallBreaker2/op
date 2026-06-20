#pragma once
#ifndef KIERO_HPP
#define KIERO_HPP

#ifndef KIERO_ASSERT
# include <cassert>
# define KIERO_ASSERT assert
#endif

// You can change this macro to your own logging system
// But this must support printf-like arguments
#ifndef KIERO_DBG_MSG
# ifdef _DEBUG
#  include <cstdio>
#  define KIERO_DBG_MSG(msg, ...) printf(msg "\n", ##__VA_ARGS__)
# else
#  define KIERO_DBG_MSG(msg, ...) (void)(0)
# endif
#endif

#define KIERO_UNUSED(x) (void)(x)

namespace kiero {

// You MUST specify similar block in your implementation this way:
// Implementation_<IMPLEMENTATION_SHORT_NAME> = KIERO_IMPL_FREE_SLOT
enum {
  Implementation_Nil,
};

using Error = int;

// You should specify similar block in your implementation this way:
// Error_<IMPLEMENTATION_SHORT_NAME>_<ERROR_NAME>
enum {
  Error_Nil,            // should be return if no error occured
  Error_Unknown,        // should be never return, but sometimes needed
  Error_ModuleNotFound, // should be return if specific module was not found
  Error_MethodNotFound, // should be return if specific method was not found
  Error_BaseIndex,      // should be used as first index for your errors
};

template<int Impl>
Error locate(void* in, void* out)
{
  KIERO_UNUSED(in);
  KIERO_UNUSED(out);

  static_assert(Impl == Implementation_Nil,
      "Did you forget about implementation?"
  );

  return Error_Nil;
}

} // namespace kiero

// In every implementation header
// you MUST do undef for KIERO_IMPL_CURR_SLOT
// and set this to your new implementation index this way:
//#undef KIERO_IMPL_CURR_SLOT
//#define KIERO_IMPL_CURR_SLOT kiero::Implementation_MyRender
#define KIERO_IMPL_CURR_SLOT kiero::Implementation_Nil
#define KIERO_IMPL_FREE_SLOT (KIERO_IMPL_CURR_SLOT + 1)

// You can use this for override first index
// This is necessary, for example, to save the index
// when building the static library (see CMakeLists.txt).
#ifdef KIERO_IMPL_FIRST_SLOT
# undef KIERO_IMPL_CURR_SLOT
# define KIERO_IMPL_CURR_SLOT KIERO_IMPL_FIRST_SLOT
#endif

#endif // KIERO_HPP
