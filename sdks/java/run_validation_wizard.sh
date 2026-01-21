#!/bin/bash

# DecentriLicense Java SDK - Validation Wizard Runner
# This script runs the validation wizard with proper library path

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Set paths
LIB_DIR="$SCRIPT_DIR/lib"
JAR_FILE="$SCRIPT_DIR/target/decenlicense-1.0.0.jar"

# Check if JAR exists
if [ ! -f "$JAR_FILE" ]; then
    echo "❌ JAR file not found: $JAR_FILE"
    echo "💡 Please run: mvn package"
    exit 1
fi

# Run with proper library path
# Use ARM64 native Java if available, otherwise fall back to system Java
if [ -f "/opt/homebrew/opt/openjdk@17/bin/java" ]; then
    JAVA_CMD="/opt/homebrew/opt/openjdk@17/bin/java"
else
    JAVA_CMD="java"
fi

$JAVA_CMD -Djava.library.path="$LIB_DIR" \
     -cp "$JAR_FILE:$HOME/.m2/repository/com/google/code/gson/gson/2.10.1/gson-2.10.1.jar" \
     com.decentrilicense.ValidationWizard
