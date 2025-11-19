#!/bin/bash

# 设置变量
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$SCRIPT_DIR/build_unix"
INSTALL_DIR="$SOURCE_DIR/Bin"

# 检测操作系统
OS=$(uname -s)
ARCH=$(uname -m)

echo "========================================"
echo "Building CYCoroutine for $OS ($ARCH)"
echo "========================================"

# 创建构建目录
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 检测CMake
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake is not installed or not in PATH."
    exit 1
fi

# 检测编译器
if ! command -v clang++ &> /dev/null && ! command -v g++ &> /dev/null; then
    echo "Error: No suitable C++ compiler found (clang++ or g++)."
    exit 1
fi

# 设置编译器
if command -v clang++ &> /dev/null; then
    export CC=clang
    export CXX=clang++
    echo "Using Clang compiler"
else
    export CC=gcc
    export CXX=g++
    echo "Using GCC compiler"
fi

# macOS特定设置
if [[ "$OS" == "Darwin" ]]; then
    # 检测Xcode
    if ! xcode-select -p &> /dev/null; then
        echo "Error: Xcode command line tools are not installed."
        echo "Run 'xcode-select --install' to install them."
        exit 1
    fi
    
    # 设置最低macOS版本
    MACOS_MIN_VERSION="11.0"
    
    # 创建构建目录
    mkdir -p macos_build
    cd macos_build
    
    # 构建动态库
    echo ""
    echo "Building shared library for all macOS architectures (ARM64, x64)..."
    
    # 配置CMake，同时构建ARM64和x64
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOS_MIN_VERSION" \
          -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
          -DBUILD_EXAMPLES=ON \
          -DBUILD_SHARED_LIBS=ON \
          -DBUILD_STATIC_LIBS=OFF \
          "$SOURCE_DIR/Build"
    
    if [ $? -ne 0 ]; then
        echo "Error: CMake configuration failed for macOS shared library."
        exit 1
    fi
    
    # 构建
    make -j$(sysctl -n hw.ncpu) CYCoroutine_shared
    
    if [ $? -ne 0 ]; then
        echo "Error: Build failed for macOS shared library."
        exit 1
    fi
    
    # 构建示例
    make -j$(sysctl -n hw.ncpu) CYCoroutineExample
    
    if [ $? -ne 0 ]; then
        echo "Error: Example build failed for macOS shared library."
        exit 1
    fi
    
    # 构建静态库
    echo ""
    echo "Building static library for all macOS architectures (ARM64, x64)..."
    
    # 配置CMake，同时构建ARM64和x64
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOS_MIN_VERSION" \
          -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
          -DBUILD_EXAMPLES=ON \
          -DBUILD_SHARED_LIBS=OFF \
          -DBUILD_STATIC_LIBS=ON \
          "$SOURCE_DIR/Build"
    
    if [ $? -ne 0 ]; then
        echo "Error: CMake configuration failed for macOS static library."
        exit 1
    fi
    
    # 构建
    make -j$(sysctl -n hw.ncpu) CYCoroutine_static
    
    if [ $? -ne 0 ]; then
        echo "Error: Build failed for macOS static library."
        exit 1
    fi
    
    # 构建示例
    make -j$(sysctl -n hw.ncpu) CYCoroutineExample
    
    if [ $? -ne 0 ]; then
        echo "Error: Example build failed for macOS static library."
        exit 1
    fi
    
    cd ..
    
# Linux特定设置
elif [[ "$OS" == "Linux" ]]; then
    # 构建当前架构
    echo ""
    echo "Building for $ARCH..."
    
    # 构建动态库
    echo "Building shared library for $ARCH..."
    
    # 配置CMake
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_EXAMPLES=ON \
          -DBUILD_SHARED_LIBS=ON \
          -DBUILD_STATIC_LIBS=OFF \
          "$SOURCE_DIR/Build"
    
    if [ $? -ne 0 ]; then
        echo "Error: CMake configuration failed for Linux shared library."
        exit 1
    fi
    
    # 构建
    make -j$(nproc) CYCoroutine_shared
    
    if [ $? -ne 0 ]; then
        echo "Error: Build failed for Linux shared library."
        exit 1
    fi
    
    # 构建示例
    make -j$(nproc) CYCoroutineExample
    
    if [ $? -ne 0 ]; then
        echo "Error: Example build failed for Linux shared library."
        exit 1
    fi
    
    # 构建静态库
    echo "Building static library for $ARCH..."
    
    # 配置CMake
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_EXAMPLES=ON \
          -DBUILD_SHARED_LIBS=OFF \
          -DBUILD_STATIC_LIBS=ON \
          "$SOURCE_DIR/Build"
    
    if [ $? -ne 0 ]; then
        echo "Error: CMake configuration failed for Linux static library."
        exit 1
    fi
    
    # 构建
    make -j$(nproc) CYCoroutine_static
    
    if [ $? -ne 0 ]; then
        echo "Error: Build failed for Linux static library."
        exit 1
    fi
    
    # 构建示例
    make -j$(nproc) CYCoroutineExample
    
    if [ $? -ne 0 ]; then
        echo "Error: Example build failed for Linux static library."
        exit 1
    fi
fi

echo ""
echo "========================================"
echo "Build completed successfully!"
echo "Output files are in: $INSTALL_DIR"
echo "========================================"