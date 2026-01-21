#!/bin/bash

# Node.js语言绑定打包脚本

echo "Building Node.js package..."

# 设置变量
PACKAGE_NAME="decentrilicense-nodejs"
VERSION="1.0.0"
DIST_DIR="../../dist/packages/nodejs"
BUILD_DIR="../../dist/build/nodejs"

# 创建必要的目录
mkdir -p "$DIST_DIR"
mkdir -p "$BUILD_DIR"

# 进入Node.js绑定目录
cd ../bindings/nodejs

# 使用npm构建包
echo "Building package with npm..."
npm install
npm run build

# 创建npm包
echo "Creating npm package..."
npm pack

# 复制npm包到分发目录
cp *.tgz "$DIST_DIR/"

# 创建源代码包
echo "Creating source distribution..."
tar -czf "$DIST_DIR/${PACKAGE_NAME}-src-${VERSION}.tar.gz" \
    --exclude='*.tgz' \
    --exclude='node_modules/' \
    --exclude='.git' \
    --exclude='*.tar.gz' \
    --exclude='*.zip' \
    --exclude='build/' \
    .

# 创建安装说明
cat > "$DIST_DIR/README.md" << EOF
# DecentriLicense Node.js SDK Package

## 安装说明

### 方法1: 使用npm安装
\`\`\`bash
npm install decentrilicense-nodejs-${VERSION}.tgz
\`\`\`

### 方法2: 从源代码安装
1. 解压源代码包:
   \`\`\`bash
   tar -xzf ${PACKAGE_NAME}-src-${VERSION}.tar.gz
   \`\`\`

2. 进入解压后的目录:
   \`\`\`bash
   cd ${PACKAGE_NAME}-src-${VERSION}
   \`\`\`

3. 安装依赖并构建:
   \`\`\`bash
   npm install
   npm run build
   \`\`\`

4. 在您的项目中使用:
   \`\`\`javascript
   const DecentriLicense = require('decentrilicense-nodejs');
   \`\`\`

## 使用示例

请参考 example.js 文件了解如何使用该SDK。

## 版本信息

版本: ${VERSION}
发布日期: $(date +%Y-%m-%d)
EOF

echo "Node.js package built successfully!"
echo "Package location: $DIST_DIR"