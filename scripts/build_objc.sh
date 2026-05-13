#!/bin/bash

# Build script for Objective-C SDK
echo "Building Objective-C SDK..."

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
OBJC_SDK_DIR="$PROJECT_DIR/sdks/objc"

cd "$OBJC_SDK_DIR"

# Copy dl-core dynamic library and header
mkdir -p "$OBJC_SDK_DIR/lib"
if ls "$PROJECT_DIR/dl-core/build/libdecentrilicense."* 1> /dev/null 2>&1; then
    cp "$PROJECT_DIR/dl-core/build/libdecentrilicense."* "$OBJC_SDK_DIR/lib/"
fi

# Try to compile validation wizard
if command -v clang &> /dev/null; then
    echo "🔨 编译Objective-C验证向导..."
    clang -framework Foundation \
          -I"$PROJECT_DIR/dl-core/include" \
          -L"$PROJECT_DIR/dl-core/build" \
          -ldecentrilicense \
          -Wl,-rpath,"$PROJECT_DIR/dl-core/build" \
          -o validation_wizard \
          validation_wizard.m DecentriLicenseClient.m 2>&1
    if [ $? -eq 0 ]; then
        echo "✅ Objective-C验证向导编译完成"
    else
        echo "⚠️  Objective-C验证向导编译失败"
    fi
fi

echo "Objective-C SDK packaged successfully!"
