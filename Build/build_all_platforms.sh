#!/bin/bash

# Multi-platform build helper for CYCoroutine
# Builds macOS, iOS, and Android matrices with minimal configuration flags

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CMAKE_SOURCE_DIR="$PROJECT_ROOT/Build"
OUTPUT_DIR="$PROJECT_ROOT/Bin"
mkdir -p "$OUTPUT_DIR"

MACOS_DEPLOYMENT_TARGET=${MACOS_DEPLOYMENT_TARGET:-"11.0"}
IOS_DEPLOYMENT_TARGET=${IOS_DEPLOYMENT_TARGET:-"14.0"}
ANDROID_API_LEVEL_DEFAULT=${ANDROID_API_LEVEL:-"23"}

MAC_ARCHES_DEFAULT=("arm64" "x86_64")
IOS_ARCHES_DEFAULT=("arm64" "x86_64")
ANDROID_ABIS_DEFAULT=("arm64-v8a" "armeabi-v7a" "x86_64" "x86")
BUILD_TYPES_DEFAULT=("Release" "Debug")
SHARED_KINDS_DEFAULT=("OFF" "ON")
DEFAULT_PLATFORMS=("macos" "ios" "android")

MAC_ARCHES=("${MAC_ARCHES_DEFAULT[@]}")
IOS_ARCHES=("${IOS_ARCHES_DEFAULT[@]}")
ANDROID_ABIS=("${ANDROID_ABIS_DEFAULT[@]}")
BUILD_TYPES=("${BUILD_TYPES_DEFAULT[@]}")
SHARED_KINDS=("${SHARED_KINDS_DEFAULT[@]}")
SELECTED_PLATFORMS=()

detect_jobs() {
    if command -v sysctl >/dev/null 2>&1; then
        sysctl -n hw.logicalcpu 2>/dev/null && return
    fi
    if command -v nproc >/dev/null 2>&1; then
        nproc && return
    fi
    echo 4
}
BUILD_JOBS=${BUILD_JOBS:-$(detect_jobs)}

usage() {
    cat <<'EOF'
Usage: build_all_platforms.sh [options]

Options:
  --platforms macos,ios,android        Comma-separated list of platforms.
  --build-types Release,Debug          Comma-separated build types.
  --shared-kinds static,shared         Library flavors (static=OFF, shared=ON).
  --mac-arches arm64,x86_64            macOS architectures to build.
  --ios-arches arm64,x86_64            iOS architectures to build.
  --android-abis arm64-v8a,...         Android ABIs to build.
  --android-api-level <level>          Default Android API level (per ABI mins enforced).
  -h, --help                           Show this help and exit.

Environment overrides:
  MACOS_DEPLOYMENT_TARGET, IOS_DEPLOYMENT_TARGET, ANDROID_API_LEVEL, BUILD_JOBS.
EOF
}

map_platform_dir() {
    case "$1" in
        macos) echo "macOS" ;;
        ios) echo "iOS" ;;
        android) echo "Android" ;;
        *) echo "$1" ;;
    esac
}

platform_enabled() {
    local needle=$1
    local platform
    for platform in "${SELECTED_PLATFORMS[@]}"; do
        if [ "$platform" = "$needle" ]; then
            return 0
        fi
    done
    return 1
}

shared_ext_by_platform() {
    case "$1" in
        macos|ios) echo "dylib" ;;
        android) echo "so" ;;
        *) echo "a" ;;
    esac
}

ios_sysroot_for_arch() {
    local arch=$1
    case "$arch" in
        arm64) echo "iphoneos" ;;
        x86_64|arm64-simulator) echo "iphonesimulator" ;;
        *)
            log_error "Unsupported iOS architecture: $arch"
            exit 1
            ;;
    esac
}

detect_android_sdk() {
    local -a candidates=(
        "${ANDROID_SDK_ROOT:-}"
        "${ANDROID_HOME:-}"
        "${HOME}/Library/Android/sdk"
        "${HOME}/Android/Sdk"
        "${HOME}/Android/sdk"
        "/usr/local/share/android-sdk"
        "/opt/android-sdk"
    )
    local path
    for path in "${candidates[@]}"; do
        if [ -n "$path" ] && [ -d "$path" ]; then
            echo "$path"
            return
        fi
    done
}

