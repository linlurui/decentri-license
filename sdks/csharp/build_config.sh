#!/bin/bash
# DecentriLicense C# SDK 配置
# 此文件由一键打包程序自动更新

# ============================================
# 一键打包程序会替换下面这一行
# ============================================
# AUTO_REPLACE_DYLIB_PATH_START
export DYLIB_PATH_OVERRIDE=""
# AUTO_REPLACE_DYLIB_PATH_END

# 如果一键打包已设置绝对路径，使用它；否则自动计算
if [ -z "$DYLIB_PATH_OVERRIDE" ]; then
    SDK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    export DYLIB_PATH="${SDK_DIR}/../../dl-core/build"
else
    export DYLIB_PATH="$DYLIB_PATH_OVERRIDE"
fi

# .NET运行时需要设置
export DYLD_LIBRARY_PATH="${DYLIB_PATH}:${DYLD_LIBRARY_PATH}"
export LD_LIBRARY_PATH="${DYLIB_PATH}:${LD_LIBRARY_PATH}"  # Linux
