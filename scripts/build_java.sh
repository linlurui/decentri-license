#!/bin/bash

# Build script for Java binding
echo "Building Java binding..."

# Navigate to script directory (relative to script location)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
JAVA_SDK_DIR="$PROJECT_DIR/sdks/java"

cd "$JAVA_SDK_DIR"

# Do not overwrite existing Java SDK sources.
# Build only if build.sh exists.
if [ -f "build.sh" ]; then
    chmod +x build.sh
    ./build.sh
else
    echo "Warning: build.sh not found in sdks/java. Skipping Java build."
fi

# Copy dl-core dynamic library for runtime loading (keep example paths unchanged)
mkdir -p "$JAVA_SDK_DIR/lib"
if ls "$PROJECT_DIR/dl-core/build/libdecentrilicense."* 1> /dev/null 2>&1; then
    cp "$PROJECT_DIR/dl-core/build/libdecentrilicense."* "$JAVA_SDK_DIR/lib/"
fi

echo "Java binding packaged successfully!"