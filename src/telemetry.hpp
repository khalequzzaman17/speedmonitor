/**
 * SpeedMonitor - Real-time Network Speed Monitor for Windows 10/11
 * Author: Khalequzzaman
 * GitHub: https://github.com/khalequzzaman17/speedmonitor
 * License: MIT
 */

#pragma once

#include "types.hpp"
#include <iphlpapi.h>
#include <mutex>
#include <memory>

class TelemetryEngine {
public:
    TelemetryEngine();
    ~TelemetryEngine();

    // Adapter discovery
    std::vector<NetworkAdapterItem> EnumerateAdapters();
    bool SetActiveAdapter(const std::wstring& adapterDescription);
    bool SetActiveAdapterIndex(NET_IFINDEX ifIndex);
    NET_IFINDEX GetActiveAdapterIndex() const;
    std::wstring GetActiveAdapterDescription() const;

    // Measurement Tick (Called on 1000ms periodic timer)
    SpeedSnapshot ProcessTick(BitrateUnit unit, int precision);

    // Session controls
    void ResetSessionCounters();
    uint64_t GetSessionReceived() const;
    uint64_t GetSessionSent() const;

private:
    mutable std::mutex m_mutex;
    NET_IFINDEX m_activeIfIndex = 0;
    std::wstring m_activeAdapterDesc;

    // 64-bit NDIS metrics
    uint64_t m_prevInOctets = 0;
    uint64_t m_prevOutOctets = 0;
    bool m_hasBaseline = false;

    // High resolution timer
    LARGE_INTEGER m_qpcFrequency{};
    LARGE_INTEGER m_lastQpcTime{};

    // Instantaneous and session metrics
    double m_lastDownloadBps = 0.0;
    double m_lastUploadBps = 0.0;
    uint64_t m_sessionReceivedBytes = 0;
    uint64_t m_sessionSentBytes = 0;

    void AutoDetectDefaultAdapter();
};
