cmake_minimum_required(VERSION 3.16)
project(CYCoroutine VERSION 1.0.0 LANGUAGES CXX)

# /FS - Force synchronous PDB writes (required when multiple cl.exe write to same PDB)
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /FS")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /FS")
endif()

# ---------------------------------------------------------------------------
# CYCommon dependency
# CYCommon is a sibling library under the same ThirdParty root:
#   ThirdParty/
#     CYCommon/     <- CYCOMMON_ROOT_DIR
#     CYCoroutine/  <- CYCOROUTINE_ROOT_DIR (this project)
# ---------------------------------------------------------------------------
if(NOT DEFINED CYCOMMON_ROOT_DIR)
    get_filename_component(CYCOMMON_ROOT_DIR "${CMAKE_SOURCE_DIR}/../../CYCommon" ABSOLUTE)
endif()
if(NOT EXISTS "${CYCOMMON_ROOT_DIR}/Build/CMakeLists.txt")
    message(FATAL_ERROR "CYCommon not found at ${CYCOMMON_ROOT_DIR}. "
        "Ensure CYCommon is checked out as a sibling of CYCoroutine under ThirdParty/.")
endif()

# CYCOMMON_INCLUDE_DIR is the same for all platforms - set it once here.
if(NOT DEFINED CYCOMMON_INCLUDE_DIR)
    set(CYCOMMON_INCLUDE_DIR "${CYCOMMON_ROOT_DIR}/Inc")
endif()

# Check if the calling build script passed CYCOMMON_LIB_PATH as a CMake -D argument
# (e.g., from CYLogger's build_macos.sh deps build).
if(DEFINED CYCOMMON_LIB_PATH AND EXISTS "${CYCOMMON_LIB_PATH}")
    message(STATUS "Using CYCommon library from CYCOMMON_LIB_PATH (cmake var): ${CYCOMMON_LIB_PATH}")
    set(_CYCOMMON_STATIC_LIB "${CYCOMMON_LIB_PATH}")
    set(CYCOMMON_INCLUDE_DIR "${CYCOMMON_ROOT_DIR}/Inc")
    set(_CYCOMMON_NEED_BUILD FALSE)
    set(_CYCOMMON_PATH_FROM_ENV TRUE)
# Also check CYCOMMON_LIB_PATH from the shell environment (set by build scripts).
elseif(DEFINED ENV{CYCOMMON_LIB_PATH})
    set(_CYCOMMON_LIB_PATH_FROM_ENV "$ENV{CYCOMMON_LIB_PATH}")
    if(EXISTS "${_CYCOMMON_LIB_PATH_FROM_ENV}")
        message(STATUS "Using CYCommon library from CYCOMMON_LIB_PATH (env): ${_CYCOMMON_LIB_PATH_FROM_ENV}")
        set(_CYCOMMON_STATIC_LIB "${_CYCOMMON_LIB_PATH_FROM_ENV}")
        set(CYCOMMON_INCLUDE_DIR "${CYCOMMON_ROOT_DIR}/Inc")
        set(_CYCOMMON_NEED_BUILD FALSE)
        set(_CYCOMMON_PATH_FROM_ENV TRUE)
    else()
        message(WARNING "CYCOMMON_LIB_PATH env is set to '${_CYCOMMON_LIB_PATH_FROM_ENV}' but the file does not exist. Will search for CYCommon library.")
    endif()
endif()

# Convert CMAKE_BUILD_TYPE to lowercase for directory lookup
string(TOLOWER "${CMAKE_BUILD_TYPE}" _CYCOMMON_BUILD_TYPE_LOWER)

# Default: assume we need to build CYCommon if library not found
set(_CYCOMMON_NEED_BUILD TRUE)

# Check Android platform first
if(ANDROID)
    # Debug builds use libCYCommonD.a on Android to match CYCommon's OUTPUT_NAME
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_CYCOMMON_LIB_NAME "libCYCommonD.a")
    else()
        set(_CYCOMMON_LIB_NAME "libCYCommon.a")
    endif()
    set(_CYCOMMON_EXPECTED_LIB "${CYCOMMON_ROOT_DIR}/Bin/Android/${CMAKE_ANDROID_ARCH_ABI}/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LIB_NAME}")
    if(EXISTS "${_CYCOMMON_EXPECTED_LIB}")
        message(STATUS "Found CYCommon library (Android): ${_CYCOMMON_EXPECTED_LIB}")
        set(_CYCOMMON_STATIC_LIB "${_CYCOMMON_EXPECTED_LIB}")
        set(_CYCOMMON_NEED_BUILD FALSE)
    endif()
elseif(APPLE)
    # macOS/iOS platform
    # For iOS, prefer TARGET_ARCH (which distinguishes arm64 device from arm64-simulator).
    # For macOS, fall back to CMAKE_OSX_ARCHITECTURES or host arch.
    if(IOS)
        if(DEFINED TARGET_ARCH AND NOT "${TARGET_ARCH}" STREQUAL "")
            set(_CYCOMMON_ARCH_PATH "${TARGET_ARCH}")
        elseif(DEFINED CMAKE_OSX_ARCHITECTURES AND NOT CMAKE_OSX_ARCHITECTURES STREQUAL "")
            set(_CYCOMMON_ARCH_PATH "${CMAKE_OSX_ARCHITECTURES}")
        else()
            set(_CYCOMMON_ARCH_PATH "arm64")
        endif()
        set(_CYCOMMON_LIB_NAME "libCYCommon.a")
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(_CYCOMMON_LIB_NAME "libCYCommonD.a")
        endif()
        # Check CYCommon's own Bin/iOS directory
        set(_CYCOMMON_EXPECTED_LIB "${CYCOMMON_ROOT_DIR}/Bin/iOS/${_CYCOMMON_ARCH_PATH}/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LIB_NAME}")
        if((NOT BUILD_SHARED_LIBS OR IOS) AND EXISTS "${_CYCOMMON_EXPECTED_LIB}")
            message(STATUS "Found CYCommon library (iOS): ${_CYCOMMON_EXPECTED_LIB}")
            set(_CYCOMMON_STATIC_LIB "${_CYCOMMON_EXPECTED_LIB}")
            set(_CYCOMMON_NEED_BUILD FALSE)
        endif()
    else()
        # macOS
        set(_CYCOMMON_ARCH_PATH "x86_64")
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            set(_CYCOMMON_ARCH_PATH "x86")
        endif()
        if(DEFINED CMAKE_OSX_ARCHITECTURES AND NOT CMAKE_OSX_ARCHITECTURES STREQUAL "")
            set(_CYCOMMON_ARCH_PATH "${CMAKE_OSX_ARCHITECTURES}")
        endif()
        set(_CYCOMMON_LIB_NAME "libCYCommon.a")
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(_CYCOMMON_LIB_NAME "libCYCommonD.a")
        endif()
        # Check CYCommon's Bin/MacOS directory
        set(_CYCOMMON_EXPECTED_LIB "${CYCOMMON_ROOT_DIR}/Bin/MacOS/${_CYCOMMON_ARCH_PATH}/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LIB_NAME}")
        if(BUILD_STATIC_LIBS AND EXISTS "${_CYCOMMON_EXPECTED_LIB}")
            message(STATUS "Found CYCommon library (macOS): ${_CYCOMMON_EXPECTED_LIB}")
            set(_CYCOMMON_STATIC_LIB "${_CYCOMMON_EXPECTED_LIB}")
            set(_CYCOMMON_NEED_BUILD FALSE)
        endif()
    endif()
