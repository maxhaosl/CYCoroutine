@echo off
setlocal enableextensions enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."
set "SOURCE_DIR=%SCRIPT_DIR%"
if "%SOURCE_DIR:~-1%"=="\" set "SOURCE_DIR=%SOURCE_DIR:~0,-1%"
set "BUILD_ROOT=%SCRIPT_DIR%out"

set "BUILD_TYPE=%~1"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=Release"

set "LIB_TYPE=%~2"
if "%LIB_TYPE%"=="" set "LIB_TYPE=Static"

set "TARGET_ARCH=%~3"
if "%TARGET_ARCH%"=="" set "TARGET_ARCH=x64"

set "CRT_TYPE=%~4"
if "%CRT_TYPE%"=="" set "CRT_TYPE=MD"

set "BUILD_TYPE=%BUILD_TYPE:"=%"
set "LIB_TYPE=%LIB_TYPE:"=%"
set "TARGET_ARCH=%TARGET_ARCH:"=%"
set "CRT_TYPE=%CRT_TYPE:"=%"

call :NormalizeBuildType BUILD_TYPE
if errorlevel 1 exit /b 1

call :NormalizeLibType LIB_TYPE
if errorlevel 1 exit /b 1

if /I "%LIB_TYPE%"=="Shared" (
    echo Error: CYCoroutine build scripts now only support static libraries.
    exit /b 1
)

call :NormalizeArch TARGET_ARCH
if errorlevel 1 exit /b 1

call :ValidateRuntime CRT_TYPE
if errorlevel 1 exit /b 1

set "EFFECTIVE_RUNTIME="
call :ResolveRuntimeForConfig "%BUILD_TYPE%" "%CRT_TYPE%" EFFECTIVE_RUNTIME
if errorlevel 1 exit /b 1

set "VS_ENV_ARCH=%CMAKE_ARCH%"
if /I "%VS_ENV_ARCH%"=="Win32" set "VS_ENV_ARCH=x86"
if /I "%VS_ENV_ARCH%"=="x86" set "VS_ENV_ARCH=x86"
if /I "%VS_ENV_ARCH%"=="x86_64" set "VS_ENV_ARCH=x64"
if /I "%VS_ENV_ARCH%"=="x64" set "VS_ENV_ARCH=x64"

call :EnsureVisualStudioEnvironment "%VS_ENV_ARCH%"
if errorlevel 1 exit /b 1

call :SelectGenerator
if errorlevel 1 exit /b 1

set "LIB_KIND=static"
set "BUILD_SHARED=OFF"
set "BUILD_STATIC=ON"
set "TARGET_NAME=CYCoroutine_static"
set "ENABLE_EXAMPLES=ON"
if /I "%LIB_TYPE%"=="Shared" (
    set "LIB_KIND=shared"
    set "BUILD_SHARED=ON"
    set "TARGET_NAME=CYCoroutine_shared"
    REM Shared builds still rely on static library for the samples
    set "BUILD_STATIC=ON"
    set "ENABLE_EXAMPLES=OFF"
)

set "BUILD_DIR=%BUILD_ROOT%\%OUTPUT_ARCH%\%LIB_KIND%\%EFFECTIVE_RUNTIME%\%BUILD_TYPE%"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo ========================================
echo Building CYCoroutine
echo   Build Type : %BUILD_TYPE%
echo   Library    : %LIB_TYPE%
echo   Arch       : %OUTPUT_ARCH%
echo   CRT        : %EFFECTIVE_RUNTIME%
echo   Generator  : %GENERATOR%
echo   Build Dir  : %BUILD_DIR%
echo ========================================

pushd "%BUILD_DIR%" >nul
cmake -G "%GENERATOR%" -A %CMAKE_ARCH% ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DBUILD_SHARED_LIBS=%BUILD_SHARED% ^
    -DBUILD_STATIC_LIBS=%BUILD_STATIC% ^
    -DBUILD_EXAMPLES=%ENABLE_EXAMPLES% ^
    -DWINDOWS_RUNTIME=%EFFECTIVE_RUNTIME% ^
    "%SOURCE_DIR%"
if errorlevel 1 (
    popd >nul
    echo Error: CMake configuration failed.
    exit /b 1
)

