#pragma once

namespace config
{

struct Config
{
    bool allowDive = true;       // can we perform a reload while doing a dive?
    bool allowAir = true;        // can we perform a reload while in the air (jump pack or falling)
    bool verboseLogging = false; //
};

// reads mods\flexible-stationary-reloads.ini
Config read();
} // namespace config
