#!/bin/bash

# Java语言绑定打包脚本

echo "Building Java package..."

# 设置变量
PACKAGE_NAME="decentrilicense-java"
VERSION="1.0.0"
DIST_DIR="../../dist/packages/java"
BUILD_DIR="../../dist/build/java"

# 创建必要的目录
mkdir -p "$DIST_DIR"
mkdir -p "$BUILD_DIR"

# 进入Java绑定目录
cd ../bindings/java

# 使用Maven构建jar包
echo "Building JAR package with Maven..."
mvn clean package

# 复制jar包到分发目录
cp target/*.jar "$DIST_DIR/"

# 创建源代码包
echo "Creating source distribution..."
tar -czf "$DIST_DIR/${PACKAGE_NAME}-src-${VERSION}.tar.gz" \
    --exclude='*.jar' \
    --exclude='target/' \
    --exclude='.git' \
    --exclude='*.tar.gz' \
    --exclude='*.zip' \
    .

# 创建安装说明
cat > "$DIST_DIR/README.md" << EOF
# DecentriLicense Java SDK Package

## 安装说明

### Maven集成
在您的pom.xml文件中添加以下依赖:
\`\`\`xml
<dependency>
    <groupId>com.decentrilicense</groupId>
    <artifactId>decentrilicense-java</artifactId>
    <version>${VERSION}</version>
</dependency>
\`\`\`

### 手动安装
1. 下载JAR文件并将它添加到您的项目classpath中。

2. 如果您想从源代码构建:
   \`\`\`bash
   tar -xzf ${PACKAGE_NAME}-src-${VERSION}.tar.gz
   cd ${PACKAGE_NAME}-src-${VERSION}
   mvn clean package
   \`\`\`

## 使用示例

请参考 src/examples/ 目录了解如何使用该SDK。

## 版本信息

版本: ${VERSION}
发布日期: $(date +%Y-%m-%d)
EOF

echo "Java package built successfully!"
echo "Package location: $DIST_DIR"