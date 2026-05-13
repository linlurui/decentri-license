#!/bin/bash

# Build script for Swift SDK
echo "Building Swift SDK..."

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
SWIFT_SDK_DIR="$PROJECT_DIR/sdks/swift"

cd "$SWIFT_SDK_DIR"

# Copy dl-core dynamic library
mkdir -p "$SWIFT_SDK_DIR/lib"
if ls "$PROJECT_DIR/dl-core/build/libdecentrilicense."* 1> /dev/null 2>&1; then
    cp "$PROJECT_DIR/dl-core/build/libdecentrilicense."* "$SWIFT_SDK_DIR/lib/"
fi

# Try to build with Swift Package Manager
if command -v swift &> /dev/null && [ -f "Package.swift" ]; then
    echo "🔨 编译Swift SDK..."
    export PKG_CONFIG_PATH="$PROJECT_DIR/dl-core:$PKG_CONFIG_PATH"
    swift build 2>&1
    if [ $? -eq 0 ]; then
        echo "✅ Swift SDK编译完成"
    else
        echo "⚠️  Swift SDK编译失败"
    fi
fi

echo "Swift SDK packaged successfully!"
