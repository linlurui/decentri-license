#!/bin/bash

# Build script for DecentriLicense Java binding
echo "Building DecentriLicense Java binding..."

# Compile Java sources
mkdir -p target/classes
javac -d target/classes \
  src/main/java/com/decentrilicense/ActivationResult.java \
  src/main/java/com/decentrilicense/DecentriLicenseClient.java \
  src/main/java/com/decentrilicense/DeviceState.java \
  src/main/java/com/decentrilicense/Example.java \
  src/main/java/com/decentrilicense/LicenseException.java \
  src/main/java/com/decentrilicense/StatusResult.java \
  src/main/java/com/decentrilicense/Token.java \
  src/main/java/com/decentrilicense/ValidationWizard.java \
  src/main/java/com/decentrilicense/VerificationResult.java

# Compile smoke test (no Maven/JUnit required)
mkdir -p target/test-classes
javac -cp target/classes -d target/test-classes tests/ApiImportSmokeTest.java

# Create JAR file
jar cf target/decenlicense-1.0.0.jar -C target/classes .

echo "Build completed! JAR file created at target/decenlicense-1.0.0.jar"
