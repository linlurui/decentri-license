#!/bin/bash

# Rust语言绑定打包脚本

echo "Building Rust package..."

# 设置变量
PACKAGE_NAME="decentrilicense-rust"
VERSION="1.0.0"
DIST_DIR="../../dist/packages/rust"
BUILD_DIR="../../dist/build/rust"

# 创建必要的目录
mkdir -p "$DIST_DIR"
mkdir -p "$BUILD_DIR"

# 进入Rust绑定目录
cd ../bindings/rust

# 使用Cargo构建包
echo "Building package with Cargo..."
cargo build --release

# 创建crate包
echo "Creating crate package..."
cargo package

# 复制crate包到分发目录
cp target/package/*.crate "$DIST_DIR/"

# 创建源代码包
echo "Creating source distribution..."
tar -czf "$DIST_DIR/${PACKAGE_NAME}-src-${VERSION}.tar.gz" \
    --exclude='*.crate' \
    --exclude='target/' \
    --exclude='.git' \
    --exclude='*.tar.gz' \
    --exclude='*.zip' \
    --exclude='*.so' \
    --exclude='*.dylib' \
    --exclude='*.dll' \
    .

# 创建安装说明
cat > "$DIST_DIR/README.md" << EOF
# DecentriLicense Rust SDK Package

## 安装说明

### 方法1: 使用Cargo安装
在您的Cargo.toml文件中添加以下依赖:
\`\`\`toml
[dependencies]
decentrilicense = "${VERSION}"
\`\`\`

### 方法2: 从crates.io安装
\`\`\`bash
cargo install decentrilicense
\`\`\`

### 方法3: 从本地crate安装
\`\`\`bash
cargo install --path .
\`\`\`

### 方法4: 从源代码构建
1. 解压源代码包:
   \`\`\`bash
   tar -xzf ${PACKAGE_NAME}-src-${VERSION}.tar.gz
   \`\`\`

2. 进入解压后的目录:
   \`\`\`bash
   cd ${PACKAGE_NAME}-src-${VERSION}
   \`\`\`

3. 构建:
   \`\`\`bash
   cargo build --release
   \`\`\`

## 使用示例

请参考 examples/ 目录了解如何使用该SDK。

## 版本信息

版本: ${VERSION}
发布日期: $(date +%Y-%m-%d)
EOF

echo "Rust package built successfully!"
echo "Package location: $DIST_DIR"