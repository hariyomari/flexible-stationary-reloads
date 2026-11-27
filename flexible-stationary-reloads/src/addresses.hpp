#pragma once

#include <cstdint>

namespace rva
{
// patches
constexpr uintptr_t kBlockSlot1 = 0x775800;   // blocking list entry: mov [rsp+68h], 66
constexpr uintptr_t kBlockSlot2 = 0x775812;   // mov [rsp+70h], 98
constexpr uintptr_t kBlockSlot3 = 0x775820;   // mov [rsp+78h], 101
constexpr uintptr_t kBlockSlot4 = 0x775829;   // mov [rsp+80h], 97 (12 bytes)
constexpr uintptr_t kBlockCheck = 0x7758B2;   // jnz: "has a blocking state"
constexpr uintptr_t kMovementFlag = 0x7758BC; // jnz: movement record +0x0F
constexpr uintptr_t kState69Check = 0xA7C364; // cmp + jnz short in the reload button handler
constexpr uintptr_t kCancelRule = 0x22510E0;  // stationary cancel rule's key state (data)

// diagnostic hooks
constexpr uintptr_t kCanReload = 0x775580;
constexpr uintptr_t kStartReload = 0x774B60;
constexpr uintptr_t kRequestReload = 0xA7C870;
constexpr uintptr_t kEndReload = 0xA7CCC0;
constexpr uintptr_t kStatesChanged = 0xA3F220;
constexpr uintptr_t kShouldInterrupt = 0xA7BEA0;
} // namespace rva
