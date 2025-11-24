#!/bin/bash

# Configure base paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(dirname "$SCRIPT_DIR")"

# Detect host operating system
OS=$(uname -s)

echo "========================================"
echo "CYCoroutine Cross-Platform Build Script"
echo "========================================"
echo "Detected OS: $OS"

# Dispatch to the platform-specific builder
case "$OS" in
    Darwin*)
        echo "Building for macOS..."
        "$SCRIPT_DIR/build_unix.sh"
        ;;
    Linux*)
        echo "Building for Linux..."
        "$SCRIPT_DIR/build_unix.sh"
        ;;
    CYGWIN*|MINGW*|MSYS*)
        echo "Building for Windows..."
        "$SCRIPT_DIR/build_windows.bat"
        ;;
    *)
        echo "Error: Unsupported operating system: $OS"
        exit 1
        ;;
esac

# Check build status and report
if [ $? -eq 0 ]; then
    echo ""
    echo "========================================"
    echo "Build completed successfully!"
    echo "Output files are in: $SOURCE_DIR/Bin"
    echo "========================================"
else
    echo ""
    echo "========================================"
    echo "Build failed!"
    echo "========================================"
    exit 1
fi