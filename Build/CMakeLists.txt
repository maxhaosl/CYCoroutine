cmake_minimum_required(VERSION 3.16)
project(CYCoroutine VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 确保所有目标使用C++20
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20")
endif()

# 添加库类型选项
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_STATIC_LIBS "Build static libraries" OFF)

# Windows运行时库选项
if(WIN32)
    set(WINDOWS_RUNTIME "MD" CACHE STRING "Windows runtime library")
    set_property(CACHE WINDOWS_RUNTIME PROPERTY STRINGS "MD" "MT" "MDd" "MTd")
    
    if(WINDOWS_RUNTIME MATCHES "MD")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    elseif(WINDOWS_RUNTIME MATCHES "MT")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
    elseif(WINDOWS_RUNTIME MATCHES "MDd")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
    elseif(WINDOWS_RUNTIME MATCHES "MTd")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
    endif()
endif()

# 设置输出目录结构
if(WIN32)
    set(PLATFORM_NAME "Windows")
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(ARCH_NAME "x86")
    else()
        set(ARCH_NAME "x64")
    endif()
elseif(APPLE)
    if(IOS)
        set(PLATFORM_NAME "iOS")
    else()
        set(PLATFORM_NAME "macOS")
    endif()
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(ARCH_NAME "arm64")
    else()
        set(ARCH_NAME "x64")
    endif()
elseif(ANDROID)
    set(PLATFORM_NAME "Android")
    if(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
        set(ARCH_NAME "armv7")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        set(ARCH_NAME "arm64")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
        set(ARCH_NAME "x86")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
        set(ARCH_NAME "x64")
    endif()
elseif(UNIX)
    set(PLATFORM_NAME "Linux")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(ARCH_NAME "arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686")
        set(ARCH_NAME "x86")
    else()
        set(ARCH_NAME "x64")
    endif()
endif()

# 设置输出目录
set(OUTPUT_BASE_DIR ${CMAKE_SOURCE_DIR}/../Bin/${PLATFORM_NAME}/${ARCH_NAME})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BASE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_BASE_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_BASE_DIR})

# 多配置生成器（如Visual Studio）的输出目录
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_BASE_DIR}/${OUTPUTCONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_BASE_DIR}/${OUTPUTCONFIG})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_BASE_DIR}/${OUTPUTCONFIG})
endforeach()

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/..)
include_directories(${CMAKE_SOURCE_DIR}/../Inc)
include_directories(${CMAKE_SOURCE_DIR}/../Src)

# 收集源文件
file(GLOB_RECURSE CYCOROUTINE_SOURCES 
    "${CMAKE_SOURCE_DIR}/../Src/*.cpp"
)

# 收集头文件
file(GLOB_RECURSE CYCOROUTINE_HEADERS 
    "${CMAKE_SOURCE_DIR}/../Inc/*.hpp"
)

# 函数：设置目标属性
function(set_target_properties_by_platform TARGET)
    if(WIN32)
        # Windows特定设置
        target_compile_definitions(${TARGET} PRIVATE 
            _WIN32_WINNT=0x0601  # Windows 7
            WIN32_LEAN_AND_MEAN
            NOMINMAX
        )
        
        # 设置为动态库
        if(${TARGET} MATCHES "shared")
            target_compile_definitions(${TARGET} PRIVATE
                CYCOROUTINE_IMPORT_API
            )
            set_target_properties(${TARGET} PROPERTIES
                WINDOWS_EXPORT_ALL_SYMBOLS ON
            )
        endif()
        
        # 设置Windows运行时库
        set_property(TARGET ${TARGET} PROPERTY
            MSVC_RUNTIME_LIBRARY "${CMAKE_MSVC_RUNTIME_LIBRARY}")
    elseif(APPLE)
        # macOS/iOS特定设置
        if(IOS)
            set_target_properties(${TARGET} PROPERTIES
                MACOSX_BUNDLE ON
                XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
            )
        endif()
        
        # 为动态库添加宏定义
        if(${TARGET} MATCHES "shared")
            target_compile_definitions(${TARGET} PRIVATE
                CYCOROUTINE_IMPORT_API
            )
        endif()
    elseif(ANDROID)
        # Android特定设置
        target_compile_definitions(${TARGET} PRIVATE 
            ANDROID
        )
        
        # 为动态库添加宏定义
        if(${TARGET} MATCHES "shared")
            target_compile_definitions(${TARGET} PRIVATE
                CYCOROUTINE_IMPORT_API
            )
        endif()
    elseif(UNIX)
        # Linux/Unix特定设置
        target_compile_definitions(${TARGET} PRIVATE 
            _GNU_SOURCE
        )
        
        # 为动态库添加宏定义
        if(${TARGET} MATCHES "shared")
            target_compile_definitions(${TARGET} PRIVATE
                CYCOROUTINE_IMPORT_API
            )
        endif()
    endif()
endfunction()

# 创建动态库
if(BUILD_SHARED_LIBS)
    add_library(CYCoroutine_shared SHARED
        ${CYCOROUTINE_SOURCES}
        ${CYCOROUTINE_HEADERS}
    )
    
    # 设置库的属性
    set_target_properties(CYCoroutine_shared PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER "${CYCOROUTINE_HEADERS}"
        OUTPUT_NAME "CYCoroutine"
    )
    
    # 应用平台特定设置
    set_target_properties_by_platform(CYCoroutine_shared)
    
    # 为了兼容性，创建一个别名
    add_library(CYCoroutine::shared ALIAS CYCoroutine_shared)
endif()

# 创建静态库
if(BUILD_STATIC_LIBS)
    add_library(CYCoroutine_static STATIC
        ${CYCOROUTINE_SOURCES}
        ${CYCOROUTINE_HEADERS}
    )
    
    # 设置库的属性
    set_target_properties(CYCoroutine_static PROPERTIES
        VERSION ${PROJECT_VERSION}
        PUBLIC_HEADER "${CYCOROUTINE_HEADERS}"
        OUTPUT_NAME "CYCoroutine"
    )
    
    # 应用平台特定设置
    set_target_properties_by_platform(CYCoroutine_static)
    
    # 为了兼容性，创建一个别名
    add_library(CYCoroutine::static ALIAS CYCoroutine_static)
endif()

# 添加构建示例的选项
option(BUILD_EXAMPLES "Build example applications" ON)

if(BUILD_EXAMPLES)
    # 添加示例子目录
    add_subdirectory(${CMAKE_SOURCE_DIR}/../Example ${CMAKE_BINARY_DIR}/example)
endif()

# 安装规则
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

# 打印配置信息
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