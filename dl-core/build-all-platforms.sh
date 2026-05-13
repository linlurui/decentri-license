#!/bin/bash
# 编译 dl-core 三个平台的动态库

set -e

PROJECT_ROOT=$(cd "$(dirname "$0")" && pwd)
cd "$PROJECT_ROOT"

echo "=== dl-core 多平台编译脚本 ==="
echo "项目目录: $PROJECT_ROOT"
echo ""

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 检查命令是否存在
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# 编译 macOS x86_64 版本
build_macos_x86_64() {
    echo -e "${YELLOW}>>> 编译 macOS x86_64 版本...${NC}"
    
    if ! command_exists cmake; then
        echo -e "${RED}错误: cmake 未安装${NC}"
        echo "请先安装 cmake: brew install cmake"
        return 1
    fi
    
    rm -rf build-macos-x86_64
    mkdir -p build-macos-x86_64
    
    cmake -B build-macos-x86_64 \
        -DCMAKE_TOOLCHAIN_FILE=toolchain-macos-x86_64.cmake \
        -DCMAKE_BUILD_TYPE=Release \
        .
    
    cmake --build build-macos-x86_64 --config Release -j$(sysctl -n hw.ncpu)
    
    if [ -f "build-macos-x86_64/libdecentrilicense.dylib" ]; then
        echo -e "${GREEN}✓ macOS x86_64 编译成功${NC}"
        ls -lh build-macos-x86_64/libdecentrilicense.dylib
    else
        echo -e "${RED}✗ macOS x86_64 编译失败${NC}"
        return 1
    fi
    echo ""
}

# 编译 Linux x86_64 版本 (需要 Docker)
build_linux_x86_64() {
    echo -e "${YELLOW}>>> 编译 Linux x86_64 版本...${NC}"
    
    if ! command_exists docker; then
        echo -e "${RED}错误: Docker 未安装${NC}"
        echo "请先安装 Docker Desktop"
        return 1
    fi
    
    # 检查 Docker 是否运行
    if ! docker info >/dev/null 2>&1; then
        echo -e "${RED}错误: Docker 守护进程未运行${NC}"
        echo "请先启动 Docker Desktop"
        return 1
    fi
    
    echo "使用 Docker 编译 Linux 版本..."
    
    docker run --rm \
        -v "$PROJECT_ROOT:/workdir" \
        -w /workdir \
        ubuntu:22.04 \
        bash -c "
            apt-get update -qq && \
            apt-get install -y -qq cmake build-essential libssl-dev libcurl4-openssl-dev pkg-config libsecret-1-dev && \
            rm -rf build-linux-x86_64 && \
            mkdir -p build-linux-x86_64 && \
            cmake -B build-linux-x86_64 \
                -DCMAKE_TOOLCHAIN_FILE=toolchain-linux-x86_64.cmake \
                -DCMAKE_BUILD_TYPE=Release \
                . && \
            cmake --build build-linux-x86_64 --config Release -j4
        "
    
    if [ -f "build-linux-x86_64/libdecentrilicense.so" ]; then
        echo -e "${GREEN}✓ Linux x86_64 编译成功${NC}"
        ls -lh build-linux-x86_64/libdecentrilicense.so
    else
        echo -e "${RED}✗ Linux x86_64 编译失败${NC}"
        return 1
    fi
    echo ""
}

# 编译 Windows x86_64 版本 (需要 MinGW)
build_windows_x86_64() {
    echo -e "${YELLOW}>>> 编译 Windows x86_64 版本...${NC}"
    
    if ! command_exists x86_64-w64-mingw32-gcc; then
        echo -e "${RED}错误: MinGW-w64 未安装${NC}"
        echo "请先安装 MinGW-w64: brew install mingw-w64"
        return 1
    fi
    
    # 注意：Windows 编译需要特殊的 OpenSSL 和 CURL 库
    # 这里只是一个示例，实际可能需要更复杂的配置
    
    rm -rf build-windows-x86_64
    mkdir -p build-windows-x86_64
    
    echo -e "${YELLOW}警告: Windows 编译需要 MinGW 版本的 OpenSSL 和 CURL${NC}"
    echo "请确保已安装: brew install mingw-w64 openssl mingw-w64-curl"
    
    # 这里使用 CMake 的 MinGW 工具链
    cmake -B build-windows-x86_64 \
        -DCMAKE_TOOLCHAIN_FILE=toolchain-windows-x86_64.cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
        -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
        .
    
    cmake --build build-windows-x86_64 --config Release -j4
    
    if [ -f "build-windows-x86_64/libdecentrilicense.dll" ] || [ -f "build-windows-x86_64/decentrilicense.dll" ]; then
        echo -e "${GREEN}✓ Windows x86_64 编译成功${NC}"
        ls -lh build-windows-x86_64/*.dll
    else
        echo -e "${RED}✗ Windows x86_64 编译失败${NC}"
        return 1
    fi
    echo ""
}

# 主菜单
show_menu() {
    echo "请选择要编译的平台:"
    echo "1. macOS x86_64 (本地编译)"
    echo "2. Linux x86_64 (需要 Docker)"
    echo "3. Windows x86_64 (需要 MinGW)"
    echo "4. 编译所有平台"
    echo "5. 退出"
    echo ""
}

# 主逻辑
if [ $# -eq 0 ]; then
    show_menu
    read -p "请输入选项 [1-5]: " choice
else
    choice=$1
fi

case $choice in
    1)
        build_macos_x86_64
        ;;
    2)
        build_linux_x86_64
        ;;
    3)
        build_windows_x86_64
        ;;
    4)
        echo "编译所有平台..."
        build_macos_x86_64
        build_linux_x86_64
        build_windows_x86_64
        echo -e "${GREEN}所有平台编译完成!${NC}"
        ;;
    5)
        echo "退出"
        exit 0
        ;;
    *)
        echo "无效选项"
        exit 1
        ;;
esac
