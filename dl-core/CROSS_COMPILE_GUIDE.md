# dl-core 交叉编译指南

## 概述

本指南说明如何使用 CMake 工具链文件在 **macOS** 上交叉编译出 **Windows** 和 **Linux** 的动态库。

## 快速开始

使用提供的交叉编译脚本:

```bash
cd /Volumes/workspace/project/ccait/dl-issuer/dl-core
./cross-compile.sh
```

然后选择要编译的平台。

## 工具链文件说明

| 工具链文件 | 用途 | 适用场景 |
|-----------|------|----------|
| `toolchain-macos-x86_64.cmake` | macOS x86_64 本地编译 | Apple Silicon Mac 需要 x86 库 |
| `toolchain-mingw-w64.cmake` | Windows x86_64 交叉编译 | 使用 MinGW-w64 在 macOS 编译 Windows DLL |
| `toolchain-linux-cross.cmake` | Linux x86_64 交叉编译 | 需要 Linux 交叉编译器 |

## 详细说明

### 1. macOS x86_64 (本地编译)

**前置条件**: 无需额外安装

**编译**:
```bash
cmake -B build-macos-x86_64 \
    -DCMAKE_TOOLCHAIN_FILE=toolchain-macos-x86_64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    .

cmake --build build-macos-x86_64 --config Release
```

**输出**: `build-macos-x86_64/libdecentrilicense.dylib`

---

### 2. Windows x86_64 (交叉编译)

**前置条件**:
```bash
brew install mingw-w64
```

**验证安装**:
```bash
x86_64-w64-mingw32-gcc --version
```

**⚠️ 重要**: MinGW 需要 Windows 版本的依赖库 (OpenSSL, CURL)

**方案 A - 如果已有 MinGW 版本的库**:
```bash
cmake -B build-windows \
    -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw-w64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    .

cmake --build build-windows --config Release
```

**方案 B - 使用 MSYS2 (推荐)**:
在 Windows 上使用 MSYS2 环境可以获得完整的 MinGW 库支持:

```bash
# 在 Windows MSYS2 终端中
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-openssl mingw-w64-x86_64-curl

cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**方案 C - 使用 vcpkg + MinGW**:
```bash
# 安装 vcpkg
./vcpkg install openssl:x64-mingw-static curl:x64-mingw-static

# 编译时指定 vcpkg 工具链
cmake -B build-windows \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=x64-mingw-static \
    .
```

**输出**: `build-windows/decentrilicense.dll`

---

### 3. Linux x86_64 (Docker 编译)

由于 macOS 上没有标准的 Linux 交叉编译器，**推荐使用 Docker**:

**前置条件**: Docker Desktop 已安装并运行

**编译**:
```bash
# 使用脚本
./cross-compile.sh 2

# 或手动执行
docker run --rm \
    -v $(pwd):/workdir \
    -w /workdir \
    ubuntu:22.04 \
    bash -c "
        apt-get update && \
        apt-get install -y cmake build-essential libssl-dev \
                          libcurl4-openssl-dev pkg-config libsecret-1-dev && \
        rm -rf build-linux && \
        mkdir -p build-linux && \
        cmake -B build-linux -DCMAKE_BUILD_TYPE=Release . && \
        cmake --build build-linux --config Release -j4
    "
```

**输出**: `build-linux/libdecentrilicense.so`

---

### 4. Linux x86_64 (交叉编译 - 高级)

如果你确实需要在 macOS 上本地交叉编译 Linux 库，需要构建完整的交叉编译工具链:

**步骤**:

1. 安装 crosstool-ng:
```bash
brew install crosstool-ng
```

2. 配置并构建工具链:
```bash
mkdir -p ~/cross && cd ~/cross
ct-ng x86_64-unknown-linux-gnu
ct-ng build
```

3. 使用工具链编译:
```bash
export PATH=~/cross/x86_64-unknown-linux-gnu/bin:$PATH

cmake -B build-linux \
    -DCMAKE_TOOLCHAIN_FILE=toolchain-linux-cross.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    .

cmake --build build-linux
```

**注意**: 构建交叉编译工具链需要数小时时间。

---

## 各平台编译对比

| 平台 | 方法 | 难度 | 推荐度 |
|------|------|------|--------|
| macOS x86_64 | 本地 | ⭐ 简单 | ⭐⭐⭐⭐⭐ |
| Linux x86_64 | Docker | ⭐ 简单 | ⭐⭐⭐⭐⭐ |
| Linux x86_64 | 交叉编译 | ⭐⭐⭐⭐⭐ 极难 | ⭐ |
| Windows x86_64 | MinGW | ⭐⭐⭐ 中等 | ⭐⭐⭐ |
| Windows x86_64 | 本地 VS | ⭐⭐ 较简单 | ⭐⭐⭐⭐⭐ |

---

## 常见问题

### Q: 为什么 MinGW 编译会找不到 OpenSSL?

MinGW 需要 Windows 版本的 OpenSSL 库，而不是 macOS 的版本。

**解决方案**:
1. 在 Windows 上使用 MSYS2 或 Visual Studio 编译 (推荐)
2. 使用 vcpkg 安装 MinGW 版本的 OpenSSL
3. 手动下载 MinGW 版本的 OpenSSL 预编译库

### Q: 可以只编译一个平台的库吗?

可以，使用脚本或手动指定工具链:
```bash
# 只编译 Windows
./cross-compile.sh 3

# 只编译 Linux (Docker)
./cross-compile.sh 2
```

### Q: 如何验证交叉编译的库?

**Windows DLL**:
```bash
# 使用 file 命令检查
file build-windows/decentrilicense.dll
# 应输出: PE32+ executable (DLL) (console) x86-64, for MS Windows
```

**Linux SO**:
```bash
# 使用 readelf (需要安装 binutils)
x86_64-linux-gnu-readelf -h build-linux/libdecentrilicense.so
# 或使用 Docker
docker run --rm -v $(pwd):/w ubuntu:22.04 file /w/build-linux/libdecentrilicense.so
```

### Q: Apple Silicon Mac 可以编译 x86_64 库吗?

可以，使用 macOS x86_64 工具链:
```bash
cmake -B build -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## 推荐的开发工作流

1. **macOS 开发**: 本地编译 macOS 版本进行开发和测试
2. **Linux 发布**: 使用 Docker 编译 Linux 版本
3. **Windows 发布**: 在 Windows 虚拟机或使用 CI/CD 服务 (GitHub Actions) 编译

## CI/CD 自动化

参考 `.github/workflows/build.yml` 实现多平台自动编译:

```yaml
strategy:
  matrix:
    os: [ubuntu-latest, macos-latest, windows-latest]
    include:
      - os: ubuntu-latest
        output: libdecentrilicense.so
      - os: macos-latest
        output: libdecentrilicense.dylib
      - os: windows-latest
        output: decentrilicense.dll
```

---

## 总结

- ✅ **macOS**: 本地编译最简单
- ✅ **Linux**: Docker 编译最简单
- ⚠️ **Windows**: 建议在 Windows 环境编译，或准备 MinGW 依赖库

对于生产环境，建议使用 CI/CD 服务 (GitHub Actions, GitLab CI) 进行多平台编译。
