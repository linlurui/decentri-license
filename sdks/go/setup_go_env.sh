#!/bin/bash
# setup_go_env.sh - 一键配置Go环境,让go run可以直接使用
# 使用方法: source ./setup_go_env.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "🔧 正在配置DecentriLicense Go SDK环境..."
echo ""

# 加载构建配置
if source "${SCRIPT_DIR}/build_config.sh"; then
    echo ""
    echo "✅ 环境配置成功!"
    echo ""
    echo "📋 配置信息:"
    echo "   动态库路径: ${DYLIB_PATH}"
    echo "   CGO_LDFLAGS: ${CGO_LDFLAGS}"
    echo ""
    echo "🎯 现在你可以在此终端直接使用:"
    echo "   cd sdks/go/validation_wizard"
    echo "   go run validation_wizard.go"
    echo ""
    echo "💡 提示:"
    echo "   • 此配置仅在当前终端session有效"
    echo "   • 新终端需要重新执行: source ./setup_go_env.sh"
    echo "   • 或添加到 ~/.zshrc: echo 'source ${SCRIPT_DIR}/build_config.sh' >> ~/.zshrc"
    echo ""
else
    echo "❌ 环境配置失败"
    return 1
fi
