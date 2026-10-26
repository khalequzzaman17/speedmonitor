@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo Building SpeedMonitor for Windows 10/11
echo Author: Khalequzzaman
echo GitHub: https://github.com/khalequzzaman17/speedmonitor
echo =======================================================

:: Setup build directory
if not exist "bin" mkdir bin

:: Find G++ compiler and windres (Auto-detect PATH, Scoop MinGW via %USERPROFILE%, or standard locations)
where g++ >nul 2>nul
if %ERRORLEVEL% equ 0 (
    set "GXX=g++"
    set "WINDRES=windres"
) else if exist "%USERPROFILE%\scoop\apps\mingw\current\bin\g++.exe" (
    set "GXX=%USERPROFILE%\scoop\apps\mingw\current\bin\g++.exe"
    set "WINDRES=%USERPROFILE%\scoop\apps\mingw\current\bin\windres.exe"
) else if exist "%LOCALAPPDATA%\Programs\mingw64\bin\g++.exe" (
    set "GXX=%LOCALAPPDATA%\Programs\mingw64\bin\g++.exe"
    set "WINDRES=%LOCALAPPDATA%\Programs\mingw64\bin\windres.exe"
) else if exist "C:\mingw64\bin\g++.exe" (
    set "GXX=C:\mingw64\bin\g++.exe"
    set "WINDRES=C:\mingw64\bin\windres.exe"
) else (
    set "GXX=g++"
    set "WINDRES=windres"
)

echo Using compiler: %GXX%
echo Using windres:  %WINDRES%

:: Compile Windows Resource File (Embeds Author: Khalequzzaman, Version, Icon, Product Info)
echo Compiling resources (Author: Khalequzzaman, Icon: app.ico) ...
"%WINDRES%" -i "src\resource.rc" -o "src\resource.o" --include-dir=src

:: Source files
set "SOURCES=src\main.cpp src\telemetry.cpp src\config.cpp src\taskbar.cpp src\resource.o"

:: Compilation flags
set "CXXFLAGS=-std=c++20 -O2 -s -mwindows -municode -Wall -Wextra -Wno-unused-parameter -static -static-libgcc -static-libstdc++ -Wl,--subsystem,windows"
set "LDFLAGS=-liphlpapi -lws2_32 -lgdi32 -luser32 -lshell32 -luxtheme"

echo Compiling SpeedMonitor.exe ...
"%GXX%" %CXXFLAGS% %SOURCES% %LDFLAGS% -o "bin\SpeedMonitor.exe"

if %ERRORLEVEL% equ 0 (
    echo.
    echo [SUCCESS] Built bin\SpeedMonitor.exe successfully!
    dir "bin\SpeedMonitor.exe" | findstr "SpeedMonitor.exe"
    del "src\resource.o" 2>nul
) else (
    echo.
    echo [ERROR] Build failed with error code %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)

endlocal
