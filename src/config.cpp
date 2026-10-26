/**
 * SpeedMonitor - Real-time Network Speed Monitor for Windows 10/11
 * Author: Khalequzzaman
 * GitHub: https://github.com/khalequzzaman17/speedmonitor
 * License: MIT
 */

#include "config.hpp"
#include <shlobj.h>
#include <sstream>

ConfigManager::ConfigManager() {
    WCHAR appData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appData))) {
        std::wstring dir = std::wstring(appData) + L"\\SpeedMonitor";
        EnsureDirectoryExists(dir);
        m_configPath = dir + L"\\config.ini";
    } else {
        m_configPath = L"config.ini";
    }
}

ConfigManager::~ConfigManager() = default;

std::wstring ConfigManager::GetConfigDirectory() const {
    size_t lastSlash = m_configPath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        return m_configPath.substr(0, lastSlash);
    }
    return L".";
}

std::wstring ConfigManager::GetConfigFilePath() const {
    return m_configPath;
}

void ConfigManager::EnsureDirectoryExists(const std::wstring& path) {
    CreateDirectoryW(path.c_str(), nullptr);
}

bool ConfigManager::Load(AppSettings& settings) {
    const WCHAR* path = m_configPath.c_str();

    WCHAR buf[512];
    GetPrivateProfileStringW(L"Core", L"SelectedAdapter", L"", buf, 512, path);
    settings.selectedAdapterDesc = buf;

    settings.bitrateUnit = static_cast<BitrateUnit>(GetPrivateProfileIntW(L"Core", L"BitrateUnit", static_cast<int>(BitrateUnit::KiBPerSec), path));
    settings.precision = GetPrivateProfileIntW(L"Core", L"Precision", 2, path);
    settings.monitoringEnabled = (GetPrivateProfileIntW(L"Core", L"MonitoringEnabled", 1, path) != 0);

    GetPrivateProfileStringW(L"Layout", L"FontFamily", L"Segoe UI", buf, 512, path);
    settings.fontFamily = buf;

    settings.fontSizePt = GetPrivateProfileIntW(L"Layout", L"FontSizePt", 8, path);
    settings.fontBold = (GetPrivateProfileIntW(L"Layout", L"FontBold", 0, path) != 0);
    settings.textColor = static_cast<COLORREF>(GetPrivateProfileIntW(L"Layout", L"TextColor", RGB(255, 255, 255), path));
    settings.glowSize = GetPrivateProfileIntW(L"Layout", L"GlowSize", 2, path);

    GetPrivateProfileStringW(L"Layout", L"PrefixDownload", L"D: ", buf, 512, path);
    settings.prefixDownload = buf;

    GetPrivateProfileStringW(L"Layout", L"PrefixUpload", L"U: ", buf, 512, path);
    settings.prefixUpload = buf;

    settings.swapOrder = (GetPrivateProfileIntW(L"Layout", L"SwapOrder", 0, path) != 0);
    settings.horizontalMarginPx = GetPrivateProfileIntW(L"Layout", L"HorizontalMarginPx", 8, path);
    settings.customWidthPx = GetPrivateProfileIntW(L"Layout", L"CustomWidthPx", 86, path);
    settings.customHeightPx = GetPrivateProfileIntW(L"Layout", L"CustomHeightPx", 40, path);

    settings.dockToTaskbar = (GetPrivateProfileIntW(L"Position", L"DockToTaskbar", 1, path) != 0);
    settings.windowPosX = GetPrivateProfileIntW(L"Position", L"PosX", 100, path);
    settings.windowPosY = GetPrivateProfileIntW(L"Position", L"PosY", 100, path);

    return true;
}

bool ConfigManager::Save(const AppSettings& settings) {
    const WCHAR* path = m_configPath.c_str();

    WritePrivateProfileStringW(L"Core", L"SelectedAdapter", settings.selectedAdapterDesc.c_str(), path);
    WritePrivateProfileStringW(L"Core", L"BitrateUnit", std::to_wstring(static_cast<int>(settings.bitrateUnit)).c_str(), path);
    WritePrivateProfileStringW(L"Core", L"Precision", std::to_wstring(settings.precision).c_str(), path);
    WritePrivateProfileStringW(L"Core", L"MonitoringEnabled", settings.monitoringEnabled ? L"1" : L"0", path);

    WritePrivateProfileStringW(L"Layout", L"FontFamily", settings.fontFamily.c_str(), path);
    WritePrivateProfileStringW(L"Layout", L"FontSizePt", std::to_wstring(settings.fontSizePt).c_str(), path);
    WritePrivateProfileStringW(L"Layout", L"FontBold", settings.fontBold ? L"1" : L"0", path);
    WritePrivateProfileStringW(L"Layout", L"TextColor", std::to_wstring(settings.textColor).c_str(), path);
    WritePrivateProfileStringW(L"Layout", L"GlowSize", std::to_wstring(settings.glowSize).c_str(), path);
    WritePrivateProfileStringW(L"Layout", L"PrefixDownload", settings.prefixDownload.c_str(), path);
    WritePrivateProfileStringW(L"Layout", L"PrefixUpload", settings.prefixUpload.c_str(), path);
    WritePrivateProfileStringW(L"Layout", L"SwapOrder", settings.swapOrder ? L"1" : L"0", path);
    WritePrivateProfileStringW(L"Layout", L"HorizontalMarginPx", std::to_wstring(settings.horizontalMarginPx).c_str(), path);
    WritePrivateProfileStringW(L"Layout", L"CustomWidthPx", std::to_wstring(settings.customWidthPx).c_str(), path);
    WritePrivateProfileStringW(L"Layout", L"CustomHeightPx", std::to_wstring(settings.customHeightPx).c_str(), path);

    WritePrivateProfileStringW(L"Position", L"DockToTaskbar", settings.dockToTaskbar ? L"1" : L"0", path);
    WritePrivateProfileStringW(L"Position", L"PosX", std::to_wstring(settings.windowPosX).c_str(), path);
    WritePrivateProfileStringW(L"Position", L"PosY", std::to_wstring(settings.windowPosY).c_str(), path);

    return true;
}
