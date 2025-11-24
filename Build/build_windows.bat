@echo off
setlocal

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
    echo.
    echo You can also try to find and run vcvarsall.bat manually:
    echo   "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64
    echo   or
    echo   "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    echo.
    pause
    exit /b 1
)

REM 获取Visual Studio版本
set GENERATOR=
for /f "tokens=*" %%i in ('cmake --help ^| findstr "Visual Studio 17 2022"') do (
    set GENERATOR=Visual Studio 17 2022
    goto :generator_found
)
for /f "tokens=*" %%i in ('cmake --help ^| findstr "Visual Studio 16 2019"') do (
    set GENERATOR=Visual Studio 16 2019
    goto :generator_found
)
:generator_found
if "%GENERATOR%"=="" set GENERATOR=Visual Studio 16 2019

echo Using generator: %GENERATOR%
echo Build type: %BUILD_TYPE%
echo Shared runtime: %SHARED_RUNTIME%
echo Static runtime: %STATIC_RUNTIME%

REM 构建x64动态库
echo.
echo Building for x64 Shared (%SHARED_RUNTIME%)...
cmake -G "%GENERATOR%" -A x64 -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DBUILD_EXAMPLES=ON -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=OFF -DWINDOWS_RUNTIME=%SHARED_RUNTIME% "%SOURCE_DIR%/Build"
if %ERRORLEVEL% neq 0 (
    echo Error: CMake configuration failed for x64 Shared (%SHARED_RUNTIME%).
    pause
    exit /b 1
)

cmake --build . --config %BUILD_TYPE% --target CYCoroutine_shared --parallel
if %ERRORLEVEL% neq 0 (
    echo Error: Build failed for x64 Shared (%SHARED_RUNTIME%).
    pause
    exit /b 1
)

REM 构建x64静态库
echo.
echo Building for x64 Static (%STATIC_RUNTIME%)...
cmake -G "%GENERATOR%" -A x64 -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DBUILD_EXAMPLES=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -DWINDOWS_RUNTIME=%STATIC_RUNTIME% "%SOURCE_DIR%/Build"
if %ERRORLEVEL% neq 0 (
    echo Error: CMake configuration failed for x64 Static (%STATIC_RUNTIME%).
    pause
    exit /b 1
)

cmake --build . --config %BUILD_TYPE% --target CYCoroutine_static --parallel
if %ERRORLEVEL% neq 0 (
    echo Error: Build failed for x64 Static (%STATIC_RUNTIME%).
    pause
    exit /b 1
)

REM 构建示例
echo.
echo Building examples...
cmake --build . --config %BUILD_TYPE% --target CYCoroutineExample --parallel
if %ERRORLEVEL% neq 0 (
    echo Warning: Example build failed, but libraries were built successfully.
)

REM 构建x86动态库
echo.
echo Building for x86 Shared (%SHARED_RUNTIME%)...
cmake -G "%GENERATOR%" -A Win32 -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DBUILD_EXAMPLES=ON -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=OFF -DWINDOWS_RUNTIME=%SHARED_RUNTIME% "%SOURCE_DIR%/Build"
if %ERRORLEVEL% neq 0 (
    echo Error: CMake configuration failed for x86 Shared (%SHARED_RUNTIME%).
    pause
    exit /b 1
)

cmake --build . --config %BUILD_TYPE% --target CYCoroutine_shared --parallel
if %ERRORLEVEL% neq 0 (
    echo Error: Build failed for x86 Shared (%SHARED_RUNTIME%).
    pause
    exit /b 1
)

REM 构建x86静态库
echo.
echo Building for x86 Static (%STATIC_RUNTIME%)...
cmake -G "%GENERATOR%" -A Win32 -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DBUILD_EXAMPLES=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -DWINDOWS_RUNTIME=%STATIC_RUNTIME% "%SOURCE_DIR%/Build"
if %ERRORLEVEL% neq 0 (
    echo Error: CMake configuration failed for x86 Static (%STATIC_RUNTIME%).
    pause
    exit /b 1
)

cmake --build . --config %BUILD_TYPE% --target CYCoroutine_static --parallel
if %ERRORLEVEL% neq 0 (
    echo Error: Build failed for x86 Static (%STATIC_RUNTIME%).
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo Output files are in: %INSTALL_DIR%
echo ========================================

REM 显示生成的文件
echo.
echo Generated files:
if exist "%INSTALL_DIR%\Windows\x86_64\%SHARED_RUNTIME%\%BUILD_TYPE%\CYCoroutine.dll" (
    echo   x64 Shared: CYCoroutine.dll, CYCoroutine.lib
)
if exist "%INSTALL_DIR%\Windows\x86_64\%STATIC_RUNTIME%\%BUILD_TYPE%\CYCoroutine.lib" (
    echo   x64 Static: CYCoroutine.lib
)
if exist "%INSTALL_DIR%\Windows\x86\%SHARED_RUNTIME%\%BUILD_TYPE%\CYCoroutine.dll" (
    echo   x86 Shared: CYCoroutine.dll, CYCoroutine.lib
)
if exist "%INSTALL_DIR%\Windows\x86\%STATIC_RUNTIME%\%BUILD_TYPE%\CYCoroutine.lib" (
    echo   x86 Static: CYCoroutine.lib
)

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