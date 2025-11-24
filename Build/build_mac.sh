#!/bin/bash

# 设置变量
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$SCRIPT_DIR/build_mac"
INSTALL_DIR="$SOURCE_DIR/Bin"

# 获取传递的架构参数
TARGET_ARCH=${1:-"universal"}

echo "========================================"
echo "Building CYCoroutine for macOS ($TARGET_ARCH)"
echo "========================================"

# 检测操作系统
OS=$(uname -s)
if [[ "$OS" != "Darwin" ]]; then
    echo "Error: This script is intended for macOS only."
    exit 1
fi

# 检测Xcode
if ! xcode-select -p &> /dev/null; then
    echo "Error: Xcode command line tools are not installed."
    echo "Run 'xcode-select --install' to install them."
    exit 1
fi

# 检测CMake
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake is not installed or not in PATH."
    exit 1
fi

# 检测编译器
if ! command -v clang++ &> /dev/null; then
    echo "Error: Clang++ is not installed or not in PATH."
    exit 1
fi

# 设置编译器
export CC=clang
export CXX=clang++
echo "Using Clang compiler"

# 设置最低macOS版本
MACOS_MIN_VERSION="11.0"

build_libraries() {
    local arch=$1
    local lib_type=$2
    
    echo ""
    echo "Building $lib_type library for $arch architecture..."
    
    local build_subdir="build_mac_${arch}_${lib_type}"
    local build_path="$SCRIPT_DIR/$build_subdir"
    mkdir -p "$build_path"
    cd "$build_path"
    
    # 配置CMake
    if [ "$lib_type" = "shared" ]; then
        cmake -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOS_MIN_VERSION" \
              -DCMAKE_OSX_ARCHITECTURES="$arch" \
              -DCMAKE_SYSTEM_PROCESSOR="$arch" \
              -DBUILD_EXAMPLES=OFF \
              -DBUILD_SHARED_LIBS=ON \
              -DBUILD_STATIC_LIBS=OFF \
              "$SOURCE_DIR/Build"
    else
        cmake -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOS_MIN_VERSION" \
              -DCMAKE_OSX_ARCHITECTURES="$arch" \
              -DCMAKE_SYSTEM_PROCESSOR="$arch" \
              -DBUILD_EXAMPLES=OFF \
              -DBUILD_SHARED_LIBS=OFF \
              -DBUILD_STATIC_LIBS=ON \
              "$SOURCE_DIR/Build"
    fi
    
    if [ $? -ne 0 ]; then
        echo "Error: CMake configuration failed for $arch $lib_type library."
        exit 1
    fi
    
    # 构建
    if [ "$lib_type" = "shared" ]; then
        make -j$(sysctl -n hw.ncpu) CYCoroutine_shared
    else
        make -j$(sysctl -n hw.ncpu) CYCoroutine_static
    fi
    
    if [ $? -ne 0 ]; then
        echo "Error: Build failed for $arch $lib_type library."
        exit 1
    fi
    
    cd "$SCRIPT_DIR"
}

# 根据目标架构构建
case "$TARGET_ARCH" in
    "x86_64")
        build_libraries "x86_64" "static"
        build_libraries "x86_64" "shared"
        ;;
    "arm64")
        build_libraries "arm64" "static"
        build_libraries "arm64" "shared"
        ;;
    "universal"|"")
        # 默认行为，构建通用二进制
        # 创建构建目录
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        
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
        ;;
    *)
        echo "Error: Unknown architecture '$TARGET_ARCH'. Supported architectures: x86_64, arm64, universal"
        exit 1
        ;;
esac

echo ""
echo "========================================"
echo "macOS build completed successfully!"
echo "Output files are in: $INSTALL_DIR"
echo "========================================"