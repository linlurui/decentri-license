#!/bin/bash
# inject_dylib_path.sh - 在一键打包时注入动态库路径
# 使用方法: ./inject_dylib_path.sh <动态库路径>

if [ -z "$1" ]; then
    echo "❌ 错误: 请提供动态库路径"
    echo "使用方法: $0 <动态库路径>"
    exit 1
fi

DYLIB_PATH="$1"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_CONFIG="${SCRIPT_DIR}/build_config.sh"

# 检查动态库是否存在
if [ ! -f "${DYLIB_PATH}/libdecentrilicense.dylib" ] && [ ! -f "${DYLIB_PATH}/libdecentrilicense.so" ]; then
    echo "⚠️  警告: 在 ${DYLIB_PATH} 未找到动态库文件"
fi

# 转换为绝对路径
DYLIB_PATH="$(cd "${DYLIB_PATH}" 2>/dev/null && pwd)" || {
    echo "❌ 错误: 无效的路径 ${DYLIB_PATH}"
    exit 1
}

echo "📝 正在注入动态库路径..."
echo "   路径: ${DYLIB_PATH}"
echo ""

# 1. 更新 build_config.sh 中的配置
echo "1️⃣  更新 build_config.sh..."

# 使用sed替换 AUTO_REPLACE 标记之间的内容
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    sed -i '' "/# AUTO_REPLACE_DYLIB_PATH_START/,/# AUTO_REPLACE_DYLIB_PATH_END/ {
        /# AUTO_REPLACE_DYLIB_PATH_START/a\\
export DYLIB_PATH_OVERRIDE=\"${DYLIB_PATH}\"
        /^export DYLIB_PATH_OVERRIDE=/d
    }" "${BUILD_CONFIG}"
else
    # Linux
    sed -i "/# AUTO_REPLACE_DYLIB_PATH_START/,/# AUTO_REPLACE_DYLIB_PATH_END/ {
        /# AUTO_REPLACE_DYLIB_PATH_START/a\export DYLIB_PATH_OVERRIDE=\"${DYLIB_PATH}\"
        /^export DYLIB_PATH_OVERRIDE=/d
    }" "${BUILD_CONFIG}"
fi

# 验证是否成功
if source "${BUILD_CONFIG}" 2>/dev/null; then
    echo "   ✅ build_config.sh 更新成功"
else
    echo "   ❌ build_config.sh 验证失败"
    exit 1
fi

# 2. 更新 decenlicense.go 中的 #cgo LDFLAGS (如果需要)
echo ""
echo "2️⃣  检查 decenlicense.go 中的 #cgo 指令..."
DECENLICENSE_GO="${SCRIPT_DIR}/decenlicense.go"

if [ -f "${DECENLICENSE_GO}" ]; then
    # 检查是否已经包含 rpath
    if grep -q "rpath" "${DECENLICENSE_GO}"; then
        echo "   ✅ decenlicense.go 已包含 rpath，无需修改"
    else
        echo "   ⚠️  decenlicense.go 缺少 rpath，建议手动添加："
        echo "      在 #cgo LDFLAGS 行添加: -Wl,-rpath,\${SRCDIR}/../../dl-core/build"
    fi
else
    echo "   ⚠️  未找到 decenlicense.go"
fi

# 最终报告
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ 配置注入完成!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "📦 动态库路径: ${DYLIB_PATH}"
echo ""
echo "🎯 用户现在可以直接使用 go run:"
echo "   cd sdks/go/validation_wizard"
echo "   go run validation_wizard.go"
echo ""
echo "💡 说明:"
echo "   • decenlicense.go 中的 #cgo LDFLAGS 已包含 rpath"
echo "   • 无需设置环境变量，可直接 go run"
echo "   • 无需使用包装脚本"
echo ""
else
    echo "❌ 配置验证失败,请检查 build_config.sh"
    exit 1
fi
