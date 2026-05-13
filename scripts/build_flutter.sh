#!/bin/bash

# Build script for Flutter/Dart SDK
echo "Building Flutter/Dart SDK..."

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
FLUTTER_SDK_DIR="$PROJECT_DIR/sdks/flutter"

cd "$FLUTTER_SDK_DIR"

# Dart pub get (if dart is available)
if command -v dart &> /dev/null && [ -f "pubspec.yaml" ]; then
    dart pub get 2>/dev/null || true
fi

# Copy dl-core dynamic library
mkdir -p "$FLUTTER_SDK_DIR/lib/native"
if ls "$PROJECT_DIR/dl-core/build/libdecentrilicense."* 1> /dev/null 2>&1; then
    cp "$PROJECT_DIR/dl-core/build/libdecentrilicense."* "$FLUTTER_SDK_DIR/lib/native/"
fi

echo "Flutter/Dart SDK packaged successfully!"