ndk_has_toolchain() {
    local ndk_root=$1
    if [ -n "$ndk_root" ] && [ -f "$ndk_root/build/cmake/android.toolchain.cmake" ]; then
        return 0
    fi
    return 1
}

detect_android_ndk() {
    local sdk_root=$1
    local -a candidates=(
        "${ANDROID_NDK_HOME:-}"
        "${ANDROID_NDK_ROOT:-}"
        "${ANDROID_NDK:-}"
    )
    local path
    for path in "${candidates[@]}"; do
        if [ -n "$path" ] && [ -d "$path" ] && ndk_has_toolchain "$path"; then
            echo "$path"
            return
        fi
    done

    if [ -d "$sdk_root/ndk" ]; then
        local candidate
        for candidate in $(ls -1 "$sdk_root/ndk" | sort -r); do
            local full_path="$sdk_root/ndk/$candidate"
            if [ -d "$full_path" ] && ndk_has_toolchain "$full_path"; then
                echo "$full_path"
                return
            fi
        done
    fi

    if [ -d "$sdk_root/ndk-bundle" ] && ndk_has_toolchain "$sdk_root/ndk-bundle"; then
        echo "$sdk_root/ndk-bundle"
    fi
}

prepare_android_toolchain() {
    if [ "${ANDROID_SETUP_STATE:-}" = "ready" ]; then
        return 0
    fi

    local sdk_root
    sdk_root=$(detect_android_sdk)
    if [ -z "$sdk_root" ]; then
        log_error "Android SDK not found. Install it or set ANDROID_SDK_ROOT."
        exit 1
    fi

    local ndk_root
    ndk_root=$(detect_android_ndk "$sdk_root")
    if [ -z "$ndk_root" ]; then
        log_error "Android NDK not found under $sdk_root. Install it via sdkmanager."
        exit 1
    fi

    export ANDROID_SDK_ROOT="$sdk_root"
    export ANDROID_HOME="$sdk_root"
    export ANDROID_NDK_HOME="$ndk_root"
    export ANDROID_NDK_ROOT="$ndk_root"
    ANDROID_SETUP_STATE="ready"
    log_info "Detected Android SDK at $ANDROID_SDK_ROOT"
    log_info "Detected Android NDK at $ANDROID_NDK_HOME"
}

android_api_for_abi() {
    local abi=$1
    local min_api
    case "$abi" in
        armeabi-v7a|x86) min_api=19 ;;
        *) min_api=21 ;;
    esac

    local requested="$ANDROID_API_LEVEL_DEFAULT"
    if ! [[ $requested =~ ^[0-9]+$ ]]; then
        requested=$min_api
    fi

    if [ "$requested" -lt "$min_api" ]; then
        echo "$min_api"
    else
        echo "$requested"
    fi
}

build_slice() {
    local platform=$1
    local arch=$2
    local build_type=$3
    local shared=$4
    local deployment_target=${5:-}

    local shared_tag static_flag target_label
    if [ "$shared" = "ON" ]; then
        shared_tag="shared"
        static_flag="OFF"
        target_label="CYCoroutine_shared"
    else
        shared_tag="static"
        static_flag="ON"
        target_label="CYCoroutine_static"
    fi

    local build_dir="$SCRIPT_DIR/build_${platform}_${arch}_${build_type}_${shared_tag}"

    log_info "Building ${platform} / ${arch} / ${build_type} / ${shared_tag}"

    local -a cmake_args=(
        -S "$CMAKE_SOURCE_DIR"
        -B "$build_dir"
        "-DCMAKE_BUILD_TYPE=$build_type"
        "-DBUILD_SHARED_LIBS=$shared"
        "-DBUILD_STATIC_LIBS=$static_flag"
        "-DBUILD_EXAMPLES=OFF"
        "-DTARGET_ARCH=$arch"
    )

    case "$platform" in
        macos)
            cmake_args+=("-DCMAKE_OSX_DEPLOYMENT_TARGET=$deployment_target")
            cmake_args+=("-DCMAKE_OSX_ARCHITECTURES=$arch")
            ;;
        ios)
            cmake_args+=("-DCMAKE_SYSTEM_NAME=iOS")
            cmake_args+=("-DCMAKE_OSX_ARCHITECTURES=$arch")
            cmake_args+=("-DCMAKE_OSX_DEPLOYMENT_TARGET=$deployment_target")
            cmake_args+=("-DCMAKE_OSX_SYSROOT=$(ios_sysroot_for_arch "$arch")")
            ;;
        android)
            prepare_android_toolchain
            cmake_args+=("-DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake")
            cmake_args+=("-DANDROID_ABI=$arch")
            cmake_args+=("-DANDROID_PLATFORM=android-$deployment_target")
            cmake_args+=("-DANDROID_STL=c++_static")
            ;;
        *)
            log_error "Unknown platform slice: $platform"
            exit 1
            ;;
    esac

    cmake "${cmake_args[@]}"
    cmake --build "$build_dir" --target "$target_label" --parallel "$BUILD_JOBS"

    log_info "Finished ${platform} / ${arch} / ${build_type} / ${shared_tag}"
}

