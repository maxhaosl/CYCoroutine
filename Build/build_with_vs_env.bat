@echo off
echo Setting up Visual Studio 2022 Enterprise environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64

if %ERRORLEVEL% neq 0 (
    echo Error: Failed to setup Visual Studio environment.
    echo Trying VS2019...
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64
    if %ERRORLEVEL% neq 0 (
        echo Error: Failed to setup any Visual Studio environment.
        pause
        exit /b 1
    )
)

echo Visual Studio environment setup complete.
echo.
echo Starting build process...
call build_windows.bat %*