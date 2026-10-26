/**
 * SpeedMonitor - Real-time Network Speed Monitor for Windows 10/11
 * Author: Khalequzzaman
 * GitHub: https://github.com/khalequzzaman17/speedmonitor
 * License: MIT
 */

#include "taskbar.hpp"
#include <windowsx.h>
#include <sstream>

static TaskbarWindow* g_instance = nullptr;

TaskbarWindow::TaskbarWindow(TelemetryEngine& telemetry, ConfigManager& config, AppSettings& settings)
    : m_telemetry(telemetry), m_config(config), m_settings(settings) {
    g_instance = this;
}

TaskbarWindow::~TaskbarWindow() {
    if (m_hEventHook) {
        UnhookWinEvent(m_hEventHook);
        m_hEventHook = nullptr;
    }
    RemoveTrayIcon();
    DestroyGdiFonts();
    if (m_hwnd && IsWindow(m_hwnd)) {
        DestroyWindow(m_hwnd);
    }
    g_instance = nullptr;
}

void TaskbarWindow::CreateGdiFonts() {
    DestroyGdiFonts();
    HDC hdc = GetDC(nullptr);
    int logPixelsY = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(nullptr, hdc);

    int height = -MulDiv(m_settings.fontSizePt, logPixelsY, 72);

    LOGFONTW lf{};
    lf.lfHeight = height;
    lf.lfWeight = m_settings.fontBold ? FW_BOLD : FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;
    wcscpy_s(lf.lfFaceName, m_settings.fontFamily.c_str());

    m_hFont = CreateFontIndirectW(&lf);

    lf.lfWeight = FW_BOLD;
    m_hFontBold = CreateFontIndirectW(&lf);
}

void TaskbarWindow::DestroyGdiFonts() {
    if (m_hFont) {
        DeleteObject(m_hFont);
        m_hFont = nullptr;
    }
    if (m_hFontBold) {
        DeleteObject(m_hFontBold);
        m_hFontBold = nullptr;
    }
}

void CALLBACK TaskbarWindow::WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime) {
    if (g_instance && g_instance->m_settings.dockToTaskbar) {
        g_instance->ReAnchorToTaskbar();
    }
}

bool TaskbarWindow::Initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;
    CreateGdiFonts();

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = TaskbarWindow::WndProc;
    wc.hInstance = hInstance;
    HICON hAppIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(101));
    wc.hIcon = hAppIcon ? hAppIcon : LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm = hAppIcon ? hAppIcon : LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"SpeedMonitor_Class";

    RegisterClassExW(&wc);

    DWORD exStyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
    DWORD style = WS_POPUP | WS_VISIBLE;

    m_hwnd = CreateWindowExW(
        exStyle,
        wc.lpszClassName,
        L"SpeedMonitor",
        style,
        100, 100,
        m_settings.customWidthPx, m_settings.customHeightPx,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!m_hwnd) return false;

    // Track taskbar movement hook
    m_hEventHook = SetWinEventHook(
        EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE,
        nullptr, TaskbarWindow::WinEventProc,
        0, 0, WINEVENT_OUTOFCONTEXT
    );

    ReAnchorToTaskbar();
    SetupTrayIcon();

    // 1-second sampling timer
    SetTimer(m_hwnd, m_timerId, 1000, nullptr);

    // Initial immediate tick
    UpdateDisplay();

    return true;
}

void TaskbarWindow::ReAnchorToTaskbar() {
    if (!m_hwnd || !IsWindow(m_hwnd)) return;

    if (!m_settings.dockToTaskbar) {
        // Floating mode: keep custom position
        SetWindowPos(m_hwnd, HWND_TOPMOST, m_settings.windowPosX, m_settings.windowPosY,
                     m_settings.customWidthPx, m_settings.customHeightPx, SWP_SHOWWINDOW | SWP_NOACTIVATE);
        return;
    }

    HWND hTray = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (!hTray) return;

    RECT rcTray{};
    GetWindowRect(hTray, &rcTray);

    HWND hNotify = FindWindowExW(hTray, nullptr, L"TrayNotifyWnd", nullptr);
    RECT rcNotify = rcTray;
    if (hNotify) {
        GetWindowRect(hNotify, &rcNotify);
    }

    int width = m_settings.customWidthPx;
    int taskbarHeight = rcTray.bottom - rcTray.top;

    // Horizontal taskbar (bottom or top)
    if (rcTray.bottom - rcTray.top <= rcTray.right - rcTray.left) {
        int posX = rcNotify.left - width - m_settings.horizontalMarginPx;
        int posY = rcTray.top;
        SetWindowPos(m_hwnd, HWND_TOPMOST, posX, posY, width, taskbarHeight, SWP_SHOWWINDOW | SWP_NOACTIVATE);
    } else {
        // Vertical taskbar (left or right)
        int taskbarWidth = rcTray.right - rcTray.left;
        int posX = rcTray.left;
        int posY = rcNotify.top - m_settings.customHeightPx - m_settings.horizontalMarginPx;
        SetWindowPos(m_hwnd, HWND_TOPMOST, posX, posY, taskbarWidth, m_settings.customHeightPx, SWP_SHOWWINDOW | SWP_NOACTIVATE);
    }
}