elseif(UNIX AND NOT APPLE)
    # Linux platform
    set(_CYCOMMON_ARCH_PATH "x86_64")
    if(DEFINED TARGET_ARCH AND NOT "${TARGET_ARCH}" STREQUAL "")
        set(_CYCOMMON_ARCH_PATH "${TARGET_ARCH}")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(_CYCOMMON_ARCH_PATH "x86")
    endif()
    
    # Debug builds use libCYCommonD.a, Release builds use libCYCommon.a
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_CYCOMMON_LIB_NAME "libCYCommonD.a")
    else()
        set(_CYCOMMON_LIB_NAME "libCYCommon.a")
    endif()
    
    string(TOLOWER "${CMAKE_BUILD_TYPE}" _CYCOMMON_BT_LOWER)
    if(DEFINED CYLOGGER_ROOT_DIR)
        set(_CYCOMMON_LOGGER_LINUX_LIB "${CYLOGGER_ROOT_DIR}/Bin/Linux/${_CYCOMMON_ARCH_PATH}/${_CYCOMMON_BT_LOWER}/${_CYCOMMON_LIB_NAME}")
        if(EXISTS "${_CYCOMMON_LOGGER_LINUX_LIB}")
            message(STATUS "Found CYCommon library in CYLogger Bin (Linux): ${_CYCOMMON_LOGGER_LINUX_LIB}")
            set(_CYCOMMON_STATIC_LIB "${_CYCOMMON_LOGGER_LINUX_LIB}")
            set(_CYCOMMON_NEED_BUILD FALSE)
        endif()
    endif()
    # Fallback: CYCommon's own Bin/Linux tree (Release/Debug subdirs)
    if(_CYCOMMON_NEED_BUILD)
        set(_CYCOMMON_EXPECTED_LIB "${CYCOMMON_ROOT_DIR}/Bin/Linux/${_CYCOMMON_ARCH_PATH}/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LIB_NAME}")
        if(EXISTS "${_CYCOMMON_EXPECTED_LIB}")
            message(STATUS "Found CYCommon library (Linux): ${_CYCOMMON_EXPECTED_LIB}")
            set(_CYCOMMON_STATIC_LIB "${_CYCOMMON_EXPECTED_LIB}")
            set(_CYCOMMON_NEED_BUILD FALSE)
        endif()
    endif()
endif()  # Close inner platform detection block

# Second search block: Try CYLogger's Bin/iOS path (for CYLogger deps builds).
if(NOT DEFINED _CYCOMMON_PATH_FROM_ENV)
    set(_CYCOMMON_ARCH_PATH "x86_64")
    if(DEFINED TARGET_ARCH AND NOT "${TARGET_ARCH}" STREQUAL "")
        if(TARGET_ARCH STREQUAL "x86" OR TARGET_ARCH STREQUAL "Win32")
            set(_CYCOMMON_ARCH_PATH "x86")
        elseif(TARGET_ARCH STREQUAL "x64" OR TARGET_ARCH STREQUAL "x86_64")
            set(_CYCOMMON_ARCH_PATH "x86_64")
        else()
            set(_CYCOMMON_ARCH_PATH "${TARGET_ARCH}")
        endif()
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(_CYCOMMON_ARCH_PATH "x86")
    endif()

    set(_CYCOMMON_RUNTIME_FOLDER "${WINDOWS_RUNTIME}")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(_CYCOMMON_RUNTIME_FOLDER STREQUAL "MD")
            set(_CYCOMMON_RUNTIME_FOLDER "MDD")
        elseif(_CYCOMMON_RUNTIME_FOLDER STREQUAL "MT")
            set(_CYCOMMON_RUNTIME_FOLDER "MTD")
        endif()
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_CYCOMMON_LIB_NAME "CYCommonD.lib")
    else()
        set(_CYCOMMON_LIB_NAME "CYCommon.lib")
    endif()
endif()  # Close guard: if(NOT DEFINED _CYCOMMON_PATH_FROM_ENV)

