[English](README.md) | [中文](README_zh.md)

# CYCoroutine
Coroutine library based on C++20

Coroutine library based on C++ 20, supports cross-platform, has a simple and easy-to-use interface, and is continuously updated.

## Build Scripts Usage

The project provides platform-specific build scripts to simplify the build process:

### General Build Scripts

#### build.sh (Cross-platform)
A general build script for macOS and Linux systems that automatically detects the operating system and calls the corresponding build script.
```bash
chmod +x Build/build.sh
./Build/build.sh
```

### Platform-Specific Build Scripts

#### build_mac.sh (macOS)
Script specifically for building macOS versions, supporting Intel (x86_64) and Apple Silicon (arm64) architectures.
```bash
chmod +x Build/build_mac.sh
./Build/build_mac.sh
```
> After both arm64 and x86_64 slices are built, the script automatically uses `lipo` to generate `Bin/macOS/universal/<config>/libCYCoroutine.{a,dylib}`.

#### build_unix.sh (Unix/Linux)
Script for building Unix/Linux versions.
```bash
chmod +x Build/build_unix.sh
./Build/build_unix.sh
```
> Linux builds will automatically search for `clang-17`/`clang++-17` as the default compiler pair; if the system installation location is different, you can set the `CYLOGGER_CC` / `CYLOGGER_CXX` environment variables before running the script to override, for example:
> ```bash
> export CYLOGGER_CC=/opt/llvm/bin/clang
> export CYLOGGER_CXX=/opt/llvm/bin/clang++
> ./Build/build_unix.sh
> ```

#### build_windows.bat / build_windows_all.bat (Windows)
Windows batch scripts only generate static libraries (`CYCoroutine.lib`) for direct linking to CYLogger:
```batch
REM Single build: BuildType [Release|Debug], LibType fixed to Static, Arch [x64|x86], CRT [MD|MT]
Build\build_windows.bat Release Static x64 MD

REM Enumerate all combinations (static libraries only)
Build\build_windows_all.bat
```

> Output directory follows `Bin\Windows\<arch>\<CRT>\<config>\CYCoroutine.lib`, consistent with CYLogger's dependency detection logic. Passing `Shared` will be rejected by the script to avoid the old DLL process being called again.

#### build_ios.sh (iOS)
Script for building iOS versions, supporting device (arm64) and simulator (x86_64, arm64-simulator) architectures.
```bash
chmod +x Build/build_ios.sh
./Build/build_ios.sh
```
> As long as at least two architecture artifacts exist (e.g., arm64 and x86_64), the script will automatically generate universal libraries `Bin/iOS/universal/<config>/libCYCoroutine.{a,dylib}`.

#### build_android.sh (Android)
Script for building Android versions, supporting multiple ABI architectures.
```bash
chmod +x Build/build_android.sh
./Build/build_android.sh
```

### Build Output

All build artifacts are output to the `Bin` folder in the project root directory, organized by platform and architecture:

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

The universal directories for macOS / iOS will contain `libCYCoroutine.a` and `libCYCoroutine.dylib`, which can be directly used for projects requiring fat binaries; shared libraries will also automatically include versioned symlinks (e.g., `libCYCoroutine.1.0.0.dylib`).

## Example Code

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
