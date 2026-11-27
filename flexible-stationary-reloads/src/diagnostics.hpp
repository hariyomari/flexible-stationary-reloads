#pragma once

#include <cstdint>

namespace diagnostics
{
enum class LogLevel : uint8_t
{
    Info,
    Debug
};

void log(LogLevel logLevel, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
void installHooks();
} // namespace diagnostics
