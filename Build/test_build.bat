@echo off
echo Testing build script...

REM Check VS environment
if not defined VCINSTALLDIR (
    echo Setting up VS environment...
    call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64
)

echo VS Environment: %VCINSTALLDIR%
echo Build Type: Release

REM Set paths
set BUILD_DIR=%~dp0build_windows
set SOURCE_DIR=%~dp0..

echo Build Dir: %BUILD_DIR%
echo Source Dir: %SOURCE_DIR%

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

echo Running CMake configuration...
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=OFF -DWINDOWS_RUNTIME=MD "%SOURCE_DIR%/Build"

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed
    pause
    exit /b 1
)

echo CMake configuration successful
pause