#!/bin/bash

# 设置变量
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$SCRIPT_DIR/build_android"
INSTALL_DIR="$SOURCE_DIR/Bin"

# 检测Android NDK
if [ -z "$ANDROID_NDK_HOME" ] && [ -z "$ANDROID_NDK_ROOT" ]; then
    # 尝试常见路径
    if [ -d "$HOME/Library/Android/sdk/ndk" ]; then
        export ANDROID_NDK_HOME=$(ls -1 "$HOME/Library/Android/sdk/ndk" | sort -r | head -n 1 | xargs -I {} echo "$HOME/Library/Android/sdk/ndk/{}")
    elif [ -d "$HOME/Android/Sdk/ndk" ]; then
        export ANDROID_NDK_HOME=$(ls -1 "$HOME/Android/Sdk/ndk" | sort -r | head -n 1 | xargs -I {} echo "$HOME/Android/Sdk/ndk/{}")
    else
        echo "Error: Android NDK not found."
        echo "Please set ANDROID_NDK_HOME environment variable to your NDK path."
        exit 1
    fi
fi

# 使用ANDROID_NDK_ROOT如果ANDROID_NDK_HOME未设置
if [ -z "$ANDROID_NDK_HOME" ] && [ -n "$ANDROID_NDK_ROOT" ]; then
    export ANDROID_NDK_HOME="$ANDROID_NDK_ROOT"
fi

echo "========================================"
echo "Building CYCoroutine for Android"
echo "Using NDK: $ANDROID_NDK_HOME"
echo "========================================"

# 创建构建目录
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 检查CMake
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake is not installed or not in PATH."
    exit 1
fi

# 设置Android API级别
ANDROID_API_LEVEL=21

# 定义要构建的ABI列表
ABIS=("armeabi-v7a" "arm64-v8a" "x86" "x86_64")

# 获取CPU核心数的函数
get_cpu_count() {
    if command -v nproc &> /dev/null; then
        nproc
    elif command -v sysctl &> /dev/null; then
        sysctl -n hw.ncpu
    else
        echo 1
    fi
}

# 为每个ABI构建
for ABI in "${ABIS[@]}"; do
    echo ""
    echo "Building for ABI: $ABI"
    
    # 创建ABI特定的构建目录
    mkdir -p "$ABI"
    cd "$ABI"
    
    # 构建动态库
    echo "Building shared library for $ABI..."
    
    # 配置CMake
    cmake \
        -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="$ABI" \
        -DANDROID_NATIVE_API_LEVEL="$ANDROID_API_LEVEL" \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_EXAMPLES=ON \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_STATIC_LIBS=OFF \
        "$SCRIPT_DIR"
    
    if [ $? -ne 0 ]; then
        echo "Error: CMake configuration failed for ABI $ABI shared library."
        exit 1
    fi
    
    # 构建
    make -j$(get_cpu_count) CYCoroutine_shared
    
    if [ $? -ne 0 ]; then
        echo "Error: Build failed for ABI $ABI shared library."
        exit 1
    fi
    
    # 构建示例
    make -j$(get_cpu_count) CYCoroutineExample
    
    if [ $? -ne 0 ]; then
        echo "Error: Example build failed for ABI $ABI shared library."
        exit 1
    fi
    
    # 构建静态库
    echo "Building static library for $ABI..."
    
    # 配置CMake
    cmake \
        -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI="$ABI" \
        -DANDROID_NATIVE_API_LEVEL="$ANDROID_API_LEVEL" \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_EXAMPLES=ON \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_STATIC_LIBS=ON \
        "$SCRIPT_DIR"
    
    if [ $? -ne 0 ]; then
        echo "Error: CMake configuration failed for ABI $ABI static library."
        exit 1
    fi
    
    # 构建
    make -j$(get_cpu_count) CYCoroutine_static
    
    if [ $? -ne 0 ]; then
        echo "Error: Build failed for ABI $ABI static library."
        exit 1
    fi
    
    # 构建示例
    make -j$(get_cpu_count) CYCoroutineExample
    
    if [ $? -ne 0 ]; then
        echo "Error: Example build failed for ABI $ABI static library."
        exit 1
    fi
    
    cd ..
done

echo ""
echo "========================================"
echo "Android build completed successfully!"
echo "Output files are in: $INSTALL_DIR"
echo "========================================"