if(NOT ANDROID AND NOT _CYCOMMON_PATH_FROM_ENV)
    # Check CYLogger's Bin directory first (where build_windows_all.bat outputs CYCommon)
    if(DEFINED CYLOGGER_ROOT_DIR)
        # Try both x64 and x86_64 naming conventions
        set(_CYCOMMON_LOGGER_BIN_LIB "${CYLOGGER_ROOT_DIR}/Bin/Windows/${TARGET_ARCH}/${_CYCOMMON_RUNTIME_FOLDER}/${_CYCOMMON_BUILD_TYPE_LOWER}/${_CYCOMMON_LIB_NAME}")
        if(NOT EXISTS "${_CYCOMMON_LOGGER_BIN_LIB}")
            # Try alternative architecture name
            if(TARGET_ARCH STREQUAL "x64")
                set(_CYCOMMON_LOGGER_BIN_LIB "${CYLOGGER_ROOT_DIR}/Bin/Windows/x86_64/${_CYCOMMON_RUNTIME_FOLDER}/${_CYCOMMON_BUILD_TYPE_LOWER}/${_CYCOMMON_LIB_NAME}")
            else()
                set(_CYCOMMON_LOGGER_BIN_LIB "${CYLOGGER_ROOT_DIR}/Bin/Windows/x64/${_CYCOMMON_RUNTIME_FOLDER}/${_CYCOMMON_BUILD_TYPE_LOWER}/${_CYCOMMON_LIB_NAME}")
            endif()
        endif()
        if(BUILD_STATIC_LIBS AND EXISTS "${_CYCOMMON_LOGGER_BIN_LIB}")
            message(STATUS "Found CYCommon library in CYLogger Bin: ${_CYCOMMON_LOGGER_BIN_LIB}")
            set(_CYCOMMON_STATIC_LIB "${_CYCOMMON_LOGGER_BIN_LIB}")
            set(_CYCOMMON_NEED_BUILD FALSE)
        endif()
    endif()

    # Fallback: Check CYCommon's own Bin directory
    if(_CYCOMMON_NEED_BUILD)
        set(_CYCOMMON_EXPECTED_LIB "${CYCOMMON_ROOT_DIR}/Bin/Windows/${_CYCOMMON_ARCH_PATH}/${_CYCOMMON_RUNTIME_FOLDER}/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LIB_NAME}")
        if(BUILD_STATIC_LIBS AND EXISTS "${_CYCOMMON_EXPECTED_LIB}")
            message(STATUS "Found CYCommon library: ${_CYCOMMON_EXPECTED_LIB}")
            set(_CYCOMMON_STATIC_LIB "${_CYCOMMON_EXPECTED_LIB}")
            set(_CYCOMMON_NEED_BUILD FALSE)
        else()
            # Try alternative architecture path (x86_64 vs x64 naming)
            set(_CYCOMMON_ARCH_PATH_ALT "x86_64")
            if(_CYCOMMON_ARCH_PATH STREQUAL "x86_64")
                set(_CYCOMMON_ARCH_PATH_ALT "x64")
            endif()
            set(_CYCOMMON_EXPECTED_LIB_ALT "${CYCOMMON_ROOT_DIR}/Bin/Windows/${_CYCOMMON_ARCH_PATH_ALT}/${_CYCOMMON_RUNTIME_FOLDER}/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LIB_NAME}")
            if(BUILD_STATIC_LIBS AND EXISTS "${_CYCOMMON_EXPECTED_LIB_ALT}")
                message(STATUS "Found CYCommon library (alt path): ${_CYCOMMON_EXPECTED_LIB_ALT}")
                set(_CYCOMMON_STATIC_LIB "${_CYCOMMON_EXPECTED_LIB_ALT}")
                set(_CYCOMMON_NEED_BUILD FALSE)
            endif()
        endif()
    endif()

    if(_CYCOMMON_NEED_BUILD)
        message(STATUS "CYCommon library not found; building it as part of CYCoroutine")

        # Forward matching build options to CYCommon
        # CYCommon's build scripts expect x86_64 for 64-bit, not x64
        if(_CYCOMMON_ARCH_PATH STREQUAL "x64")
            set(_CYCOMMON_ARCH_ARG "x86_64")
        else()
            set(_CYCOMMON_ARCH_ARG "${_CYCOMMON_ARCH_PATH}")
        endif()
        
        if(WIN32)
            # Use forward slashes for CMake, they work on Windows
            set(_CYCOMMON_BUILD_CMD "${CYCOMMON_ROOT_DIR}/Build/build_windows.bat")
            execute_process(
                COMMAND cmd /c "${_CYCOMMON_BUILD_CMD} ${CMAKE_BUILD_TYPE} Static ${_CYCOMMON_ARCH_ARG} ${WINDOWS_RUNTIME}"
                WORKING_DIRECTORY "${CYCOMMON_ROOT_DIR}/Build"
                RESULT_VARIABLE _CYCOMMON_BUILD_RESULT
                TIMEOUT 600
            )
        elseif(APPLE AND NOT IOS)
            # macOS uses build_mac.sh (single-arch build helper).
            # Pass the existing CYCommon library path so CMake can find it without searching.
            # The library is already at CYCommon/Bin/MacOS/<arch>/<BT>/libCYCommon[|D].a
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                set(_CYCOMMON_LIB_FILE "libCYCommonD.a")
            else()
                set(_CYCOMMON_LIB_FILE "libCYCommon.a")
            endif()
            set(_CYCOMMON_LIB_PATH "${CYCOMMON_ROOT_DIR}/Bin/MacOS/${_CYCOMMON_ARCH_ARG}/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LIB_FILE}")
            execute_process(
                COMMAND "${CYCOMMON_ROOT_DIR}/Build/build_mac.sh" "${CMAKE_BUILD_TYPE}" OFF "${_CYCOMMON_ARCH_ARG}" "" "${_CYCOMMON_LIB_PATH}"
                WORKING_DIRECTORY "${CYCOMMON_ROOT_DIR}/Build"
                RESULT_VARIABLE _CYCOMMON_BUILD_RESULT
                TIMEOUT 600
            )
        elseif(IOS)
            # iOS: use build_all_ios.sh via bash to build CYCommon.
            # TARGET_ARCH distinguishes arm64 device from arm64-simulator.
            # CYCOMMON_OUTPUT_ARCH must be set so CYCommon outputs to the correct arch subdirectory.
            if(DEFINED TARGET_ARCH AND NOT "${TARGET_ARCH}" STREQUAL "")
                set(_CYCOMMON_IOS_ARCH "${TARGET_ARCH}")
            elseif(DEFINED CMAKE_OSX_ARCHITECTURES AND NOT "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "")
                set(_CYCOMMON_IOS_ARCH "${CMAKE_OSX_ARCHITECTURES}")
            else()
                set(_CYCOMMON_IOS_ARCH "arm64")
            endif()
            execute_process(
                COMMAND /bin/bash "${CYCOMMON_ROOT_DIR}/Build/build_all_ios.sh" "${CMAKE_BUILD_TYPE}" OFF "${_CYCOMMON_IOS_ARCH}"
                WORKING_DIRECTORY "${CYCOMMON_ROOT_DIR}/Build"
                RESULT_VARIABLE _CYCOMMON_BUILD_RESULT
                TIMEOUT 600
            )
        else()
            # Linux: prefer CYLogger Bin/Linux/<arch>/<config>/ when integrated
            set(_CYCOMMON_LINUX_BUILD_CMD "${CMAKE_COMMAND}" -E env)
            if(DEFINED CYLOGGER_ROOT_DIR)
                string(TOLOWER "${CMAKE_BUILD_TYPE}" _CYLOGGER_BT_LOWER)
                list(APPEND _CYCOMMON_LINUX_BUILD_CMD
                    "CYCOMMON_OUTPUT_DIR=${CYLOGGER_ROOT_DIR}/Bin/Linux/${_CYCOMMON_ARCH_ARG}/${_CYLOGGER_BT_LOWER}")
            endif()
            list(APPEND _CYCOMMON_LINUX_BUILD_CMD
                "${CYCOMMON_ROOT_DIR}/Build/build_linux.sh"
                "${CMAKE_BUILD_TYPE}"
                "${_CYCOMMON_ARCH_ARG}")
            execute_process(
                COMMAND ${_CYCOMMON_LINUX_BUILD_CMD}
                WORKING_DIRECTORY "${CYCOMMON_ROOT_DIR}/Build"
                RESULT_VARIABLE _CYCOMMON_BUILD_RESULT
                TIMEOUT 600
            )
        endif()

        if(_CYCOMMON_BUILD_RESULT AND NOT _CYCOMMON_BUILD_RESULT EQUAL 0)
            message(FATAL_ERROR "CYCommon build failed with exit code ${_CYCOMMON_BUILD_RESULT}. "
                "Build CYCommon manually before building CYCoroutine.")
        endif()
    endif()
endif()

# Skip the second search block too if CYCOMMON_LIB_PATH was already provided
# or if a valid library was already found by the first search block.
if(NOT _CYCOMMON_PATH_FROM_ENV AND NOT _CYCOMMON_STATIC_LIB)
# Locate CYCommon's include directory and library
set(CYCOMMON_INCLUDE_DIR "${CYCOMMON_ROOT_DIR}/Inc")
if(NOT EXISTS "${CYCOMMON_INCLUDE_DIR}")
    message(FATAL_ERROR "CYCommon include directory not found: ${CYCOMMON_INCLUDE_DIR}")
endif()

# Find CYCommon's library files
set(_CYCOMMON_LIB_DIR "${CYCOMMON_ROOT_DIR}/Bin")

# Check if CYLogger's Bin directory has CYCommon library (preferred location)
# This avoids triggering rebuild of CYCommon when it was built by CYLogger's build script
if(WIN32 AND DEFINED CYLOGGER_ROOT_DIR)
    set(_CYCOMMON_LOGGER_BIN "${CYLOGGER_ROOT_DIR}/Bin/Windows/${TARGET_ARCH}/${WINDOWS_RUNTIME}/${CMAKE_BUILD_TYPE}")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_CYCOMMON_LOGGER_BIN_LIB "${_CYCOMMON_LOGGER_BIN}/CYCommonD.lib")
    else()
        set(_CYCOMMON_LOGGER_BIN_LIB "${_CYCOMMON_LOGGER_BIN}/CYCommon.lib")
    endif()
    if(EXISTS "${_CYCOMMON_LOGGER_BIN_LIB}")
        set(_CYCOMMON_STATIC_LIB "${_CYCOMMON_LOGGER_BIN_LIB}")
        message(STATUS "CYCommon library found in CYLogger Bin: ${_CYCOMMON_STATIC_LIB}")
    endif()
