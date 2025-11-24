#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CMAKE_SOURCE_DIR="$PROJECT_ROOT/Build"
OUTPUT_BASE="$PROJECT_ROOT/Bin"
mkdir -p "$OUTPUT_BASE"

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

IOS_DEPLOYMENT_TARGET=${IOS_DEPLOYMENT_TARGET:-"14.0"}
BUILD_TYPES=(${CYCOROUTINE_BUILD_TYPES:-Release})
IOS_ARCHES=(${CYCOROUTINE_IOS_ARCHES:-arm64 x86_64})
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

    local build_dir="$SCRIPT_DIR/build_ios_${arch}_${build_type}_${shared_tag}"
    local sysroot
    sysroot=$(ios_sysroot_for_arch "$arch")

    echo "=== iOS / $arch / $build_type / $shared_tag ==="

    cmake -S "$CMAKE_SOURCE_DIR" \
          -B "$build_dir" \
          -DCMAKE_SYSTEM_NAME=iOS \
          -DCMAKE_OSX_ARCHITECTURES="$arch" \
          -DCMAKE_OSX_SYSROOT="$sysroot" \
          -DCMAKE_OSX_DEPLOYMENT_TARGET="$IOS_DEPLOYMENT_TARGET" \
          -DCMAKE_BUILD_TYPE="$build_type" \
          -DBUILD_SHARED_LIBS="$shared_flag" \
          -DBUILD_STATIC_LIBS="$static_flag" \
          -DBUILD_EXAMPLES=OFF

    cmake --build "$build_dir" --target "$target_name" --parallel "$BUILD_JOBS"

    local arch_dir="$OUTPUT_BASE/iOS/$arch/$build_type"
    echo "Artifacts: $arch_dir"
}

combine_universal() {
    local build_type=$1
    local shared_flag=$2

    if ! command -v lipo >/dev/null 2>&1; then
        echo "Skipping iOS universal ${build_type}/${shared_flag}: lipo not found."
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

    local -a candidate_arches=("${IOS_ARCHES[@]}")
    local canonical
    for canonical in arm64 x86_64 arm64-simulator; do
        local seen=0
        local existing
        for existing in "${candidate_arches[@]}"; do
            if [ "$existing" = "$canonical" ]; then
                seen=1
                break
            fi
        done
        if [ "$seen" -eq 0 ]; then
            candidate_arches+=("$canonical")
        fi
    done

    local -a inputs=()
    local arch
    for arch in "${candidate_arches[@]}"; do
        local lib_path="$OUTPUT_BASE/iOS/$arch/$build_type/libCYCoroutine.$suffix"
        if [ -f "$lib_path" ]; then
            local duplicate=0
            if [ "${#inputs[@]}" -gt 0 ]; then
                local existing_path
                for existing_path in "${inputs[@]}"; do
                    if [ "$existing_path" = "$lib_path" ]; then
                        duplicate=1
                        break
                    fi
                done
            fi
            [ "$duplicate" -eq 1 ] && continue
            inputs+=("$lib_path")
        fi
    done

    if [ ${#inputs[@]} -lt 2 ]; then
        echo "Skipping iOS $build_type $lib_desc universal library: need at least two slices."
        return
    fi

    local dest_dir="$OUTPUT_BASE/iOS/universal/$build_type"
    mkdir -p "$dest_dir"
    local dest="$dest_dir/libCYCoroutine.$suffix"

    echo "Creating iOS $build_type $lib_desc universal library (${#inputs[@]} slices)..."
    if [ "${#inputs[@]}" -gt 0 ]; then
        lipo -create "${inputs[@]}" -output "$dest"
    fi

    if [ "$shared_flag" = "ON" ]; then
        local versioned
        for versioned in "$OUTPUT_BASE/iOS/arm64/$build_type"/libCYCoroutine.*.dylib; do
            [ -e "$versioned" ] || continue
            local base="$(basename "$versioned")"
            [ "$base" = "libCYCoroutine.dylib" ] && continue
            ln -sf "libCYCoroutine.dylib" "$dest_dir/$base"
        done
    fi

    echo "Universal artifact: $dest"
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
            build_slice "$arch" "$build_type" "$shared_flag"
        done
        combine_universal "$build_type" "$shared_flag"
    done
done

echo "iOS builds complete. See $OUTPUT_BASE/iOS/* for artifacts."

