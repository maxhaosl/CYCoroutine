@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Building CYCoroutine for Windows
echo ========================================

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release
set BUILD_TYPE=%BUILD_TYPE:"=%

call :NormalizeBuildType BUILD_TYPE
if errorlevel 1 exit /b 1

set BUILD_DIR=%~dp0build_windows
set INSTALL_DIR=%~dp0..\Bin
set SOURCE_DIR=%~dp0..

set SHARED_RUNTIME=
call :ResolveRuntimeForConfig "%BUILD_TYPE%" "MD" SHARED_RUNTIME
if errorlevel 1 exit /b 1

set STATIC_RUNTIME=
call :ResolveRuntimeForConfig "%BUILD_TYPE%" "MT" STATIC_RUNTIME
if errorlevel 1 exit /b 1

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
echo Building for x64 Shared (!SHARED_RUNTIME!)...
cmake -G "!GENERATOR!" -A x64 -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DBUILD_EXAMPLES=ON -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=OFF -DWINDOWS_RUNTIME=!SHARED_RUNTIME! "%SOURCE_DIR%/Build"
if %ERRORLEVEL% neq 0 (
    echo Error: CMake configuration failed for x64 Shared (!SHARED_RUNTIME!).
    pause
    exit /b 1
)

cmake --build . --config %BUILD_TYPE% --target CYCoroutine_shared
if %ERRORLEVEL% neq 0 (
    echo Error: Build failed for x64 Shared (!SHARED_RUNTIME!).
    pause
    exit /b 1
)

cmake --build . --config %BUILD_TYPE% --target CYCoroutineExample
if %ERRORLEVEL% neq 0 (
    echo Error: Example build failed for x64 Shared (!SHARED_RUNTIME!).
    pause
    exit /b 1
)

REM 构建x64静态库
echo.
echo Building for x64 Static (!STATIC_RUNTIME!)...
cmake -G "!GENERATOR!" -A x64 -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DBUILD_EXAMPLES=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -DWINDOWS_RUNTIME=!STATIC_RUNTIME! "%SOURCE_DIR%/Build"
if %ERRORLEVEL% neq 0 (
    echo Error: CMake configuration failed for x64 Static (!STATIC_RUNTIME!).
    pause
    exit /b 1
)

cmake --build . --config %BUILD_TYPE% --target CYCoroutine_static
if %ERRORLEVEL% neq 0 (
    echo Error: Build failed for x64 Static (!STATIC_RUNTIME!).
    pause
    exit /b 1
)

cmake --build . --config %BUILD_TYPE% --target CYCoroutineExample
if %ERRORLEVEL% neq 0 (
    echo Error: Example build failed for x64 Static (!STATIC_RUNTIME!).
    pause
    exit /b 1
)

REM 构建x86动态库
echo.
echo Building for x86 Shared (!SHARED_RUNTIME!)...
cmake -G "!GENERATOR!" -A Win32 -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DBUILD_EXAMPLES=ON -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=OFF -DWINDOWS_RUNTIME=!SHARED_RUNTIME! "%SOURCE_DIR%/Build"
if %ERRORLEVEL% neq 0 (
    echo Error: CMake configuration failed for x86 Shared (!SHARED_RUNTIME!).
    pause
    exit /b 1
)

cmake --build . --config %BUILD_TYPE% --target CYCoroutine_shared
if %ERRORLEVEL% neq 0 (
    echo Error: Build failed for x86 Shared (!SHARED_RUNTIME!).
    pause
    exit /b 1
)

cmake --build . --config %BUILD_TYPE% --target CYCoroutineExample
if %ERRORLEVEL% neq 0 (
    echo Error: Example build failed for x86 Shared (!SHARED_RUNTIME!).
    pause
    exit /b 1
)

REM 构建x86静态库
echo.
echo Building for x86 Static (!STATIC_RUNTIME!)...
cmake -G "!GENERATOR!" -A Win32 -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DBUILD_EXAMPLES=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -DWINDOWS_RUNTIME=!STATIC_RUNTIME! "%SOURCE_DIR%/Build"
if %ERRORLEVEL% neq 0 (
    echo Error: CMake configuration failed for x86 Static (!STATIC_RUNTIME!).
    pause
    exit /b 1
)

cmake --build . --config %BUILD_TYPE% --target CYCoroutine_static
if %ERRORLEVEL% neq 0 (
    echo Error: Build failed for x86 Static (!STATIC_RUNTIME!).
    pause
    exit /b 1
)

cmake --build . --config %BUILD_TYPE% --target CYCoroutineExample
if %ERRORLEVEL% neq 0 (
    echo Error: Example build failed for x86 Static (!STATIC_RUNTIME!).
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo Output files are in: %INSTALL_DIR%
echo ========================================
pause
goto :eof

:NormalizeBuildType
set "VAR_NAME=%~1"
call set "CURRENT_VALUE=%%%VAR_NAME%%%"
if "%CURRENT_VALUE%"=="" set "CURRENT_VALUE=Release"

if /I "%CURRENT_VALUE%"=="DEBUG" (
    set CURRENT_VALUE=Debug
) else if /I "%CURRENT_VALUE%"=="RELEASE" (
    set CURRENT_VALUE=Release
) else (
    echo Invalid build type "%CURRENT_VALUE%". Use Release or Debug.
    exit /b 1
)

set "%VAR_NAME%=%CURRENT_VALUE%"
exit /b 0

:ResolveRuntimeForConfig
set "CFG=%~1"
set "BASE=%~2"
set "OUT_VAR=%~3"

if /I "%CFG%"=="Debug" (
    if /I "%BASE%"=="MT" (
        set RESULT=MTD
    ) else (
        set RESULT=MDD
    )
) else if /I "%CFG%"=="Release" (
    set RESULT=%BASE%
) else (
    echo Unsupported build type "%CFG%".
    exit /b 1
)

set "%OUT_VAR%=%RESULT%"
exit /b 0