endif()

# Determine platform and architecture specific path
if(WIN32)
    # Windows logic
    # Prefer explicit TARGET_ARCH from build scripts so standalone CYCoroutine builds
    # resolve the same architecture naming as CYCommon outputs.
    if(DEFINED TARGET_ARCH AND NOT "${TARGET_ARCH}" STREQUAL "")
        if(TARGET_ARCH STREQUAL "x64")
            set(_CYCOMMON_ARCH_PATH "x86_64")
            set(_CYCOMMON_ARCH_PATH_ALT "x64")
        elseif(TARGET_ARCH STREQUAL "x86_64")
            set(_CYCOMMON_ARCH_PATH "x86_64")
            set(_CYCOMMON_ARCH_PATH_ALT "x64")
        elseif(TARGET_ARCH STREQUAL "Win32")
            set(_CYCOMMON_ARCH_PATH "x86")
            set(_CYCOMMON_ARCH_PATH_ALT "x86")
        elseif(TARGET_ARCH STREQUAL "x86")
            set(_CYCOMMON_ARCH_PATH "x86")
            set(_CYCOMMON_ARCH_PATH_ALT "x86")
        else()
            set(_CYCOMMON_ARCH_PATH "${TARGET_ARCH}")
            set(_CYCOMMON_ARCH_PATH_ALT "x86_64")
        endif()
    else()
        set(_CYCOMMON_ARCH_PATH "x64")
        set(_CYCOMMON_ARCH_PATH_ALT "x86_64")  # Alternative naming used by CYCommon
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            set(_CYCOMMON_ARCH_PATH "x86")
            set(_CYCOMMON_ARCH_PATH_ALT "x86")
        endif()
    endif()

    set(_CYCOMMON_RUNTIME_FOLDER "${WINDOWS_RUNTIME}")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(_CYCOMMON_RUNTIME_FOLDER STREQUAL "MD")
            set(_CYCOMMON_RUNTIME_FOLDER "MDD")
        elseif(_CYCOMMON_RUNTIME_FOLDER STREQUAL "MT")
            set(_CYCOMMON_RUNTIME_FOLDER "MTD")
        endif()
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_CYCOMMON_LIB_NAME "CYCommonD.lib")
    else()
        set(_CYCOMMON_LIB_NAME "CYCommon.lib")
    endif()
    set(_CYCOMMON_EXPECTED_LIB "${_CYCOMMON_LIB_DIR}/Windows/${_CYCOMMON_ARCH_PATH}/${_CYCOMMON_RUNTIME_FOLDER}/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LIB_NAME}")

    if(EXISTS "${_CYCOMMON_EXPECTED_LIB}")
        set(_CYCOMMON_STATIC_LIB "${_CYCOMMON_EXPECTED_LIB}")
        message(STATUS "CYCommon static library (matched): ${_CYCOMMON_STATIC_LIB}")
    else()
        # Try alternative architecture path (x86_64 vs x64 naming)
        set(_CYCOMMON_EXPECTED_LIB_ALT "${_CYCOMMON_LIB_DIR}/Windows/${_CYCOMMON_ARCH_PATH_ALT}/${_CYCOMMON_RUNTIME_FOLDER}/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LIB_NAME}")
        if(EXISTS "${_CYCOMMON_EXPECTED_LIB_ALT}")
            set(_CYCOMMON_STATIC_LIB "${_CYCOMMON_EXPECTED_LIB_ALT}")
            message(STATUS "CYCommon static library (alt path): ${_CYCOMMON_STATIC_LIB}")
        else()
            set(_CYCOMMON_KNOWN_PATHS
                "${_CYCOMMON_LIB_DIR}/Windows/x64/MD/Release/CYCommon.lib"
                "${_CYCOMMON_LIB_DIR}/Windows/x64/MT/Release/CYCommon.lib"
                "${_CYCOMMON_LIB_DIR}/Windows/x86/MD/Release/CYCommon.lib"
                "${_CYCOMMON_LIB_DIR}/Windows/x86/MT/Release/CYCommon.lib"
                "${_CYCOMMON_LIB_DIR}/Windows/x86_64/MD/Release/CYCommon.lib"
                "${_CYCOMMON_LIB_DIR}/Windows/x86_64/MT/Release/CYCommon.lib"
                "${_CYCOMMON_LIB_DIR}/Windows/x64/MDD/Debug/CYCommonD.lib"
                "${_CYCOMMON_LIB_DIR}/Windows/x64/MTD/Debug/CYCommonD.lib"
                "${_CYCOMMON_LIB_DIR}/Windows/x86/MDD/Debug/CYCommonD.lib"
                "${_CYCOMMON_LIB_DIR}/Windows/x86/MTD/Debug/CYCommonD.lib"
                "${_CYCOMMON_LIB_DIR}/Windows/x86_64/MDD/Debug/CYCommonD.lib"
                "${_CYCOMMON_LIB_DIR}/Windows/x86_64/MTD/Debug/CYCommonD.lib"
            )
            foreach(_lib_path ${_CYCOMMON_KNOWN_PATHS})
                if(EXISTS "${_lib_path}")
                    set(_CYCOMMON_STATIC_LIB "${_lib_path}")
                    message(WARNING "CYCommon library path mismatch. Expected: ${_CYCOMMON_EXPECTED_LIB}, Using: ${_CYCOMMON_STATIC_LIB}.")
                    break()
                endif()
            endforeach()
        endif()
    endif()
