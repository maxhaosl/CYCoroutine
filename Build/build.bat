@echo off
setlocal enabledelayedexpansion

echo ========================================
echo CYCoroutine Cross-Platform Build Script
echo ========================================
echo Detected OS: Windows

echo Building for Windows...
call "%~dp0build_windows.bat"

REM 检查构建是否成功
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo Build completed successfully!
    echo Output files are in: %~dp0..\Bin
    echo ========================================
) else (
    echo.
    echo ========================================
    echo Build failed!
    echo ========================================
    exit /b 1
)