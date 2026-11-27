#include "patches.hpp"

#include "addresses.hpp"
#include "config.hpp"
#include "diagnostics.hpp"
#include "globals.hpp"

#include <cstdint>
#include <cstring>
#include <iterator>
#include <vector>

using diagnostics::LogLevel;
using globals::hfApi;
using globals::hfGame;
using globals::hfMod;

namespace
{

enum class When : uint8_t
{
    AnyAllowed,
    AllowAir,
    NothingBlocked,
};

struct Patch
{
    const char* name;
    uintptr_t rva;
    uint8_t original[9];
    uint8_t replacement[9];
    size_t size;
    When when;
};

struct StateSlot
{
    uintptr_t rva;
    uint8_t original[12];
    size_t size;
    size_t valueOffset;
};

constexpr Patch kPatches[] = {
    {
        .name = "movement flag check",
        .rva = rva::kMovementFlag,
        .original = {0x0F, 0x85, 0x5B, 0xFD, 0xFF, 0xFF},
        .replacement = {0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00},
        .size = 6,
        .when = When::AllowAir,
    },
    {
        .name = "stationary reload state 69 check",
        .rva = rva::kState69Check,
        .original = {0x44, 0x38, 0x68, 0x01, 0x75, 0x60},
        .replacement = {0x44, 0x38, 0x68, 0x01, 0xEB, 0x60},
        .size = 6,
        .when = When::AnyAllowed,
    },
    {
        .name = "blocking states check",
        .rva = rva::kBlockCheck,
        .original = {0x0F, 0x85, 0x65, 0xFD, 0xFF, 0xFF},
        .replacement = {0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00},
        .size = 6,
        .when = When::NothingBlocked,
    },
    {
        .name = "stationary reload cancel rule",
        .rva = rva::kCancelRule,
        .original = {0x14, 0x00, 0x00, 0x00},
        .replacement = {0x00, 0x00, 0x00, 0x00},
        .size = 4,
        .when = When::AnyAllowed,
    },
};

constexpr StateSlot kSlots[] = {
    {
        .rva = rva::kBlockSlot1,
        .original = {0x48, 0xC7, 0x44, 0x24, 0x68, 0x42, 0x00, 0x00, 0x00},
        .size = 9,
        .valueOffset = 5,
    },
    {
        .rva = rva::kBlockSlot2,
        .original = {0x48, 0xC7, 0x44, 0x24, 0x70, 0x62, 0x00, 0x00, 0x00},
        .size = 9,
        .valueOffset = 5,
    },
    {
        .rva = rva::kBlockSlot3,
        .original = {0x48, 0xC7, 0x44, 0x24, 0x78, 0x65, 0x00, 0x00, 0x00},
        .size = 9,
        .valueOffset = 5,
    },
    {
        .rva = rva::kBlockSlot4,
        .original = {0x48, 0xC7, 0x84, 0x24, 0x80, 0x00, 0x00, 0x00, 0x61, 0x00, 0x00, 0x00},
        .size = 12,
        .valueOffset = 8,
    },
};

bool isWanted(const When when, const config::Config& config, const bool nothingBlocked)
{
    switch (when)
    {
        case When::AnyAllowed:
            return config.allowAir || config.allowDive;
        case When::AllowAir:
            return config.allowAir;
        case When::NothingBlocked:
            return nothingBlocked;
    }
    return false;
}
} // namespace

namespace patches
{
Result apply(const config::Config& config)
{
    auto r = Result{.success = false, .count = 0, .expected = 0};

    std::vector<uint32_t> blocked;
    if (!config.allowAir)
    {
        blocked.push_back(98); // jump pack thrusting
        blocked.push_back(97); // jump pack, in the air
    }
    if (!config.allowDive)
    {
        blocked.push_back(101); // diving
    }
    if (!config.allowAir && !config.allowDive)
        blocked.push_back(66); // airborne
    const bool nothingBlocked = blocked.empty();

    // verify the wanted patches
    for (const Patch& patch : kPatches)
    {
        if (!isWanted(patch.when, config, nothingBlocked))
            continue;

        uint8_t current[9] = {};
        if (hfApi->read(hfGame + patch.rva, current, patch.size) != 0 ||
            std::memcmp(current, patch.original, patch.size) != 0)
        {
            diagnostics::log(LogLevel::Info,
                             "%s: the bytes are not the expected ones (another game build?)",
                             patch.name);
            return r;
        }
    }

    // verify all of the slots
    for (const StateSlot& slot : kSlots)
    {
        uint8_t current[12] = {};
        if (hfApi->read(hfGame + slot.rva, current, slot.size) != 0 ||
            std::memcmp(current, slot.original, slot.size) != 0)
        {
            diagnostics::log(LogLevel::Info,
                             "0x%llX: the bytes are not the expected ones (another game build?)",
                             static_cast<unsigned long long>(slot.rva));
            return r;
        }
    }

    // write the slots if something is blocked
    if (!nothingBlocked)
    {
        for (size_t i = 0; i < std::size(kSlots); ++i)
        {
            // slot i gets blocked[i]; if the list is shorter than 4, repeat the first state
            const uint32_t state = (i < blocked.size()) ? blocked[i] : blocked[0];
            const StateSlot& slot = kSlots[i];

            r.expected++;
            if (hfApi->write(hfGame + slot.rva + slot.valueOffset, &state, sizeof(state)) != 0)
                diagnostics::log(LogLevel::Info,
                                 "slot 0x%llX: could not be written",
                                 static_cast<unsigned long long>(slot.rva));
            else
                r.count++;
        }
    }

    // write the wanted patches
    for (const Patch& patch : kPatches)
    {
        if (!isWanted(patch.when, config, nothingBlocked))
        {
            diagnostics::log(LogLevel::Info, "%s: skipped", patch.name);
            continue;
        }

        r.expected++;
        if (hfApi->write(hfGame + patch.rva, patch.replacement, patch.size) != 0)
            diagnostics::log(LogLevel::Info, "%s: could not be written", patch.name);
        else
        {
            diagnostics::log(LogLevel::Info, "%s: patched", patch.name);
            r.count++;
        }
    }

    for (const uint32_t state : blocked)
        diagnostics::log(LogLevel::Info, "blocked state: %u", state);

    r.success = (r.count == r.expected);
    return r;
}
} // namespace patches
