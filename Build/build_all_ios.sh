#!/bin/bash

# ============================================================
# CYCoroutine iOS Build Script
#   Usage: build_ios.sh [all|Release|Debug]
#   Default: all
# ============================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CYCOMMON_ROOT="$(dirname "$PROJECT_ROOT")/CYCommon"
CMAKE_SOURCE_DIR="$PROJECT_ROOT/Build"
OUTPUT_BASE="$PROJECT_ROOT/Bin"
mkdir -p "$OUTPUT_BASE"

# Verify CYCommon exists
if [ ! -f "$CYCOMMON_ROOT/Build/CMakeLists.txt" ]; then
    echo "Error: CYCommon not found at $CYCOMMON_ROOT"
    echo "Ensure CYCommon is checked out as a sibling of CYCoroutine under ThirdParty/"
    exit 1
fi

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "iOS builds require a macOS host."
    exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
    echo "CMake is required but was not found in PATH."
    exit 1
fi

if ! command -v xcodebuild >/dev/null 2>&1; then
    echo "Xcode command line tools are missing. Run 'xcode-select --install' first."
    exit 1
fi

# ---------- Arguments ----------
BUILD_TYPE="${1:-all}"

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

IOS_DEPLOYMENT_TARGET=${IOS_DEPLOYMENT_TARGET:-"14.0"}
IOS_ARCHES=(${CYCOROUTINE_IOS_ARCHES:-arm64 x86_64 arm64-simulator})
SHARED_KINDS=(${CYCOROUTINE_SHARED_KINDS:-OFF ON})

detect_jobs() {
    if command -v sysctl >/dev/null 2>&1; then
        sysctl -n hw.ncpu 2>/dev/null && return
    fi
    echo 4
}
BUILD_JOBS=${BUILD_JOBS:-$(detect_jobs)}

ios_sysroot_for_arch() {
    local arch=$1
    case "$arch" in
        arm64) echo "iphoneos" ;;
        x86_64|arm64-simulator) echo "iphonesimulator" ;;
        *)
            echo "Unsupported iOS architecture: $arch"
            exit 1
            ;;
    esac
}

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

    local cycommon_build_dir="$CYCOMMON_ROOT/Build/build_ios_${arch}_${build_type}_${shared_tag}"
    local sysroot
    sysroot=$(ios_sysroot_for_arch "$arch")
    echo "=== Building CYCommon (iOS / $arch / $build_type / $shared_tag) ==="

    # Determine the actual compiler architecture.
    # arm64-simulator -> arm64 (compiler), arm64 (device) -> arm64, x86_64 -> x86_64
    local cmake_arch="$arch"
    if [ "$arch" = "arm64-simulator" ]; then
        cmake_arch="arm64"
    fi

    cmake -S "$CYCOMMON_ROOT/Build" \
          -B "$cycommon_build_dir" \
          -DCMAKE_SYSTEM_NAME=iOS \
          -DCMAKE_OSX_ARCHITECTURES="$cmake_arch" \
          -DCMAKE_OSX_SYSROOT="$sysroot" \
          -DCMAKE_OSX_DEPLOYMENT_TARGET="$IOS_DEPLOYMENT_TARGET" \
          -DCMAKE_BUILD_TYPE="$build_type" \
          -DBUILD_SHARED_LIBS=OFF \
          -DBUILD_STATIC_LIBS=ON \
          -DCYCOMMON_OUTPUT_ARCH="$arch" \
          -DCYCOMMON_OUTPUT_DIR_IS_FINAL=ON

    cmake --build "$cycommon_build_dir" --target CYCommon_static --parallel "$BUILD_JOBS"
}