cmake --build . --config %BUILD_TYPE% --target %TARGET_NAME%
if errorlevel 1 (
    popd >nul
    echo Error: Build failed.
    exit /b 1
)

if /I "%ENABLE_EXAMPLES%"=="ON" (
    cmake --build . --config %BUILD_TYPE% --target CYCoroutineExample
    if errorlevel 1 (
        popd >nul
        echo Error: Example build failed.
        exit /b 1
    )
)

popd >nul

echo.
echo Build completed successfully!
echo Output directory: %PROJECT_ROOT%\Bin\Windows\%OUTPUT_ARCH%\%EFFECTIVE_RUNTIME%\%BUILD_TYPE%
exit /b 0

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

:NormalizeLibType
set "VAR_NAME=%~1"
call set "CURRENT_VALUE=%%%VAR_NAME%%%"
if "%CURRENT_VALUE%"=="" set "CURRENT_VALUE=Static"

if /I "%CURRENT_VALUE%"=="STATIC" (
    set CURRENT_VALUE=Static
) else if /I "%CURRENT_VALUE%"=="SHARED" (
    set CURRENT_VALUE=Shared
) else (
    echo Invalid library type "%CURRENT_VALUE%". Use Static or Shared.
    exit /b 1
)

set "%VAR_NAME%=%CURRENT_VALUE%"
exit /b 0

:NormalizeArch
set "VAR_NAME=%~1"
call set "CURRENT_VALUE=%%%VAR_NAME%%%"
if "%CURRENT_VALUE%"=="" set "CURRENT_VALUE=x64"
set "CURRENT_VALUE=%CURRENT_VALUE:"=%"

if /I "%CURRENT_VALUE%"=="X64" (
    set CURRENT_VALUE=x64
    set OUTPUT_ARCH=x86_64
    set CMAKE_ARCH=x64
) else if /I "%CURRENT_VALUE%"=="X86_64" (
    set CURRENT_VALUE=x86_64
    set OUTPUT_ARCH=x86_64
    set CMAKE_ARCH=x64
) else if /I "%CURRENT_VALUE%"=="X86" (
    set CURRENT_VALUE=x86
    set OUTPUT_ARCH=x86
    set CMAKE_ARCH=Win32
) else if /I "%CURRENT_VALUE%"=="WIN32" (
    set CURRENT_VALUE=Win32
    set OUTPUT_ARCH=x86
    set CMAKE_ARCH=Win32
) else (
    echo Invalid architecture "%CURRENT_VALUE%". Use x64 or x86.
    exit /b 1
)

set "%VAR_NAME%=%CURRENT_VALUE%"
exit /b 0

:ValidateRuntime
set "VAR_NAME=%~1"
call set "CURRENT_VALUE=%%%VAR_NAME%%%"
if "%CURRENT_VALUE%"=="" set "CURRENT_VALUE=MD"
set "CURRENT_VALUE=%CURRENT_VALUE:"=%"

set "CURRENT_VALUE_UPPER=%CURRENT_VALUE%"
if /I "%CURRENT_VALUE_UPPER%"=="MT" (
    set CURRENT_VALUE=MT
) else if /I "%CURRENT_VALUE_UPPER%"=="MD" (
    set CURRENT_VALUE=MD
) else if /I "%CURRENT_VALUE_UPPER%"=="MTD" (
    set CURRENT_VALUE=MTD
) else if /I "%CURRENT_VALUE_UPPER%"=="MDD" (
    set CURRENT_VALUE=MDD
) else (
    echo Invalid CRT runtime "%CURRENT_VALUE%". Use MT, MD, MTD, or MDD.
    exit /b 1
)

set "%VAR_NAME%=%CURRENT_VALUE%"
exit /b 0

:ResolveRuntimeForConfig
set "CFG=%~1"
set "BASE=%~2"
set "OUT_VAR=%~3"

if /I "%CFG%"=="Debug" (
    if /I "%BASE%"=="MDD" (
        set RESULT=MDD
    ) else if /I "%BASE%"=="MTD" (
        set RESULT=MTD
    ) else if /I "%BASE%"=="MT" (
        set RESULT=MTD
    ) else (
        set RESULT=MDD
    )
) else if /I "%CFG%"=="Release" (
    if /I "%BASE%"=="MT" (
        set RESULT=MT
    ) else if /I "%BASE%"=="MDD" (
        echo Error: MDD is not valid for Release build. Use MD instead.
        exit /b 1
    ) else if /I "%BASE%"=="MTD" (
        echo Error: MTD is not valid for Release build. Use MT instead.
        exit /b 1
    ) else (
        set RESULT=MD
    )
) else (
    echo Unsupported build type "%CFG%".
    exit /b 1
)

