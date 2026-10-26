/**
 * SpeedMonitor - Real-time Network Speed Monitor for Windows 10/11
 * Author: Khalequzzaman
 * GitHub: https://github.com/khalequzzaman17/speedmonitor
 * License: MIT
 */

#include "types.hpp"
#include "telemetry.hpp"
#include "config.hpp"
#include "taskbar.hpp"

// Define DPI awareness constant if not provided by older headers
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#endif

typedef BOOL(WINAPI* PFN_SetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);

static void InitializeDpiAwareness() {
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        auto pfnSetDpi = reinterpret_cast<PFN_SetProcessDpiAwarenessContext>(
            reinterpret_cast<void*>(GetProcAddress(hUser32, "SetProcessDpiAwarenessContext"))
        );
        if (pfnSetDpi) {
            pfnSetDpi(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, PWSTR /*pCmdLine*/, int /*nCmdShow*/) {
    // 1. Single Instance Protection
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"Local\\SpeedMonitor_SingleInstanceMutex");
    if (hMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMutex);
        HWND existingHwnd = FindWindowW(L"SpeedMonitor_Class", nullptr);
        if (existingHwnd) {
            // Signal running instance or flash it
            SetForegroundWindow(existingHwnd);
        }
        return 0;
    }

    // 2. DPI Awareness for sharp crisp rendering on high-DPI displays
    InitializeDpiAwareness();

    // 3. Load Persistent Configuration
    ConfigManager config;
    AppSettings settings;
    config.Load(settings);

    // 4. Initialize Network Telemetry Engine
    TelemetryEngine telemetry;
    if (!settings.selectedAdapterDesc.empty()) {
        telemetry.SetActiveAdapter(settings.selectedAdapterDesc);
    }

    // 5. Initialize Taskbar Floating / Docked UI
    TaskbarWindow taskbar(telemetry, config, settings);
    if (!taskbar.Initialize(hInstance)) {
        if (hMutex) {
            CloseHandle(hMutex);
        }
        return 1;
    }

    // 6. Run Native Windows Message Loop
    taskbar.RunMessageLoop();

    // 7. Save Settings on Clean Exit
    config.Save(settings);

    if (hMutex) {
        CloseHandle(hMutex);
    }

    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR /*lpCmdLine*/, int nCmdShow) {
    return wWinMain(hInstance, hPrevInstance, GetCommandLineW(), nCmdShow);
}