# For arm64-simulator shared build: the CMake build outputs to arm64/ directory
# (CMAKE_OSX_ARCHITECTURES=arm64 for both device and arm64-simulator).
# We need to save the device dylib BEFORE this build and restore it AFTER.
save_arm64_device_dylib() {
    local build_type=$1
    local device_dir="$OUTPUT_BASE/iOS/device/$build_type"
    mkdir -p "$device_dir"
    local arm64_shared="$OUTPUT_BASE/iOS/arm64/$build_type/libCYCoroutine.dylib"
    local arm64_shared_d="$OUTPUT_BASE/iOS/arm64/$build_type/libCYCoroutineD.dylib"
    if [ -f "$arm64_shared" ]; then
        cp "$arm64_shared" "$device_dir/libCYCoroutine.dylib"
        echo "  Saved arm64 device dylib before arm64-simulator build"
    fi
    if [ -f "$arm64_shared_d" ]; then
        cp "$arm64_shared_d" "$device_dir/libCYCoroutineD.dylib"
        echo "  Saved arm64 device dylib (Debug) before arm64-simulator build"
    fi
}

restore_arm64_device_dylib() {
    local build_type=$1
    local device_dir="$OUTPUT_BASE/iOS/device/$build_type"
    local arm64_shared="$OUTPUT_BASE/iOS/arm64/$build_type/libCYCoroutine.dylib"
    local arm64_shared_d="$OUTPUT_BASE/iOS/arm64/$build_type/libCYCoroutineD.dylib"
    if [ -f "$device_dir/libCYCoroutine.dylib" ]; then
        cp "$device_dir/libCYCoroutine.dylib" "$arm64_shared"
        echo "  Restored arm64 device dylib after arm64-simulator build"
    fi
    if [ -f "$device_dir/libCYCoroutineD.dylib" ]; then
        cp "$device_dir/libCYCoroutineD.dylib" "$arm64_shared_d"
        echo "  Restored arm64 device dylib (Debug) after arm64-simulator build"
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

    # Before arm64-simulator shared build: save the device dylib (will be overwritten by simulator)
    if [ "$arch" = "arm64-simulator" ] && [ "$shared_flag" = "ON" ]; then
        save_arm64_device_dylib "$build_type"
    fi

    local build_dir="$SCRIPT_DIR/build_ios_${arch}_${build_type}_${shared_tag}"

    echo "=== iOS / $arch / $build_type / $shared_tag ==="

    local sysroot cmake_arch
    case "$arch" in
        arm64) sysroot="iphoneos"; cmake_arch="arm64" ;;
        x86_64) sysroot="iphonesimulator"; cmake_arch="x86_64" ;;
        arm64-simulator) sysroot="iphonesimulator"; cmake_arch="arm64" ;;
        *)
            echo "Unsupported iOS architecture: $arch"
            exit 1
            ;;
    esac

    local _cycommon_lib_name="libCYCommon.a"
    if [ "$build_type" = "Debug" ]; then
        _cycommon_lib_name="libCYCommonD.a"
    fi
    cmake -S "$CMAKE_SOURCE_DIR" \
          -B "$build_dir" \
          -DCMAKE_SYSTEM_NAME=iOS \
          -DCMAKE_OSX_ARCHITECTURES="$cmake_arch" \
          -DCMAKE_OSX_SYSROOT="$sysroot" \
          -DCMAKE_OSX_DEPLOYMENT_TARGET="$IOS_DEPLOYMENT_TARGET" \
          -DCMAKE_BUILD_TYPE="$build_type" \
          -DBUILD_SHARED_LIBS="$shared_flag" \
          -DBUILD_STATIC_LIBS="$static_flag" \
          -DBUILD_EXAMPLES=OFF \
          -DTARGET_ARCH="$arch" \
          -DCYCOMMON_LIB_PATH="$CYCOMMON_ROOT/Bin/iOS/$arch/$_cycommon_lib_name"

    cmake --build "$build_dir" --target "$target_name" --parallel "$BUILD_JOBS"

    local arch_dir="$OUTPUT_BASE/iOS/$arch/$build_type"
    mkdir -p "$arch_dir"
    echo "Artifacts: $arch_dir"

    # Determine the actual CYCoroutine output filename
    local suffix_val
    if [ "$shared_flag" = "ON" ]; then
        suffix_val="dylib"
    else
        suffix_val="a"
    fi
    local cycoroutine_lib_name
    if [ "$build_type" = "Debug" ]; then
        cycoroutine_lib_name="libCYCoroutineD.$suffix_val"
    else
        cycoroutine_lib_name="libCYCoroutine.$suffix_val"
    fi

    # Copy CYCoroutine output from the actual cmake output directory
    # On iOS, arm64-simulator builds output to arm64/ directory (CMAKE_OSX_ARCHITECTURES=arm64)
    # so we need to check the actual location and copy to the correct arch dir.
    local cmake_out_dir="$OUTPUT_BASE/iOS/$arch/$build_type"
    local cmake_out_file="$cmake_out_dir/$cycoroutine_lib_name"

    # If not found at expected path, try the arm64/ fallback (for arm64-simulator)
    if [ ! -f "$cmake_out_file" ] && [ "$arch" = "arm64-simulator" ]; then
        cmake_out_file="$OUTPUT_BASE/iOS/arm64/$build_type/$cycoroutine_lib_name"
    fi

    if [ -f "$cmake_out_file" ]; then
        local src_basename="$(basename "$cmake_out_file")"
        if [ "$cmake_out_file" != "$arch_dir/$src_basename" ]; then
            cp "$cmake_out_file" "$arch_dir/"
            echo "  Copied CYCoroutine: $src_basename -> $arch_dir/"
        else
            echo "  CYCoroutine already in place: $src_basename"
        fi
    fi

    # Copy CYCommon into CYCoroutine's arch output directory
    local cycommon_lib_name
    if [ "$build_type" = "Debug" ]; then
        cycommon_lib_name="libCYCommonD.a"
    else
        cycommon_lib_name="libCYCommon.a"
    fi
    local cycommon_src="$CYCOMMON_ROOT/Bin/iOS/$arch/$cycommon_lib_name"
    if [ -f "$cycommon_src" ]; then
        cp "$cycommon_src" "$arch_dir/"
        echo "  Copied CYCommon: $cycommon_lib_name -> $arch_dir/"
    fi

    # After arm64-simulator shared build: restore the device dylib that was overwritten
    if [ "$arch" = "arm64-simulator" ] && [ "$shared_flag" = "ON" ]; then
        restore_arm64_device_dylib "$build_type"
    fi

    normalize_and_copy_dylibs "$arch" "$build_type" "$shared_flag"
}