elseif(APPLE)
    # macOS/iOS logic
    if(IOS)
        set(_PLATFORM "iOS")
    else()
        set(_PLATFORM "macOS")
    endif()

    # Get the architecture from TARGET_ARCH (preferred, set by build script) or CMAKE_OSX_ARCHITECTURES.
    # TARGET_ARCH is preferred because it distinguishes arm64 (device) from arm64-simulator
    # (both map to CMAKE_OSX_ARCHITECTURES=arm64 but have different TARGET_ARCH values).
    if(DEFINED TARGET_ARCH AND NOT "${TARGET_ARCH}" STREQUAL "")
        set(_ARCH "${TARGET_ARCH}")
    elseif(DEFINED CMAKE_OSX_ARCHITECTURES AND NOT "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "")
        set(_ARCH "${CMAKE_OSX_ARCHITECTURES}")
    endif()
    if(NOT _ARCH)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(_ARCH "arm64")
        else()
            set(_ARCH "x86_64")
        endif()
    endif()

    if(IOS)
        # iOS specific logic - distinguish between device and simulator using TARGET_ARCH
        # TARGET_ARCH is "arm64" for device, "arm64-simulator" for simulator ARM, "x86_64" for simulator x86.
        if(_ARCH STREQUAL "arm64")
            # Could be device or arm64-simulator. Distinguish by CMAKE_OSX_SYSROOT.
            if(CMAKE_OSX_SYSROOT MATCHES "iPhoneOS\.platform")
                # Device build
                set(_CYCOMMON_EXPECTED_LIB "${_CYCOMMON_LIB_DIR}/${_PLATFORM}/arm64/${CMAKE_BUILD_TYPE}/libCYCommon.a")
                if(NOT EXISTS "${_CYCOMMON_EXPECTED_LIB}")
                    set(_CYCOMMON_EXPECTED_LIB "${_CYCOMMON_LIB_DIR}/${_PLATFORM}/device/${CMAKE_BUILD_TYPE}/libCYCommon.a")
                endif()
            else()
                # Simulator ARM build (arm64-simulator)
                set(_CYCOMMON_EXPECTED_LIB "${_CYCOMMON_LIB_DIR}/${_PLATFORM}/arm64-simulator/${CMAKE_BUILD_TYPE}/libCYCommon.a")
                if(NOT EXISTS "${_CYCOMMON_EXPECTED_LIB}")
                    set(_CYCOMMON_EXPECTED_LIB "${_CYCOMMON_LIB_DIR}/${_PLATFORM}/simulator/${CMAKE_BUILD_TYPE}/libCYCommon.a")
                endif()
            endif()
        elseif(_ARCH STREQUAL "arm64-simulator")
            # arm64-simulator target arch (explicit)
            set(_CYCOMMON_EXPECTED_LIB "${_CYCOMMON_LIB_DIR}/${_PLATFORM}/arm64-simulator/${CMAKE_BUILD_TYPE}/libCYCommon.a")
            if(NOT EXISTS "${_CYCOMMON_EXPECTED_LIB}")
                set(_CYCOMMON_EXPECTED_LIB "${_CYCOMMON_LIB_DIR}/${_PLATFORM}/simulator/${CMAKE_BUILD_TYPE}/libCYCommon.a")
            endif()
        else()
            # x86_64 simulator
            set(_CYCOMMON_EXPECTED_LIB "${_CYCOMMON_LIB_DIR}/${_PLATFORM}/x86_64/${CMAKE_BUILD_TYPE}/libCYCommon.a")
            if(NOT EXISTS "${_CYCOMMON_EXPECTED_LIB}")
                set(_CYCOMMON_EXPECTED_LIB "${_CYCOMMON_LIB_DIR}/${_PLATFORM}/simulator/${CMAKE_BUILD_TYPE}/libCYCommon.a")
            endif()
        endif()
        # Debug builds use libCYCommonD.a
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            string(REPLACE "libCYCommon.a" "libCYCommonD.a" _CYCOMMON_EXPECTED_LIB "${_CYCOMMON_EXPECTED_LIB}")
        endif()
    else()
        # macOS
        # Determine architecture for fallback search - must match what CYCommon/Bin/MacOS/ uses
        if(DEFINED TARGET_ARCH AND NOT "${TARGET_ARCH}" STREQUAL "")
            set(_ARCH "${TARGET_ARCH}")
        elseif(DEFINED CMAKE_OSX_ARCHITECTURES AND NOT "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "")
            set(_ARCH "${CMAKE_OSX_ARCHITECTURES}")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(_ARCH "arm64")
        else()
            set(_ARCH "x86_64")
        endif()
        set(_CYCOMMON_EXPECTED_LIB "${_CYCOMMON_LIB_DIR}/${_PLATFORM}/${_ARCH}/${CMAKE_BUILD_TYPE}/libCYCommon.a")
    endif()
    
    if(EXISTS "${_CYCOMMON_EXPECTED_LIB}")
        set(_CYCOMMON_STATIC_LIB "${_CYCOMMON_EXPECTED_LIB}")
        message(STATUS "CYCommon static library (matched): ${_CYCOMMON_STATIC_LIB}")
    else()
        # Fallback to other possible paths
        # Build debug/release lib name
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(_CYCOMMON_LIB_FALLBACK_NAME "libCYCommonD.a")
        else()
            set(_CYCOMMON_LIB_FALLBACK_NAME "libCYCommon.a")
        endif()
        if(IOS)
            set(_CYCOMMON_KNOWN_PATHS
                "${_CYCOMMON_LIB_DIR}/${_PLATFORM}/device/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LIB_FALLBACK_NAME}"
                "${_CYCOMMON_LIB_DIR}/${_PLATFORM}/simulator/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LIB_FALLBACK_NAME}"
                "${_CYCOMMON_LIB_DIR}/${_PLATFORM}/universal/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LIB_FALLBACK_NAME}"
                "${_CYCOMMON_LIB_DIR}/${_PLATFORM}/arm64-simulator/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LIB_FALLBACK_NAME}"
                "${_CYCOMMON_LIB_DIR}/${_PLATFORM}/x86_64/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LIB_FALLBACK_NAME}"
                "${_CYCOMMON_LIB_DIR}/${_PLATFORM}/arm64/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LIB_FALLBACK_NAME}"
            )
        else()
            # macOS fallback: search in order that matches expected arch first
            set(_CYCOMMON_KNOWN_PATHS
                "${_CYCOMMON_LIB_DIR}/MacOS/${_ARCH}/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LIB_FALLBACK_NAME}"
                "${_CYCOMMON_LIB_DIR}/MacOS/x86_64/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LIB_FALLBACK_NAME}"
                "${_CYCOMMON_LIB_DIR}/MacOS/arm64/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LIB_FALLBACK_NAME}"
                "${_CYCOMMON_LIB_DIR}/MacOS/universal/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LIB_FALLBACK_NAME}"
            )
        endif()
        foreach(_lib_path ${_CYCOMMON_KNOWN_PATHS})
            if(EXISTS "${_lib_path}")
                set(_CYCOMMON_STATIC_LIB "${_lib_path}")
                message(STATUS "Found CYCommon library: ${_CYCOMMON_STATIC_LIB}")
                break()
            endif()
        endforeach()
    endif()
else()
    # Linux/Android logic
    if(ANDROID)
        set(_PLATFORM "Android")
        set(_ARCH "${CMAKE_ANDROID_ARCH_ABI}")
    else()
        set(_PLATFORM "Linux")
        if(DEFINED TARGET_ARCH AND NOT "${TARGET_ARCH}" STREQUAL "")
            set(_ARCH "${TARGET_ARCH}")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(_ARCH "arm64")
        else()
            set(_ARCH "x86_64")
        endif()
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT ANDROID)
        set(_CYCOMMON_LINUX_LIB_NAME "libCYCommonD.a")
    else()
        set(_CYCOMMON_LINUX_LIB_NAME "libCYCommon.a")
    endif()
    string(TOLOWER "${CMAKE_BUILD_TYPE}" _CYCOMMON_BT_LOWER)
    if(NOT ANDROID AND DEFINED CYLOGGER_ROOT_DIR)
        set(_CYCOMMON_LOGGER_LINUX_LIB "${CYLOGGER_ROOT_DIR}/Bin/Linux/${_ARCH}/${_CYCOMMON_BT_LOWER}/${_CYCOMMON_LINUX_LIB_NAME}")
        if(EXISTS "${_CYCOMMON_LOGGER_LINUX_LIB}")
            set(_CYCOMMON_STATIC_LIB "${_CYCOMMON_LOGGER_LINUX_LIB}")
            message(STATUS "CYCommon static library (CYLogger Bin): ${_CYCOMMON_STATIC_LIB}")
        endif()
    endif()
    if(NOT _CYCOMMON_STATIC_LIB)
        set(_CYCOMMON_EXPECTED_LIB "${_CYCOMMON_LIB_DIR}/${_PLATFORM}/${_ARCH}/${CMAKE_BUILD_TYPE}/${_CYCOMMON_LINUX_LIB_NAME}")
        if(EXISTS "${_CYCOMMON_EXPECTED_LIB}")
            set(_CYCOMMON_STATIC_LIB "${_CYCOMMON_EXPECTED_LIB}")
            message(STATUS "CYCommon static library (matched): ${_CYCOMMON_STATIC_LIB}")
        endif()
    endif()
endif()

if(_CYCOMMON_STATIC_LIB)
    message(STATUS "CYCommon static library: ${_CYCOMMON_STATIC_LIB}")
    # Create an IMPORTED target for CYCommon::static when we have a concrete library path.
    # This avoids relying on alias targets that may not exist in external builds.
    if(NOT TARGET CYCommon_static)
        add_library(CYCommon_static STATIC IMPORTED)
        set_target_properties(CYCommon_static PROPERTIES
            IMPORTED_LOCATION "${_CYCOMMON_STATIC_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${CYCOMMON_INCLUDE_DIR}"
        )
    endif()
    if(NOT TARGET CYCommon::static)
        add_library(CYCommon::static ALIAS CYCommon_static)
    endif()
endif()
endif()  # endif for: if(NOT _CYCOMMON_PATH_FROM_ENV) - second search block

# ---------------------------------------------------------------------------
# Standard project setup follows
# ---------------------------------------------------------------------------

# Configure the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Force every target to compile as C++20
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
endif()

# Enable PDB generation for all build types (including Release)
# This ensures .pdb files are generated for debugging
if(MSVC)
    # Use /Zi for PDB generation (debug info in separate file)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Zi")
    # Disable optimization for better debugging experience (optional)
    # set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Od")
    # Generate program database even in Release
    set(CMAKE_MSVC_RUNTIME_LIBRARY_MINSIZEREL_DEFAULT "MultiThreaded")
    set(CMAKE_MSVC_RUNTIME_LIBRARY_RELWITHDEBINFO_DEFAULT "MultiThreaded")
endif()

# Library flavor toggles
if(NOT DEFINED BUILD_SHARED_LIBS)
    option(BUILD_SHARED_LIBS "Build shared libraries" ON)
endif()
if(NOT DEFINED BUILD_STATIC_LIBS)
    option(BUILD_STATIC_LIBS "Build static libraries" ON)
endif()

# Windows runtime library selection
if(WIN32)
    set(_CYCOROUTINE_VALID_RUNTIMES "MD" "MT" "MDD" "MTD")
    if(NOT DEFINED WINDOWS_RUNTIME)
        set(WINDOWS_RUNTIME "MD" CACHE STRING "Windows runtime library")
    else()
        set(WINDOWS_RUNTIME "${WINDOWS_RUNTIME}" CACHE STRING "Windows runtime library" FORCE)
    endif()
    set_property(CACHE WINDOWS_RUNTIME PROPERTY STRINGS "MD" "MT" "MDD" "MTD")
    
    string(TOUPPER "${WINDOWS_RUNTIME}" _CYCOROUTINE_RUNTIME_UPPER)
    list(FIND _CYCOROUTINE_VALID_RUNTIMES "${_CYCOROUTINE_RUNTIME_UPPER}" _CYCOROUTINE_RUNTIME_INDEX)
    if(_CYCOROUTINE_RUNTIME_INDEX EQUAL -1)
        message(FATAL_ERROR "WINDOWS_RUNTIME must be one of MT, MD, MTD, or MDD (got ${WINDOWS_RUNTIME}).")
    endif()
    
    if(_CYCOROUTINE_RUNTIME_UPPER STREQUAL "MT")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING "MSVC runtime selection" FORCE)
    elseif(_CYCOROUTINE_RUNTIME_UPPER STREQUAL "MTD")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug" CACHE STRING "MSVC runtime selection" FORCE)
    elseif(_CYCOROUTINE_RUNTIME_UPPER STREQUAL "MDD")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL" CACHE STRING "MSVC runtime selection" FORCE)
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL" CACHE STRING "MSVC runtime selection" FORCE)
    endif()
    message(STATUS "Windows Runtime: ${_CYCOROUTINE_RUNTIME_UPPER}")
