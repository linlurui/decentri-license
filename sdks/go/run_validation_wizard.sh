#!/bin/bash

echo "🎯 运行 DecentriLicense Go SDK 验证向导"
echo "========================================"

cd "$(dirname "$0")"

# 检查dl-core是否已编译
echo ""
echo "📦 步骤1: 检查dl-core状态..."
if [ ! -f "../../dl-core/build/libdecentrilicense.dylib" ]; then
    echo "❌ dl-core未编译，请先运行: go run ../../dl-issuer/main.go pack-core"
    exit 1
fi
echo "✅ dl-core已编译"

# 编译验证向导
echo ""
echo "🔨 步骤2: 编译验证向导..."
go build -o wizard ./validation_wizard/validation_wizard.go
if [ $? -ne 0 ]; then
    echo "❌ 验证向导编译失败"
    echo "编译错误："
    go build -o wizard ./validation_wizard/validation_wizard.go 2>&1
    exit 1
fi

# 设置库路径
echo ""
echo "🔧 步骤2.5: 设置库路径..."
# 设置指向SDK自己的lib目录的相对路径，这样一键打包后会自动使用新库
install_name_tool -add_rpath "@executable_path/../lib" wizard

echo "✅ 验证向导编译成功"

# 运行验证向导
echo ""
echo "🚀 步骤3: 启动验证向导..."
echo "DecentriLicense SDK 验证向导"
echo "============================="
echo ""
echo "这是一个交互式向导程序，支持以下功能："
echo "0. 🔑 选择产品公钥 - 选择要使用的产品公钥文件"
echo "1. 🔓 激活令牌 - 使用令牌激活许可证"
echo "2. ✅ 校验令牌 - 验证令牌有效性"
echo "3. 📊 记账信息 - 记录和查询使用情况"
echo "4. 🔗 信任链验证 - 验证令牌信任链"
echo "5. 🎯 综合验证 - 执行完整的验证流程"
echo "6. 🚪 退出"
echo ""
echo "📖 详细文档：./README.md"
echo ""
echo "🔧 准备工作："
echo "请确保当前目录下有产品公钥文件 (product_public.pem 或 public_*.pem)"
echo ""
echo ""
echo "开始运行向导："
echo ""

# 使用环境变量运行
env DYLD_LIBRARY_PATH="$PROJECT_ROOT/dl-core/build:$DYLD_LIBRARY_PATH" ./wizard

# 清理
echo ""
echo "🧹 清理临时文件..."
rm -f wizard
echo "✅ 清理完成"

echo ""
echo "🎉 验证向导运行完成!"