# Strip CMake version suffixes from dylibs and rename to unversioned name.
# E.g. libCYCoroutine.1.0.0.dylib -> libCYCoroutine.dylib
normalize_and_copy_dylibs() {
    local arch=$1
    local build_type=$2
    local shared_flag=$3

    if [ "$shared_flag" != "ON" ]; then
        return
    fi

    local output_name="CYCoroutine"
    if [ "$build_type" = "Debug" ]; then
        output_name="CYCoroutineD"
    fi
    local src_dir="$OUTPUT_BASE/iOS/$arch/$build_type"

    # Normalize the dylib: strip versioned suffixes and clean up versioned files
    for base in "lib${output_name}"; do
        for ver_suffix in ".1.0.0.dylib" ".1.dylib"; do
            local ver_file="$src_dir/${base}${ver_suffix}"
            if [ -f "$ver_file" ]; then
                local plain_file="$src_dir/${base}.dylib"
                rm -f "$plain_file"
                cp "$ver_file" "$plain_file"
                echo "  Normalized dylib: $(basename "$ver_file") -> $(basename "$plain_file")"
                # Clean up versioned files now that we have the normalized copy
                rm -f "$src_dir/${base}.1.0.0.dylib" "$src_dir/${base}.1.dylib"
                break
            fi
        done
    done
}

