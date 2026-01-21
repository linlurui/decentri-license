#!/bin/bash

# Build script for Node.js binding
echo "Building Node.js binding..."

# Navigate to script directory (relative to script location)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
NODEJS_SDK_DIR="$PROJECT_DIR/sdks/nodejs"

cd "$NODEJS_SDK_DIR"

# Build native addon in-place
if ! command -v node &> /dev/null; then
    echo "Warning: node could not be found. Skipping Node.js build."
    exit 0
fi

if ! command -v npm &> /dev/null; then
    echo "Warning: npm could not be found. Skipping Node.js build."
    exit 0
fi

if ! command -v node-gyp &> /dev/null; then
    echo "Warning: node-gyp could not be found. Skipping Node.js native addon build."
    exit 0
fi

npm install
node-gyp configure
node-gyp build

# Copy dl-core dynamic library for runtime loading (keep example paths unchanged)
mkdir -p "$NODEJS_SDK_DIR/lib"
if ls "$PROJECT_DIR/dl-core/build/libdecentrilicense."* 1> /dev/null 2>&1; then
    cp "$PROJECT_DIR/dl-core/build/libdecentrilicense."* "$NODEJS_SDK_DIR/lib/"
fi

echo "Node.js binding built and packaged successfully!"