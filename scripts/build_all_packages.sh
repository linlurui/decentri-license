#!/bin/bash

# 构建所有语言绑定包的主控脚本

echo "Building all language binding packages..."

# 设置变量
DIST_ROOT_DIR="../dist/packages"
BUILD_ROOT_DIR="../dist/build"

# 创建必要的目录
mkdir -p "$DIST_ROOT_DIR"
mkdir -p "$BUILD_ROOT_DIR"

# 构建Go包
echo "====================================="
echo "Building Go package..."
echo "====================================="
./build_go_package.sh

# 构建Python包
echo "====================================="
echo "Building Python package..."
echo "====================================="
./build_python_package.sh

# 构建Java包
echo "====================================="
echo "Building Java package..."
echo "====================================="
./build_java_package.sh

# 构建Node.js包
echo "====================================="
echo "Building Node.js package..."
echo "====================================="
./build_nodejs_package.sh

# 构建Rust包
echo "====================================="
echo "Building Rust package..."
echo "====================================="
./build_rust_package.sh

# 构建C#包
echo "====================================="
echo "Building C# package..."
echo "====================================="
./build_csharp_package.sh

echo "====================================="
echo "All packages built successfully!"
echo "Packages are located in: $DIST_ROOT_DIR"
echo "====================================="