endif()

# Output directory layout
if(WIN32)
    set(PLATFORM_NAME "Windows")
    # Prefer TARGET_ARCH if explicitly specified
    if(DEFINED TARGET_ARCH)
        if(TARGET_ARCH STREQUAL "x86_64" OR TARGET_ARCH STREQUAL "x64" OR TARGET_ARCH STREQUAL "amd64")
            set(ARCH_NAME "x86_64")
        elseif(TARGET_ARCH STREQUAL "x86" OR TARGET_ARCH STREQUAL "Win32" OR TARGET_ARCH STREQUAL "i386" OR TARGET_ARCH STREQUAL "i686")
            set(ARCH_NAME "x86")
        else()
            set(ARCH_NAME "${TARGET_ARCH}")
        endif()
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(ARCH_NAME "x86")
    else()
        # Default to x86_64 for 64-bit Windows (not x64)
        set(ARCH_NAME "x86_64")
    endif()
elseif(APPLE)
    if(IOS)
        set(PLATFORM_NAME "iOS")
    else()
        set(PLATFORM_NAME "MacOS")
    endif()
    set(_CYCOROUTINE_OSX_ARCH_LIST "")
    set(_CYCOROUTINE_OSX_ARCH_DEFINED FALSE)
    if(DEFINED CMAKE_OSX_ARCHITECTURES)
        string(STRIP "${CMAKE_OSX_ARCHITECTURES}" _CYCOROUTINE_OSX_ARCH_STRIPPED)
        if(NOT "${_CYCOROUTINE_OSX_ARCH_STRIPPED}" STREQUAL "")
            set(_CYCOROUTINE_OSX_ARCH_DEFINED TRUE)
            set(_CYCOROUTINE_OSX_ARCH_LIST ${CMAKE_OSX_ARCHITECTURES})
            list(JOIN _CYCOROUTINE_OSX_ARCH_LIST ";" _CYCOROUTINE_OSX_ARCH_JOINED)
        endif()
    endif()
    # Prefer CMAKE_OSX_ARCHITECTURES as it explicitly specifies the target architecture
    # This is especially important when cross-compiling (e.g., x86_64 host building for arm64)
    if(_CYCOROUTINE_OSX_ARCH_DEFINED)
        if(_CYCOROUTINE_OSX_ARCH_JOINED MATCHES "arm64")
            set(ARCH_NAME "arm64")
        elseif(_CYCOROUTINE_OSX_ARCH_JOINED MATCHES "x86_64")
            set(ARCH_NAME "x86_64")
        elseif(_CYCOROUTINE_OSX_ARCH_JOINED MATCHES "i386|i686")
            set(ARCH_NAME "x86")
        else()
            list(GET _CYCOROUTINE_OSX_ARCH_LIST 0 first_arch)
            if(first_arch MATCHES "arm64|aarch64")
                set(ARCH_NAME "arm64")
            elseif(first_arch MATCHES "x86_64|AMD64")
                set(ARCH_NAME "x86_64")
            elseif(first_arch MATCHES "i386|i686")
                set(ARCH_NAME "x86")
            else()
                set(ARCH_NAME ${first_arch})
            endif()
        endif()
    # Fall back to CMAKE_SYSTEM_PROCESSOR if CMAKE_OSX_ARCHITECTURES is not set
    elseif(DEFINED CMAKE_SYSTEM_PROCESSOR)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(ARCH_NAME "arm64")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
            set(ARCH_NAME "x86_64")
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686")
            set(ARCH_NAME "x86")
        else()
            set(ARCH_NAME ${CMAKE_SYSTEM_PROCESSOR})
        endif()
    # Default fallback
    else()
        set(ARCH_NAME "x86_64")
    endif()
