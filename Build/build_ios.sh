#!/bin/bash

# 设置变量
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$SCRIPT_DIR/build_ios"
INSTALL_DIR="$SOURCE_DIR/Bin"

echo "========================================"
echo "Building CYCoroutine for iOS"
echo "========================================"

# 检测Xcode
if ! command -v xcodebuild &> /dev/null; then
    echo "Error: Xcode is not installed or not in PATH."
    exit 1
fi

# 检测iOS SDK
IOS_SDK_PATH=$(xcodebuild -version -sdk iphoneos Path)
if [ -z "$IOS_SDK_PATH" ]; then
    echo "Error: iOS SDK not found."
    exit 1
fi

echo "Using iOS SDK: $IOS_SDK_PATH"

# 创建构建目录
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 检查CMake
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake is not installed or not in PATH."
    exit 1
fi

# 设置最低iOS版本
IOS_MIN_VERSION="14.0"

# 创建构建目录
mkdir -p ios_build
cd ios_build

# 构建动态库
echo ""
echo "Building shared library for all iOS architectures (ARM64 device, x86_64 simulator)..."

# 配置CMake，同时构建ARM64和x86_64
cmake \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$IOS_MIN_VERSION" \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_EXAMPLES=ON \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_STATIC_LIBS=OFF \
    "$SCRIPT_DIR"

if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed for iOS shared library."
    exit 1
fi

# 构建
make -j$(sysctl -n hw.ncpu) CYCoroutine_shared

if [ $? -ne 0 ]; then
    echo "Error: Build failed for iOS shared library."
    exit 1
fi

# 构建示例
make -j$(sysctl -n hw.ncpu) CYCoroutineExample

if [ $? -ne 0 ]; then
    echo "Error: Example build failed for iOS shared library."
    exit 1
fi

# 构建静态库
echo ""
echo "Building static library for all iOS architectures (ARM64 device, x86_64 simulator)..."

# 配置CMake，同时构建ARM64和x86_64
cmake \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$IOS_MIN_VERSION" \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_EXAMPLES=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_STATIC_LIBS=ON \
    "$SCRIPT_DIR"

if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed for iOS static library."
    exit 1
fi

# 构建
make -j$(sysctl -n hw.ncpu) CYCoroutine_static

if [ $? -ne 0 ]; then
    echo "Error: Build failed for iOS static library."
    exit 1
fi

# 构建示例
make -j$(sysctl -n hw.ncpu) CYCoroutineExample

if [ $? -ne 0 ]; then
    echo "Error: Example build failed for iOS static library."
    exit 1
fi

cd ..

echo ""
echo "========================================"
echo "iOS build completed successfully!"
echo "Output files are in: $INSTALL_DIR"
echo "========================================"