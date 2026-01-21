#!/bin/bash

# Go语言绑定打包脚本

echo "Building Go package..."

# 设置变量
PACKAGE_NAME="decentrilicense-go"
VERSION="1.0.0"
DIST_DIR="./packages/go"
BUILD_DIR="./build/go"

# 创建必要的目录
mkdir -p "$DIST_DIR"
mkdir -p "$BUILD_DIR"

# 进入Go绑定目录
cd ../bindings/go

# 构建Go模块
echo "Initializing Go module..."
go mod tidy

# 创建版本标签
echo "Creating version tag..."
git tag "v$VERSION" 2>/dev/null || echo "Tag v$VERSION already exists"

# 打包源代码
echo "Packaging source code..."
tar -czf "../../dist/$DIST_DIR/${PACKAGE_NAME}-src-${VERSION}.tar.gz" \
    --exclude='*.tar.gz' \
    --exclude='*.zip' \
    --exclude='.git' \
    --exclude='main' \
    --exclude='*.exe' \
    .

# 创建简单的安装说明
cat > "../../dist/$DIST_DIR/README.md" << EOF
# DecentriLicense Go SDK Package

## 安装说明

1. 解压源代码包:
   \`\`\`bash
   tar -xzf ${PACKAGE_NAME}-src-${VERSION}.tar.gz
   \`\`\`

2. 进入解压后的目录并初始化模块:
   \`\`\`bash
   cd decentrilicense-go
   go mod tidy
   \`\`\`

3. 在您的项目中使用:
   \`\`\`go
   import "path/to/decentrilicense-go"
   \`\`\`

## 使用示例

请参考 example.go 文件了解如何使用该SDK。

## 版本信息

版本: ${VERSION}
发布日期: $(date +%Y-%m-%d)
EOF

echo "Go package built successfully!"
echo "Package location: ../../dist/$DIST_DIR"