#!/bin/bash

# 设置变量
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(dirname "$SCRIPT_DIR")"

# 检测操作系统
OS=$(uname -s)

echo "========================================"
echo "CYCoroutine Cross-Platform Build Script"
echo "========================================"
echo "Detected OS: $OS"

# 根据操作系统选择构建脚本
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

# 检查构建是否成功
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