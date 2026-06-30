#pragma once

#include "../runtime/AutomationModes.h"
#include "../runtime/RuntimeEnvironment.h"

#include <string>

namespace op::hook {

inline std::wstring ResolveHookModuleName(bool target_is64) {
    const std::wstring current = RuntimeEnvironment::getOpName();
    if (target_is64 == (OP64 != 0) && !current.empty()) {
        return current;
    }

    if (current.find(L"op_c_api") != std::wstring::npos) {
        return target_is64 ? L"op_c_api_x64.dll" : L"op_c_api_x86.dll";
    }
    return target_is64 ? L"op_x64.dll" : L"op_x86.dll";
}

} // namespace op::hook
