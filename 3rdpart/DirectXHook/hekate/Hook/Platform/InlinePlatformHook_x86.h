#pragma once

#ifdef _M_IX86

#include "Hook/Platform/InlinePlatformHook.h"

namespace Hekate
{
namespace Hook
{
namespace Platform
{

class InlinePlatformHook_x86 final : public InlinePlatformHook
{
public:
    InlinePlatformHook_x86();
    ~InlinePlatformHook_x86() = default;

    const bool HookImpl(const DWORD_PTR originalAddress, const DWORD_PTR hookAddress,
        const DWORD_PTR trampolineAddress, const size_t overwriteSize) override;

    const bool UnhookImpl(const DWORD_PTR originalAddress, const DWORD_PTR trampolineAddress,
        const size_t jumpOffset) override;

private:
    static const int HOOK_SIZE = 6;
};

}
}
}

#endif
