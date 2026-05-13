#!/bin/bash
# 立即编译脚本 - 编译所有可用平台

set -e

cd "$(dirname "$0")"

echo "=== dl-core 多平台编译 ==="
echo "时间: $(date)"
echo ""

# 颜色
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# 1. 编译 macOS ARM64 (Apple Silicon)
echo "[1/3] 编译 macOS ARM64..."
rm -rf build-macos-arm64
mkdir -p build-macos-arm64
cmake -B build-macos-arm64 -DCMAKE_BUILD_TYPE=Release . > /dev/null 2>&1
if cmake --build build-macos-arm64 --config Release -j4 > /dev/null 2>&1; then
    echo -e "${GREEN}✓ macOS ARM64 编译成功${NC}"
    ls -lh build-macos-arm64/libdecentrilicense.dylib 2>/dev/null || true
else
    echo -e "${RED}✗ macOS ARM64 编译失败${NC}"
fi
echo ""

# 2. 检查并编译 Windows (MinGW)
echo "[2/3] 编译 Windows x86_64 (MinGW)..."
if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
    rm -rf build-windows
    mkdir -p build-windows
    cmake -B build-windows \
        -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw-w64.cmake \
        -DCMAKE_BUILD_TYPE=Release . > /dev/null 2>&1
    if cmake --build build-windows --config Release -j4 > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Windows x86_64 编译成功${NC}"
        ls -lh build-windows/*.dll 2>/dev/null || true
    else
        echo -e "${RED}✗ Windows x86_64 编译失败${NC}"
    fi
else
    echo -e "${YELLOW}⚠ MinGW 未安装，跳过 Windows 编译${NC}"
    echo "安装命令: brew install mingw-w64"
fi
echo ""

# 3. 检查并编译 Linux (Docker)
echo "[3/3] 编译 Linux x86_64 (Docker)..."
if command -v docker >/dev/null 2>&1 && docker info >/dev/null 2>&1; then
    if docker run --rm -v "$(pwd):/workdir" -w /workdir alpine:latest echo "test" >/dev/null 2>&1; then
        echo "Docker 可用，开始编译..."
        docker run --rm \
            -v "$(pwd):/workdir" \
            -w /workdir \
            alpine:latest \
            sh -c "
                apk add --no-cache cmake make gcc g++ openssl-dev curl-dev pkgconfig libsecret-dev 2>&1 | tail -3
                rm -rf build-linux
                mkdir -p build-linux
                cmake -B build-linux -DCMAKE_BUILD_TYPE=Release . 2>&1 | tail -3
                cmake --build build-linux --config Release -j4 2>&1 | tail -5
            "
        if [ -f build-linux/libdecentrilicense.so ]; then
            echo -e "${GREEN}✓ Linux x86_64 编译成功${NC}"
            ls -lh build-linux/libdecentrilicense.so
        else
            echo -e "${RED}✗ Linux x86_64 编译失败${NC}"
        fi
    else
        echo -e "${YELLOW}⚠ Docker 镜像拉取失败${NC}"
        echo "请重启 Docker Desktop 后重试"
    fi
else
    echo -e "${YELLOW}⚠ Docker 不可用${NC}"
fi
echo ""

# 总结
echo "=== 编译结果汇总 ==="
echo ""
for dir in build-macos-arm64 build-windows build-linux; do
    if [ -d "$dir" ]; then
        echo "📁 $dir:"
        files=$(ls $dir/*.dylib $dir/*.so $dir/*.dll 2>/dev/null)
        if [ -n "$files" ]; then
            echo "$files" | while read f; do
                echo "   ✓ $(basename $f) - $(ls -lh "$f" | awk '{print $5}')"
            done
        else
            echo "   (无输出文件)"
        fi
    fi
done
echo ""
echo "完成时间: $(date)"