set "%OUT_VAR%=%RESULT%"
exit /b 0

:SelectGenerator
set "GENERATOR=%CMAKE_GENERATOR%"
if defined GENERATOR (
    exit /b 0
)

for %%G in ("Visual Studio 17 2022" "Visual Studio 16 2019" "Visual Studio 15 2017") do (
    call :IsGeneratorAvailable "%%~G"
    if not errorlevel 1 (
        set "GENERATOR=%%~G"
        exit /b 0
    )
)

echo Error: No supported Visual Studio generator detected. Install Visual Studio C++ workloads or set CMAKE_GENERATOR.
exit /b 1

:IsGeneratorAvailable
set "GEN_NAME=%~1"
cmake --help | findstr /C:"%GEN_NAME%" >nul 2>&1
exit /b %ERRORLEVEL%

:EnsureVisualStudioEnvironment
set "REQUESTED_ARCH=%~1"
if "%REQUESTED_ARCH%"=="" set "REQUESTED_ARCH=x64"

set "CURRENT_ARCH=%VSCMD_ARG_TGT_ARCH%"
if defined CURRENT_ARCH (
    if /I "%CURRENT_ARCH%"=="amd64" set "CURRENT_ARCH=x64"
)

if defined VCINSTALLDIR (
    if /I "%CURRENT_ARCH%"=="%REQUESTED_ARCH%" (
        exit /b 0
    )
)

call :SetupVisualStudioEnvironment "%REQUESTED_ARCH%"
exit /b %ERRORLEVEL%

:SetupVisualStudioEnvironment
set "TARGET_VS_ARCH=%~1"
if "%TARGET_VS_ARCH%"=="" set "TARGET_VS_ARCH=x64"

set "VS_INSTALL_DIR="
call :FindVsInstallPath VS_INSTALL_DIR
if errorlevel 1 exit /b 1

set "VSDEV_CMD=%VS_INSTALL_DIR%\Common7\Tools\VsDevCmd.bat"
set "VCVARSALL=%VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvarsall.bat"

set "VCVARS_ARCH=%TARGET_VS_ARCH%"
if /I "%VCVARS_ARCH%"=="x64" set "VCVARS_ARCH=amd64"

if exist "%VSDEV_CMD%" (
    call "%VSDEV_CMD%" -no_logo -arch=%VCVARS_ARCH%
) else if exist "%VCVARSALL%" (
    call "%VCVARSALL%" %VCVARS_ARCH%
) else (
    echo Error: Unable to locate VsDevCmd.bat or vcvarsall.bat in "%VS_INSTALL_DIR%".
    exit /b 1
)

if errorlevel 1 (
    echo Error: Failed to initialize Visual Studio environment for %TARGET_VS_ARCH%.
    exit /b 1
)

echo Automatically configured Visual Studio environment (%TARGET_VS_ARCH%).
exit /b 0

:FindVsInstallPath
set "OUT_VAR=%~1"
call :TryVsInstallFromEnv VS_INSTALL_DIR
if defined VS_INSTALL_DIR (
    set "%OUT_VAR%=%VS_INSTALL_DIR%"
    exit /b 0
)

set "VSWHERE_EXE="
call :LocateVsWhere VSWHERE_EXE
if not errorlevel 1 (
    call :QueryVsWhere "%VSWHERE_EXE%" VS_INSTALL_DIR
    if defined VS_INSTALL_DIR (
        set "%OUT_VAR%=%VS_INSTALL_DIR%"
        exit /b 0
    )
)

call :SearchDefaultVsPaths VS_INSTALL_DIR
if defined VS_INSTALL_DIR (
    set "%OUT_VAR%=%VS_INSTALL_DIR%"
    exit /b 0
)

echo Error: Failed to locate any Visual Studio installation. Please install VS 2017+ with C++ workload or set VSINSTALLDIR.
exit /b 1

