/**
 * SpeedMonitor - Real-time Network Speed Monitor for Windows 10/11
 * Author: Khalequzzaman
 * GitHub: https://github.com/khalequzzaman17/speedmonitor
 * License: MIT
 */

#pragma once

#include "types.hpp"
#include <string>

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();

    bool Load(AppSettings& settings);
    bool Save(const AppSettings& settings);

    std::wstring GetConfigDirectory() const;
    std::wstring GetConfigFilePath() const;

private:
    std::wstring m_configPath;
    void EnsureDirectoryExists(const std::wstring& path);
};
