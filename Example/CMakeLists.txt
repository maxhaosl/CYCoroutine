cmake_minimum_required(VERSION 3.16)
project(CYCoroutineExample VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 确保所有目标使用C++20
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20")
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
set(OUTPUT_BASE_DIR ${CMAKE_SOURCE_DIR}/../Bin/Example/${PLATFORM_NAME}/${ARCH_NAME})
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
include_directories(${CMAKE_SOURCE_DIR}/../Inc)

# 如果在主CMakeLists.txt中构建示例，则需要添加库目录
if(NOT TARGET CYCoroutine_shared AND NOT TARGET CYCoroutine_static)
    # 添加库目录
    link_directories(${CMAKE_SOURCE_DIR}/../../Build)
endif()

# 创建示例可执行文件
add_executable(CYCoroutineExample 
    main.cpp
)

# 链接CYCoroutine库
if(TARGET CYCoroutine_shared)
    target_link_libraries(CYCoroutineExample 
        CYCoroutine_shared
    )
    # 添加依赖关系，确保先构建库再构建示例
    add_dependencies(CYCoroutineExample CYCoroutine_shared)
    
    # 为使用动态库添加宏定义
    target_compile_definitions(CYCoroutineExample PRIVATE
        CYCOROUTINE_IMPORT_API
    )
elseif(TARGET CYCoroutine_static)
    target_link_libraries(CYCoroutineExample 
        CYCoroutine_static
    )
    # 添加依赖关系，确保先构建库再构建示例
    add_dependencies(CYCoroutineExample CYCoroutine_static)
else()
    # 默认情况，尝试链接共享库
    target_link_libraries(CYCoroutineExample 
        CYCoroutine_shared
    )
    # 添加依赖关系，确保先构建库再构建示例
    add_dependencies(CYCoroutineExample CYCoroutine_shared)
    
    # 为使用动态库添加宏定义
    target_compile_definitions(CYCoroutineExample PRIVATE
        CYCOROUTINE_IMPORT_API
    )
endif()

# 平台特定设置
if(WIN32)
    # Windows特定设置
    target_compile_definitions(CYCoroutineExample PRIVATE 
        _WIN32_WINNT=0x0601  # Windows 7
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
    
    # 设置Windows运行时库
    set_property(TARGET CYCoroutineExample PROPERTY
        MSVC_RUNTIME_LIBRARY "${CMAKE_MSVC_RUNTIME_LIBRARY}")
    
    # 根据架构设置输出名称
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set_target_properties(CYCoroutineExample PROPERTIES
            OUTPUT_NAME "CYCoroutineExample_x86"
            RUNTIME_OUTPUT_NAME "CYCoroutineExample_x86"
        )
    else()
        set_target_properties(CYCoroutineExample PROPERTIES
            OUTPUT_NAME "CYCoroutineExample_x64"
            RUNTIME_OUTPUT_NAME "CYCoroutineExample_x64"
        )
    endif()
elseif(APPLE)
    # macOS/iOS特定设置
    if(IOS)
        set_target_properties(CYCoroutineExample PROPERTIES
            MACOSX_BUNDLE ON
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
        )
        
        # 根据架构设置输出名称
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set_target_properties(CYCoroutineExample PROPERTIES
                OUTPUT_NAME "CYCoroutineExample_ios_arm64"
                RUNTIME_OUTPUT_NAME "CYCoroutineExample_ios_arm64"
            )
        else()
            set_target_properties(CYCoroutineExample PROPERTIES
                OUTPUT_NAME "CYCoroutineExample_ios_simulator"
                RUNTIME_OUTPUT_NAME "CYCoroutineExample_ios_simulator"
            )
        endif()
    else()
        # macOS架构设置
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set_target_properties(CYCoroutineExample PROPERTIES
                OUTPUT_NAME "CYCoroutineExample_arm64"
                RUNTIME_OUTPUT_NAME "CYCoroutineExample_arm64"
            )
        else()
            set_target_properties(CYCoroutineExample PROPERTIES
                OUTPUT_NAME "CYCoroutineExample_x64"
                RUNTIME_OUTPUT_NAME "CYCoroutineExample_x64"
            )
        endif()
    endif()
elseif(ANDROID)
    # Android特定设置
    target_compile_definitions(CYCoroutineExample PRIVATE 
        ANDROID
    )
    
    # 根据ABI设置输出名称
    if(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
        set_target_properties(CYCoroutineExample PROPERTIES
            OUTPUT_NAME "CYCoroutineExample_armv7"
            RUNTIME_OUTPUT_NAME "CYCoroutineExample_armv7"
        )
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        set_target_properties(CYCoroutineExample PROPERTIES
            OUTPUT_NAME "CYCoroutineExample_arm64"
            RUNTIME_OUTPUT_NAME "CYCoroutineExample_arm64"
        )
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
        set_target_properties(CYCoroutineExample PROPERTIES
            OUTPUT_NAME "CYCoroutineExample_x86"
            RUNTIME_OUTPUT_NAME "CYCoroutineExample_x86"
        )
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
        set_target_properties(CYCoroutineExample PROPERTIES
            OUTPUT_NAME "CYCoroutineExample_x64"
            RUNTIME_OUTPUT_NAME "CYCoroutineExample_x64"
        )
    endif()
elseif(UNIX)
    # Linux/Unix特定设置
    target_compile_definitions(CYCoroutineExample PRIVATE 
        _GNU_SOURCE
    )
    
    # 根据架构设置输出名称
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set_target_properties(CYCoroutineExample PROPERTIES
            OUTPUT_NAME "CYCoroutineExample_arm64"
            RUNTIME_OUTPUT_NAME "CYCoroutineExample_arm64"
        )
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686")
        set_target_properties(CYCoroutineExample PROPERTIES
            OUTPUT_NAME "CYCoroutineExample_x86"
            RUNTIME_OUTPUT_NAME "CYCoroutineExample_x86"
        )
    else()
        set_target_properties(CYCoroutineExample PROPERTIES
            OUTPUT_NAME "CYCoroutineExample_x64"
            RUNTIME_OUTPUT_NAME "CYCoroutineExample_x64"
        )
    endif()
endif()

# 安装规则
if(IOS)
    install(TARGETS CYCoroutineExample
        BUNDLE DESTINATION ${OUTPUT_BASE_DIR}
    )
else()
    install(TARGETS CYCoroutineExample
        RUNTIME DESTINATION ${OUTPUT_BASE_DIR}
    )
endif()

# 打印配置信息
message(STATUS "CYCoroutineExample Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Platform: ${PLATFORM_NAME}")
message(STATUS "Architecture: ${ARCH_NAME}")
message(STATUS "Output Directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
if(WIN32)
    message(STATUS "Windows Runtime: ${WINDOWS_RUNTIME}")
endif()