[English](README.md) | [中文](README_zh.md)

# CYCoroutine
基于 C++20 的协程库

基于 C++ 20 的协程库，支持跨平台，具有简单易用的接口，持续更新中。

## 构建脚本详细使用说明

项目提供了多个平台专用的构建脚本，以简化构建过程：

### 通用构建脚本

#### build.sh (跨平台)
适用于 macOS 和 Linux 系统的通用构建脚本，会自动检测操作系统并调用相应的构建脚本。
```bash
chmod +x Build/build.sh
./Build/build.sh
```

### 平台专用构建脚本

#### build_mac.sh (macOS)
专门用于构建 macOS 版本的脚本，支持 Intel (x86_64) 和 Apple Silicon (arm64) 架构。
```bash
chmod +x Build/build_mac.sh
./Build/build_mac.sh
```
> 当 arm64 与 x86_64 两个切片都构建完成后，脚本会自动使用 `lipo` 生成 `Bin/macOS/universal/<配置>/libCYCoroutine.{a,dylib}`。

#### build_unix.sh (Unix/Linux)
用于构建 Unix/Linux 版本的脚本。
```bash
chmod +x Build/build_unix.sh
./Build/build_unix.sh
```
> Linux 构建会优先自动查找 `clang-17`/`clang++-17` 作为默认编译器对；如果系统安装位置不同，可在运行脚本前设置 `CYLOGGER_CC` / `CYLOGGER_CXX` 环境变量进行覆盖，例如：
> ```bash
> export CYLOGGER_CC=/opt/llvm/bin/clang
> export CYLOGGER_CXX=/opt/llvm/bin/clang++
> ./Build/build_unix.sh
> ```

#### build_windows.bat / build_windows_all.bat (Windows)
Windows 平台的批处理脚本只生成静态库 (`CYCoroutine.lib`)，以便直接链接到 CYLogger：
```batch
REM 单次构建：BuildType [Release|Debug], LibType 固定为 Static, Arch [x64|x86], CRT [MD|MT]
Build\build_windows.bat Release Static x64 MD

REM 枚举全部组合（同样仅静态库）
Build\build_windows_all.bat
```

> 输出目录遵循 `Bin\Windows\<arch>\<CRT>\<config>\CYCoroutine.lib`，与 CYLogger 的依赖检测逻辑保持一致。传入 `Shared` 将被脚本拒绝，以避免旧的 DLL 流程再次被调用。

#### build_ios.sh (iOS)
用于构建 iOS 版本的脚本，支持设备 (arm64) 和模拟器 (x86_64、arm64-simulator) 架构。
```bash
chmod +x Build/build_ios.sh
./Build/build_ios.sh
```
> 只要存在至少两个架构的产物（例如 arm64 与 x86_64），脚本就会自动生成 `Bin/iOS/universal/<配置>/libCYCoroutine.{a,dylib}` 通用库。

#### build_android.sh (Android)
用于构建 Android 版本的脚本，支持多种 ABI 架构。
```bash
chmod +x Build/build_android.sh
./Build/build_android.sh
```

### 构建输出

所有构建产物都会输出到项目根目录的 `Bin` 文件夹中，按照平台和架构进行组织：

```
Bin/
├── macOS/
│   ├── arm64/
│   ├── x86_64/
│   └── universal/
├── iOS/
│   ├── arm64/
│   ├── x86_64/
│   ├── arm64-simulator/
│   └── universal/
├── Windows/
│   ├── x86/
│   └── x64/
├── Linux/
│   ├── x86/
│   └── x64/
└── Android/
    ├── armeabi-v7a/
    ├── arm64-v8a/
    ├── x86/
    └── x86_64/
```

macOS / iOS 通用目录下会出现 `libCYCoroutine.a` 与 `libCYCoroutine.dylib`，可直接用于需要 fat binaries 的项目；共享库还会自动附带版本化软链接（例如 `libCYCoroutine.1.0.0.dylib`）。

## 示例代码

```cpp
#include "CYCoroutine/CYCoroutine.hpp"
#include <iostream>
#include <thread>

int main()
{
    std::cout << "Main threadId=" << std::this_thread::get_id() << std::endl;

    auto result = CYInlineCoro()->Submit([]()-> int {
        std::cout << "CYInlineCoro - hello world threadId=" << std::this_thread::get_id() << std::endl;
        return 5;
        });

   result.Get();

   auto result2 = CYThreadPoolCoro()->Submit([]()-> int {
       std::cout << "CYThreadPoolCoro - hello world threadId=" << std::this_thread::get_id() << std::endl;
       return 5;
       });

   result2.Get();

   auto result3 = CYBackgroundCoro()->Submit([]()-> int {
       std::cout << "CYBackgroundCoro - hello world threadId=" << std::this_thread::get_id() << std::endl;
       return 5;
       });

   result3.Get();

   auto result4 = CYThreadCoro()->Submit([]()-> int {
       std::cout << "CYThreadCoro - hello world threadId=" << std::this_thread::get_id() << std::endl;
       return 5;
       });

   result4.Get();

   CYCoroFree();

   return 0;
}
```

