#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CMAKE_SOURCE_DIR="$PROJECT_ROOT/Build"
OUTPUT_BASE="$PROJECT_ROOT/Bin"
mkdir -p "$OUTPUT_BASE"
source "$SCRIPT_DIR/output_layout.sh"

BUILD_TYPE=${1:-Release}
LIB_TYPE_RAW=${2:-Static}
RAW_ARCH=${3:-$(uname -m)}

canonicalize_linux_arch() {
    local token lower
    token=$(printf '%s' "$1" | xargs)
    lower=$(printf '%s' "$token" | tr '[:upper:]' '[:lower:]')
    case "$lower" in
        x86_64|amd64|x64)
            echo "x86_64"
            ;;
        i386|i686|x86)
            echo "x86"
            ;;
        arm64|aarch64)
            echo "arm64"
            ;;
        *)
            echo "$token"
            ;;
    esac
}

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

detect_jobs() {
    if command -v nproc >/dev/null 2>&1; then
        nproc && return
    fi
    if command -v sysctl >/dev/null 2>&1; then
        sysctl -n hw.logicalcpu 2>/dev/null && return
    fi
    echo 4
}

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

TARGET_ARCH=$(canonicalize_linux_arch "$RAW_ARCH")
LIB_TYPE=$(normalize_lib_type "$LIB_TYPE_RAW")
CC_BIN=$(detect_compiler "${CYCOROUTINE_CC:-}" "clang-17")
CXX_BIN=$(detect_compiler "${CYCOROUTINE_CXX:-}" "clang++-17")

if [ -z "$CC_BIN" ] || [ -z "$CXX_BIN" ]; then
    echo "Error: clang-17 toolchain not found (set CYCOROUTINE_CC/CYCOROUTINE_CXX to override)." >&2
    exit 1
fi

if [ "$TARGET_ARCH" = "x86" ]; then
    ARCH_FLAG_DESC="(32-bit)"
elif [ "$TARGET_ARCH" = "x86_64" ]; then
    ARCH_FLAG_DESC="(64-bit)"
else
    ARCH_FLAG_DESC=""
fi

echo "Building CYCoroutine for Linux..."
echo "Build Type: $BUILD_TYPE"
echo "Library Type: $LIB_TYPE"
if [ "$RAW_ARCH" != "$TARGET_ARCH" ]; then
    echo "Target Architecture: $TARGET_ARCH $ARCH_FLAG_DESC (requested: $RAW_ARCH)"
else
    echo "Target Architecture: $TARGET_ARCH $ARCH_FLAG_DESC"
fi

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

BUILD_SUBDIR="build_linux_${TARGET_ARCH}_${BUILD_TYPE}_${LIB_TAG}"
BUILD_PATH="$SCRIPT_DIR/$BUILD_SUBDIR"
mkdir -p "$BUILD_PATH"

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

C_FLAGS=()
CXX_FLAGS=()
EXE_LINKER_FLAGS=()

if [ "$TARGET_ARCH" = "x86" ]; then
    C_FLAGS+=("-m32")
    CXX_FLAGS+=("-m32")
    EXE_LINKER_FLAGS+=("-m32")
elif [ "$TARGET_ARCH" = "x86_64" ]; then
    C_FLAGS+=("-m64")
    CXX_FLAGS+=("-m64")
fi

if [ ${#C_FLAGS[@]} -gt 0 ]; then
    CMAKE_ARGS+=("-DCMAKE_C_FLAGS=${C_FLAGS[*]}")
fi
if [ ${#CXX_FLAGS[@]} -gt 0 ]; then
    CMAKE_ARGS+=("-DCMAKE_CXX_FLAGS=${CXX_FLAGS[*]}")
fi
if [ ${#EXE_LINKER_FLAGS[@]} -gt 0 ]; then
    CMAKE_ARGS+=("-DCMAKE_EXE_LINKER_FLAGS=${EXE_LINKER_FLAGS[*]}")
fi

cmake "${CMAKE_ARGS[@]}"
cmake --build "$BUILD_PATH" --target "$TARGET_NAME" --parallel "$(detect_jobs)"

OUTPUT_DIR="$(platform_slice_dir linux "$TARGET_ARCH" "$BUILD_TYPE")"
mkdir -p "$OUTPUT_DIR"

# 复制构建产物到输出目录
if [ "$LIB_TYPE" = "Shared" ]; then
    # 复制共享库
    find "$BUILD_PATH" -name "libCYCoroutine_shared.so" -type f -exec cp {} "$OUTPUT_DIR/" \;
    echo "Shared library copied to: $OUTPUT_DIR/libCYCoroutine_shared.so"
else
    # 复制静态库
    find "$BUILD_PATH" -name "libCYCoroutine_static.a" -type f -exec cp {} "$OUTPUT_DIR/" \;
    echo "Static library copied to: $OUTPUT_DIR/libCYCoroutine_static.a"
fi

echo "CYCoroutine artifacts staged at: $OUTPUT_DIR"


