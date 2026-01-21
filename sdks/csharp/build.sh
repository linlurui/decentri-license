#!/bin/bash

# Build script for DecentriLicense C# binding
echo "Building DecentriLicense C# binding..."

# Restore NuGet packages
dotnet restore

# Build the project
dotnet build

# Create NuGet package
dotnet pack --configuration Release

echo "Build completed! NuGet package created in bin/Release/"
