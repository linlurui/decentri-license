#!/bin/bash
# 交叉编译脚本 - 在 macOS 上编译 Windows 和 Linux 动态库

set -e

PROJECT_ROOT=$(cd "$(dirname "$0")" && pwd)
cd "$PROJECT_ROOT"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== dl-core 交叉编译脚本 ===${NC}"
echo "在 macOS 上编译其他平台的动态库"
echo ""

# 检查命令是否存在
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# 检查 MinGW 安装
check_mingw() {
    echo -e "${YELLOW}检查 MinGW-w64 安装...${NC}"
    
    if command_exists x86_64-w64-mingw32-gcc; then
        echo -e "${GREEN}✓ MinGW-w64 已安装${NC}"
        x86_64-w64-mingw32-gcc --version | head -1
        return 0
    fi
    
    echo -e "${RED}✗ MinGW-w64 未安装${NC}"
    echo ""
    echo "安装方法:"
    echo "  brew install mingw-w64"
    echo ""
    echo "或者从源码编译 (耗时较长):"
    echo "  1. 下载: https://sourceforge.net/projects/mingw-w64/files/"
    echo "  2. 解压到 /opt 或 /usr/local"
    echo ""
    return 1
}

# 检查 Linux 交叉编译器
check_linux_cross() {
    echo -e "${YELLOW}检查 Linux 交叉编译器...${NC}"
    
    if command_exists x86_64-linux-gnu-gcc; then
        echo -e "${GREEN}✓ Linux 交叉编译器已安装${NC}"
        x86_64-linux-gnu-gcc --version | head -1
        return 0
    fi
    
    echo -e "${RED}✗ Linux 交叉编译器未安装${NC}"
    echo ""
    echo "安装方法 (选择一种):"
    echo ""
    echo "方法 1 - 使用 Docker (推荐):"
    echo "  不需要安装交叉编译器，使用 Docker 容器编译"
    echo ""
    echo "方法 2 - 安装预编译的交叉编译器:"
    echo "  brew install x86_64-linux-gnu-binutils"
    echo "  (注: 可能需要手动构建完整的交叉编译工具链)"
    echo ""
    return 1
}

