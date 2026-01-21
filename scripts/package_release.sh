#!/bin/bash

# Package release script for DecentriLicense
echo "Packaging DecentriLicense release..."

# Create release directory
RELEASE_VERSION="1.0.0"
RELEASE_NAME="decentrilicense-sdk-$RELEASE_VERSION"
mkdir -p releases/$RELEASE_NAME

# Copy all language bindings
cp -r c go python java nodejs rust csharp releases/$RELEASE_NAME/

# Copy main README
cp README.md releases/$RELEASE_NAME/

# Create a release manifest
cat > releases/$RELEASE_NAME/manifest.txt << EOF
DecentriLicense SDK Release v$RELEASE_VERSION
=====================================

This release contains pre-built bindings for multiple languages:

- C (core library)
- Go
- Python
- Java
- Node.js
- Rust
- C#

Each directory contains:
- Source code or pre-built libraries
- Installation instructions
- Example usage code

For documentation, visit: https://github.com/decentrilicense/decentri-license
EOF

# Create ZIP archive
cd releases
zip -r $RELEASE_NAME.zip $RELEASE_NAME

echo "Release packaged successfully!"
echo "Release file: releases/$RELEASE_NAME.zip"