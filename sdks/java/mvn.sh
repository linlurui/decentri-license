#!/bin/bash

# 一键修复脚本 - 设置 Java 环境并运行 Maven

echo "🔧 正在配置 Java 环境..."

# 设置 JAVA_HOME 为 ARM64 Java
export JAVA_HOME=/opt/homebrew/opt/openjdk@17
export PATH="$JAVA_HOME/bin:$PATH"

# 显示当前 Java 版本
echo "✅ 使用 Java 版本:"
java -version 2>&1 | head -1

# 运行 Maven 命令
echo ""
echo "🚀 运行 Maven..."
mvn "$@"
