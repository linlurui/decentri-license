#!/bin/bash

# Build script for Go binding
echo "Building Go binding..."

# Navigate to script directory (relative to script location)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
GO_SDK_DIR="$PROJECT_DIR/sdks/go"

# Work directly in the go directory instead of go-dist
mkdir -p "$GO_SDK_DIR"

# Remove unnecessary files from the main directory
rm -f "$GO_SDK_DIR/go.mod" "$GO_SDK_DIR/go.sum"

# Create a simple installation script
cat > "$GO_SDK_DIR/install.sh" << 'EOF'
#!/bin/bash

# Installation script for DecentriLicense Go binding
echo "Installing DecentriLicense Go binding..."

# Create module if it doesn't exist
if [ ! -f "go.mod" ]; then
    go mod init myproject
fi

# Add the DecentriLicense dependency
echo "Adding DecentriLicense Go binding to your project..."
echo "Please manually copy the decenlicense directory to your project and import it as:"
echo "import \"path/to/decenlicense\""

echo "Installation instructions:"
echo "1. Copy the 'decenlicense' directory to your project"
echo "2. Import in your Go code: import \"./decenlicense\""
echo "3. Use the API as described in the documentation"
EOF

chmod +x "$GO_SDK_DIR/install.sh"

echo "Go binding packaged successfully!"