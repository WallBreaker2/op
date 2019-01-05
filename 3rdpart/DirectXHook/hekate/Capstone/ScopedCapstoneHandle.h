#pragma once

#include "capstone/include/capstone.h"

namespace Hekate
{
namespace Capstone
{

class ScopedCapstoneHandle final
{
public:
    ScopedCapstoneHandle() = delete;

    ScopedCapstoneHandle(const cs_arch architecture, const cs_mode mode);
    ~ScopedCapstoneHandle();

    const csh &Get() const;
    const csh &operator()() const;
    const bool IsValid() const;

private:
    csh m_handle;
    bool m_bIsValid;
};

}
}