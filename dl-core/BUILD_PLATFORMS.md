# dl-core 多平台编译指南

## 概述

本文档说明如何使用 CMake 工具链文件编译 dl-core 库的多个平台版本。

## 已创建的工具链文件

| 平台 | 工具链文件 | 状态 |
|------|-----------|------|
| macOS x86_64 | `toolchain-macos-x86_64.cmake` | ✅ 已编译 |
| Linux x86_64 | `toolchain-linux-x86_64.cmake` | ⏳ 待编译 |
| Windows x86_64 | `toolchain-windows-x86_64.cmake` | ⏳ 待编译 |

## 编译结果

### macOS x86_64

- **输出文件**: `build-macos-x86_64/libdecentrilicense.dylib`
- **状态**: ✅ 编译成功
- **编译时间**: 2026-05-13
- **编译器**: AppleClang 17.0.0
- **OpenSSL**: 3.6.2 (Homebrew)

### Linux x86_64

- **输出文件**: `build-linux-x86_64/libdecentrilicense.so`
- **状态**: ⏳ 需要在 Linux 环境或 Docker 中编译

### Windows x86_64

- **输出文件**: `build-windows-x86_64/decentrilicense.dll`
- **状态**: ⏳ 需要在 Windows 或 MinGW 环境编译

## 快速编译

使用提供的编译脚本:

```bash
./build-all-platforms.sh
```

或指定平台:
```bash
./build-all-platforms.sh 1  # macOS
./build-all-platforms.sh 2  # Linux (Docker)
./build-all-platforms.sh 3  # Windows (MinGW)
./build-all-platforms.sh 4  # 所有平台
```

## 手动编译

### macOS x86_64

```bash
cd /Volumes/workspace/project/ccait/dl-issuer/dl-core
rm -rf build-macos-x86_64
mkdir -p build-macos-x86_64

cmake -B build-macos-x86_64 \
    -DCMAKE_TOOLCHAIN_FILE=toolchain-macos-x86_64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    .

cmake --build build-macos-x86_64 --config Release -j4
```

**前置条件**:
- macOS 11.0+
- CMake 3.15+
- Xcode Command Line Tools
- OpenSSL (brew install openssl)
- CURL (系统自带)

### Linux x86_64

#### 方法 1: 本地编译 (推荐)

在 Linux 机器上执行:

```bash
cd /path/to/dl-core
rm -rf build-linux-x86_64
mkdir -p build-linux-x86_64

cmake -B build-linux-x86_64 \
    -DCMAKE_TOOLCHAIN_FILE=toolchain-linux-x86_64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    .

cmake --build build-linux-x86_64 --config Release -j4
```

**前置条件**:
- Ubuntu 22.04+ / CentOS 8+ / Debian 11+
- CMake 3.15+
- GCC 9+
- OpenSSL development libraries
- libcurl development libraries
- libsecret development libraries

**安装依赖**:
```bash
# Ubuntu/Debian
sudo apt-get install -y cmake build-essential libssl-dev libcurl4-openssl-dev pkg-config libsecret-1-dev

# CentOS/RHEL
sudo yum install -y cmake gcc gcc-c++ openssl-devel libcurl-devel pkgconfig libsecret-devel
```

#### 方法 2: Docker 编译

在 macOS 上使用 Docker 编译:

```bash
cd /Volumes/workspace/project/ccait/dl-issuer/dl-core

docker run --rm \
    -v $(pwd):/workdir \
    -w /workdir \
    ubuntu:22.04 \
    bash -c "
        apt-get update && \
        apt-get install -y cmake build-essential libssl-dev libcurl4-openssl-dev pkg-config libsecret-1-dev && \
        rm -rf build-linux-x86_64 && \
        mkdir -p build-linux-x86_64 && \
        cmake -B build-linux-x86_64 -DCMAKE_BUILD_TYPE=Release . && \
        cmake --build build-linux-x86_64 --config Release -j4
    "
```

### Windows x86_64

#### 方法 1: Visual Studio (推荐)

在 Windows 机器上使用 Visual Studio:

```cmd
cd C:\path\to\dl-core
rmdir /s /q build-windows-x86_64
mkdir build-windows-x86_64

cmake -B build-windows-x86_64 ^
    -G "Visual Studio 16 2019" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    .

cmake --build build-windows-x86_64 --config Release
```

**前置条件**:
- Windows 10/11
- Visual Studio 2019 或 2022
- CMake 3.15+
- OpenSSL (vcpkg 或预编译二进制)
- CURL (vcpkg 或预编译二进制)

**使用 vcpkg 安装依赖**:
```cmd
vcpkg install openssl:x64-windows curl:x64-windows
```

#### 方法 2: MinGW-w64 (跨平台)

在 macOS 上使用 MinGW 交叉编译:

```bash
# 安装 MinGW
brew install mingw-w64

# 编译
cd /Volumes/workspace/project/ccait/dl-issuer/dl-core
rm -rf build-windows-x86_64
mkdir -p build-windows-x86_64

cmake -B build-windows-x86_64 \
    -DCMAKE_TOOLCHAIN_FILE=toolchain-windows-x86_64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    .

cmake --build build-windows-x86_64 --config Release -j4
```

**注意**: MinGW 编译需要 Windows 版本的 OpenSSL 和 CURL 库。

## 输出文件

编译完成后，各平台的输出文件如下:

| 平台 | 动态库文件 | 静态库文件 |
|------|-----------|-----------|
| macOS x86_64 | `libdecentrilicense.dylib` | `libdecentrilicense.a` |
| Linux x86_64 | `libdecentrilicense.so` | `libdecentrilicense.a` |
| Windows x86_64 | `decentrilicense.dll` | `decentrilicense.lib` |

## 验证编译结果

### macOS

```bash
# 检查库文件
file build-macos-x86_64/libdecentrilicense.dylib

# 查看依赖
otool -L build-macos-x86_64/libdecentrilicense.dylib

# 查看符号表
nm -gU build-macos-x86_64/libdecentrilicense.dylib
```

### Linux

```bash
# 检查库文件
file build-linux-x86_64/libdecentrilicense.so

# 查看依赖
ldd build-linux-x86_64/libdecentrilicense.so

# 查看符号表
nm -D build-linux-x86_64/libdecentrilicense.so
```

### Windows

```cmd
# 检查库文件 (使用 VS 开发者命令提示符)
dumpbin /headers build-windows-x86_64\decentrilicense.dll

# 查看导出符号
dumpbin /exports build-windows-x86_64\decentrilicense.dll
```

## 常见问题

### macOS 编译警告

如果遇到 OpenSSL 版本警告:
```
ld: warning: building for macOS-11.0, but linking with dylib which was built for newer version
```

这是正常现象，不会影响库的功能。

### Linux Docker 编译失败

确保 Docker Desktop 已启动:
```bash
docker info
```

### Windows 编译找不到 OpenSSL

确保正确设置了 CMAKE_PREFIX_PATH:
```cmd
cmake -B build-windows-x86_64 -DCMAKE_PREFIX_PATH=C:\vcpkg\installed\x64-windows ...
```

## 下一步

编译完成后，可以将各平台的库文件复制到对应的 SDK 目录:

```bash
# 复制到 SDK 目录 (示例)
cp build-macos-x86_64/libdecentrilicense.dylib ../sdks/cpp/lib/macos/
cp build-linux-x86_64/libdecentrilicense.so ../sdks/cpp/lib/linux/
cp build-windows-x86_64/decentrilicense.dll ../sdks/cpp/lib/windows/
```
