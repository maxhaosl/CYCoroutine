#!/bin/bash

# ============================================================
# CYCoroutine Android Full Build - all ABIs + all BuildTypes
# ============================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

BUILD_TYPES=("Debug" "Release")
ANDROID_ABIS=("arm64-v8a" "armeabi-v7a" "x86" "x86_64")

failed=0
for BUILD_TYPE in "${BUILD_TYPES[@]}"; do
    for ABI in "${ANDROID_ABIS[@]}"; do
        echo "========================================"
        echo "Building CYCoroutine for Android ABI: $ABI"
        echo "  Build Type: $BUILD_TYPE"
        echo "========================================"

        if "$SCRIPT_DIR/build_android.sh" "$BUILD_TYPE" "$ABI"; then
            echo "Successfully built CYCoroutine for $ABI ($BUILD_TYPE)"
        else
            echo "Failed to build CYCoroutine for $ABI ($BUILD_TYPE)"
            failed=1
        fi
        echo ""
    done
done

echo "========================================"
if [ "$failed" -eq 0 ]; then
    echo "All CYCoroutine Android builds completed successfully!"
else
    echo "Some CYCoroutine Android builds FAILED!"
    exit 1
fi
echo "========================================"

echo ""
echo "Generated libraries:"
find "$SCRIPT_DIR/../Bin/Android" -name "*.a" -o -name "*.so" 2>/dev/null | sort
