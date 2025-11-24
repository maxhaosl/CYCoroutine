#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CMAKE_SOURCE_DIR="$PROJECT_ROOT/Build"
OUTPUT_BASE="$PROJECT_ROOT/Bin"
mkdir -p "$OUTPUT_BASE"

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "This script must be run on macOS."
    exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
    echo "CMake is required but was not found in PATH."
    exit 1
fi

if ! xcode-select -p >/dev/null 2>&1; then
    echo "Xcode command line tools are missing. Run 'xcode-select --install' first."
    exit 1
fi

MACOS_DEPLOYMENT_TARGET=${MACOS_DEPLOYMENT_TARGET:-"11.0"}
BUILD_TYPES=(${CYCOROUTINE_BUILD_TYPES:-Release})
MAC_ARCHES=(${CYCOROUTINE_MAC_ARCHES:-arm64 x86_64})
SHARED_KINDS=(${CYCOROUTINE_SHARED_KINDS:-OFF ON})

detect_jobs() {
    if command -v sysctl >/dev/null 2>&1; then
        sysctl -n hw.ncpu 2>/dev/null && return
    fi
    if command -v nproc >/dev/null 2>&1; then
        nproc && return
    fi
    echo 4
}
BUILD_JOBS=${BUILD_JOBS:-$(detect_jobs)}

build_slice() {
    local arch=$1
    local build_type=$2
    local shared_flag=$3

    local shared_tag static_flag target_name
    if [ "$shared_flag" = "ON" ]; then
        shared_tag="shared"
        static_flag="OFF"
        target_name="CYCoroutine_shared"
    else
        shared_tag="static"
        static_flag="ON"
        target_name="CYCoroutine_static"
    fi

    local build_dir="$SCRIPT_DIR/build_macos_${arch}_${build_type}_${shared_tag}"

    echo "=== macOS / $arch / $build_type / $shared_tag ==="

    cmake -S "$CMAKE_SOURCE_DIR" \
          -B "$build_dir" \
          -DCMAKE_BUILD_TYPE="$build_type" \
          -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOS_DEPLOYMENT_TARGET" \
          -DCMAKE_OSX_ARCHITECTURES="$arch" \
          -DBUILD_SHARED_LIBS="$shared_flag" \
          -DBUILD_STATIC_LIBS="$static_flag" \
          -DBUILD_EXAMPLES=OFF

    cmake --build "$build_dir" --target "$target_name" --parallel "$BUILD_JOBS"

    local arch_dir="$OUTPUT_BASE/macOS/$arch/$build_type"
    echo "Artifacts: $arch_dir"
}

combine_universal() {
    local build_type=$1
    local shared_flag=$2

    if ! command -v lipo >/dev/null 2>&1; then
        echo "Skipping macOS universal ${build_type}/${shared_flag}: lipo not found."
        return
    fi

    local suffix lib_desc
    if [ "$shared_flag" = "ON" ]; then
        suffix="dylib"
        lib_desc="shared"
    else
        suffix="a"
        lib_desc="static"
    fi

    local src_arm="$OUTPUT_BASE/macOS/arm64/$build_type/libCYCoroutine.$suffix"
    local src_x86="$OUTPUT_BASE/macOS/x86_64/$build_type/libCYCoroutine.$suffix"

    if [ ! -f "$src_arm" ] || [ ! -f "$src_x86" ]; then
        echo "Skipping macOS $build_type $lib_desc universal library: missing slices."
        [ ! -f "$src_arm" ] && echo "  Missing: $src_arm"
        [ ! -f "$src_x86" ] && echo "  Missing: $src_x86"
        return
    fi

    local dest_dir="$OUTPUT_BASE/macOS/universal/$build_type"
    mkdir -p "$dest_dir"
    local dest="$dest_dir/libCYCoroutine.$suffix"

    echo "Creating macOS $build_type $lib_desc universal library..."
    lipo -create "$src_arm" "$src_x86" -output "$dest"

    if [ "$shared_flag" = "ON" ]; then
        local versioned
        for versioned in "$OUTPUT_BASE/macOS/arm64/$build_type"/libCYCoroutine.*.dylib; do
            [ -e "$versioned" ] || continue
            local base="$(basename "$versioned")"
            [ "$base" = "libCYCoroutine.dylib" ] && continue
            ln -sf "libCYCoroutine.dylib" "$dest_dir/$base"
        done
    fi

    echo "Universal artifact: $dest"
}

echo "========================================"
echo "Building CYCoroutine for macOS"
echo "Output root: $OUTPUT_BASE"
echo "Architectures: ${MAC_ARCHES[*]}"
echo "Build types: ${BUILD_TYPES[*]}"
echo "Shared kinds: ${SHARED_KINDS[*]}"
echo "========================================"

for build_type in "${BUILD_TYPES[@]}"; do
    for shared_flag in "${SHARED_KINDS[@]}"; do
        for arch in "${MAC_ARCHES[@]}"; do
            build_slice "$arch" "$build_type" "$shared_flag"
        done
        combine_universal "$build_type" "$shared_flag"
    done
done

echo "macOS builds complete. See $OUTPUT_BASE/macOS/* for artifacts."

