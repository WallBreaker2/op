#pragma once

#include <Windows.h>

namespace Hekate
{
namespace Hook
{

enum class HookType
{
    eInlineHook = 0
};

class HookBase
{
public:
    HookBase() = delete;

    HookBase(HookBase &&obj);
    HookBase &operator=(HookBase &&obj);

    HookBase(const HookType &hookType, const DWORD_PTR originalAddress, const DWORD_PTR hookAddress);
    virtual ~HookBase() = default;

    const DWORD_PTR OriginalAddress() const;
    const DWORD_PTR HookAddress() const;
    const bool IsHooked() const;

    const bool Install();
    const bool Remove();

    const HookType Type() const;

    bool operator==(const HookBase &obj) const;
    bool operator!=(const HookBase &obj) const;

private:
    virtual const bool InstallImpl() = 0;
    virtual const bool RemoveImpl() = 0;

    HookType m_type;

protected:
    DWORD_PTR m_originalAddress;
    DWORD_PTR m_hookAddress;

    bool m_bIsHooked;
};

}
}