elseif(ANDROID)
    set(PLATFORM_NAME "Android")
    if(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
        set(ARCH_NAME "armeabi-v7a")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        set(ARCH_NAME "arm64-v8a")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
        set(ARCH_NAME "x86")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
        set(ARCH_NAME "x86_64")
    endif()
elseif(UNIX)
    set(PLATFORM_NAME "Linux")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(ARCH_NAME "arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686")
        set(ARCH_NAME "x86")
    else()
        set(ARCH_NAME "x86_64")
    endif()
endif()

# Use absolute output directories to keep artifacts organized
# Match CYLogger's output directory structure for consistency
if(DEFINED CYLOGGER_ROOT_DIR)
    # For iOS, use TARGET_ARCH (which distinguishes arm64-simulator from arm64-device)
    # instead of ARCH_NAME (which is always "arm64" for both).
    if(IOS AND DEFINED TARGET_ARCH AND NOT "${TARGET_ARCH}" STREQUAL "")
        get_filename_component(OUTPUT_BASE_DIR "${CYLOGGER_ROOT_DIR}/Bin/${PLATFORM_NAME}/${TARGET_ARCH}" ABSOLUTE)
    else()
        get_filename_component(OUTPUT_BASE_DIR "${CYLOGGER_ROOT_DIR}/Bin/${PLATFORM_NAME}/${ARCH_NAME}" ABSOLUTE)
    endif()
else()
    # Standalone build: output to CYCoroutine's own Bin directory (sibling of Build/)
    get_filename_component(OUTPUT_BASE_DIR "${CMAKE_SOURCE_DIR}/../Bin/${PLATFORM_NAME}/${ARCH_NAME}" ABSOLUTE)
endif()

# On Windows, include runtime library type in output path (MD/MT)
if(WIN32 AND DEFINED WINDOWS_RUNTIME)
    set(OUTPUT_BASE_DIR "${OUTPUT_BASE_DIR}/${WINDOWS_RUNTIME}")
endif()

# For multi-config generators (Visual Studio, Xcode), do NOT add build type to base path
# The multi-config loop below handles per-configuration directories
# For single-config generators (Ninja, Make), add build type to base path
if(NOT CMAKE_CONFIGURATION_TYPES)
    # Single-config generator - add build type to path (use CMAKE_BUILD_TYPE as-is, capitalized)
    if(CMAKE_BUILD_TYPE)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}/${CMAKE_BUILD_TYPE}")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}/${CMAKE_BUILD_TYPE}")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}/${CMAKE_BUILD_TYPE}")
    else()
        # Fallback if CMAKE_BUILD_TYPE is not set
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BASE_DIR})
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_BASE_DIR})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_BASE_DIR})
    endif()
else()
    # Multi-config generator - set base directories (per-config will be set below)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}")
endif()

# Multi-config generator output directories (Visual Studio, Xcode, etc.)
# Use lowercase configuration names for consistency
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOLOWER "${OUTPUTCONFIG}" OUTPUTCONFIG_LOWER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${OUTPUT_BASE_DIR}/${OUTPUTCONFIG_LOWER}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${OUTPUT_BASE_DIR}/${OUTPUTCONFIG_LOWER}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${OUTPUT_BASE_DIR}/${OUTPUTCONFIG_LOWER}")
endforeach()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Src)
# Add both CYCommon root and CYCommon Inc so both #include "CYCommon/X.hpp" and #include "X.hpp" work
include_directories(${CYCOMMON_ROOT_DIR})
include_directories(${CYCOMMON_INCLUDE_DIR})

