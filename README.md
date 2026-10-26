# SpeedMonitor

[![Author: Khalequzzaman](https://img.shields.io/badge/Author-Khalequzzaman-blue.svg)](https://github.com/khalequzzaman17)
[![C++20](https://img.shields.io/badge/Language-C%2B%2B20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![Platform](https://img.shields.io/badge/Platform-Windows%2010%20%2F%2011%20(x64)-brightgreen.svg)](https://www.microsoft.com/windows)
[![Binary Size](https://img.shields.io/badge/Binary%20Size-%3C%201%20MB%20Standalone-orange.svg)](#features)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Zero Dependencies](https://img.shields.io/badge/Dependencies-Zero%20(Pure%20Win32)-purple.svg)](#architecture)

A modern, ultra-lightweight network speed monitor designed specifically for **Windows 10 and Windows 11**, developed by **Khalequzzaman**.

Inspired by classic bandwidth monitors, **SpeedMonitor** completely solves modern Windows compatibility, crash, and integration issues without patching system files or relying on deprecated COM interfaces.

---

## ⚡ The Problem: Why Old Bandwidth Monitors Fail on Windows 10/11

Older utilities (such as NetSpeedMonitor 2010) were designed during the Windows XP / Vista / 7 era and fail completely on modern operating systems:

1. **Deprecated `IDeskBand` Architecture**: Modern Windows 11 completely removed support for shell deskband toolbars in `explorer.exe`. Legacy deskband tools cannot be enabled or loaded into the taskbar.
2. **Explorer Crash Vulnerability**: Older tools ran in-process as a shell extension DLL (`nsm.dll`). Any crash or unhandled exception immediately killed the entire Windows Shell (`explorer.exe`), taking down the taskbar and open applications.
3. **Gigabit/Fiber Rollover Glitches**: Legacy 32-bit counter metrics (`MIB_IFROW.dwInOctets`) rollover every ~34 seconds on modern Gigabit fiber networks, creating massive telemetry spikes or negative readings.
4. **No High-DPI Scaling**: Fixed bitmap rendering appears blurry or micro-sized on 2K/4K/Retina displays.

---

## 🚀 The Solution: What SpeedMonitor Delivers

**SpeedMonitor** is built from the ground up as a native C++20 clean-room application that delivers peak performance while resolving every modern limitation:

- 🛡️ **Out-of-Process Isolation**: Runs as an independent process (`SpeedMonitor.exe`). It communicates visually with the desktop and **can never crash Explorer**.
- 📌 **Dynamic Taskbar Docking & Floating**:
  - Automatically anchors itself adjacent to the Windows notification tray (`TrayNotifyWnd`) using real-time Windows Event hooks (`EVENT_OBJECT_LOCATIONCHANGE`).
  - Works natively on both Windows 10 and Windows 11 taskbars.
  - Can be unlocked and dragged anywhere on your desktop as a floating HUD.
- 📶 **Modern NDIS 6.0+ 64-bit Telemetry**: Employs `GetIfEntry2` (`MIB_IF_ROW2`) with high-resolution `QueryPerformanceCounter` (QPC) timing. Zero rollover spikes, even on 10 Gbps networks!
- ⚖️ **Exact Bitrate Conversions**: Supports all 9 standard conversion modes with precision formatting:
  - `kbit/s` (Decimal kilobits)
  - `Kibit/s` (Binary kibibits)
  - `kB/s` (Decimal kilobytes)
  - `KiB/s` (Binary kibibytes — Default)
  - `Mbit/s` (Decimal megabits)
  - `Mibit/s` (Binary mebibits)
  - `MB/s` (Decimal megabytes)
  - `MiB/s` (Binary mebibytes)
  - `MB/min` (Volume per minute)
- 🖥️ **High-DPI ClearType Typography**: Supports Per-Monitor DPI Awareness v2 with double-buffered GDI rendering and customizable fonts (Segoe UI default).
- 🪶 **Ultra-Lightweight (<1 MB)**: Statically linked standalone executable. Zero runtime dependencies, no .NET runtimes, no Visual C++ Redistributable packages required!
- ⚙️ **Portable INI Configuration**: Saves settings automatically to `%APPDATA%\SpeedMonitor\config.ini`.
- 🏷️ **Embedded Windows Metadata**: Version, author (`Khalequzzaman`), and product metadata are embedded directly into the PE executable via `resource.rc`.

---

## 📸 Interface & Controls

- **Left-Click & Drag**: Move the floating window (when taskbar auto-docking is disabled).
- **Right-Click**: Opens the context menu:
  - **Network Adapters**: List all active and connected hardware/virtual interfaces (Wi-Fi, Ethernet, VPN, etc.) with real-time selection.
  - **Bitrate Units**: One-click switching between `KiB/s`, `kB/s`, `Mbit/s`, `MB/s`, etc.
  - **Decimal Precision**: Adjust precision (0, 1, 2, or 3 decimal places).
  - **Dock to Taskbar**: Toggle between auto-docking next to the tray or free-floating.
  - **Pause / Resume Monitoring**: Temporarily pause telemetry sampling.
  - **Reset Counters**: Reset session total downloaded / uploaded statistics.
  - **About SpeedMonitor**: View author details and repository link.
  - **Exit**: Cleanly unhooks, saves configuration, and terminates.
- **System Tray Icon**: Persistent notification area icon with real-time tooltip showing session traffic.

---

## 🛠️ Building from Source

### Prerequisites
- **GCC / MinGW-w64** (C++20 capable, GCC 13+) with `windres`.

### Build with MinGW (Windows / Command Prompt)
Run the automated build script:
```cmd
build.bat
```
The optimized, stripped binary will be generated at:
```
bin\SpeedMonitor.exe  (Size: ~0.97 MB)
```

### Build with Make
```bash
make
```

---

## 📁 Repository Structure

```
├── src/
│   ├── main.cpp                   # Application entry point & single-instance mutex
│   ├── types.hpp                  # Data models, telemetry structs, unit conversion
│   ├── telemetry.hpp / .cpp       # NDIS 6.0+ GetIfEntry2 & QPC sampling engine
│   ├── config.hpp / .cpp          # Persistent INI configuration manager
│   ├── taskbar.hpp / .cpp         # Out-of-process taskbar hook & GDI renderer
│   └── resource.rc                # Windows Version & Author metadata resource
├── build.bat                      # One-click build script for MinGW
├── Makefile                       # Cross-platform Makefile
├── LICENSE                        # MIT License
└── README.md                      # Project documentation
```

---

## 👨‍💻 Author

**Khalequzzaman**
- GitHub: [@khalequzzaman17](https://github.com/khalequzzaman17)
- Repository: [speedmonitor](https://github.com/khalequzzaman17/speedmonitor)

---

## 📄 License

This project is licensed under the [MIT License](LICENSE) - Copyright (c) 2026 Khalequzzaman.