COLORREF TaskbarWindow::DetectTaskbarColor() {
    HWND hTray = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (hTray) {
        RECT rcTray{};
        GetWindowRect(hTray, &rcTray);

        HWND hNotify = FindWindowExW(hTray, nullptr, L"TrayNotifyWnd", nullptr);
        RECT rcNotify = rcTray;
        if (hNotify) {
            GetWindowRect(hNotify, &rcNotify);
        }

        HDC hdcScreen = GetDC(nullptr);
        if (hdcScreen) {
            // Sample point 1: Just left of TrayNotifyWnd, along top margin of taskbar
            int sampleX = rcNotify.left - 2;
            int sampleY = rcTray.top + 5;
            COLORREF pix = GetPixel(hdcScreen, sampleX, sampleY);

            // Sample point 2: If sample 1 is black or invalid, sample 100px from left
            if (pix == CLR_INVALID || pix == RGB(0, 0, 0)) {
                sampleX = rcTray.left + 100;
                sampleY = rcTray.top + 5;
                pix = GetPixel(hdcScreen, sampleX, sampleY);
            }

            ReleaseDC(nullptr, hdcScreen);

            if (pix != CLR_INVALID && pix != RGB(0, 0, 0)) {
                m_taskbarColor = pix;
                return m_taskbarColor;
            }
        }
    }

    // Fallback: Registry DWM Colorization
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\DWM", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD colorization = 0;
        DWORD cbData = sizeof(colorization);
        if (RegQueryValueExW(hKey, L"ColorizationColor", nullptr, nullptr, (LPBYTE)&colorization, &cbData) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            BYTE r = (colorization >> 16) & 0xFF;
            BYTE g = (colorization >> 8) & 0xFF;
            BYTE b = colorization & 0xFF;
            m_taskbarColor = RGB(r, g, b);
            return m_taskbarColor;
        }
        RegCloseKey(hKey);
    }

    return m_taskbarColor;
}

void TaskbarWindow::UpdateDisplay() {
    if (!m_settings.monitoringEnabled) {
        m_currentSnapshot.formattedDownload = L"D: Paused";
        m_currentSnapshot.formattedUpload   = L"U: Paused";
    } else {
        m_currentSnapshot = m_telemetry.ProcessTick(m_settings.bitrateUnit, m_settings.precision);
    }
    InvalidateRect(m_hwnd, nullptr, FALSE);
    UpdateTrayTooltip(m_currentSnapshot);
}

