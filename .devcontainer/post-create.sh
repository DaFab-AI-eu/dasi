#!/bin/bash
set -e

echo "Setting up DASI development environment..."

BUILD_DIR=/workspace/build

# Ensure clean build directory
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# Build DASI and dependencies via the bundle
cmake -S /workspace/bundle -B "$BUILD_DIR" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH="/usr/local;/usr/local/lib64/cmake" \
    -DAWSSDK_ROOT=/usr/local \
    -DAWSSDK_DIR=/usr/local/lib64/cmake/AWSSDK \
    -DCMAKE_INSTALL_PREFIX=/opt/ecmwf \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build "$BUILD_DIR" --parallel
sudo cmake --install "$BUILD_DIR"

# Install pydasi in editable mode
cmake --build "$BUILD_DIR" --target pydasi_develop