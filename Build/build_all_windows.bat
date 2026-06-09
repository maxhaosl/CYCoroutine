@echo off
setlocal enableextensions enabledelayedexpansion

REM Change to script directory first so all relative paths are correct
pushd "%~dp0" >nul 2>&1
if errorlevel 1 (
    echo Error: Could not change to script directory
    exit /b 1
)
set "SCRIPT_DIR=%cd%"

REM Resolve PROJECT_ROOT first (this normalizes the path even if it doesn't exist yet)
set "PROJECT_ROOT=%cd%\.."
for %%I in ("!PROJECT_ROOT!") do set "PROJECT_ROOT=%%~fI"

REM Now CYCommon is a sibling of PROJECT_ROOT
set "CYCOMMON_ROOT=%PROJECT_ROOT%\..\CYCommon"
for %%I in ("!CYCOMMON_ROOT!") do set "CYCOMMON_ROOT=%%~fI"
set "CYCOMMON_BUILD_SCRIPT=!CYCOMMON_ROOT!\Build\build_windows.bat"
popd >nul 2>&1

if not exist "!CYCOMMON_ROOT!\Build\CMakeLists.txt" (
    echo Error: CYCommon not found at "!CYCOMMON_ROOT!"
    exit /b 1
)

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

for %%A in (%ARCH_LIST%) do (
    for %%L in (%LIB_LIST%) do (
        for %%C in (%BUILD_LIST%) do (
            for %%R in (%CRT_LIST%) do (
                if /I not "%%L"=="Shared" (
                    call :ValidateBuildRuntimeCombination "%%C" "%%R"
                    if not errorlevel 1 (
                        call :EnsureCYCommon "%%A" "%%C" "%%R"
                        if errorlevel 1 (
                            echo CYCommon build failed for %%A / %%C / %%R
                            exit /b 1
                        )
                        echo ---- Building CYCoroutine %%A ^| %%L ^| %%C ^| %%R ----
                        call "!SCRIPT_DIR!\build_windows.bat" "!SCRIPT_DIR!" %%C %%L %%A %%R
                        if errorlevel 1 (
                            echo CYCoroutine build failed for %%A / %%L / %%C / %%R
                            exit /b 1
                        )
                        call :CopyCYCommonArtifacts "%%A" "%%C" "%%R"
                        if errorlevel 1 (
                            echo Failed to copy CYCommon artifacts for %%A / %%C / %%R
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

:NormalizeArchForCYCommon
set "INPUT_ARCH=%~1"
set "OUTPUT_VAR=%~2"

if /I "%INPUT_ARCH%"=="x64" (
    set "%OUTPUT_VAR%=x86_64"
) else if /I "%INPUT_ARCH%"=="x86_64" (
    set "%OUTPUT_VAR%=x86_64"
) else if /I "%INPUT_ARCH%"=="x86" (
    set "%OUTPUT_VAR%=x86"
) else if /I "%INPUT_ARCH%"=="Win32" (
    set "%OUTPUT_VAR%=x86"
) else (
    set "%OUTPUT_VAR%=%INPUT_ARCH%"
)

exit /b 0

:EnsureCYCommon
set "RAW_ARCH=%~1"
set "BUILD_CFG=%~2"
set "CRT_INPUT=%~3"
call :NormalizeArchForCYCommon "%RAW_ARCH%" CYCOMMON_ARCH
if errorlevel 1 exit /b 1

set "CHECK_RUNTIME=%CRT_INPUT%"
if /I "%BUILD_CFG%"=="Debug" (
    if /I "%CHECK_RUNTIME%"=="MD" set "CHECK_RUNTIME=MDD"
    if /I "%CHECK_RUNTIME%"=="MT" set "CHECK_RUNTIME=MTD"
)

if /I "%BUILD_CFG%"=="Debug" (
    set "CYCOMMON_LIB_NAME=CYCommonD.lib"
) else (
    set "CYCOMMON_LIB_NAME=CYCommon.lib"
)

set "CYCOMMON_LIB_PATH=%CYCOMMON_ROOT%\Bin\Windows\%CYCOMMON_ARCH%\%CHECK_RUNTIME%\%BUILD_CFG%\%CYCOMMON_LIB_NAME%"
if exist "%CYCOMMON_LIB_PATH%" (
    echo CYCommon already present: %CYCOMMON_LIB_NAME% for %CYCOMMON_ARCH% / %BUILD_CFG% / %CHECK_RUNTIME%
    exit /b 0
)

echo Building CYCommon for %CYCOMMON_ARCH% / %BUILD_CFG% / %CHECK_RUNTIME% ...
call "%CYCOMMON_BUILD_SCRIPT%" %BUILD_CFG% Static %CYCOMMON_ARCH% %CHECK_RUNTIME%
if errorlevel 1 (
    exit /b 1
)
exit /b 0

:CopyCYCommonArtifacts
set "RAW_ARCH=%~1"
set "BUILD_CFG=%~2"
set "CRT_INPUT=%~3"
call :NormalizeArchForCYCommon "%RAW_ARCH%" COPY_ARCH
if errorlevel 1 exit /b 1
set "COPY_RUNTIME=%CRT_INPUT%"
if /I "%BUILD_CFG%"=="Debug" (
    if /I "%COPY_RUNTIME%"=="MD" set "COPY_RUNTIME=MDD"
    if /I "%COPY_RUNTIME%"=="MT" set "COPY_RUNTIME=MTD"
)
set "SRC_DIR=%CYCOMMON_ROOT%\Bin\Windows\%COPY_ARCH%\%COPY_RUNTIME%\%BUILD_CFG%"
set "DST_DIR=%PROJECT_ROOT%\Bin\Windows\%COPY_ARCH%\%COPY_RUNTIME%\%BUILD_CFG%"
if not exist "%DST_DIR%" mkdir "%DST_DIR%" >nul 2>&1
if /I "%BUILD_CFG%"=="Debug" (
    copy /Y "%SRC_DIR%\CYCommonD.lib" "%DST_DIR%\" >nul 2>&1
    if errorlevel 1 exit /b 1
    copy /Y "%SRC_DIR%\CYCommonD.pdb" "%DST_DIR%\" >nul 2>&1
    if errorlevel 1 exit /b 1
) else (
    copy /Y "%SRC_DIR%\CYCommon.lib" "%DST_DIR%\" >nul 2>&1
    if errorlevel 1 exit /b 1
    copy /Y "%SRC_DIR%\CYCommon.pdb" "%DST_DIR%\" >nul 2>&1
    if errorlevel 1 exit /b 1
)
exit /b 0

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

