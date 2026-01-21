#!/bin/bash

# Package script for C++ SDK (assumes dl-core has already been built)
echo "Packaging C++ SDK..."

# Navigate to script directory (relative to script location)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
DL_CORE_DIR="$PROJECT_DIR/dl-core"

echo "Script directory: $SCRIPT_DIR"
echo "Project directory: $PROJECT_DIR"
echo "DL Core directory: $DL_CORE_DIR"

BUILD_DIR="$DL_CORE_DIR/build"

# Create SDK directory structure
mkdir -p "$PROJECT_DIR/sdks/cpp/lib"
mkdir -p "$PROJECT_DIR/sdks/cpp/include"

# Copy built libraries (if they exist)
if ls "$BUILD_DIR"/libdecentrilicense.* 1> /dev/null 2>&1; then
    cp "$BUILD_DIR"/libdecentrilicense.* "$PROJECT_DIR/sdks/cpp/lib/"
else
    echo "Warning: No libdecentrilicense.* files found in $BUILD_DIR"
fi

# Copy header files
cp -r "$DL_CORE_DIR/include"/* "$PROJECT_DIR/sdks/cpp/include/"

# Copy third-party ASIO headers required by public headers
if [ -d "$DL_CORE_DIR/third-party/asio" ]; then
    cp -r "$DL_CORE_DIR/third-party/asio/asio" "$PROJECT_DIR/sdks/cpp/include/" 2>/dev/null || true
    cp -r "$DL_CORE_DIR/third-party/asio/asio.hpp" "$PROJECT_DIR/sdks/cpp/include/" 2>/dev/null || true
fi

echo "C++ SDK packaged successfully!"