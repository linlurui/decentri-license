#!/bin/bash
# DecentriLicense Validation Wizard 编译脚本
# 读取 build_config.sh 中的配置进行编译

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 读取构建配置
if [ -f "${SCRIPT_DIR}/../build_config.sh" ]; then
    echo "📖 读取构建配置: ${SCRIPT_DIR}/../build_config.sh"
    source "${SCRIPT_DIR}/../build_config.sh"
else
    echo "⚠️  未找到 build_config.sh，使用默认配置"
    export CGO_LDFLAGS="-L../../dl-core/build -ldecentrilicense"
    # 设置默认路径
    DYLIB_PATH="${SCRIPT_DIR}/../../../dl-core/build"
fi

echo "🔧 CGO_LDFLAGS: ${CGO_LDFLAGS}"
echo "📦 开始编译 validation_wizard..."

cd "${SCRIPT_DIR}"
go build -o validation_wizard validation_wizard.go

if [ $? -eq 0 ]; then
    echo "✅ 编译成功: ${SCRIPT_DIR}/validation_wizard"

    # 【新增】使用 install_name_tool 添加 rpath
    if [ -n "$DYLIB_PATH" ]; then
        echo "🔧 添加 rpath 到可执行文件..."
        install_name_tool -add_rpath "$DYLIB_PATH" validation_wizard 2>/dev/null || {
            echo "⚠️  无法添加 rpath（可能已存在或权限不足）"
        }
        echo "✅ rpath 已设置: $DYLIB_PATH"
    fi

    echo ""
    echo "运行方式:"
    echo "  1. 直接运行（推荐）: ./validation_wizard"
    echo "  2. 使用启动脚本: ./run.sh"
    echo "  3. go run: source ../build_config.sh && go run validation_wizard.go"
else
    echo "❌ 编译失败"
    exit 1
fi