# 编译 Windows 版本 (MinGW)
build_windows() {
    echo ""
    echo -e "${BLUE}>>> 编译 Windows x86_64 版本${NC}"
    
    if ! check_mingw; then
        return 1
    fi
    
    # 查找 OpenSSL for MinGW
    local mingw_openssl_path=""
    for path in /opt/homebrew/opt/openssl@3/lib /usr/local/opt/openssl@3/lib; do
        if [ -f "$path/libssl.a" ] || [ -f "$path/libssl.dll.a" ]; then
            mingw_openssl_path="$path"
            break
        fi
    done
    
    if [ -z "$mingw_openssl_path" ]; then
        echo -e "${YELLOW}警告: 未找到 MinGW 版本的 OpenSSL${NC}"
        echo "Windows 编译需要 Windows 版本的 OpenSSL 库。"
        echo ""
        echo "解决方案:"
        echo "1. 在 Windows 上直接使用 Visual Studio 编译 (推荐)"
        echo "2. 使用 MSYS2 + MinGW 在 Windows 上编译"
        echo "3. 使用 vcpkg 管理 MinGW 依赖"
        echo ""
        echo -e "${YELLOW}尝试使用 CMake 查找 MinGW 库...${NC}"
    fi
    
    rm -rf build-windows-mingw
    mkdir -p build-windows-mingw
    
    # 配置 CMake
    cmake -B build-windows-mingw \
        -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw-w64.cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$mingw_openssl_path" \
        .
    
    # 编译
    cmake --build build-windows-mingw --config Release -j$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    if [ -f "build-windows-mingw/decentrilicense.dll" ] || [ -f "build-windows-mingw/libdecentrilicense.dll" ]; then
        echo -e "${GREEN}✓ Windows 编译成功${NC}"
        ls -lh build-windows-mingw/*.dll 2>/dev/null || ls -lh build-windows-mingw/*.a
    else
        echo -e "${RED}✗ Windows 编译可能失败或缺少依赖${NC}"
        echo "请检查输出日志"
    fi
}

# 编译 Linux 版本 (交叉编译)
build_linux_cross() {
    echo ""
    echo -e "${BLUE}>>> 编译 Linux x86_64 版本 (交叉编译)${NC}"
    
    if ! check_linux_cross; then
        echo ""
        echo -e "${YELLOW}切换到 Docker 编译方式...${NC}"
        build_linux_docker
        return
    fi
    
    rm -rf build-linux-cross
    mkdir -p build-linux-cross
    
    cmake -B build-linux-cross \
        -DCMAKE_TOOLCHAIN_FILE=toolchain-linux-cross.cmake \
        -DCMAKE_BUILD_TYPE=Release \
        .
    
    cmake --build build-linux-cross --config Release -j$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    if [ -f "build-linux-cross/libdecentrilicense.so" ]; then
        echo -e "${GREEN}✓ Linux 交叉编译成功${NC}"
        ls -lh build-linux-cross/libdecentrilicense.so
    else
        echo -e "${RED}✗ Linux 交叉编译失败${NC}"
    fi
}

# 编译 Linux 版本 (Docker)
build_linux_docker() {
    echo ""
    echo -e "${BLUE}>>> 编译 Linux x86_64 版本 (Docker)${NC}"
    
    if ! command_exists docker; then
        echo -e "${RED}错误: Docker 未安装${NC}"
        echo "请先安装 Docker Desktop"
        return 1
    fi
    
    if ! docker info >/dev/null 2>&1; then
        echo -e "${RED}错误: Docker 守护进程未运行${NC}"
        echo "请先启动 Docker Desktop"
        return 1
    fi
    
    echo "正在启动 Docker 容器进行编译..."
    
    docker run --rm \
        -v "$PROJECT_ROOT:/workdir" \
        -w /workdir \
        -e CC=gcc \
        -e CXX=g++ \
        ubuntu:22.04 \
        bash -c '
            set -e
            echo "更新包列表..."
            apt-get update -qq
            
            echo "安装编译依赖..."
            apt-get install -y -qq \
                cmake \
                build-essential \
                libssl-dev \
                libcurl4-openssl-dev \
                pkg-config \
                libsecret-1-dev
            
            echo "清理旧构建..."
            rm -rf build-linux-docker
            mkdir -p build-linux-docker
            
            echo "配置 CMake..."
            cmake -B build-linux-docker \
                -DCMAKE_BUILD_TYPE=Release \
                .
            
            echo "开始编译..."
            cmake --build build-linux-docker --config Release -j$(nproc)
            
            echo "编译完成!"
            ls -lh build-linux-docker/libdecentrilicense.so
        '
    
    if [ -f "build-linux-docker/libdecentrilicense.so" ]; then
        echo -e "${GREEN}✓ Linux Docker 编译成功${NC}"
        ls -lh build-linux-docker/libdecentrilicense.so
    else
        echo -e "${RED}✗ Linux Docker 编译失败${NC}"
    fi
}

# 编译 macOS 版本 (本地)
build_macos() {
    echo ""
    echo -e "${BLUE}>>> 编译 macOS x86_64 版本 (本地)${NC}"
    
    if ! command_exists cmake; then
        echo -e "${RED}错误: cmake 未安装${NC}"
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
        echo -e "${GREEN}✓ macOS 编译成功${NC}"
        ls -lh build-macos-x86_64/libdecentrilicense.dylib
    else
        echo -e "${RED}✗ macOS 编译失败${NC}"
    fi
}

# 显示菜单
show_menu() {
    echo ""
    echo "请选择要编译的平台:"
    echo "  1. macOS x86_64 (本地)"
    echo "  2. Linux x86_64 (Docker - 无需安装交叉编译器)"
    echo "  3. Windows x86_64 (需要 MinGW-w64)"
    echo "  4. 编译所有平台"
    echo "  5. 安装交叉编译工具"
    echo "  6. 退出"
    echo ""
}

# 安装交叉编译工具
install_tools() {
    echo ""
    echo -e "${BLUE}安装交叉编译工具${NC}"
    echo ""
    
    echo "1. 安装 MinGW-w64 (用于编译 Windows 库):"
    echo "   brew install mingw-w64"
    echo ""
    
    echo "2. Linux 交叉编译器推荐方案:"
    echo "   使用 Docker (最简单，无需安装额外工具)"
    echo "   或: brew install x86_64-linux-gnu-binutils"
    echo ""
    
    read -p "是否现在安装 MinGW-w64? (y/N): " answer
    if [ "$answer" = "y" ] || [ "$answer" = "Y" ]; then
        echo "正在安装 MinGW-w64..."
        brew install mingw-w64
    fi
}

# 编译所有平台
build_all() {
    echo ""
    echo -e "${BLUE}=== 编译所有平台 ===${NC}"
    
    build_macos
    build_linux_docker
    
    if check_mingw >/dev/null 2>&1; then
        build_windows
    else
        echo ""
        echo -e "${YELLOW}跳过 Windows 编译 (MinGW 未安装)${NC}"
    fi
    
    echo ""
    echo -e "${GREEN}=== 编译摘要 ===${NC}"
    echo ""
    
    for dir in build-macos-x86_64 build-linux-docker build-windows-mingw; do
        if [ -d "$dir" ]; then
            echo "📁 $dir:"
            ls -lh $dir/*.dylib $dir/*.so $dir/*.dll 2>/dev/null || echo "   (无输出文件)"
        fi
    done
}

# 主逻辑
main() {
    # 检查是否在 macOS 上运行
    if [ "$(uname)" != "Darwin" ]; then
        echo -e "${YELLOW}警告: 此脚本设计在 macOS 上运行${NC}"
        echo "当前系统: $(uname)"
        echo ""
    fi
    
    if [ $# -eq 0 ]; then
        show_menu
        read -p "请输入选项 [1-6]: " choice
    else
        choice=$1
    fi
    
    case $choice in
        1)
            build_macos
            ;;
        2)
            build_linux_docker
            ;;
        3)
            build_windows
            ;;
        4)
            build_all
            ;;
        5)
            install_tools
            ;;
        6)
            echo "退出"
            exit 0
            ;;
        *)
            echo -e "${RED}无效选项${NC}"
            exit 1
            ;;
    esac
}

main "$@"
