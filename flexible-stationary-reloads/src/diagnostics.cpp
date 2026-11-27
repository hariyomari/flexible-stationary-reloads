#include "diagnostics.hpp"

#include "addresses.hpp"
#include "globals.hpp"

#include <cstdarg>
#include <cstdio>

using globals::hfApi;
using globals::hfGame;
using globals::hfMod;

namespace
{
using StatesChangedFn = char (*)(int64_t stateBlock, const uint64_t* changed, uint8_t added);
using RequestReloadFn = bool (*)(int64_t component);
using StartReloadFn = char (*)(int64_t component, uint32_t weapon, char a3);
using EndReloadFn = int64_t (*)(int64_t handler, char a2, int64_t a3);
using CanReloadFn = bool (*)(int64_t component, uint32_t weapon, char a3, char a4);

StatesChangedFn gStatesChangedOriginal = nullptr;
RequestReloadFn gRequestReloadOriginal = nullptr;
StartReloadFn gStartReloadOriginal = nullptr;
EndReloadFn gEndReloadOriginal = nullptr;
CanReloadFn gCanReloadOriginal = nullptr;

char statesChangedDetour(const int64_t stateBlock, const uint64_t* changed, const uint8_t added)
{
    for (int word = 0; word < 3; ++word)
        for (int bit = 0; bit < 64; ++bit)
            if (changed[word] & (1ULL << bit))
                hfApi->log(hfMod, "state %d %s", word * 64 + bit, added ? "ON" : "off");
    return gStatesChangedOriginal(stateBlock, changed, added);
}

bool requestReloadDetour(const int64_t component)
{
    const uintptr_t caller = reinterpret_cast<uintptr_t>(__builtin_return_address(0)) - hfGame;

    const bool result = gRequestReloadOriginal(component);

    hfApi->log(hfMod,
               "request reload, result=%d, called from game.dll+0x%llX",
               result,
               static_cast<unsigned long long>(caller));

    return result;
}

char startReloadDetour(const int64_t component, const uint32_t weapon, const char a3)
{
    const uintptr_t caller = reinterpret_cast<uintptr_t>(__builtin_return_address(0)) - hfGame;

    const char result = gStartReloadOriginal(component, weapon, a3);

    hfApi->log(hfMod,
               "start reload, weapon=%u -> %d, a3=%d, called from game.dll+0x%llX",
               weapon,
               result,
               a3,
               static_cast<unsigned long long>(caller));

    return result;
}

int64_t endReloadDetour(const int64_t handler, const char a2, const int64_t a3)
{
    const uintptr_t caller = reinterpret_cast<uintptr_t>(__builtin_return_address(0)) - hfGame;
    hfApi->log(hfMod,
               "end/cancel reload, a2=%d, called from game.dll+0x%llX",
               a2,
               static_cast<unsigned long long>(caller));

    if (a2 == 1 && a3 != 0)
    {
        const uint64_t* changed = reinterpret_cast<const uint64_t*>(a3);
        hfApi->log(hfMod,
                   "  changed states: %016llX %016llX %016llX",
                   static_cast<unsigned long long>(changed[0]),
                   static_cast<unsigned long long>(changed[1]),
                   static_cast<unsigned long long>(changed[2]));
    }

    return gEndReloadOriginal(handler, a2, a3);
}

bool canReloadDetour(const int64_t component, const uint32_t weapon, const char a3, const char a4)
{
    const bool result = gCanReloadOriginal(component, weapon, a3, a4);

    static int lastResult = -1;
    if (a3 == 0 && a4 == 0 && result != lastResult)
    {
        hfApi->log(hfMod, "can reload? weapon=%u -> %d", weapon, result);
        lastResult = result;
    }
    return result;
}
} // namespace

namespace diagnostics
{
void log(const LogLevel logLevel, const char* fmt, ...)
{
    char message[1024];
    {
        va_list args;
        va_start(args, fmt);
        vsnprintf(message, sizeof(message), fmt, args);
        va_end(args);
    }

    hfApi->log(hfMod, "%s", message);
}

void installHooks()
{
    if (hfApi->hook(hfMod,
                    hfGame + rva::kStatesChanged,
                    reinterpret_cast<void*>(statesChangedDetour),
                    reinterpret_cast<void**>(&gStatesChangedOriginal)) != 0)
        hfApi->log(hfMod, "states changed?: could not hook");

    if (hfApi->hook(hfMod,
                    hfGame + rva::kCanReload,
                    reinterpret_cast<void*>(canReloadDetour),
                    reinterpret_cast<void**>(&gCanReloadOriginal)) != 0)
        hfApi->log(hfMod, "can reload?: could not hook");

    if (hfApi->hook(hfMod,
                    hfGame + rva::kEndReload,
                    reinterpret_cast<void*>(endReloadDetour),
                    reinterpret_cast<void**>(&gEndReloadOriginal)) != 0)
        hfApi->log(hfMod, "end reload?: could not hook");

    if (hfApi->hook(hfMod,
                    hfGame + rva::kRequestReload,
                    reinterpret_cast<void*>(requestReloadDetour),
                    reinterpret_cast<void**>(&gRequestReloadOriginal)) != 0)
        hfApi->log(hfMod, "request reload?: could not hook");

    if (hfApi->hook(hfMod,
                    hfGame + rva::kStartReload,
                    reinterpret_cast<void*>(startReloadDetour),
                    reinterpret_cast<void**>(&gStartReloadOriginal)) != 0)
        hfApi->log(hfMod, "start reload?: could not hook");
}
} // namespace diagnostics
