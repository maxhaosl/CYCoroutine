@echo off
setlocal enableextensions enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."

set "ARCH_LIST=%~1"
if "%ARCH_LIST%"=="" set "ARCH_LIST=x64,x86"

set "LIB_LIST=%~2"
if "%LIB_LIST%"=="" set "LIB_LIST=Static"

set "BUILD_LIST=%~3"
if "%BUILD_LIST%"=="" set "BUILD_LIST=Release,Debug"

set "CRT_LIST=%~4"
if "%CRT_LIST%"=="" set "CRT_LIST=MD,MT"

set "ARCH_LIST=%ARCH_LIST:,= %"
set "LIB_LIST=%LIB_LIST:,= %"
set "BUILD_LIST=%BUILD_LIST:,= %"
set "CRT_LIST=%CRT_LIST:,= %"

echo ========================================
echo Building CYCoroutine for Windows (matrix)
echo Root: %PROJECT_ROOT%
echo Architectures: %ARCH_LIST%
echo Libraries   : %LIB_LIST%
echo Build types : %BUILD_LIST%
echo CRT runtimes: %CRT_LIST%
echo ========================================
echo.

for %%A in (%ARCH_LIST%) do (
    for %%L in (%LIB_LIST%) do (
        for %%C in (%BUILD_LIST%) do (
            for %%R in (%CRT_LIST%) do (
                if /I not "%%L"=="Shared" (
                    call :ValidateBuildRuntimeCombination "%%C" "%%R"
                    if not errorlevel 1 (
                        echo ---- Building %%A ^| %%L ^| %%C ^| %%R ----
                        call "%SCRIPT_DIR%build_windows.bat" %%C %%L %%A %%R
                        if errorlevel 1 (
                            echo Build failed for %%A / %%L / %%C / %%R
                            exit /b 1
                        )
                    ) else (
                        echo Skipping invalid combination: %%C with %%R
                    )
                ) else (
                    echo Skipping %%A ^| %%L ^| %%C ^| %%R (shared libraries are disabled)
                )
                echo.
            )
        )
    )
)

echo ========================================
echo All requested Windows builds finished
echo ========================================
goto :eof

:ValidateBuildRuntimeCombination
set "CFG=%~1"
set "RUNTIME=%~2"
set "RUNTIME=%RUNTIME:"=%"

if /I "%CFG%"=="Debug" (
    if /I "%RUNTIME%"=="MDD" exit /b 0
    if /I "%RUNTIME%"=="MTD" exit /b 0
    if /I "%RUNTIME%"=="MD" exit /b 0
    if /I "%RUNTIME%"=="MT" exit /b 0
    exit /b 1
) else if /I "%CFG%"=="Release" (
    if /I "%RUNTIME%"=="MD" exit /b 0
    if /I "%RUNTIME%"=="MT" exit /b 0
    exit /b 1
) else (
    exit /b 1
)

exit /b 0

