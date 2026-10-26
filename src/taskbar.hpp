/**
 * SpeedMonitor - Real-time Network Speed Monitor for Windows 10/11
 * Author: Khalequzzaman
 * GitHub: https://github.com/khalequzzaman17/speedmonitor
 * License: MIT
 */

#pragma once

#include "types.hpp"
#include "telemetry.hpp"
#include "config.hpp"
#include <shellapi.h>

class TaskbarWindow {
public:
    TaskbarWindow(TelemetryEngine& telemetry, ConfigManager& config, AppSettings& settings);
    ~TaskbarWindow();

    bool Initialize(HINSTANCE hInstance);
    void RunMessageLoop();
    void UpdateDisplay();
    void ReAnchorToTaskbar();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime);

    LRESULT HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void OnPaint(HDC hdc);
    void ShowContextMenu(int x, int y);
    void ShowSettingsDialog();
    void ShowConnectionsDialog();

    TelemetryEngine& m_telemetry;
    ConfigManager& m_config;
    AppSettings& m_settings;

    HINSTANCE m_hInstance = nullptr;
    HWND m_hwnd = nullptr;
    HFONT m_hFont = nullptr;
    HFONT m_hFontBold = nullptr;
    HWINEVENTHOOK m_hEventHook = nullptr;
    UINT_PTR m_timerId = 1;
    NOTIFYICONDATAW m_nid{};

    SpeedSnapshot m_currentSnapshot{};
    bool m_isDragging = false;
    POINT m_dragStart{};

    void CreateGdiFonts();
    void DestroyGdiFonts();
    void SetupTrayIcon();
    void RemoveTrayIcon();
    void UpdateTrayTooltip(const SpeedSnapshot& snapshot);
};
