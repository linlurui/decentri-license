#!/bin/bash

# Build script for C# binding
echo "Building C# binding..."

# Navigate to script directory (relative to script location)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
CSHARP_SDK_DIR="$PROJECT_DIR/sdks/csharp"

cd "$CSHARP_SDK_DIR"

# Do not overwrite existing C# SDK sources.
if command -v dotnet &> /dev/null; then
    if ls *.csproj 1> /dev/null 2>&1; then
        dotnet restore
        dotnet build
    else
        echo "Warning: no .csproj found in sdks/csharp. Skipping C# build."
    fi
else
    echo "Warning: dotnet not found. Skipping C# build."
fi

# Copy dl-core dynamic library for runtime loading (keep example paths unchanged)
mkdir -p "$CSHARP_SDK_DIR/lib"
if ls "$PROJECT_DIR/dl-core/build/libdecentrilicense."* 1> /dev/null 2>&1; then
    cp "$PROJECT_DIR/dl-core/build/libdecentrilicense."* "$CSHARP_SDK_DIR/lib/"
fi

echo "C# binding packaged successfully!"