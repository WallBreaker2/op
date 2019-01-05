#pragma once

#ifdef _M_AMD64

#include "Hook/Platform/InlinePlatformHook.h"

#include <array>

#include <Windows.h>

namespace Hekate
{
namespace Hook
{
namespace Platform
{

class InlinePlatformHook_x64 final : public InlinePlatformHook
{
public:
    InlinePlatformHook_x64();
    ~InlinePlatformHook_x64() = default;

    const bool HookImpl(const DWORD_PTR originalAddress, const DWORD_PTR hookAddress,
		const DWORD_PTR jumpbackAddress, const size_t overwriteSize) override;
    const bool UnhookImpl(const DWORD_PTR originalAddress, const DWORD_PTR trampolineAddress,
        const size_t jumpOffset) override;

private:
    static const int HOOK_SIZE = 16;
};

}
}
}
#endif
