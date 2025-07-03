#!/bin/bash
#################################################################################
# build.sh - Sample build script for Mahogany using CMake
#################################################################################
# This script demonstrates how to build Mahogany with various configurations.
# Modify the variables below to suit your environment.

set -e  # Exit on error

# Configuration variables
BUILD_TYPE=${BUILD_TYPE:-Release}
BUILD_SHARED=${BUILD_SHARED:-OFF}
ENABLE_PYTHON=${ENABLE_PYTHON:-ON}
ENABLE_SSL=${ENABLE_SSL:-ON}
BUILD_TESTS=${BUILD_TESTS:-ON}
INSTALL_PREFIX=${INSTALL_PREFIX:-/usr/local}

# Build directory
BUILD_DIR=build

echo "=== Mahogany CMake Build Script ==="
echo "Build type: $BUILD_TYPE"
echo "Shared libs: $BUILD_SHARED" 
echo "Python support: $ENABLE_PYTHON"
echo "SSL support: $ENABLE_SSL"
echo "Build tests: $BUILD_TESTS"
echo "Install prefix: $INSTALL_PREFIX"
echo ""

# Clean build directory if requested
if [ "$1" = "clean" ]; then
    echo "Cleaning build directory..."
    rm -rf $BUILD_DIR
    echo "Clean complete."
    exit 0
fi

# Create build directory
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Configure with CMake
echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DBUILD_SHARED_LIBS=$BUILD_SHARED \
    -DENABLE_PYTHON=$ENABLE_PYTHON \
    -DENABLE_SSL=$ENABLE_SSL \
    -DBUILD_TESTS=$BUILD_TESTS \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX

# Build
echo "Building..."
make -j$(nproc)

# Run tests if enabled
if [ "$BUILD_TESTS" = "ON" ]; then
    echo "Running tests..."
    ctest --output-on-failure
fi

echo ""
echo "=== Build Complete ==="
echo "To install: cd $BUILD_DIR && make install"
echo "To package: cd $BUILD_DIR && make package"