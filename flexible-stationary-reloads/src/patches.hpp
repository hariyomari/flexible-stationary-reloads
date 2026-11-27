#pragma once

#include "addresses.hpp"
#include "config.hpp"
#include "globals.hpp"

#include <cstdint>
#include <iterator>

namespace patches
{
struct Result
{
    bool success;
    uint8_t count;
    uint8_t expected;
};

Result apply(const config::Config& config);
} // namespace patches
