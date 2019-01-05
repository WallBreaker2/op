#pragma once

#include "plog/Log.h"

namespace Hekate
{

#define BOOLIFY(x) !!(x)
#define PAGE_SIZE 0x1000

namespace Common
{

template <typename Function>
const bool ResolveImport(HMODULE module, Function &winsockFnc, const std::string name)
{
    winsockFnc = (Function)GetProcAddress(module, name.c_str());
    if (winsockFnc == nullptr)
    {
        LOG_ERROR << "Could not find function " << name.c_str()
            << " Error = 0x" << std::hex << GetLastError();
        return false;
    }

    LOG_DEBUG << "Found function " << name.c_str() << " at address 0x"
        << std::hex << winsockFnc;

    return true;
}

}
}