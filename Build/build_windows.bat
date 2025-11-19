@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Building CYCoroutine for Windows
echo ========================================

REM 设置变量
set BUILD_DIR=%~dp0build_windows
set INSTALL_DIR=%~dp0..\Bin
set SOURCE_DIR=%~dp0..

REM 创建构建目录
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM 检查Visual Studio环境
if not defined VCINSTALLDIR (
    echo Error: Visual Studio environment not detected.
    echo Please run this script from a Visual Studio Developer Command Prompt.
    pause
    exit /b 1
)

REM 获取Visual Studio版本
set GENERATOR=
for /f "tokens=*" %%i in ('cmake --help ^| findstr "Visual Studio"') do (
    if "!GENERATOR!"=="" set GENERATOR=%%i
)
if "!GENERATOR!"=="" set GENERATOR=Visual Studio 16 2019

REM 构建x64动态库
echo.
echo Building for x64 Shared (MD)...
cmake -G "!GENERATOR!" -A x64 -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=OFF -DWINDOWS_RUNTIME=MD "%SOURCE_DIR%/Build"
if %ERRORLEVEL% neq 0 (
    echo Error: CMake configuration failed for x64 Shared (MD).
    pause
    exit /b 1
)

cmake --build . --config Release --target CYCoroutine_shared
if %ERRORLEVEL% neq 0 (
    echo Error: Build failed for x64 Shared (MD).
    pause
    exit /b 1
)

cmake --build . --config Release --target CYCoroutineExample
if %ERRORLEVEL% neq 0 (
    echo Error: Example build failed for x64 Shared (MD).
    pause
    exit /b 1
)

REM 构建x64静态库
echo.
echo Building for x64 Static (MT)...
cmake -G "!GENERATOR!" -A x64 -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -DWINDOWS_RUNTIME=MT "%SOURCE_DIR%/Build"
if %ERRORLEVEL% neq 0 (
    echo Error: CMake configuration failed for x64 Static (MT).
    pause
    exit /b 1
)

cmake --build . --config Release --target CYCoroutine_static
if %ERRORLEVEL% neq 0 (
    echo Error: Build failed for x64 Static (MT).
    pause
    exit /b 1
)

cmake --build . --config Release --target CYCoroutineExample
if %ERRORLEVEL% neq 0 (
    echo Error: Example build failed for x64 Static (MT).
    pause
    exit /b 1
)

REM 构建x86动态库
echo.
echo Building for x86 Shared (MD)...
cmake -G "!GENERATOR!" -A Win32 -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=OFF -DWINDOWS_RUNTIME=MD "%SOURCE_DIR%/Build"
if %ERRORLEVEL% neq 0 (
    echo Error: CMake configuration failed for x86 Shared (MD).
    pause
    exit /b 1
)

cmake --build . --config Release --target CYCoroutine_shared
if %ERRORLEVEL% neq 0 (
    echo Error: Build failed for x86 Shared (MD).
    pause
    exit /b 1
)

cmake --build . --config Release --target CYCoroutineExample
if %ERRORLEVEL% neq 0 (
    echo Error: Example build failed for x86 Shared (MD).
    pause
    exit /b 1
)

REM 构建x86静态库
echo.
echo Building for x86 Static (MT)...
cmake -G "!GENERATOR!" -A Win32 -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -DWINDOWS_RUNTIME=MT "%SOURCE_DIR%/Build"
if %ERRORLEVEL% neq 0 (
    echo Error: CMake configuration failed for x86 Static (MT).
    pause
    exit /b 1
)

cmake --build . --config Release --target CYCoroutine_static
if %ERRORLEVEL% neq 0 (
    echo Error: Build failed for x86 Static (MT).
    pause
    exit /b 1
)

cmake --build . --config Release --target CYCoroutineExample
if %ERRORLEVEL% neq 0 (
    echo Error: Example build failed for x86 Static (MT).
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo Output files are in: %INSTALL_DIR%
echo ========================================
pause