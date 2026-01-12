#!/bin/bash
set -e

echo "Setting up DASI development environment..."

# Fix ownership of build volume (mounted as root)
sudo chown -R developer:developer /workspace/build

# Configure
cmake -S /workspace -B /workspace/build \
    -DCMAKE_INSTALL_PREFIX=/opt/ecmwf \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_TESTING=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build /workspace/build --parallel $(nproc)
sudo cmake --install /workspace/build

mkdir -p /workspace/pydasi/src/backend/libs/Linux
cp /opt/ecmwf/lib/libdasi.so* /workspace/pydasi/src/backend/libs/Linux/
pip install -e "/workspace/pydasi[tests]"