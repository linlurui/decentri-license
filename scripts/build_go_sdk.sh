#!/bin/bash

# Build script for Go SDK
echo "Building Go SDK..."

# Navigate to script directory (relative to script location)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
GO_SDK_DIR="$PROJECT_DIR/sdks/go"

echo "Script directory: $SCRIPT_DIR"
echo "Project directory: $PROJECT_DIR"
echo "Go SDK directory: $GO_SDK_DIR"

cd "$GO_SDK_DIR"

# Check if go is installed
if ! command -v go &> /dev/null
then
    echo "Error: go could not be found. Please install Go to build the Go SDK."
    exit 1
fi

# Run go mod tidy to ensure dependencies are up to date
go mod tidy

# Build the module
go build ./...

# Note: Go SDK files are already in the correct directory structure
# No need to copy files as we're building in-place

echo "Go SDK built and packaged successfully!"