combine_universal() {
    local platform=$1
    local build_type=$2
    local shared=$3

    local platform_dir
    platform_dir=$(map_platform_dir "$platform")
    local suffix
    if [ "$shared" = "ON" ]; then
        suffix=$(shared_ext_by_platform "$platform")
    else
        suffix="a"
    fi

    if ! command -v lipo >/dev/null 2>&1; then
        log_warn "lipo not available; skipping universal artifact for $platform_dir $build_type $suffix"
        return
    fi

    local dest_dir="$OUTPUT_DIR/$platform_dir/universal/$build_type"
    mkdir -p "$dest_dir"

    local arm_path="$OUTPUT_DIR/$platform_dir/arm64/$build_type/libCYCoroutine.$suffix"
    local x86_path="$OUTPUT_DIR/$platform_dir/x86_64/$build_type/libCYCoroutine.$suffix"
    local dest_path="$dest_dir/libCYCoroutine.$suffix"

    if [ -f "$arm_path" ] && [ -f "$x86_path" ]; then
        log_info "Creating $platform_dir universal $( [ "$shared" = "ON" ] && echo "shared" || echo "static") library for $build_type"
        lipo -create "$arm_path" "$x86_path" -output "$dest_path"
    else
        log_warn "Missing slices for $platform_dir $build_type $suffix; skipping universal merge"
    fi
}

build_macos_matrix() {
    local deployment_target=$1
    for build_type in "${BUILD_TYPES[@]}"; do
        for shared in "${SHARED_KINDS[@]}"; do
            local arch
            for arch in "${MAC_ARCHES[@]}"; do
                build_slice "macos" "$arch" "$build_type" "$shared" "$deployment_target"
            done
            combine_universal "macos" "$build_type" "$shared"
        done
    done
}

build_ios_matrix() {
    local deployment_target=$1
    for build_type in "${BUILD_TYPES[@]}"; do
        for shared in "${SHARED_KINDS[@]}"; do
            local arch
            for arch in "${IOS_ARCHES[@]}"; do
                build_slice "ios" "$arch" "$build_type" "$shared" "$deployment_target"
            done
            combine_universal "ios" "$build_type" "$shared"
        done
    done
}

build_android_matrix() {
    prepare_android_toolchain
    for build_type in "${BUILD_TYPES[@]}"; do
        for shared in "${SHARED_KINDS[@]}"; do
            local abi
            for abi in "${ANDROID_ABIS[@]}"; do
                local api_level
                api_level=$(android_api_for_abi "$abi")
                build_slice "android" "$abi" "$build_type" "$shared" "$api_level"
            done
        done
    done
}

