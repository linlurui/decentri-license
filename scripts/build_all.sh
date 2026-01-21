#!/bin/bash

# Build script for all language bindings
echo "Building all language bindings..."

# Make all build scripts executable
chmod +x *.sh

# Build each language binding
echo "Building C binding..."
./build_c.sh

echo "Building Go binding..."
./build_go.sh

echo "Building Python binding..."
./build_python.sh

echo "Building Java binding..."
./build_java.sh

echo "Building Node.js binding..."
./build_nodejs.sh

echo "Building Rust binding..."
./build_rust.sh

echo "Building C# binding..."
./build_csharp.sh

echo "All language bindings packaged successfully!"

# Show the directory structure
echo "Final directory structure:"
ls -la