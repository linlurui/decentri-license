#!/bin/bash

# Build script for Rust binding
echo "Building Rust binding..."

# Navigate to script directory (relative to script location)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
RUST_SDK_DIR="$PROJECT_DIR/sdks/rust"

cd "$RUST_SDK_DIR"

# Do not overwrite existing Rust SDK sources.
# Build only if Cargo.toml exists.
if command -v cargo &> /dev/null && [ -f "Cargo.toml" ]; then
    cargo build
else
    echo "Warning: cargo not found or Cargo.toml missing. Skipping Rust build."
fi

# Copy dl-core dynamic library for runtime loading (keep example paths unchanged)
mkdir -p "$RUST_SDK_DIR/lib"
if ls "$PROJECT_DIR/dl-core/build/libdecentrilicense."* 1> /dev/null 2>&1; then
    cp "$PROJECT_DIR/dl-core/build/libdecentrilicense."* "$RUST_SDK_DIR/lib/"
fi

echo "Rust binding packaged successfully!"