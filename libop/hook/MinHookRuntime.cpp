#include "MinHookRuntime.h"

#include "MinHook.h"

#include <mutex>

namespace op::hook {
namespace {

std::mutex g_minHookMutex;
int g_minHookRefs = 0;

} // namespace

bool AcquireMinHook() {
    std::lock_guard<std::mutex> lock(g_minHookMutex);
    if (g_minHookRefs == 0) {
        const MH_STATUS status = MH_Initialize();
        if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED)
            return false;
    }
    ++g_minHookRefs;
    return true;
}

void ReleaseMinHook() {
    std::lock_guard<std::mutex> lock(g_minHookMutex);
    if (g_minHookRefs <= 0)
        return;

    --g_minHookRefs;
    if (g_minHookRefs == 0)
        MH_Uninitialize();
}

} // namespace op::hook
