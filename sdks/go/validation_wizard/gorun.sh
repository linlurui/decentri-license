#!/bin/bash
# gorun.sh - 自动配置并运行 validation_wizard
# 使用方法: ./gorun.sh

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 加载构建配置 (会自动查找动态库并设置环境变量)
echo "🔄 加载构建配置..."
source "${SCRIPT_DIR}/../build_config.sh"

if [ $? -ne 0 ]; then
    echo "❌ 加载构建配置失败"
    exit 1
fi

# 进入源码目录
cd "${SCRIPT_DIR}"

# 使用 go run 运行程序
# CGO_LDFLAGS 中已经包含了 rpath，所以可以直接运行
echo ""
echo "🚀 运行 validation_wizard..."
echo ""
go run validation_wizard.go "$@"

# 检查退出状态
if [ $? -ne 0 ]; then
    echo ""
    echo "❌ 程序运行失败"
    exit 1
fi
