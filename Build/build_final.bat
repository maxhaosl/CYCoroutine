@echo off
echo Setting up Visual Studio environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64

echo.
echo Building CYCoroutine for Windows...
echo.

set BUILD_TYPE=Release
set BUILD_DIR=%~dp0build_windows
set SOURCE_DIR=%~dp0..
set GENERATOR=Visual Studio 17 2022

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

echo Building x64 Release MD (Shared)...
cmake -G "%GENERATOR%" -A x64 -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DBUILD_EXAMPLES=ON -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=OFF -DWINDOWS_RUNTIME=MD "%SOURCE_DIR%/Build"
if %ERRORLEVEL% neq 0 goto :error

cmake --build . --config %BUILD_TYPE% --target CYCoroutine_shared --parallel
if %ERRORLEVEL% neq 0 goto :error

echo.
echo Building x64 Release MT (Static)...
cmake -G "%GENERATOR%" -A x64 -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DBUILD_EXAMPLES=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -DWINDOWS_RUNTIME=MT "%SOURCE_DIR%/Build"
if %ERRORLEVEL% neq 0 goto :error

cmake --build . --config %BUILD_TYPE% --target CYCoroutine_static --parallel
if %ERRORLEVEL% neq 0 goto :error

echo.
echo Building x86 Release MD (Shared)...
cmake -G "%GENERATOR%" -A Win32 -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DBUILD_EXAMPLES=ON -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=OFF -DWINDOWS_RUNTIME=MD "%SOURCE_DIR%/Build"
if %ERRORLEVEL% neq 0 goto :error

cmake --build . --config %BUILD_TYPE% --target CYCoroutine_shared --parallel
if %ERRORLEVEL% neq 0 goto :error

echo.
echo Building x86 Release MT (Static)...
cmake -G "%GENERATOR%" -A Win32 -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DBUILD_EXAMPLES=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -DWINDOWS_RUNTIME=MT "%SOURCE_DIR%/Build"
if %ERRORLEVEL% neq 0 goto :error

cmake --build . --config %BUILD_TYPE% --target CYCoroutine_static --parallel
if %ERRORLEVEL% neq 0 goto :error

echo.
echo ========================================
echo Build completed successfully!
echo ========================================

set INSTALL_DIR=%~dp0..\Bin
echo Output files are in: %INSTALL_DIR%
echo.
echo Generated files:
if exist "%INSTALL_DIR%\Windows\x86_64\MD\RELEASE\CYCoroutine.dll" echo   x64 MD Shared: CYCoroutine.dll, CYCoroutine.lib
if exist "%INSTALL_DIR%\Windows\x86_64\MT\RELEASE\CYCoroutine.lib" echo   x64 MT Static: CYCoroutine.lib
if exist "%INSTALL_DIR%\Windows\x86\MD\RELEASE\CYCoroutine.dll" echo   x86 MD Shared: CYCoroutine.dll, CYCoroutine.lib
if exist "%INSTALL_DIR%\Windows\x86\MT\RELEASE\CYCoroutine.lib" echo   x86 MT Static: CYCoroutine.lib

goto :end

:error
echo.
echo ========================================
echo BUILD FAILED!
echo ========================================
pause
exit /b 1

:end
pause