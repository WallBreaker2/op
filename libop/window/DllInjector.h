#pragma once
#include "../runtime/Types.h"
namespace op {

class DllInjector {
  public:
    DllInjector();
    ~DllInjector();
    static BOOL EnablePrivilege(BOOL enable);
    //
    static long InjectDll(DWORD pid, LPCTSTR dllPath, long &error_code);
};

} // namespace op