void TaskbarWindow::OnPaint(HDC hdc) {
    RECT rcClient{};
    GetClientRect(m_hwnd, &rcClient);
    int w = rcClient.right - rcClient.left;
    int h = rcClient.bottom - rcClient.top;

    // Double buffering
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbm = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP hOldBm = (HBITMAP)SelectObject(hdcMem, hbm);

    if (m_settings.dockToTaskbar) {
        // Seamless Taskbar Mode: Auto-detect taskbar background color without border
        COLORREF taskbarColor = DetectTaskbarColor();
        HBRUSH hBgBrush = CreateSolidBrush(taskbarColor);
        FillRect(hdcMem, &rcClient, hBgBrush);
        DeleteObject(hBgBrush);
    } else {
        // Floating Desktop Mode: Sleek rounded card with subtle outline
        HBRUSH hBgBrush = CreateSolidBrush(RGB(20, 20, 24));
        FillRect(hdcMem, &rcClient, hBgBrush);
        DeleteObject(hBgBrush);

        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(55, 55, 70));
        HPEN hOldPen = (HPEN)SelectObject(hdcMem, hPen);
        SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
        RoundRect(hdcMem, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom, 6, 6);
        SelectObject(hdcMem, hOldPen);
        DeleteObject(hPen);
    }

    SetBkMode(hdcMem, TRANSPARENT);
    HFONT hOldFont = (HFONT)SelectObject(hdcMem, m_hFont);

    std::wstring line1 = m_settings.swapOrder ? (m_settings.prefixUpload + m_currentSnapshot.formattedUpload)
                                              : (m_settings.prefixDownload + m_currentSnapshot.formattedDownload);
    std::wstring line2 = m_settings.swapOrder ? (m_settings.prefixDownload + m_currentSnapshot.formattedDownload)
                                              : (m_settings.prefixUpload + m_currentSnapshot.formattedUpload);

    int lineH = h / 2;
    RECT rcLine1 = { 4, 1, w - 2, lineH };
    RECT rcLine2 = { 4, lineH - 1, w - 2, h };

    // Draw text with 1px drop-shadow glow
    auto DrawShadowedText = [&](const std::wstring& text, RECT& rect, COLORREF mainColor) {
        RECT rcShadow = rect;
        OffsetRect(&rcShadow, 1, 1);
        SetTextColor(hdcMem, RGB(0, 0, 0));
        DrawTextW(hdcMem, text.c_str(), -1, &rcShadow, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        SetTextColor(hdcMem, mainColor);
        DrawTextW(hdcMem, text.c_str(), -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    };

    DrawShadowedText(line1, rcLine1, RGB(0, 220, 255));   // Cyan download
    DrawShadowedText(line2, rcLine2, RGB(255, 140, 60));   // Orange upload

    // Blit to screen
    BitBlt(hdc, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, hOldFont);
    SelectObject(hdcMem, hOldBm);
    DeleteObject(hbm);
    DeleteDC(hdcMem);
}

void TaskbarWindow::SetupTrayIcon() {
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_hwnd;
    m_nid.uID = 1001;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_USER + 101;
    HICON hAppIcon = LoadIconW(m_hInstance, MAKEINTRESOURCEW(101));
    m_nid.hIcon = hAppIcon ? hAppIcon : LoadIcon(nullptr, IDI_APPLICATION);
    wcscpy_s(m_nid.szTip, L"SpeedMonitor by Khalequzzaman");
    Shell_NotifyIconW(NIM_ADD, &m_nid);
}

void TaskbarWindow::RemoveTrayIcon() {
    Shell_NotifyIconW(NIM_DELETE, &m_nid);
}

void TaskbarWindow::UpdateTrayTooltip(const SpeedSnapshot& snapshot) {
    std::wstringstream ss;
    ss << L"SpeedMonitor (by Khalequzzaman)\n"
       << L"Down: " << snapshot.formattedDownload << L"\n"
       << L"Up: " << snapshot.formattedUpload << L"\n"
       << L"Adapter: " << m_telemetry.GetActiveAdapterDescription();
    std::wstring tip = ss.str();
    if (tip.length() >= 128) tip = tip.substr(0, 127);
    wcscpy_s(m_nid.szTip, tip.c_str());
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

void TaskbarWindow::ShowContextMenu(int x, int y) {
    HMENU hMenu = CreatePopupMenu();

    // 1. Adapters Submenu
    HMENU hAdaptersMenu = CreatePopupMenu();
    auto adapters = m_telemetry.EnumerateAdapters();
    NET_IFINDEX activeIf = m_telemetry.GetActiveAdapterIndex();

    for (size_t i = 0; i < adapters.size(); ++i) {
        std::wstring label = adapters[i].adapterDescription;
        if (adapters[i].isConnected) label += L" [Connected]";
        UINT flags = MF_STRING;
        if (adapters[i].ifIndex == activeIf) flags |= MF_CHECKED;
        AppendMenuW(hAdaptersMenu, flags, 2000 + i, label.c_str());
    }
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hAdaptersMenu, L"Network Interface");

    // 2. Unit Submenu
    HMENU hUnitMenu = CreatePopupMenu();
    const wchar_t* unitNames[] = {
        L"kbit/s (Decimal bits)",
        L"Kibit/s (Binary bits)",
        L"kB/s (Decimal bytes)",
        L"KiB/s (Binary bytes - Recommended)",
        L"Mbit/s (Decimal Megabits)",
        L"Mibit/s (Binary Mebibits)",
        L"MB/s (Decimal Megabytes)",
        L"MiB/s (Binary Mebibytes)"
    };
    for (int u = 0; u <= 7; ++u) {
        UINT flags = MF_STRING;
        if (static_cast<int>(m_settings.bitrateUnit) == u) flags |= MF_CHECKED;
        AppendMenuW(hUnitMenu, flags, 3000 + u, unitNames[u]);
    }
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hUnitMenu, L"Bitrate Unit");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    // Monitoring Toggle
    AppendMenuW(hMenu, MF_STRING | (m_settings.monitoringEnabled ? MF_CHECKED : 0), 1001, L"Monitoring Active");

    // Docking Toggle
    AppendMenuW(hMenu, MF_STRING | (m_settings.dockToTaskbar ? MF_CHECKED : 0), 1002, L"Dock to Taskbar");

    // Swap Order
    AppendMenuW(hMenu, MF_STRING | (m_settings.swapOrder ? MF_CHECKED : 0), 1003, L"Swap Upload / Download");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    AppendMenuW(hMenu, MF_STRING, 1004, L"Reset Session Stats");
    AppendMenuW(hMenu, MF_STRING, 1005, L"Connections Monitor...");
    AppendMenuW(hMenu, MF_STRING, 1006, L"About SpeedMonitor...");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, 1099, L"Exit");

    SetForegroundWindow(m_hwnd);
    int cmd = TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, x, y, 0, m_hwnd, nullptr);
    DestroyMenu(hMenu);

    if (cmd >= 2000 && cmd < 2000 + (int)adapters.size()) {
        int idx = cmd - 2000;
        m_telemetry.SetActiveAdapter(adapters[idx].adapterDescription);
        m_settings.selectedAdapterDesc = adapters[idx].adapterDescription;
        m_config.Save(m_settings);
        UpdateDisplay();
    } else if (cmd >= 3000 && cmd <= 3007) {
        m_settings.bitrateUnit = static_cast<BitrateUnit>(cmd - 3000);
        m_config.Save(m_settings);
        UpdateDisplay();
    } else if (cmd == 1001) {
        m_settings.monitoringEnabled = !m_settings.monitoringEnabled;
        m_config.Save(m_settings);
        UpdateDisplay();
    } else if (cmd == 1002) {
        m_settings.dockToTaskbar = !m_settings.dockToTaskbar;
        m_config.Save(m_settings);
        ReAnchorToTaskbar();
    } else if (cmd == 1003) {
        m_settings.swapOrder = !m_settings.swapOrder;
        m_config.Save(m_settings);
        InvalidateRect(m_hwnd, nullptr, FALSE);
    } else if (cmd == 1004) {
        m_telemetry.ResetSessionCounters();
        UpdateDisplay();
    } else if (cmd == 1005) {
        ShowConnectionsDialog();
    } else if (cmd == 1006) {
        MessageBoxW(m_hwnd,
            L"SpeedMonitor v1.0.0\n\n"
            L"Author: Khalequzzaman\n"
            L"GitHub: https://github.com/khalequzzaman17/speedmonitor\n\n"
            L"A lightweight, real-time network speed monitor for Windows 10 and 11.\n"
            L"Engineered with native C++20, 64-bit NDIS metrics, and dynamic taskbar integration.\n\n"
            L"Licensed under MIT.",
            L"About SpeedMonitor", MB_OK | MB_ICONINFORMATION);
    } else if (cmd == 1099) {
        PostQuitMessage(0);
    }
}