:TryVsInstallFromEnv
set "OUT_VAR=%~1"
set "CANDIDATE="

if defined VSINSTALLDIR (
    set "CANDIDATE=%VSINSTALLDIR%"
    call :ValidateVsInstallDir "%CANDIDATE%" VALIDATED_DIR
    if defined VALIDATED_DIR (
        set "%OUT_VAR%=%VALIDATED_DIR%"
        exit /b 0
    )
)

if defined VCINSTALLDIR (
    set "CANDIDATE=%VCINSTALLDIR%"
    for %%P in ("%CANDIDATE%\..") do set "CANDIDATE=%%~fP"
    call :ValidateVsInstallDir "%CANDIDATE%" VALIDATED_DIR
    if defined VALIDATED_DIR (
        set "%OUT_VAR%=%VALIDATED_DIR%"
        exit /b 0
    )
)

set "%OUT_VAR%="
exit /b 0

:QueryVsWhere
set "VSWHERE_EXE=%~1"
set "OUT_VAR=%~2"
set "%OUT_VAR%="

for %%A in (
    "-requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64"
    "-requires Microsoft.Component.MSBuild"
    ""
) do (
    set "ARGS=%%~A"
    if defined ARGS (
        for /f "usebackq tokens=* delims=" %%I in (`"%VSWHERE_EXE%" -latest -products * !ARGS! -property installationPath 2^>nul`) do (
            if not defined %OUT_VAR% (
                set "%OUT_VAR%=%%~I"
            )
        )
    ) else (
        for /f "usebackq tokens=* delims=" %%I in (`"%VSWHERE_EXE%" -latest -products * -property installationPath 2^>nul`) do (
            if not defined %OUT_VAR% (
                set "%OUT_VAR%=%%~I"
            )
        )
    )

    call set "CURRENT_DIR=%%%OUT_VAR%%%"
    if defined CURRENT_DIR (
        call :ValidateVsInstallDir "%%CURRENT_DIR%%" VALIDATED_DIR
        if defined VALIDATED_DIR (
            set "%OUT_VAR%=%VALIDATED_DIR%"
            exit /b 0
        ) else (
            set "%OUT_VAR%="
        )
    )
)

exit /b 0

:SearchDefaultVsPaths
set "OUT_VAR=%~1"
set "%OUT_VAR%="

for %%B in ("%ProgramFiles(x86)%\Microsoft Visual Studio" "%ProgramFiles%\Microsoft Visual Studio") do (
    if exist "%%~B" (
        for %%Y in (2022 2019 2017) do (
            for %%E in (BuildTools Community Professional Enterprise Preview) do (
                set "CAND=%%~B\%%~Y\%%~E"
                call :ValidateVsInstallDir "!CAND!" VALIDATED_DIR
                if defined VALIDATED_DIR (
                    set "%OUT_VAR%=!VALIDATED_DIR!"
                    exit /b 0
                )
            )
        )
    )
)

exit /b 0

:ValidateVsInstallDir
set "TARGET_PATH=%~1"
set "OUT_VAR=%~2"
set "%OUT_VAR%="

if "%TARGET_PATH%"=="" exit /b 0
if not exist "%TARGET_PATH%" exit /b 0

if exist "%TARGET_PATH%\Common7\Tools\VsDevCmd.bat" (
    set "%OUT_VAR%=%TARGET_PATH%"
    exit /b 0
)

if exist "%TARGET_PATH%\VC\Auxiliary\Build\vcvarsall.bat" (
    set "%OUT_VAR%=%TARGET_PATH%"
    exit /b 0
)

exit /b 0

:LocateVsWhere
set "OUT_VAR=%~1"
set "FOUND_PATH="

for %%P in ("%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" "%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe") do (
    if exist "%%~P" (
        set "FOUND_PATH=%%~P"
        goto :VsWhereLocated
    )
)

for /f "usebackq tokens=* delims=" %%I in (`where vswhere 2^>nul`) do (
    if not defined FOUND_PATH (
        set "FOUND_PATH=%%~I"
    )
)

:VsWhereLocated
if not defined FOUND_PATH (
    echo Error: vswhere.exe not found. Install Visual Studio 2017+ or add vswhere to PATH.
    exit /b 1
)

set "%OUT_VAR%=%FOUND_PATH%"
exit /b 0