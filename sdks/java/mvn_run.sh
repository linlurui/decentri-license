#!/bin/bash

# Maven wrapper script that ensures correct Java version is used
# This script sets JAVA_HOME to ARM64 Java before running Maven

# Set JAVA_HOME to ARM64 Java
export JAVA_HOME=/opt/homebrew/opt/openjdk@17

# Run Maven with all passed arguments
mvn "$@"