combine_fat() {
    local platform=$1
    local build_type=$2
    local shared_flag=$3

    if ! command -v lipo >/dev/null 2>&1; then
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

    local output_name="CYCoroutine"
    if [ "$build_type" = "Debug" ]; then
        output_name="CYCoroutineD"
    fi

    # Normalize dylibs before combining (strip version suffixes)
    if [ "$shared_flag" = "ON" ]; then
        for arch in arm64 x86_64 arm64-simulator; do
            normalize_and_copy_dylibs "$arch" "$build_type" "$shared_flag"
        done
    fi

    # Explicit per-platform to avoid `set -u` issues with empty array expansion.
    local _lib1="" _lib2="" _count=0
    local -a _inputs=()

    case "$platform" in
        device)
            _lib1="$OUTPUT_BASE/iOS/arm64/$build_type/lib${output_name}.$suffix"
            _count=1
            ;;
        simulator)
            _lib1="$OUTPUT_BASE/iOS/x86_64/$build_type/lib${output_name}.$suffix"
            _lib2="$OUTPUT_BASE/iOS/arm64-simulator/$build_type/lib${output_name}.$suffix"
            _count=2
            ;;
        universal)
            _lib1="$OUTPUT_BASE/iOS/arm64/$build_type/lib${output_name}.$suffix"
            _lib2="$OUTPUT_BASE/iOS/x86_64/$build_type/lib${output_name}.$suffix"
            _count=2
            ;;
    esac

    [ -n "$_lib1" ] && [ -f "$_lib1" ] && _inputs+=("$_lib1")
    [ -n "$_lib2" ] && [ -f "$_lib2" ] && _inputs+=("$_lib2")

    if [ ${#_inputs[@]} -eq 0 ]; then
        echo "  Skipping ${platform} ${lib_desc} fat binary (no slices found)"
        return
    fi

    local dest_dir="$OUTPUT_BASE/iOS/$platform/$build_type"
    mkdir -p "$dest_dir"

    if [ ${#_inputs[@]} -ge 2 ]; then
        local dest="$dest_dir/lib${output_name}.$suffix"
        lipo -create "${_inputs[@]}" -output "$dest"
        echo "  Created ${platform} ${lib_desc} fat: $dest (${#_inputs[@]} slices)"
    else
        # Single slice: copy instead of lipo
        cp "${_inputs[0]}" "$dest_dir/lib${output_name}.$suffix"
        echo "  Copied ${platform} ${lib_desc} single-arch: $dest_dir/lib${output_name}.$suffix (1 slice)"
    fi

    # Also copy CYCommon into platform fat dir
    local cycommon_lib_name
    if [ "$build_type" = "Debug" ]; then
        cycommon_lib_name="libCYCommonD.a"
    else
        cycommon_lib_name="libCYCommon.a"
    fi
    if [ "$platform" = "universal" ] || [ "$platform" = "device" ]; then
        local cycommon_src="$CYCOMMON_ROOT/Bin/iOS/arm64/$cycommon_lib_name"
        if [ -f "$cycommon_src" ]; then
            cp "$cycommon_src" "$dest_dir/"
        fi
    elif [ "$platform" = "simulator" ]; then
        local cycommon_src="$CYCOMMON_ROOT/Bin/iOS/x86_64/$cycommon_lib_name"
        if [ -f "$cycommon_src" ]; then
            cp "$cycommon_src" "$dest_dir/"
        fi
    fi
}

echo "========================================"
echo "Building CYCoroutine for iOS"
echo "Output root: $OUTPUT_BASE"
echo "Architectures: ${IOS_ARCHES[*]}"
echo "Build types: ${BUILD_TYPES[*]}"
echo "Shared kinds: ${SHARED_KINDS[*]}"
echo "========================================"

for build_type in "${BUILD_TYPES[@]}"; do
    for shared_flag in "${SHARED_KINDS[@]}"; do
        for arch in "${IOS_ARCHES[@]}"; do
            build_cycommon_slice "$arch" "$build_type" "$shared_flag"
            build_slice "$arch" "$build_type" "$shared_flag"
        done
        combine_fat "device" "$build_type" "$shared_flag"
        combine_fat "simulator" "$build_type" "$shared_flag"
        combine_fat "universal" "$build_type" "$shared_flag"
    done
done

echo "iOS builds complete. See $OUTPUT_BASE/iOS/* for artifacts."
