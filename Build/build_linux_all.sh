#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CMAKE_SOURCE_DIR="$PROJECT_ROOT/Build"
OUTPUT_BASE="$PROJECT_ROOT/Bin"
mkdir -p "$OUTPUT_BASE"
source "$SCRIPT_DIR/output_layout.sh"

# 固定架构为 x86_64
TARGET_ARCH="x86_64"

# 构建配置数组
declare -a BUILD_CONFIGS=(
    "Debug:Static"
    "Debug:Shared"
    "Release:Static"
    "Release:Shared"
)

# 检测并行作业数
detect_jobs() {
    if command -v nproc >/dev/null 2>&1; then
        nproc && return
    fi
    if command -v sysctl >/dev/null 2>&1; then
        sysctl -n hw.logicalcpu 2>/dev/null && return
    fi
    echo 4
}

# 检测编译器
detect_compiler() {
    local override="$1" required="$2" resolved
    if [ -n "$override" ]; then
        echo "$override"
        return
    fi
    if command -v "$required" >/dev/null 2>&1; then
        resolved=$(command -v "$required")
        echo "$resolved"
        return
    fi
    echo ""
}

# 规范化库类型
normalize_lib_type() {
    local token
    token=$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]')
    case "$token" in
        shared|on|dyn|dynamic)
            echo "Shared"
            ;;
        static|off)
            echo "Static"
            ;;
        *)
            echo "Static"
            ;;
    esac
}

# 检测编译器
CC_BIN=$(detect_compiler "${CYCOROUTINE_CC:-}" "clang-17")
CXX_BIN=$(detect_compiler "${CYCOROUTINE_CXX:-}" "clang++-17")

if [ -z "$CC_BIN" ] || [ -z "$CXX_BIN" ]; then
    echo "Error: clang-17 toolchain not found (set CYCOROUTINE_CC/CYCOROUTINE_CXX to override)." >&2
    exit 1
fi

echo "Building CYCoroutine for Linux x86_64 (All configurations)..."
echo "Compiler: $CC_BIN / $CXX_BIN"
echo "Parallel jobs: $(detect_jobs)"
echo ""

# 遍历所有构建配置
for config in "${BUILD_CONFIGS[@]}"; do
    IFS=':' read -r BUILD_TYPE LIB_TYPE_RAW <<< "$config"
    LIB_TYPE=$(normalize_lib_type "$LIB_TYPE_RAW")
    
    echo "========================================"
    echo "Building: $BUILD_TYPE $LIB_TYPE"
    echo "========================================"
    
    # 设置构建参数
    if [ "$LIB_TYPE" = "Shared" ]; then
        BUILD_SHARED="ON"
        BUILD_STATIC="OFF"
        TARGET_NAME="CYCoroutine_shared"
        LIB_TAG="shared"
    else
        BUILD_SHARED="OFF"
        BUILD_STATIC="ON"
        TARGET_NAME="CYCoroutine_static"
        LIB_TAG="static"
    fi
    
    # 创建构建目录
    BUILD_SUBDIR="build_linux_${TARGET_ARCH}_${BUILD_TYPE}_${LIB_TAG}"
    BUILD_PATH="$SCRIPT_DIR/$BUILD_SUBDIR"
    mkdir -p "$BUILD_PATH"
    
    # 准备 CMake 参数
    CMAKE_ARGS=(
        -S "$CMAKE_SOURCE_DIR"
        -B "$BUILD_PATH"
        "-DCMAKE_C_COMPILER=$CC_BIN"
        "-DCMAKE_CXX_COMPILER=$CXX_BIN"
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
        "-DBUILD_SHARED_LIBS=$BUILD_SHARED"
        "-DBUILD_STATIC_LIBS=$BUILD_STATIC"
        "-DBUILD_EXAMPLES=OFF"
        "-DTARGET_ARCH=$TARGET_ARCH"
        "-DCMAKE_SYSTEM_PROCESSOR_OVERRIDE=$TARGET_ARCH"
        "-DCMAKE_CXX_STANDARD=20"
        "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
    )
    
    # 设置架构标志
    C_FLAGS=("-m64")
    CXX_FLAGS=("-m64")
    
    if [ ${#C_FLAGS[@]} -gt 0 ]; then
        CMAKE_ARGS+=("-DCMAKE_C_FLAGS=${C_FLAGS[*]}")
    fi
    if [ ${#CXX_FLAGS[@]} -gt 0 ]; then
        CMAKE_ARGS+=("-DCMAKE_CXX_FLAGS=${CXX_FLAGS[*]}")
    fi
    
    # 执行构建
    cmake "${CMAKE_ARGS[@]}"
    cmake --build "$BUILD_PATH" --target "$TARGET_NAME" --parallel "$(detect_jobs)"
    
    # 创建输出目录
    OUTPUT_DIR="$(platform_slice_dir linux "$TARGET_ARCH" "$BUILD_TYPE")"
    mkdir -p "$OUTPUT_DIR"
    
    echo "Completed: $BUILD_TYPE $LIB_TYPE"
    echo "Artifacts staged at: $OUTPUT_DIR"
    echo ""
done

echo "========================================"
echo "All builds completed successfully!"
echo "========================================"

# 列出所有生成的库文件
echo ""
echo "Generated libraries:"
for config in "${BUILD_CONFIGS[@]}"; do
    IFS=':' read -r BUILD_TYPE LIB_TYPE_RAW <<< "$config"
    LIB_TYPE=$(normalize_lib_type "$LIB_TYPE_RAW")
    LIB_TAG=$(echo "$LIB_TYPE" | tr '[:upper:]' '[:lower:]')
    
    OUTPUT_DIR="$(platform_slice_dir linux "$TARGET_ARCH" "$BUILD_TYPE")"
    
    if [ "$LIB_TYPE" = "Shared" ]; then
        echo "- $OUTPUT_DIR/libCYCoroutine_shared.so"
    else
        echo "- $OUTPUT_DIR/libCYCoroutine_static.a"
    fi
done