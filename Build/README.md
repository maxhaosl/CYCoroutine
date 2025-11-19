# CYCoroutine 构建指南

本目录包含用于构建 CYCoroutine 库的 CMake 文件和构建脚本，支持多个平台和架构。

## 支持的平台

- Windows (x86, x64)
- macOS (Intel x64, Apple Silicon ARM64)
- Linux (x86, x64, ARM64)
- Android (armeabi-v7a, arm64-v8a, x86, x86_64)
- iOS (ARM64, x86_64 模拟器)

## 构建要求

### 通用要求
- CMake 3.16 或更高版本
- C++20 兼容的编译器

### 平台特定要求

#### Windows
- Visual Studio 2019 或更高版本
- 从 Visual Studio Developer Command Prompt 运行构建脚本

#### macOS
- Xcode 12.0 或更高版本
- macOS 10.15 或更高版本

#### Linux
- GCC 10.0 或更高版本，或 Clang 10.0 或更高版本

#### Android
- Android NDK 21 或更高版本
- 设置 ANDROID_NDK_HOME 环境变量

#### iOS
- Xcode 12.0 或更高版本
- iOS 11.0 或更高版本

## 构建方法

### 自动构建（推荐）

#### Windows
```batch
build.bat
```

#### macOS/Linux
```bash
chmod +x build.sh
./build.sh
```

### 平台专用构建脚本

#### macOS 专用构建脚本
专门用于构建 macOS 版本的脚本，支持 Intel (x64) 和 Apple Silicon (arm64) 架构。
```bash
chmod +x build_mac.sh
./build_mac.sh
```

#### Unix/Linux 通用构建脚本
用于构建 Unix/Linux 版本的脚本。
```bash
chmod +x build_unix.sh
./build_unix.sh
```

#### Windows 构建脚本
用于构建 Windows 版本的批处理脚本。
```batch
build_windows.bat
```

#### iOS 构建脚本
用于构建 iOS 版本的脚本，支持设备 (arm64) 和模拟器 (x86_64) 架构。
```bash
chmod +x build_ios.sh
./build_ios.sh
```

#### Android 构建脚本
用于构建 Android 版本的脚本，支持多种 ABI 架构。
```bash
chmod +x build_android.sh
./build_android.sh
```

### 手动构建

#### Windows
```batch
build_windows.bat
```

#### macOS/Linux
```bash
chmod +x build_unix.sh
./build_unix.sh
```

#### Android
```bash
chmod +x build_android.sh
./build_android.sh
```

#### iOS
```bash
chmod +x build_ios.sh
./build_ios.sh
```

## 输出文件

构建完成后，库文件和示例可执行文件将位于 `../Bin` 目录中，文件名格式如下：

### 库文件
- Windows: `CYCoroutine_x86.dll`, `CYCoroutine_x64.dll` (及对应的 .lib 文件)
- macOS: `libCYCoroutine_universal.dylib`, `libCYCoroutine_x64.dylib`, `libCYCoroutine_arm64.dylib`
- Linux: `libCYCoroutine_x64.so`, `libCYCoroutine_arm64.so`
- Android: `libCYCoroutine_armv7.so`, `libCYCoroutine_arm64.so`, `libCYCoroutine_x86.so`, `libCYCoroutine_x64.so`
- iOS: `libCYCoroutine_ios_universal.a`, `libCYCoroutine_ios_arm64.a`, `libCYCoroutine_ios_simulator.a`

### 示例可执行文件
- Windows: `CYCoroutineExample_x86.exe`, `CYCoroutineExample_x64.exe`
- macOS: `CYCoroutineExample_universal`, `CYCoroutineExample_x64`, `CYCoroutineExample_arm64`
- Linux: `CYCoroutineExample_x64`, `CYCoroutineExample_arm64`
- Android: `CYCoroutineExample_armv7`, `CYCoroutineExample_arm64`, `CYCoroutineExample_x86`, `CYCoroutineExample_x64`
- iOS: `libCYCoroutineExample_ios_universal.a`, `libCYCoroutineExample_ios_arm64.a`, `libCYCoroutineExample_ios_simulator.a`

### 通用库
macOS和iOS平台会自动生成通用库，包含所有支持的架构：
- macOS: `libCYCoroutine_universal.dylib`, `CYCoroutineExample_universal`
- iOS: `libCYCoroutine_ios_universal.a`, `libCYCoroutineExample_ios_universal.a`

## 使用 CMake 直接构建

如果您想直接使用 CMake 而不是构建脚本：

```bash
# 创建构建目录
mkdir build
cd build

# 配置项目
cmake ..

# 构建
cmake --build . --target CYCoroutine
```

## 故障排除

### Windows
- 确保从 Visual Studio Developer Command Prompt 运行脚本
- 检查 Visual Studio 2019 或更高版本是否正确安装

### macOS
- 确保安装了 Xcode 命令行工具：`xcode-select --install`
- 检查 macOS 版本是否满足最低要求

### Linux
- 确保安装了 C++20 兼容的编译器
- 检查 CMake 是否已安装

### Android
- 确保 ANDROID_NDK_HOME 环境变量已正确设置
- 检查 NDK 版本是否满足最低要求

### iOS
- 确保安装了 Xcode 和 iOS SDK
- 检查 iOS 部署目标版本是否正确

## 许可证

CYCoroutine 使用 MIT 许可证。详情请参阅项目根目录中的 LICENSE 文件。