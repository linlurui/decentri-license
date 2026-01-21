#!/bin/bash

# C#语言绑定打包脚本

echo "Building C# package..."

# 设置变量
PACKAGE_NAME="decentrilicense-csharp"
VERSION="1.0.0"
DIST_DIR="../../dist/packages/csharp"
BUILD_DIR="../../dist/build/csharp"

# 创建必要的目录
mkdir -p "$DIST_DIR"
mkdir -p "$BUILD_DIR"

# 进入C#绑定目录
cd ../bindings/csharp

# 使用dotnet构建包
echo "Building package with dotnet..."
dotnet build --configuration Release

# 创建NuGet包
echo "Creating NuGet package..."
dotnet pack --configuration Release

# 复制NuGet包到分发目录
cp DecentriLicense/bin/Release/*.nupkg "$DIST_DIR/"

# 创建源代码包
echo "Creating source distribution..."
tar -czf "$DIST_DIR/${PACKAGE_NAME}-src-${VERSION}.tar.gz" \
    --exclude='*.nupkg' \
    --exclude='bin/' \
    --exclude='obj/' \
    --exclude='.git' \
    --exclude='*.tar.gz' \
    --exclude='*.zip' \
    --exclude='*.so' \
    --exclude='*.dylib' \
    --exclude='*.dll' \
    .

# 创建安装说明
cat > "$DIST_DIR/README.md" << EOF
# DecentriLicense C# SDK Package

## 安装说明

### 方法1: 使用NuGet包管理器安装
在Visual Studio的包管理器控制台中运行:
\`\`\`
Install-Package DecentriLicense
\`\`\`

### 方法2: 使用.NET CLI安装
\`\`\`bash
dotnet add package DecentriLicense
\`\`\`

### 方法3: 从本地NuGet包安装
\`\`\`bash
dotnet add package DecentriLicense --source "$DIST_DIR"
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
   dotnet build --configuration Release
   \`\`\`

## 使用示例

请参考 Example/ 目录了解如何使用该SDK。

## 版本信息

版本: ${VERSION}
发布日期: $(date +%Y-%m-%d)
EOF

echo "C# package built successfully!"
echo "Package location: $DIST_DIR"