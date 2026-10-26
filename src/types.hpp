/**
 * SpeedMonitor - Real-time Network Speed Monitor for Windows 10/11
 * Author: Khalequzzaman
 * GitHub: https://github.com/khalequzzaman17/speedmonitor
 * License: MIT
 */

#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef WINVER
#define WINVER 0x0A00
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>

#include <string>
#include <vector>
#include <cstdint>

enum class BitrateUnit : int {
    KbitPerSec   = 0, // kbit/s (decimal bits / 1000)
    KibitPerSec  = 1, // Kibit/s (binary bits / 1024)
    KBPerSec     = 2, // kB/s (decimal bytes / 1000)
    KiBPerSec    = 3, // KiB/s (binary bytes / 1024) - Classic NetSpeedMonitor default
    MbitPerSec   = 4, // Mbit/s (decimal megabits / 10^6)
    MibitPerSec  = 5, // Mibit/s (binary mebibits / 2^20)
    MBPerSec     = 6, // MB/s (decimal megabytes / 10^6)
    MiBPerSec    = 7, // MiB/s (binary mebibytes / 2^20)
    MBPerMin     = 8  // MB/min (megabytes per minute)
};

struct UnitConfig {
    double multiplier;
    std::wstring suffix;
};

inline UnitConfig GetUnitConfig(BitrateUnit unit) {
    switch (unit) {
        case BitrateUnit::KbitPerSec:   return { 0.008,                L"kbit/s" };
        case BitrateUnit::KibitPerSec:  return { 0.0078125,            L"Kibit/s" };
        case BitrateUnit::KBPerSec:     return { 0.001,                L"kB/s" };
        case BitrateUnit::KiBPerSec:    return { 0.0009765625,         L"KiB/s" };
        case BitrateUnit::MbitPerSec:   return { 8.0e-6,               L"Mbit/s" };
        case BitrateUnit::MibitPerSec:  return { 7.62939453125e-6,     L"Mibit/s" };
        case BitrateUnit::MBPerSec:     return { 1.0e-6,               L"MB/s" };
        case BitrateUnit::MiBPerSec:    return { 9.5367431640625e-7,   L"MiB/s" };
        case BitrateUnit::MBPerMin:     return { 6.0e-5,               L"MB/min" };
        default:                        return { 0.0009765625,         L"KiB/s" };
    }
}

struct NetworkAdapterItem {
    NET_IFINDEX ifIndex;
    std::wstring adapterName;        // e.g. GUID
    std::wstring adapterDescription; // Friendly description e.g. "Intel Wi-Fi 6 AX201"
    bool isConnected;
};

struct SpeedSnapshot {
    double downloadBytesPerSec;
    double uploadBytesPerSec;
    uint64_t sessionBytesReceived;
    uint64_t sessionBytesSent;
    std::wstring formattedDownload;
    std::wstring formattedUpload;
};

struct AppSettings {
    std::wstring selectedAdapterDesc = L""; // Empty = automatic default active adapter
    NET_IFINDEX selectedIfIndex = 0;
    BitrateUnit bitrateUnit = BitrateUnit::KiBPerSec;
    int precision = 2; // Decimals
    bool monitoringEnabled = true;
    
    // Layout
    std::wstring fontFamily = L"Segoe UI";
    int fontSizePt = 8;
    bool fontBold = false;
    COLORREF textColor = RGB(255, 255, 255);
    int glowSize = 2;
    std::wstring prefixDownload = L"D: ";
    std::wstring prefixUpload   = L"U: ";
    bool swapOrder = false; // false = Down on top, true = Up on top
    int horizontalMarginPx = 8;
    int customWidthPx = 86;
    int customHeightPx = 40;
    
    // Positioning
    bool dockToTaskbar = true;
    int windowPosX = 0;
    int windowPosY = 0;
};
