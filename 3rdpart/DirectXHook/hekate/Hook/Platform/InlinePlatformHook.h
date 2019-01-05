#pragma once

#include "Capstone/ScopedCapstoneHandle.h"
#include "Capstone/ScopedCapstoneInstructions.h"
#include "Common/SafeObject.h"

#include "capstone/include/capstone.h"

#include <memory>
#include <vector>

#include <Windows.h>

namespace Hekate
{
namespace Hook
{
namespace Platform
{

class InlinePlatformHook
{
public:
    InlinePlatformHook() = delete;
    InlinePlatformHook(const InlinePlatformHook &copy) = delete;
    InlinePlatformHook &operator=(const InlinePlatformHook &copy) = delete;

    InlinePlatformHook(const cs_arch architecture, const cs_mode mode, const size_t hookSizeBytes);
    virtual ~InlinePlatformHook() = default;

    const DWORD_PTR Hook(const DWORD_PTR originalAddress, const DWORD_PTR hookAddress, size_t &jumpOffset);
    const bool Unhook(const DWORD_PTR originalAddress, const DWORD_PTR trampolineAddress,
        const size_t jumpOffset);

private:
    virtual const bool HookImpl(const DWORD_PTR originalAddress, const DWORD_PTR hookAddress,
        const DWORD_PTR trampolineAddress, const size_t overwriteSize) = 0;
    virtual const bool UnhookImpl(const DWORD_PTR originalAddress,
        const DWORD_PTR jumpbackAddress, const size_t jumpOffset) = 0;

    const bool SuspendThreads() const;
    const bool ResumeThreads() const;
    std::vector<Common::SafeHandle> GetThreads() const;

    LPVOID AllocateTrampoline();
    void Disassemble(const DWORD_PTR originalAddress, Capstone::ScopedCapstoneInstructions &instructions);
    void CopyToTrampoline(const LPVOID trampolineAddrses, Capstone::ScopedCapstoneInstructions &instructions,
        size_t &bytesTotal) const;
    const DWORD SetPagePermissions(const DWORD_PTR address, const DWORD permissions) const;

    const bool CanHook() const;

    const cs_arch m_architecture;
    const cs_mode m_mode;
    const size_t m_hookSizeBytes;

    bool m_bCanHook;

};

}
}
}