# Gather sources
file(GLOB_RECURSE CYCOROUTINE_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/../Src/*.cpp"
)

# Gather headers
file(GLOB_RECURSE CYCOROUTINE_HEADERS 
    "${CMAKE_CURRENT_SOURCE_DIR}/../Inc/*.hpp"
)

# Helper to apply platform-specific properties
function(set_target_properties_by_platform TARGET)
    if(WIN32)
        # Windows-specific defines
        target_compile_definitions(${TARGET} PRIVATE 
            _WIN32_WINNT=0x0601  # Windows 7
            WIN32_LEAN_AND_MEAN
            NOMINMAX
        )
        
        # Dynamic library tweaks
        if(${TARGET} MATCHES "shared")
            target_compile_definitions(${TARGET} PRIVATE
                CYCOROUTINE_EXPORT_API
            )
            set_target_properties(${TARGET} PROPERTIES
                WINDOWS_EXPORT_ALL_SYMBOLS ON
            )
        endif()
        
        # Honor the selected MSVC runtime
        set_property(TARGET ${TARGET} PROPERTY
            MSVC_RUNTIME_LIBRARY "${CMAKE_MSVC_RUNTIME_LIBRARY}")
        
        # Enable PDB generation for all configurations (including Release)
        if(MSVC)
            # Use /Zi for PDB generation
            get_target_property(TARGET_TYPE ${TARGET} TYPE)
            if(TARGET_TYPE STREQUAL "STATIC_LIBRARY")
                # For static libraries, use /Zi and specify PDB name
                set_target_properties(${TARGET} PROPERTIES
                    COMPILE_PDB_NAME "${_CYCOROUTINE_STATIC_OUTPUT_NAME}"
                    COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
                )
            else()
                set_target_properties(${TARGET} PROPERTIES
                    COMPILE_PDB_NAME "${_CYCOROUTINE_STATIC_OUTPUT_NAME}"
                    COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
                )
            endif()
        endif()
    elseif(APPLE)
        # macOS/iOS specific toggles
        if(IOS)
            set_target_properties(${TARGET} PROPERTIES
                MACOSX_BUNDLE ON
                XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
            )
        endif()
        
        # Extra defines for shared builds
        if(${TARGET} MATCHES "shared")
            target_compile_definitions(${TARGET} PRIVATE
                CYCOROUTINE_EXPORT_API
            )
        endif()
    elseif(ANDROID)
        # Android-specific flags
        target_compile_definitions(${TARGET} PRIVATE 
            ANDROID
        )
        
        # Extra defines for shared builds
        if(${TARGET} MATCHES "shared")
            target_compile_definitions(${TARGET} PRIVATE
                CYCOROUTINE_EXPORT_API
            )
        endif()
    elseif(UNIX)
        # Linux/Unix specific defines
        target_compile_definitions(${TARGET} PRIVATE 
            _GNU_SOURCE
        )
        
        # Extra defines for shared builds
        if(${TARGET} MATCHES "shared")
            target_compile_definitions(${TARGET} PRIVATE
                CYCOROUTINE_EXPORT_API
            )
        endif()
    endif()
endfunction()

# Shared library target
if(BUILD_SHARED_LIBS)
    add_library(CYCoroutine_shared SHARED
        ${CYCOROUTINE_SOURCES}
        ${CYCOROUTINE_HEADERS}
    )
    
    # Shared library metadata
    # Debug builds use CYCoroutineD on all platforms including Android
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_CYCOROUTINE_SHARED_OUTPUT_NAME "CYCoroutineD")
    else()
        set(_CYCOROUTINE_SHARED_OUTPUT_NAME "CYCoroutine")
    endif()
    set_target_properties(CYCoroutine_shared PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER "${CYCOROUTINE_HEADERS}"
        OUTPUT_NAME "${_CYCOROUTINE_SHARED_OUTPUT_NAME}"
    )
    
    # Enable PDB generation for shared library on MSVC
    if(MSVC)
        set_target_properties(CYCoroutine_shared PROPERTIES
            COMPILE_PDB_NAME "CYCoroutine"
            COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
        )
    endif()
    
    # Apply per-platform settings
    set_target_properties_by_platform(CYCoroutine_shared)
    
    if(TARGET fmt::fmt)
        target_link_libraries(CYCoroutine_shared PUBLIC fmt::fmt)
    elseif(TARGET fmt)
        target_link_libraries(CYCoroutine_shared PUBLIC fmt)
    endif()

    if(_CYCOMMON_STATIC_LIB)
        target_link_libraries(CYCoroutine_shared PUBLIC "${_CYCOMMON_STATIC_LIB}")
    elseif(_CYCOMMON_SHARED_LIB)
        target_link_libraries(CYCoroutine_shared PUBLIC "${_CYCOMMON_SHARED_LIB}")
    else()
        target_link_libraries(CYCoroutine_shared PUBLIC CYCommon::static)
    endif()

    # Provide an ALIAS for downstream targets
    add_library(CYCoroutine::shared ALIAS CYCoroutine_shared)
endif()

# Static library target
if(BUILD_STATIC_LIBS)
    add_library(CYCoroutine_static STATIC
        ${CYCOROUTINE_SOURCES}
        ${CYCOROUTINE_HEADERS}
    )
    
    # Static library metadata
    # Debug builds use CYCoroutineD on all platforms including Android
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_CYCOROUTINE_STATIC_OUTPUT_NAME "CYCoroutineD")
    else()
        set(_CYCOROUTINE_STATIC_OUTPUT_NAME "CYCoroutine")
    endif()
    set_target_properties(CYCoroutine_static PROPERTIES
        VERSION ${PROJECT_VERSION}
        PUBLIC_HEADER "${CYCOROUTINE_HEADERS}"
        OUTPUT_NAME "${_CYCOROUTINE_STATIC_OUTPUT_NAME}"
    )
    
    # Apply per-platform settings
    set_target_properties_by_platform(CYCoroutine_static)
    
    if(TARGET fmt::fmt)
        target_link_libraries(CYCoroutine_static PUBLIC fmt::fmt)
    elseif(TARGET fmt)
        target_link_libraries(CYCoroutine_static PUBLIC fmt)
    endif()

    if(_CYCOMMON_STATIC_LIB)
        target_link_libraries(CYCoroutine_static PUBLIC "${_CYCOMMON_STATIC_LIB}")
    elseif(TARGET CYCommon::static)
        target_link_libraries(CYCoroutine_static PUBLIC CYCommon::static)
    elseif(TARGET CYCommon_static)
        target_link_libraries(CYCoroutine_static PUBLIC CYCommon_static)
    elseif(TARGET CYCommon)
        target_link_libraries(CYCoroutine_static PUBLIC CYCommon)
    else()
        message(FATAL_ERROR "CYCommon static target was not created. Expected CYCommon::static or a concrete CYCommon library target.")
    endif()

    # Provide an ALIAS for downstream targets
    add_library(CYCoroutine::static ALIAS CYCoroutine_static)
endif()

# Optional examples
option(BUILD_EXAMPLES "Build example applications" ON)

if(BUILD_EXAMPLES AND EXISTS ${CMAKE_SOURCE_DIR}/../Example)
    # Build the bundled examples
    add_subdirectory(${CMAKE_SOURCE_DIR}/../Example ${CMAKE_BINARY_DIR}/example)
endif()

# Install rules
if(BUILD_SHARED_LIBS)
    install(TARGETS CYCoroutine_shared
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include/CYCoroutine
    )
endif()
if(BUILD_STATIC_LIBS)
    install(TARGETS CYCoroutine_static
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include/CYCoroutine
    )
endif()

# Diagnostics
message(STATUS "CYCoroutine Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Platform: ${PLATFORM_NAME}")
message(STATUS "Architecture: ${ARCH_NAME}")
message(STATUS "Output Directory: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "Build Examples: ${BUILD_EXAMPLES}")
message(STATUS "Build Shared Libraries: ${BUILD_SHARED_LIBS}")
message(STATUS "Build Static Libraries: ${BUILD_STATIC_LIBS}")
if(WIN32)
    message(STATUS "Windows Runtime: ${WINDOWS_RUNTIME}")
endif()