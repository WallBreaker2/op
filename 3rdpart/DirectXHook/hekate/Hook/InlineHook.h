#pragma once

#include "HookBase.h"

#include <memory>
#include <mutex>
#include <Windows.h>

#ifdef _M_IX86
#include "Platform/InlinePlatformHook_x86.h"
#elif defined _M_AMD64
#include "Platform/InlinePlatformHook_x64.h"
#else
#error "Unsupported platform"
#endif

namespace Hekate
{
namespace Hook
{

class InlineHook final : public HookBase
{
public:
    InlineHook() = delete;

    InlineHook(InlineHook &&obj);
    InlineHook &operator=(InlineHook &&obj);

    InlineHook(const DWORD_PTR originalAddress, const DWORD_PTR hookAddress);
    ~InlineHook();

    const DWORD_PTR TrampolineAddress() const;

private:
    const bool InstallImpl() override;
    const bool RemoveImpl() override;

    DWORD_PTR m_trampolineAddress;

    size_t m_jumpOffset;

    std::unique_ptr<Platform::InlinePlatformHook> m_pPlatformHook;

    static std::mutex m_hookMutex;
};

}
}