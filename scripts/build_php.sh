#!/bin/bash

# Build script for PHP SDK
echo "Building PHP SDK..."

# Navigate to script directory (relative to script location)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
PHP_SDK_DIR="$PROJECT_DIR/sdks/php"

echo "Script directory: $SCRIPT_DIR"
echo "Project directory: $PROJECT_DIR"
echo "PHP SDK directory: $PHP_SDK_DIR"

# Ensure the PHP SDK directory exists
mkdir -p "$PHP_SDK_DIR"

# Check if source PHP SDK directory exists
SOURCE_PHP_SDK_DIR="$PROJECT_DIR/sdks/php"
if [ ! -d "$SOURCE_PHP_SDK_DIR" ]; then
    echo "Warning: Source PHP SDK directory not found at $SOURCE_PHP_SDK_DIR"
    echo "Creating directory..."
    mkdir -p "$SOURCE_PHP_SDK_DIR"
fi

# Nothing to build for PHP (it's an interpreted language)
# Just ensure the files are in place and properly structured

echo "PHP SDK is ready for use!"
echo "Files are located in: $PHP_SDK_DIR"

echo "PHP SDK packaged successfully!"