#!/bin/bash

# Build script for C binding
echo "Building C binding..."

# Navigate to script directory (relative to script location)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
C_SDK_DIR="$PROJECT_DIR/sdks/c"

# Work directly in the c directory instead of c-dist
mkdir -p "$C_SDK_DIR/lib" "$C_SDK_DIR/include"

# Copy header files from sdks/c directory
cp "$C_SDK_DIR/decenlicense_c.h" "$C_SDK_DIR/include/"

# Copy dl-core dynamic library for runtime loading (keep example paths unchanged)
if ls "$PROJECT_DIR/dl-core/build/libdecentrilicense."* 1> /dev/null 2>&1; then
    cp "$PROJECT_DIR/dl-core/build/libdecentrilicense."* "$C_SDK_DIR/lib/"
fi

echo "C binding packaged successfully!"