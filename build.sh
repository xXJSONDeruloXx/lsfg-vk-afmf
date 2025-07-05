#!/bin/bash

# Build script for lsfg-vk-afmf
# This script will try to build the project locally for testing

set -e

echo "=== lsfg-vk-afmf Build Script ==="

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found. Please run this script from the project root."
    exit 1
fi

# Create build directory
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

echo "Creating build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure build
echo "Configuring CMake..."
if command -v ninja >/dev/null 2>&1; then
    CMAKE_GENERATOR="-G Ninja"
    BUILD_COMMAND="ninja -v"
else
    CMAKE_GENERATOR=""
    BUILD_COMMAND="make -j$(nproc 2>/dev/null || echo 4) VERBOSE=1"
fi

cmake .. \
    $CMAKE_GENERATOR \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
echo "Building..."
$BUILD_COMMAND

# Check results
echo "=== Build Results ==="
if [ -f "liblsfg-vk-afmf.so" ]; then
    echo "✅ Build successful!"
    echo "Library: $(pwd)/liblsfg-vk-afmf.so"
    echo "Size: $(du -h liblsfg-vk-afmf.so | cut -f1)"
    echo "Type: $(file liblsfg-vk-afmf.so)"
else
    echo "❌ Build failed - library not found"
    echo "Build artifacts:"
    ls -la
    exit 1
fi

echo "=== Done ==="