parse_cli_args() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --platforms)
                [[ $# -lt 2 ]] && { log_error "--platforms requires a value"; usage; exit 1; }
                local IFS=','
                read -r -a SELECTED_PLATFORMS <<< "$(echo "$2" | tr '[:upper:]' '[:lower:]' | tr -d '[:space:]')"
                shift 2
                ;;
            --build-types)
                [[ $# -lt 2 ]] && { log_error "--build-types requires a value"; usage; exit 1; }
                local IFS=','
                read -r -a BUILD_TYPES <<< "$(echo "$2" | tr -d '[:space:]')"
                shift 2
                ;;
            --shared-kinds)
                [[ $# -lt 2 ]] && { log_error "--shared-kinds requires a value"; usage; exit 1; }
                SHARED_KINDS=()
                local token
                local IFS=','
                for token in $2; do
                    token=$(printf '%s' "$token" | tr '[:upper:]' '[:lower:]' | tr -d '[:space:]')
                    case "$token" in
                        on|shared) SHARED_KINDS+=("ON") ;;
                        off|static) SHARED_KINDS+=("OFF") ;;
                        "") ;;
                        *) log_warn "Ignoring unknown shared kind: $token" ;;
                    esac
                done
                shift 2
                ;;
            --mac-arches)
                [[ $# -lt 2 ]] && { log_error "--mac-arches requires a value"; usage; exit 1; }
                MAC_ARCHES=()
                local token
                local IFS=','
                for token in $2; do
                    token=$(printf '%s' "$token" | tr -d '[:space:]')
                    [ -n "$token" ] && MAC_ARCHES+=("$token")
                done
                shift 2
                ;;
            --ios-arches)
                [[ $# -lt 2 ]] && { log_error "--ios-arches requires a value"; usage; exit 1; }
                IOS_ARCHES=()
                local token
                local IFS=','
                for token in $2; do
                    token=$(printf '%s' "$token" | tr -d '[:space:]')
                    [ -n "$token" ] && IOS_ARCHES+=("$token")
                done
                shift 2
                ;;
            --android-abis)
                [[ $# -lt 2 ]] && { log_error "--android-abis requires a value"; usage; exit 1; }
                ANDROID_ABIS=()
                local token
                local IFS=','
                for token in $2; do
                    token=$(printf '%s' "$token" | tr -d '[:space:]')
                    [ -n "$token" ] && ANDROID_ABIS+=("$token")
                done
                shift 2
                ;;
            --android-api-level)
                [[ $# -lt 2 ]] && { log_error "--android-api-level requires a numeric value"; usage; exit 1; }
                ANDROID_API_LEVEL_DEFAULT=$2
                shift 2
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            *)
                log_error "Unknown argument: $1"
                usage
                exit 1
                ;;
        esac
    done

    if [ ${#SELECTED_PLATFORMS[@]} -eq 0 ]; then
        SELECTED_PLATFORMS=("${DEFAULT_PLATFORMS[@]}")
    fi
    if [ ${#BUILD_TYPES[@]} -eq 0 ]; then
        BUILD_TYPES=("${BUILD_TYPES_DEFAULT[@]}")
    fi
    if [ ${#SHARED_KINDS[@]} -eq 0 ]; then
        SHARED_KINDS=("${SHARED_KINDS_DEFAULT[@]}")
    fi
    if [ ${#MAC_ARCHES[@]} -eq 0 ]; then
        MAC_ARCHES=("${MAC_ARCHES_DEFAULT[@]}")
    fi
    if [ ${#IOS_ARCHES[@]} -eq 0 ]; then
        IOS_ARCHES=("${IOS_ARCHES_DEFAULT[@]}")
    fi
    if [ ${#ANDROID_ABIS[@]} -eq 0 ]; then
        ANDROID_ABIS=("${ANDROID_ABIS_DEFAULT[@]}")
    fi
}

main() {
    parse_cli_args "$@"
    log_info "Starting CYCoroutine multi-platform build"
    log_info "Enabled platforms: ${SELECTED_PLATFORMS[*]}"
    log_info "Build types: ${BUILD_TYPES[*]}"
    log_info "Library flavors: ${SHARED_KINDS[*]}"

    if platform_enabled "macos"; then
        if [[ "$OSTYPE" == "darwin"* ]]; then
            log_info "=== macOS matrix ==="
            build_macos_matrix "$MACOS_DEPLOYMENT_TARGET"
        else
            log_warn "macOS builds require a macOS host. Skipping."
        fi
    fi

    if platform_enabled "ios"; then
        if [[ "$OSTYPE" == "darwin"* ]]; then
            log_info "=== iOS matrix ==="
            build_ios_matrix "$IOS_DEPLOYMENT_TARGET"
        else
            log_warn "iOS builds require a macOS host. Skipping."
        fi
    fi

    if platform_enabled "android"; then
        log_info "=== Android matrix ==="
        build_android_matrix
    fi

    log_info "Build artifacts are available under $OUTPUT_DIR"
}

main "$@"

