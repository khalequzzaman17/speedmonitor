/**
 * SpeedMonitor - Real-time Network Speed Monitor for Windows 10/11
 * Author: Khalequzzaman
 * GitHub: https://github.com/khalequzzaman17/speedmonitor
 * License: MIT
 */

#include "telemetry.hpp"
#include <ws2tcpip.h>
#include <sstream>
#include <iomanip>

#ifdef _MSC_VER
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

TelemetryEngine::TelemetryEngine() {
    QueryPerformanceFrequency(&m_qpcFrequency);
    QueryPerformanceCounter(&m_lastQpcTime);
    AutoDetectDefaultAdapter();
}

TelemetryEngine::~TelemetryEngine() = default;

std::vector<NetworkAdapterItem> TelemetryEngine::EnumerateAdapters() {
    std::vector<NetworkAdapterItem> result;
    
    ULONG flags = GAA_FLAG_INCLUDE_ALL_INTERFACES | GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG outBufLen = 15000;
    std::vector<BYTE> buffer(outBufLen);
    
    PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
    DWORD dwRetVal = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, pAddresses, &outBufLen);
    
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(outBufLen);
        pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
        dwRetVal = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, pAddresses, &outBufLen);
    }
    
    if (dwRetVal == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES curr = pAddresses; curr != nullptr; curr = curr->Next) {
            // Filter loopback
            if (curr->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
            
            NetworkAdapterItem item;
            item.ifIndex = curr->IfIndex;
            item.adapterName = curr->AdapterName ? std::wstring(curr->AdapterName, curr->AdapterName + strlen(curr->AdapterName)) : L"";
            item.adapterDescription = curr->Description ? curr->Description : L"Network Adapter";
            item.isConnected = (curr->OperStatus == IfOperStatusUp);
            result.push_back(item);
        }
    }
    
    return result;
}

void TelemetryEngine::AutoDetectDefaultAdapter() {
    auto adapters = EnumerateAdapters();
    // Prefer first connected non-loopback adapter
    for (const auto& a : adapters) {
        if (a.isConnected) {
            m_activeIfIndex = a.ifIndex;
            m_activeAdapterDesc = a.adapterDescription;
            m_hasBaseline = false;
            return;
        }
    }
    // Fallback to first available
    if (!adapters.empty()) {
        m_activeIfIndex = adapters[0].ifIndex;
        m_activeAdapterDesc = adapters[0].adapterDescription;
        m_hasBaseline = false;
    }
}

bool TelemetryEngine::SetActiveAdapter(const std::wstring& adapterDescription) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (adapterDescription.empty()) {
        AutoDetectDefaultAdapter();
        return true;
    }
    
    auto adapters = EnumerateAdapters();
    for (const auto& a : adapters) {
        if (a.adapterDescription == adapterDescription) {
            m_activeIfIndex = a.ifIndex;
            m_activeAdapterDesc = a.adapterDescription;
            m_hasBaseline = false;
            return true;
        }
    }
    return false;
}

bool TelemetryEngine::SetActiveAdapterIndex(NET_IFINDEX ifIndex) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto adapters = EnumerateAdapters();
    for (const auto& a : adapters) {
        if (a.ifIndex == ifIndex) {
            m_activeIfIndex = a.ifIndex;
            m_activeAdapterDesc = a.adapterDescription;
            m_hasBaseline = false;
            return true;
        }
    }
    return false;
}

NET_IFINDEX TelemetryEngine::GetActiveAdapterIndex() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeIfIndex;
}

std::wstring TelemetryEngine::GetActiveAdapterDescription() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeAdapterDesc;
}

SpeedSnapshot TelemetryEngine::ProcessTick(BitrateUnit unit, int precision) {
    std::lock_guard<std::mutex> lock(m_mutex);
    SpeedSnapshot snapshot{};
    
    LARGE_INTEGER currentQpc;
    QueryPerformanceCounter(&currentQpc);
    double elapsedSec = 1.0;
    if (m_qpcFrequency.QuadPart > 0) {
        elapsedSec = static_cast<double>(currentQpc.QuadPart - m_lastQpcTime.QuadPart) / static_cast<double>(m_qpcFrequency.QuadPart);
    }
    m_lastQpcTime = currentQpc;
    if (elapsedSec <= 0.001) elapsedSec = 1.0; // Guard div by zero

    if (m_activeIfIndex == 0) {
        AutoDetectDefaultAdapter();
    }

    MIB_IF_ROW2 ifRow{};
    ifRow.InterfaceIndex = m_activeIfIndex;

    DWORD status = GetIfEntry2(&ifRow);
    if (status != NO_ERROR) {
        // Adapter might have changed index; attempt auto-recovery
        AutoDetectDefaultAdapter();
        ifRow.InterfaceIndex = m_activeIfIndex;
        status = GetIfEntry2(&ifRow);
    }

    if (status == NO_ERROR) {
        uint64_t currIn  = ifRow.InOctets;
        uint64_t currOut = ifRow.OutOctets;

        if (!m_hasBaseline) {
            m_prevInOctets = currIn;
            m_prevOutOctets = currOut;
            m_hasBaseline = true;
            m_lastDownloadBps = 0.0;
            m_lastUploadBps = 0.0;
        } else {
            // Rollover or reset clamp
            if (m_prevInOctets > currIn) {
                m_prevInOctets = currIn;
            }
            if (m_prevOutOctets > currOut) {
                m_prevOutOctets = currOut;
            }

            uint64_t deltaInBytes  = currIn - m_prevInOctets;
            uint64_t deltaOutBytes = currOut - m_prevOutOctets;

            m_prevInOctets = currIn;
            m_prevOutOctets = currOut;

            // Normalized bytes per second
            m_lastDownloadBps = static_cast<double>(deltaInBytes) / elapsedSec;
            m_lastUploadBps   = static_cast<double>(deltaOutBytes) / elapsedSec;

            m_sessionReceivedBytes += deltaInBytes;
            m_sessionSentBytes     += deltaOutBytes;
        }
    } else {
        m_lastDownloadBps = 0.0;
        m_lastUploadBps = 0.0;
    }

    snapshot.downloadBytesPerSec = m_lastDownloadBps;
    snapshot.uploadBytesPerSec   = m_lastUploadBps;
    snapshot.sessionBytesReceived = m_sessionReceivedBytes;
    snapshot.sessionBytesSent     = m_sessionSentBytes;

    UnitConfig uc = GetUnitConfig(unit);
    double scaledDown = m_lastDownloadBps * uc.multiplier;
    double scaledUp   = m_lastUploadBps   * uc.multiplier;

    std::wstringstream ssDown;
    ssDown << std::fixed << std::setprecision(precision) << scaledDown << L" " << uc.suffix;
    snapshot.formattedDownload = ssDown.str();

    std::wstringstream ssUp;
    ssUp << std::fixed << std::setprecision(precision) << scaledUp << L" " << uc.suffix;
    snapshot.formattedUpload = ssUp.str();

    return snapshot;
}

void TelemetryEngine::ResetSessionCounters() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessionReceivedBytes = 0;
    m_sessionSentBytes = 0;
}

uint64_t TelemetryEngine::GetSessionReceived() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_sessionReceivedBytes;
}

uint64_t TelemetryEngine::GetSessionSent() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_sessionSentBytes;
}
