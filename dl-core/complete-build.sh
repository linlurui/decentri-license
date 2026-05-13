#!/bin/bash
# 完整修复和编译脚本 - 请在终端手动运行

set -e
cd "$(dirname "$0")"

echo "=========================================="
echo " dl-core 完整修复和编译脚本"
echo "=========================================="
echo ""

# 颜色
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Step 1: 强制修复 Homebrew
echo -e "${BLUE}[Step 1/7] 强制修复 Homebrew...${NC}"
echo "清理锁定文件和缓存..."
pkill -9 -f "brew" 2>/dev/null || true
pkill -9 -f "ruby" 2>/dev/null || true
pkill -9 -f "vendor-install" 2>/dev/null || true
rm -rf ~/Library/Caches/Homebrew/portable-ruby*
rm -rf ~/Library/Caches/Homebrew/downloads/*incomplete*
rm -rf /opt/homebrew/var/homebrew/locks/*
rm -rf /opt/homebrew/Library/Homebrew/vendor/portable-ruby
brew cleanup 2>/dev/null || true
echo -e "${GREEN}✓ Homebrew 已清理${NC}"
echo ""

# Step 2: 安装 MinGW
echo -e "${BLUE}[Step 2/7] 安装 MinGW-w64...${NC}"
if command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo -e "${GREEN}✓ MinGW 已安装${NC}"
    x86_64-w64-mingw32-gcc --version | head -1
else
    echo "正在安装 MinGW-w64 (约需 5-10 分钟)..."
    echo -e "${YELLOW}如果卡住，请按 Ctrl+C 后重新运行本脚本${NC}"
    HOMEBREW_NO_AUTO_UPDATE=1 brew install mingw-w64
    if command -v x86_64-w64-mingw32-gcc &> /dev/null; then
        echo -e "${GREEN}✓ MinGW 安装成功${NC}"
    else
        echo -e "${RED}✗ MinGW 安装失败${NC}"
        echo "请手动运行: brew install mingw-w64"
        exit 1
    fi
fi
echo ""

# Step 3: 重启 Docker
echo -e "${BLUE}[Step 3/7] 重启 Docker Desktop...${NC}"
if ! docker ps &>/dev/null; then
    echo "停止 Docker..."
    pkill -9 -f "Docker" 2>/dev/null || true
    sleep 2
    echo "启动 Docker..."
    open -a "Docker"
    echo "等待 Docker 启动..."
    for i in {1..60}; do
        if docker ps &>/dev/null; then
            echo -e "${GREEN}✓ Docker 已启动${NC}"
            break
        fi
        sleep 1
        echo -n "."
    done
    if ! docker ps &>/dev/null; then
        echo -e "${RED}✗ Docker 启动失败${NC}"
        echo "请手动启动 Docker Desktop"
    fi
else
    echo -e "${GREEN}✓ Docker 运行正常${NC}"
fi
echo ""

# Step 4: 验证依赖
echo -e "${BLUE}[Step 4/7] 验证依赖...${NC}"
echo "Checking MinGW..."
if command -v x86_64-w64-mingw32-gcc &>/dev/null; then
    echo -e "  ${GREEN}✓ MinGW: $(x86_64-w64-mingw32-gcc --version | head -1)${NC}"
else
    echo -e "  ${RED}✗ MinGW 不可用${NC}"
fi

echo "Checking Docker..."
if docker ps &>/dev/null; then
    echo -e "  ${GREEN}✓ Docker: $(docker version --format '{{.Client.Version}}')${NC}"
else
    echo -e "  ${RED}✗ Docker 不可用${NC}"
fi
echo ""

# Step 5: 编译 macOS
echo -e "${BLUE}[Step 5/7] 编译 macOS ARM64...${NC}"
rm -rf build-macos-arm64
mkdir -p build-macos-arm64
cmake -B build-macos-arm64 -DCMAKE_BUILD_TYPE=Release . 2>&1 | tail -5
if cmake --build build-macos-arm64 --config Release -j4; then
    echo -e "${GREEN}✓ macOS 编译成功${NC}"
    ls -lh build-macos-arm64/libdecentrilicense.dylib
else
    echo -e "${RED}✗ macOS 编译失败${NC}"
fi
echo ""

# Step 6: 编译 Windows
echo -e "${BLUE}[Step 6/7] 编译 Windows x86_64...${NC}"
if command -v x86_64-w64-mingw32-gcc &>/dev/null; then
    rm -rf build-windows
    mkdir -p build-windows
    cmake -B build-windows \
        -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw-w64.cmake \
        -DCMAKE_BUILD_TYPE=Release . 2>&1 | tail -5
    if cmake --build build-windows --config Release -j4 2>&1; then
        if [ -f "build-windows/libdecentrilicense.dll" ] || [ -f "build-windows/decentrilicense.dll" ]; then
            echo -e "${GREEN}✓ Windows 编译成功${NC}"
            ls -lh build-windows/*.dll 2>/dev/null || true
        else
            echo -e "${YELLOW}⚠ 编译完成但找不到 DLL${NC}"
            ls -la build-windows/
        fi
    else
        echo -e "${RED}✗ Windows 编译失败${NC}"
    fi
else
    echo -e "${YELLOW}⚠ 跳过 Windows 编译 (MinGW 不可用)${NC}"
fi
echo ""

# Step 7: 编译 Linux
echo -e "${BLUE}[Step 7/7] 编译 Linux x86_64...${NC}"
if docker ps &>/dev/null; then
    echo "使用 Docker 编译..."
    if docker run --rm -v "$(pwd):/workdir" -w /workdir alpine:latest echo "test" &>/dev/null; then
        docker run --rm \
            -v "$(pwd):/workdir" \
            -w /workdir \
            alpine:3.18 \
            sh -c '
                echo "安装依赖..."
                apk add --no-cache cmake make gcc g++ openssl-dev curl-dev pkgconfig libsecret-dev 2>&1 | tail -3
                echo "配置..."
                rm -rf build-linux
                mkdir -p build-linux
                cmake -B build-linux -DCMAKE_BUILD_TYPE=Release . 2>&1 | tail -3
                echo "编译..."
                cmake --build build-linux --config Release -j4 2>&1 | tail -10
            '
        if [ -f "build-linux/libdecentrilicense.so" ]; then
            echo -e "${GREEN}✓ Linux 编译成功${NC}"
            ls -lh build-linux/libdecentrilicense.so
        else
            echo -e "${RED}✗ Linux 编译失败${NC}"
        fi
    else
        echo -e "${YELLOW}⚠ Docker 镜像拉取失败${NC}"
    fi
else
    echo -e "${YELLOW}⚠ 跳过 Linux 编译 (Docker 不可用)${NC}"
fi
echo ""

# 总结
echo "=========================================="
echo -e "${BLUE}编译结果汇总${NC}"
echo "=========================================="
echo ""
found=0
for dir in build-macos-arm64 build-windows build-linux; do
    if [ -d "$dir" ]; then
        files=$(find "$dir" -name "*.dylib" -o -name "*.so" -o -name "*.dll" 2>/dev/null | head -5)
        if [ -n "$files" ]; then
            echo "📁 $dir:"
            for f in $files; do
                size=$(ls -lh "$f" 2>/dev/null | awk '{print $5}')
                echo "   ✓ $(basename $f) - $size"
                found=$((found + 1))
            done
        fi
    fi
done

if [ $found -eq 0 ]; then
    echo -e "${RED}✗ 未找到任何编译输出${NC}"
else
    echo ""
    echo -e "${GREEN}✓ 共编译 $found 个库文件${NC}"
fi

echo ""
echo "=========================================="
echo " 完成时间: $(date)"
echo "=========================================="
