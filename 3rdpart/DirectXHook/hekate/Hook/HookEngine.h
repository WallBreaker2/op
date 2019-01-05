#pragma once

#include "Hook/HookBase.h"

#include <memory>
#include <vector>

namespace Hekate
{
namespace Hook
{

class HookEngine final
{
public:
    HookEngine() = default;
    ~HookEngine() = default;

    const bool Add(const DWORD_PTR originalAddress, const DWORD_PTR hookAddress);

    const bool Remove(const DWORD_PTR originalAddress);

    const bool Exists(const DWORD_PTR originalAddress) const;
    const std::unique_ptr<HookBase> *Find(const DWORD_PTR originalAddress) const;

    const size_t Count() const;

private:
    
    std::vector<std::unique_ptr<HookBase>> m_vecHooks;
};

}
}