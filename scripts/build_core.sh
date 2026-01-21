#!/bin/bash

set -euo pipefail

echo "Building dl-core (DecentriLicense) via CMake..."

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
DL_CORE_DIR="$PROJECT_DIR/dl-core"
BUILD_DIR="$DL_CORE_DIR/build"

if ! command -v cmake &> /dev/null; then
    echo "Error: cmake could not be found. Please install cmake."
    exit 1
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Configuring..."
cmake ..

echo "Building..."
cmake --build .

echo "Running tests (ctest)..."
ctest --output-on-failure || {
    echo "Error: dl-core tests failed."
    exit 1
}

echo "Build completed!"
echo "Artifacts in: $BUILD_DIR"
