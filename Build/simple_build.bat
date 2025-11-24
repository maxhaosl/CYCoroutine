@echo off
echo Setting up Visual Studio environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64

echo.
echo Running simple build test...

set BUILD_DIR=%~dp0build_test
set SOURCE_DIR=%~dp0..

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

echo Configuring with CMake...
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release "%SOURCE_DIR%/Build"

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed
    pause
    exit /b 1
)

echo Building...
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo Build failed
    pause
    exit /b 1
)

echo Build completed successfully!
pause