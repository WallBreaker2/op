#pragma once

#include "capstone/include/capstone.h"

namespace Hekate
{
namespace Capstone
{

class ScopedCapstoneInstructions
{
public:
    ScopedCapstoneInstructions();
    ~ScopedCapstoneInstructions();

    const size_t Count() const;
    const cs_insn * const Instructions() const;

    size_t * const CountPtr();
    cs_insn **InstructionsPtr();

    const bool IsValid() const;

private:
    cs_insn *m_pInstructions;
    size_t m_count;
};

}
}