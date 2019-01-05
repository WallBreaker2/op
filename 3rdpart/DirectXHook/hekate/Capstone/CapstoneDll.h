#pragma once

#include <string>

#include "capstone/include/capstone.h"

namespace Hekate
{
namespace Capstone
{

using cs_openFnc = cs_err(__cdecl *)(cs_arch arch, cs_mode mode, csh *handle);
using cs_disasmFnc = size_t(__cdecl *)(csh handle, const uint8_t *code, size_t code_size,
    uint64_t address, size_t count, cs_insn **insn);
using cs_freeFnc = void(__cdecl *)(cs_insn *insn, size_t count);
using cs_closeFnc = cs_err(__cdecl *)(csh *handle);

extern cs_openFnc cs_open;
extern cs_disasmFnc cs_disasm;
extern cs_freeFnc cs_free;
extern cs_closeFnc cs_close;

void ResolveCapstoneImports();
void CapstoneImportError(const std::string strError);

}
}