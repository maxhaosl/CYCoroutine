# CYCorotine
Coroutine library based on C++20

Coroutine library based on C++ 20, supports cross-platform, has a simple and easy-to-use interface, and is continuously updated.

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
专门用于构建 macOS 版本的脚本，支持 Intel (x64) 和 Apple Silicon (arm64) 架构。
```bash
chmod +x Build/build_mac.sh
./Build/build_mac.sh
```

#### build_unix.sh (Unix/Linux)
用于构建 Unix/Linux 版本的脚本。
```bash
chmod +x Build/build_unix.sh
./Build/build_unix.sh
```

#### build_windows.bat (Windows)
用于构建 Windows 版本的批处理脚本。
```batch
Build\build_windows.bat
```

#### build_ios.sh (iOS)
用于构建 iOS 版本的脚本，支持设备 (arm64) 和模拟器 (x86_64) 架构。
```bash
chmod +x Build/build_ios.sh
./Build/build_ios.sh
```

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
│   └── x64/
│       ├── libCYCoroutine.dylib (动态库)
│       ├── libCYCoroutine.a (静态库)
│       └── CYCoroutineExample_x64 (示例程序)
├── iOS/
│   └── x64/
│       ├── libCYCoroutine.dylib
│       ├── libCYCoroutine.a
│       └── CYCoroutineExample_ios_simulator.app
├── Windows/
│   ├── x86/
│   └── x64/
├── Linux/
│   ├── x86/
│   └── x64/
└── Android/
    ├── armv7/
    ├── arm64/
    ├── x86/
    └── x64/
```

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
