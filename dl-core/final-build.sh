#!/bin/bash
# 最终编译脚本 - 持续运行直到所有平台编译完成

set -e
cd "$(dirname "$0")"

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  dl-core 最终编译脚本${NC}"
echo -e "${BLUE}  持续运行直到所有平台编译完成${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

SUCCESS_COUNT=0
TOTAL_COUNT=3

# 1. 编译 macOS (本地)
echo -e "${YELLOW}[1/3] 编译 macOS ARM64...${NC}"
for i in {1..3}; do
    rm -rf build-macos-final
    mkdir -p build-macos-final
    if cmake -B build-macos-final -DCMAKE_BUILD_TYPE=Release . >/dev/null 2>&1; then
        if cmake --build build-macos-final --config Release -j8 >/dev/null 2>&1; then
            if [ -f "build-macos-final/libdecentrilicense.dylib" ]; then
                echo -e "${GREEN}✓ macOS 编译成功${NC}"
                ls -lh build-macos-final/libdecentrilicense.dylib
                SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
                break
            fi
        fi
    fi
    echo "  尝试 $i/3..."
    sleep 2
done
if [ $SUCCESS_COUNT -eq 0 ]; then
    echo -e "${RED}✗ macOS 编译失败${NC}"
fi
echo ""

# 2. 编译 Windows (MinGW)
echo -e "${YELLOW}[2/3] 编译 Windows x86_64 (MinGW)...${NC}"
MINGW_READY=0

# 首先尝试安装 MinGW
if ! command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
    echo "  MinGW 未安装，尝试安装..."
    pkill -9 -f "brew" 2>/dev/null || true
    rm -rf ~/Library/Caches/Homebrew/portable-ruby* 2>/dev/null || true
    
    # 尝试安装，最多等待 10 分钟
    timeout 600 bash -c 'HOMEBREW_NO_AUTO_UPDATE=1 brew install mingw-w64 2>&1' || true
    
    # 检查是否安装成功
    if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
        echo -e "  ${GREEN}MinGW 安装成功${NC}"
        MINGW_READY=1
    else
        echo -e "  ${RED}MinGW 安装失败或超时${NC}"
    fi
else
    echo "  MinGW 已安装"
    MINGW_READY=1
fi

# 编译 Windows
if [ $MINGW_READY -eq 1 ]; then
    rm -rf build-windows-final
    mkdir -p build-windows-final
    if cmake -B build-windows-final -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw-w64.cmake -DCMAKE_BUILD_TYPE=Release . >/dev/null 2>&1; then
        if cmake --build build-windows-final --config Release -j4 >/dev/null 2>&1; then
            if [ -f "build-windows-final/libdecentrilicense.dll" ] || [ -f "build-windows-final/decentrilicense.dll" ]; then
                echo -e "${GREEN}✓ Windows 编译成功${NC}"
                ls -lh build-windows-final/*.dll 2>/dev/null || true
                SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
            else
                echo -e "${RED}✗ Windows 编译失败 (无输出文件)${NC}"
            fi
        else
            echo -e "${RED}✗ Windows 编译失败 (编译错误)${NC}"
        fi
    else
        echo -e "${RED}✗ Windows CMake 配置失败${NC}"
    fi
else
    echo -e "${YELLOW}⚠ 跳过 Windows 编译 (MinGW 不可用)${NC}"
fi
echo ""

# 3. 编译 Linux (Docker)
echo -e "${YELLOW}[3/3] 编译 Linux x86_64 (Docker)...${NC}"
DOCKER_READY=0

# 检查 Docker
if command -v docker >/dev/null 2>&1; then
    if docker ps >/dev/null 2>&1; then
        echo "  Docker 运行正常"
        DOCKER_READY=1
    else
        echo "  Docker 未运行，尝试启动..."
        pkill -9 Docker 2>/dev/null || true
        sleep 2
        open -a Docker 2>/dev/null || true
        
        # 等待 Docker 启动
        for i in {1..60}; do
            if docker ps >/dev/null 2>&1; then
                echo "  Docker 启动成功"
                DOCKER_READY=1
                break
            fi
            sleep 1
        done
    fi
else
    echo -e "  ${RED}Docker 未安装${NC}"
fi

# 编译 Linux
if [ $DOCKER_READY -eq 1 ]; then
    echo "  使用 Debian 容器编译..."
    if docker run --rm -v "$(pwd):/workdir" -w /workdir debian:bookworm bash -c "
        apt-get update -qq
        apt-get install -y -qq cmake build-essential libssl-dev libcurl4-openssl-dev pkg-config libsecret-1-dev
        rm -rf build-linux-final
        mkdir -p build-linux-final
        cmake -B build-linux-final -DCMAKE_BUILD_TYPE=Release .
        cmake --build build-linux-final --config Release -j4
    " 2>&1; then
        if [ -f "build-linux-final/libdecentrilicense.so" ]; then
            echo -e "${GREEN}✓ Linux 编译成功${NC}"
            ls -lh build-linux-final/libdecentrilicense.so
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        else
            echo -e "${RED}✗ Linux 编译失败 (无输出文件)${NC}"
        fi
    else
        echo -e "${RED}✗ Linux 编译失败${NC}"
    fi
else
    echo -e "${YELLOW}⚠ 跳过 Linux 编译 (Docker 不可用)${NC}"
fi
echo ""

# 总结
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE} 编译结果汇总${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "成功: $SUCCESS_COUNT / $TOTAL_COUNT"
echo ""

for dir in build-macos-final build-windows-final build-linux-final; do
    if [ -d "$dir" ]; then
        files=$(find "$dir" -name "*.dylib" -o -name "*.so" -o -name "*.dll" 2>/dev/null)
        if [ -n "$files" ]; then
            echo "📁 $dir:"
            for f in $files; do
                size=$(ls -lh "$f" 2>/dev/null | awk '{print $5}')
                arch=$(file "$f" 2>/dev/null | cut -d: -f2 | cut -d, -f1)
                echo "   ✓ $(basename $f) - $size -$arch"
            done
        fi
    fi
done

echo ""
echo -e "${GREEN}所有操作完成!${NC}"
echo ""
