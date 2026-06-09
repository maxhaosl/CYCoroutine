#!/bin/bash

# ============================================================
# CYCoroutine macOS Build Script
#   Usage: build_mac.sh [all|Release|Debug] [archs...]
#   Default: all "arm64 x86_64"
# ============================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CYCOMMON_ROOT="$(dirname "$PROJECT_ROOT")/CYCommon"
CMAKE_SOURCE_DIR="$PROJECT_ROOT/Build"
OUTPUT_BASE="$PROJECT_ROOT/Bin"
mkdir -p "$OUTPUT_BASE"

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "This script must be run on macOS."
    exit 1
fi

# Verify CYCommon exists
if [ ! -f "$CYCOMMON_ROOT/Build/CMakeLists.txt" ]; then
    echo "Error: CYCommon not found at $CYCOMMON_ROOT"
    echo "Ensure CYCommon is checked out as a sibling of CYCoroutine under ThirdParty/"
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

# ---------- Arguments ----------
BUILD_TYPE="${1:-all}"
if [ "$#" -ge 2 ]; then
    MAC_ARCHES=("${@:2}")
else
    MAC_ARCHES=(arm64 x86_64)
fi

# ---------- Build types ----------
case "$BUILD_TYPE" in
    all|a|A|both|b|B)
        BUILD_TYPES=(Release Debug)
        ;;
    d|D|debug)
        BUILD_TYPES=(Debug)
        ;;
    r|R|release|Release)
        BUILD_TYPES=(Release)
        ;;
    Debug)
        BUILD_TYPES=(Debug)
        ;;
    *)
        echo "ERROR: Unknown BUILD_TYPE '$BUILD_TYPE'. Use: all, Release, Debug"
        exit 1
        ;;
esac

MACOS_DEPLOYMENT_TARGET=${MACOS_DEPLOYMENT_TARGET:-"11.0"}
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

build_cycommon_slice() {
    local arch=$1
    local build_type=$2
    local shared_flag=$3

    local shared_tag
    if [ "$shared_flag" = "ON" ]; then
        shared_tag="shared"
    else
        shared_tag="static"
    fi

    echo "=== Building CYCommon (macOS / $arch / $build_type / $shared_tag) ==="

    # Build CYCommon to its own standard output directory.
    # This avoids cross-contaminating CYCommon's build cache with CYCoroutine's output paths.
    bash "$CYCOMMON_ROOT/Build/build_all_mac.sh" \
        "$build_type" OFF "$arch"

    # Copy the CYCommon static library to CYCoroutine's Bin/MacOS directory for linking.
    # CYCommon writes to CYCommon/Bin/MacOS/<arch>/<BT>/libCYCommon[|D].a
    local cycommon_lib_name="libCYCommon.a"
    [ "$build_type" = "Debug" ] && cycommon_lib_name="libCYCommonD.a"
    local cycommon_src="$CYCOMMON_ROOT/Bin/MacOS/$arch/$build_type/$cycommon_lib_name"
    local cycoro_output_dir="$OUTPUT_BASE/MacOS/$arch/$build_type"
    mkdir -p "$cycoro_output_dir"
    if [ -f "$cycommon_src" ]; then
        cp "$cycommon_src" "$cycoro_output_dir/$cycommon_lib_name"
        echo "  Copied CYCommon -> $cycoro_output_dir/$cycommon_lib_name"
    else
        echo "  Warning: CYCommon not found at $cycommon_src"
    fi
}

cache_static_artifact() {
    local arch=$1
    local build_type=$2

    local lib_name="libCYCoroutine.a"
    [ "$build_type" = "Debug" ] && lib_name="libCYCoroutineD.a"

    local src="$OUTPUT_BASE/MacOS/$arch/$build_type/$lib_name"
    local cache_dir="$OUTPUT_BASE/.static-cache/MacOS/$arch/$build_type"
    mkdir -p "$cache_dir"

    if [ -f "$src" ]; then
        cp "$src" "$cache_dir/$lib_name"
    fi
}

