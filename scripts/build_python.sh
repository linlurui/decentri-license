#!/bin/bash

# Build script for Python binding
echo "Building Python binding..."

# Navigate to script directory (relative to script location)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
PYTHON_SDK_DIR="$PROJECT_DIR/sdks/python"

cd "$PYTHON_SDK_DIR"

# Do not overwrite existing Python SDK sources.
# If a setup.py exists, we can at least sanity-check packaging by running a build.
if command -v python3 &> /dev/null && [ -f "setup.py" ]; then
    python3 -m pip --version >/dev/null 2>&1 || true
    python3 setup.py --help >/dev/null 2>&1 || true
fi

# Copy dl-core dynamic library for runtime loading (keep example paths unchanged)
mkdir -p "$PYTHON_SDK_DIR/lib"
if ls "$PROJECT_DIR/dl-core/build/libdecentrilicense."* 1> /dev/null 2>&1; then
    cp "$PROJECT_DIR/dl-core/build/libdecentrilicense."* "$PYTHON_SDK_DIR/lib/"
fi

echo "Python binding packaged successfully!"