#!/bin/bash

echo "=== DecentriLicense SDK Build Script ==="

# Exit on any error
set -e

# Store the original directory
ORIG_DIR=$(pwd)

echo "1. Creating build directory..."
mkdir -p build
cd build

echo "2. Building shared library..."
g++ -std=c++17 -fPIC -shared \
    -I../dl-core/include \
    -Ithird-party/asio \
    ../dl-core/src/crypto_utils.cpp \
    ../dl-core/src/decentrilicense_client.cpp \
    ../dl-core/src/election_manager.cpp \
    ../dl-core/src/network_manager.cpp \
    ../dl-core/src/token_manager.cpp \
    -lssl -lcrypto \
    -o libdecentrilicense.so

echo "3. Building demo executable..."
g++ -std=c++17 \
    -I../dl-core/include \
    -Ithird-party/asio \
    ../sdk/c++/demo/demo.cpp \
    -L. -ldecentrilicense -lssl -lcrypto \
    -o demo

echo "4. Building token manager example executable..."
g++ -std=c++17 \
    -I../dl-core/include \
    -Ithird-party/asio \
    ../sdk/c++/token_manager_example.cpp \
    -L. -ldecentrilicense -lssl -lcrypto \
    -o token_manager_example

echo "5. Copying library to sdk/c++ directory..."
cp libdecentrilicense.so ../sdk/c++

echo "✅ SDK build completed successfully!"
echo "Library: build/libdecentrilicense.so"
echo "Demo executable: build/demo"
echo "Token manager example: build/token_manager_example"
echo "Library also copied to: sdk/c++/libdecentrilicense.so"