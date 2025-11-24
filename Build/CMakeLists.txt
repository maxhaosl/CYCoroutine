cmake_minimum_required(VERSION 3.16)
project(CYCoroutine VERSION 1.0.0 LANGUAGES CXX)

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
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(ARCH_NAME "x86")
    else()
        set(ARCH_NAME "x86_64")
    endif()
elseif(APPLE)
    if(IOS)
        set(PLATFORM_NAME "iOS")
    else()
        set(PLATFORM_NAME "macOS")
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
    get_filename_component(OUTPUT_BASE_DIR "${CYLOGGER_ROOT_DIR}/Bin/${PLATFORM_NAME}/${ARCH_NAME}" ABSOLUTE)
else()
    get_filename_component(OUTPUT_BASE_DIR "${CMAKE_SOURCE_DIR}/../Bin/${PLATFORM_NAME}/${ARCH_NAME}" ABSOLUTE)
endif()

# On Windows, include runtime library type in output path (MD/MT)
if(WIN32 AND DEFINED WINDOWS_RUNTIME)
    set(OUTPUT_BASE_DIR "${OUTPUT_BASE_DIR}/${WINDOWS_RUNTIME}")
endif()

# For single-config generators (like Android), include build type in output path
# This matches CYLogger's output directory structure
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

# Multi-config generator output directories (Visual Studio, Xcode, etc.)
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${OUTPUT_BASE_DIR}/${OUTPUTCONFIG}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${OUTPUT_BASE_DIR}/${OUTPUTCONFIG}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${OUTPUT_BASE_DIR}/${OUTPUTCONFIG}")
endforeach()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Src)

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
    set_target_properties(CYCoroutine_shared PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER "${CYCOROUTINE_HEADERS}"
        OUTPUT_NAME "CYCoroutine"
    )
    
    # Apply per-platform settings
    set_target_properties_by_platform(CYCoroutine_shared)
    
    if(TARGET fmt::fmt)
        target_link_libraries(CYCoroutine_shared PUBLIC fmt::fmt)
    elseif(TARGET fmt)
        target_link_libraries(CYCoroutine_shared PUBLIC fmt)
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
    set_target_properties(CYCoroutine_static PROPERTIES
        VERSION ${PROJECT_VERSION}
        PUBLIC_HEADER "${CYCOROUTINE_HEADERS}"
        OUTPUT_NAME "CYCoroutine"
    )
    
    # Apply per-platform settings
    set_target_properties_by_platform(CYCoroutine_static)
    
    if(TARGET fmt::fmt)
        target_link_libraries(CYCoroutine_static PUBLIC fmt::fmt)
    elseif(TARGET fmt)
        target_link_libraries(CYCoroutine_static PUBLIC fmt)
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