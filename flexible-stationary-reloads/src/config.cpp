#include "config.hpp"

#include "diagnostics.hpp"
#include "globals.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

using diagnostics::LogLevel;
using globals::hfApi;
using globals::hfMod;

namespace fs = std::filesystem;

namespace
{
constexpr std::string_view kSettingsName = "flexible-stationary-reloads.ini";
constexpr std::string_view kTemplateName = "flexible-stationary-reloads.template.ini";

std::string_view trim(const std::string_view text)
{
    const std::size_t first = text.find_first_not_of(" \t\r");
    if (first == std::string_view::npos)
    {
        return {};
    }
    const std::size_t last = text.find_last_not_of(" \t\r");
    return text.substr(first, last - first + 1);
}

bool parseBool(const std::string_view value)
{
    return !(value == "off" || value == "no" || value == "false" || value == "0");
}

void applyLine(config::Config& config, std::string_view line)
{
    line = trim(line);

    // empty lines and comments
    if (line.empty() || line[0] == '#' || line[0] == ';')
        return;

    // look for the assignment
    const std::size_t equals = line.find("=");
    if (equals == std::string_view::npos)
        return;

    // key/value
    const std::string_view key = trim(line.substr(0, equals));
    const std::string_view value = trim(line.substr(equals + 1));

    if (key == "allow_air")
        config.allowAir = parseBool(value);
    else if (key == "allow_dive")
        config.allowDive = parseBool(value);
    else if (key == "verbose_logging")
        config.verboseLogging = parseBool(value);
    else
        diagnostics::log(LogLevel::Info,
                         "unknown setting: %.*s",
                         static_cast<int>(key.size()),
                         key.data());
}

fs::path getModFolder()
{
    const char* folder = hfApi->mod_folder(hfMod);
    if (!folder)
        return {};

    fs::path modFolder = fs::u8path(folder);
    if (!modFolder.has_filename())
        modFolder = modFolder.parent_path();
    return modFolder;
}

} // namespace

namespace config
{

Config read()
{
    Config config;

    const fs::path modFolder = getModFolder();
    if (modFolder.empty())
    {
        diagnostics::log(LogLevel::Info, "no mod folder; using the defaults");
        return config;
    }

    const fs::path settingsPath = modFolder.parent_path() / kSettingsName;
    const fs::path templatePath = modFolder / kTemplateName;
    diagnostics::log(LogLevel::Info, "settings file: %s", settingsPath.u8string().c_str());

    std::error_code ec;
    if (!fs::exists(settingsPath, ec))
    {
        fs::copy_file(templatePath, settingsPath, ec);
        if (ec)
        {
            diagnostics::log(
                LogLevel::Info,
                "could not create the settings file (the game is sandboxed?); using the "
                "defaults. to change them, copy %s to %s while the game is closed",
                templatePath.u8string().c_str(),
                settingsPath.u8string().c_str());
            return config;
        }
        diagnostics::log(LogLevel::Info, "created the settings file from the template");
    }

    std::ifstream file(settingsPath);
    if (!file.is_open())
    {
        diagnostics::log(LogLevel::Info, "couldn't open the settings file; using the defaults");
        return config;
    }

    std::string line;
    bool firstLine = true;
    while (std::getline(file, line))
    {
        // notepad can save the file with a "byte order mark" (3 invisible bytes)
        if (firstLine && line.rfind("\xEF\xBB\xBF", 0) == 0)
            line.erase(0, 3);
        firstLine = false;

        applyLine(config, line);
    }

    diagnostics::log(LogLevel::Info,
                     "settings: allow_air=%s allow_dive=%s verbose_logging=%s",
                     config.allowAir ? "on" : "off",
                     config.allowDive ? "on" : "off",
                     config.verboseLogging ? "on" : "off");
    return config;
}
} // namespace config
