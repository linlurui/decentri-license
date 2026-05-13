#!/bin/bash
# DecentriLicense Go SDK 构建配置
# 自动根据当前位置查找dl-core动态库

# ============================================
# 智能路径查找机制
# ============================================

# 获取此配置文件的绝对路径
CONFIG_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 方法1: 优先使用一键打包程序设置的路径（如果有）
# AUTO_REPLACE_DYLIB_PATH_START
export DYLIB_PATH_OVERRIDE="/Volumes/project/decentri-license-issuer/dl-core/build"
# AUTO_REPLACE_DYLIB_PATH_END

# 方法2: 检查环境变量（签发程序可以设置）
if [ -z "$DYLIB_PATH_OVERRIDE" ] && [ -n "$DECENTRI_DL_CORE_PATH" ]; then
    DYLIB_PATH_OVERRIDE="$DECENTRI_DL_CORE_PATH"
fi

# 方法3: 自动搜索（从当前目录向上查找）
if [ -z "$DYLIB_PATH_OVERRIDE" ]; then
    # 尝试多个可能的相对路径
    search_paths=(
        "${CONFIG_DIR}/../../dl-core/build"          # 标准位置
        "${CONFIG_DIR}/../../../dl-core/build"       # 上一层
        "${CONFIG_DIR}/../../../../dl-core/build"    # 再上一层
        "${PWD}/dl-core/build"                       # 当前工作目录
        "${PWD}/../dl-core/build"                    # 当前工作目录的上层
    )

    for path in "${search_paths[@]}"; do
        if [ -f "${path}/libdecentrilicense.dylib" ] || [ -f "${path}/libdecentrilicense.so" ]; then
            DYLIB_PATH_OVERRIDE="$(cd "${path}" 2>/dev/null && pwd)"
            echo "🔍 自动找到动态库: ${DYLIB_PATH_OVERRIDE}"
            break
        fi
    done
fi

# 设置最终路径
if [ -n "$DYLIB_PATH_OVERRIDE" ]; then
    export DYLIB_PATH="$DYLIB_PATH_OVERRIDE"
else
    echo "⚠️  警告: 未找到libdecentrilicense动态库"
    echo "    请设置环境变量: export DECENTRI_DL_CORE_PATH=/path/to/dl-core/build"
    exit 1
fi

# CGO编译标志 - 使用动态路径
export CGO_LDFLAGS="-L${DYLIB_PATH} -ldecentrilicense"

# 【重要】为 go run 添加 rpath 支持
# 这样即使在临时目录编译也能找到动态库
export CGO_LDFLAGS="${CGO_LDFLAGS} -Wl,-rpath,${DYLIB_PATH}"

# 运行时动态库搜索路径 (作为备用)
export DYLD_LIBRARY_PATH="${DYLIB_PATH}:${DYLD_LIBRARY_PATH}"
export LD_LIBRARY_PATH="${DYLIB_PATH}:${LD_LIBRARY_PATH}"

# 打印配置信息
echo "📦 动态库路径: ${DYLIB_PATH}"
echo "🔧 CGO_LDFLAGS: ${CGO_LDFLAGS}"