restore_static_artifact() {
    local arch=$1
    local build_type=$2

    local lib_name="libCYCoroutine.a"
    [ "$build_type" = "Debug" ] && lib_name="libCYCoroutineD.a"

    local cache_dir="$OUTPUT_BASE/.static-cache/MacOS/$arch/$build_type"
    local dst_dir="$OUTPUT_BASE/MacOS/$arch/$build_type"
    mkdir -p "$dst_dir"

    if [ -f "$cache_dir/$lib_name" ]; then
        cp "$cache_dir/$lib_name" "$dst_dir/$lib_name"
    fi
}

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

    # Ensure CYCommon static library is built first (needed for linking)
    build_cycommon_slice "$arch" "$build_type" "$shared_flag"

    # CYCOMMON_LIB_PATH points to the CYCommon static library that was copied into
    # CYCoroutine's output directory. Pass it as an env var so CYCoroutine's
    # CMakeLists.txt can find it without searching the filesystem.
    local cycommon_lib_name="libCYCommon.a"
    [ "$build_type" = "Debug" ] && cycommon_lib_name="libCYCommonD.a"
    local cycommon_lib_path="$OUTPUT_BASE/MacOS/$arch/$build_type/$cycommon_lib_name"

    cmake -S "$CMAKE_SOURCE_DIR" \
          -B "$build_dir" \
          -DCMAKE_BUILD_TYPE="$build_type" \
          -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOS_DEPLOYMENT_TARGET" \
          -DCMAKE_OSX_ARCHITECTURES="$arch" \
          -DBUILD_SHARED_LIBS="$shared_flag" \
          -DBUILD_STATIC_LIBS="$static_flag" \
          -DBUILD_EXAMPLES=OFF \
          -DCYCOMMON_LIB_PATH="$cycommon_lib_path"

    cmake --build "$build_dir" --target "$target_name" --parallel "$BUILD_JOBS"

    if [ "$shared_flag" = "OFF" ]; then
        cache_static_artifact "$arch" "$build_type"
    else
        restore_static_artifact "$arch" "$build_type"
    fi

    local arch_dir="$OUTPUT_BASE/MacOS/$arch/$build_type"
    echo "Artifacts: $arch_dir"
}

combine_universal() {
    local build_type=$1
    local shared_flag=$2

    if ! command -v lipo >/dev/null 2>&1; then
        echo "Skipping MacOS universal ${build_type}/${shared_flag}: lipo not found."
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

    # Determine the output name: Debug builds get 'D' suffix
    local output_name="CYCoroutine"
    [ "$build_type" = "Debug" ] && output_name="CYCoroutineD"

    local src_arm="$OUTPUT_BASE/MacOS/arm64/$build_type/lib${output_name}.$suffix"
    local src_x86="$OUTPUT_BASE/MacOS/x86_64/$build_type/lib${output_name}.$suffix"

    if [ ! -f "$src_arm" ] || [ ! -f "$src_x86" ]; then
        echo "Skipping MacOS $build_type $lib_desc universal library: missing slices."
        [ ! -f "$src_arm" ] && echo "  Missing: $src_arm"
        [ ! -f "$src_x86" ] && echo "  Missing: $src_x86"
        return
    fi

    local dest_dir="$OUTPUT_BASE/MacOS/universal/$build_type"
    mkdir -p "$dest_dir"
    local dest="$dest_dir/lib${output_name}.$suffix"

    echo "Creating MacOS $build_type $lib_desc universal library..."
    lipo -create "$src_arm" "$src_x86" -output "$dest"
    echo "Universal artifact: $dest"
}

# Create universal CYCommon static libraries from the per-arch slices copied into CYCoroutine's output
combine_universal_cycommon() {
    local build_type=$1

    if ! command -v lipo >/dev/null 2>&1; then
        echo "Skipping CYCommon universal: lipo not found."
        return
    fi

    local lib_name="libCYCommon.a"
    [ "$build_type" = "Debug" ] && lib_name="libCYCommonD.a"

    local src_arm="$OUTPUT_BASE/MacOS/arm64/$build_type/$lib_name"
    local src_x86="$OUTPUT_BASE/MacOS/x86_64/$build_type/$lib_name"

    if [ ! -f "$src_arm" ] || [ ! -f "$src_x86" ]; then
        echo "Skipping CYCommon universal $build_type: missing slices."
        [ ! -f "$src_arm" ] && echo "  Missing: $src_arm"
        [ ! -f "$src_x86" ] && echo "  Missing: $src_x86"
        return
    fi

    local dest_dir="$OUTPUT_BASE/MacOS/universal/$build_type"
    mkdir -p "$dest_dir"
    local dest="$dest_dir/$lib_name"

    echo "Creating CYCommon universal $build_type static library..."
    lipo -create "$src_arm" "$src_x86" -output "$dest"
    echo "Universal CYCommon artifact: $dest"
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

# Also create universal CYCommon static libraries
for build_type in "${BUILD_TYPES[@]}"; do
    combine_universal_cycommon "$build_type"
done

echo "macOS builds complete. See $OUTPUT_BASE/MacOS/* for artifacts."