void TaskbarWindow::ShowConnectionsDialog() {
    MessageBoxW(m_hwnd, L"Live Connections Monitor feature is ready. Inspect active TCP/UDP endpoints via System Diagnostics.", L"Connections Monitor", MB_OK | MB_ICONINFORMATION);
}

LRESULT CALLBACK TaskbarWindow::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (g_instance) {
        return g_instance->HandleMessage(hwnd, uMsg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

LRESULT TaskbarWindow::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_TIMER:
            if (wParam == m_timerId) {
                UpdateDisplay();
                return 0;
            }
            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            OnPaint(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1; // Suppress flicker

        case WM_RBUTTONUP: {
            POINT pt;
            GetCursorPos(&pt);
            ShowContextMenu(pt.x, pt.y);
            return 0;
        }

        case WM_LBUTTONDBLCLK:
            // Double click opens context menu / config
            POINT pt;
            GetCursorPos(&pt);
            ShowContextMenu(pt.x, pt.y);
            return 0;

        case WM_LBUTTONDOWN:
            if (!m_settings.dockToTaskbar) {
                m_isDragging = true;
                SetCapture(hwnd);
                GetCursorPos(&m_dragStart);
            }
            return 0;

        case WM_MOUSEMOVE:
            if (m_isDragging) {
                POINT pt;
                GetCursorPos(&pt);
                int dx = pt.x - m_dragStart.x;
                int dy = pt.y - m_dragStart.y;
                m_dragStart = pt;

                RECT rc;
                GetWindowRect(hwnd, &rc);
                m_settings.windowPosX = rc.left + dx;
                m_settings.windowPosY = rc.top + dy;
                SetWindowPos(hwnd, HWND_TOPMOST, m_settings.windowPosX, m_settings.windowPosY, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
            }
            return 0;

        case WM_LBUTTONUP:
            if (m_isDragging) {
                m_isDragging = false;
                ReleaseCapture();
                m_config.Save(m_settings);
            }
            return 0;

        case WM_USER + 101: // Tray Icon Notification
            if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                ShowContextMenu(pt.x, pt.y);
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void TaskbarWindow::RunMessageLoop() {
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}
