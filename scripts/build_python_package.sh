#!/bin/bash

# Python语言绑定打包脚本

echo "Building Python package..."

# 设置变量
PACKAGE_NAME="decentrilicense-python"
VERSION="1.0.0"
DIST_DIR="../../dist/packages/python"
BUILD_DIR="../../dist/build/python"

# 创建必要的目录
mkdir -p "$DIST_DIR"
mkdir -p "$BUILD_DIR"

# 进入Python绑定目录
cd ../bindings/python

# 创建wheel包
echo "Creating wheel package..."
python setup.py bdist_wheel

# 复制wheel包到分发目录
cp dist/*.whl "$DIST_DIR/"

# 创建源代码包
echo "Creating source distribution..."
python setup.py sdist

# 复制源代码包到分发目录
cp dist/*.tar.gz "$DIST_DIR/"

# 创建安装说明
cat > "$DIST_DIR/README.md" << EOF
# DecentriLicense Python SDK Package

## 安装说明

### 方法1: 使用pip安装wheel包
\`\`\`bash
pip install ${PACKAGE_NAME}-${VERSION}-py3-none-any.whl
\`\`\`

### 方法2: 使用pip安装源代码包
\`\`\`bash
pip install ${PACKAGE_NAME}-${VERSION}.tar.gz
\`\`\`

### 方法3: 从源代码安装
1. 解压源代码包:
   \`\`\`bash
   tar -xzf ${PACKAGE_NAME}-${VERSION}.tar.gz
   \`\`\`

2. 进入解压后的目录:
   \`\`\`bash
   cd ${PACKAGE_NAME}-${VERSION}
   \`\`\`

3. 安装:
   \`\`\`bash
   pip install .
   \`\`\`

## 使用示例

请参考 example.py 文件了解如何使用该SDK。

## 版本信息

版本: ${VERSION}
发布日期: $(date +%Y-%m-%d)
EOF

echo "Python package built successfully!"
echo "Package location: $DIST_DIR"