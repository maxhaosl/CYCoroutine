#!/bin/bash

# Helper utilities to keep the Bin/ directory layout consistent with
# Build/build_all_platforms.sh under CYCoroutine.

map_platform_dir() {
    local raw="$1"
    local lower
    lower=$(printf '%s' "$raw" | tr '[:upper:]' '[:lower:]')
    case "$lower" in
        macos|mac|darwin)
            echo "macOS"
            ;;
        ios|iphone|ipad)
            echo "iOS"
            ;;
        android)
            echo "Android"
            ;;
        linux|gnu-linux)
            echo "Linux"
            ;;
        windows|win|msys|mingw|cygwin)
            echo "Windows"
            ;;
        *)
            echo "$raw"
            ;;
    esac
}

platform_slice_dir() {
    local platform=$1
    local arch=$2
    local build_type=$3
    local platform_dir
    platform_dir=$(map_platform_dir "$platform")
    echo "$OUTPUT_BASE/$platform_dir/$arch/$build_type"
}

platform_universal_dir() {
    local platform=$1
    local build_type=$2
    local platform_dir
    platform_dir=$(map_platform_dir "$platform")
    echo "$OUTPUT_BASE/$platform_dir/universal/$build_type"
}


