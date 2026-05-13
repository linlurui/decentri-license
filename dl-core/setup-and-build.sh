#!/bin/bash
# 完整修复和编译脚本

set -e
cd "$(dirname "$0")"

echo "=== dl-core 完整编译脚本 ==="
echo "1. 修复环境"
echo "2. 安装依赖"
echo "3. 编译所有平台"
echo ""

# 颜色
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Step 1: 修复 Homebrew
echo -e "${BLUE}[Step 1] 修复 Homebrew...${NC}"
pkill -9 -f "brew" 2>/dev/null || true
pkill -9 -f "ruby" 2>/dev/null || true
rm -rf ~/Library/Caches/Homebrew/portable-ruby*
rm -rf /opt/homebrew/var/homebrew/locks/*
rm -rf /opt/homebrew/Library/Homebrew/vendor/portable-ruby
brew cleanup 2>/dev/null || true
echo -e "${GREEN}✓ Homebrew 已清理${NC}"

# Step 2: 安装 MinGW
echo -e "${BLUE}[Step 2] 安装 MinGW...${NC}"
if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "正在安装 MinGW-w64 (这可能需要几分钟)..."
    HOMEBREW_NO_AUTO_UPDATE=1 brew install mingw-w64 2>&1
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ MinGW 安装成功${NC}"
        x86_64-w64-mingw32-gcc --version | head -1
    else
        echo -e "${RED}✗ MinGW 安装失败${NC}"
        echo "请手动运行: brew install mingw-w64"
        exit 1
    fi
else
    echo -e "${GREEN}✓ MinGW 已安装${NC}"
fi

# Step 3: 修复 Docker
echo -e "${BLUE}[Step 3] 修复 Docker...${NC}"
if ! docker ps &>/dev/null; then
    echo "重启 Docker Desktop..."
    pkill -9 -f "Docker" || true
    sleep 2
    open -a "Docker"
    echo "等待 Docker 启动 (约30秒)..."
    for i in {1..30}; do
        if docker ps &>/dev/null; then
            echo -e "${GREEN}✓ Docker 已启动${NC}"
            break
        fi
        sleep 1
        echo -n "."
    done
else
    echo -e "${GREEN}✓ Docker 运行正常${NC}"
fi

# Step 4: 编译 macOS
echo -e "${BLUE}[Step 4] 编译 macOS...${NC}"
rm -rf build-macos-arm64
mkdir -p build-macos-arm64
cmake -B build-macos-arm64 -DCMAKE_BUILD_TYPE=Release . 2>&1 | tail -3
cmake --build build-macos-arm64 --config Release -j4 2>&1 | tail -3
if [ -f "build-macos-arm64/libdecentrilicense.dylib" ]; then
    echo -e "${GREEN}✓ macOS 编译成功${NC}"
    ls -lh build-macos-arm64/libdecentrilicense.dylib
else
    echo -e "${RED}✗ macOS 编译失败${NC}"
fi

# Step 5: 编译 Windows
echo -e "${BLUE}[Step 5] 编译 Windows...${NC}"
rm -rf build-windows
mkdir -p build-windows
cmake -B build-windows \
    -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw-w64.cmake \
    -DCMAKE_BUILD_TYPE=Release . 2>&1 | tail -3
cmake --build build-windows --config Release -j4 2>&1 | tail -5
if [ -f "build-windows/libdecentrilicense.dll" ] || [ -f "build-windows/decentrilicense.dll" ]; then
    echo -e "${GREEN}✓ Windows 编译成功${NC}"
    ls -lh build-windows/*.dll 2>/dev/null || true
else
    echo -e "${RED}✗ Windows 编译失败${NC}"
fi

# Step 6: 编译 Linux
echo -e "${BLUE}[Step 6] 编译 Linux...${NC}"
if docker ps &>/dev/null; then
    echo "使用 Docker 编译 Linux 版本..."
    docker run --rm \
        -v "$(pwd):/workdir" \
        -w /workdir \
        alpine:3.18 \
        sh -c '
            apk add --no-cache cmake make gcc g++ openssl-dev curl-dev pkgconfig libsecret-dev 2>&1 | tail -3
            rm -rf build-linux
            mkdir -p build-linux
            cmake -B build-linux -DCMAKE_BUILD_TYPE=Release . 2>&1 | tail -3
            cmake --build build-linux --config Release -j4 2>&1 | tail -5
        '
    if [ -f "build-linux/libdecentrilicense.so" ]; then
        echo -e "${GREEN}✓ Linux 编译成功${NC}"
        ls -lh build-linux/libdecentrilicense.so
    else
        echo -e "${RED}✗ Linux 编译失败${NC}"
    fi
else
    echo -e "${YELLOW}⚠ Docker 不可用，跳过 Linux 编译${NC}"
fi

# 总结
echo ""
echo -e "${BLUE}=== 编译结果汇总 ===${NC}"
echo ""
for dir in build-macos-arm64 build-windows build-linux; do
    if [ -d "$dir" ]; then
        echo "📁 $dir:"
        files=$(ls $dir/*.dylib $dir/*.so $dir/*.dll 2>/dev/null | head -5)
        if [ -n "$files" ]; then
            for f in $files; do
                size=$(ls -lh "$f" 2>/dev/null | awk '{print $5}')
                echo "   ✓ $(basename $f) - $size"
            done
        else
            echo "   (无输出文件)"
        fi
    fi
done

echo ""
echo -e "${GREEN}全部完成!${NC}"
