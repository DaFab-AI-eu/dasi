#!/usr/bin/env bash
set -euo pipefail

source /opt/rh/gcc-toolset-14/enable
source_dir="${DASI_SOURCE_DIR:-/workspace/dasi}"
bundle_dir="${BUNDLE_SOURCE_DIR:-/workspace/bundle}"
build_dir="${DASI_BUILD_DIR:-/tmp/build/dasi-bundle}"
mkdir -p "$bundle_dir" "$build_dir" "$CCACHE_DIR"
cp "$source_dir/bundle/CMakeLists.txt" "$source_dir/bundle/Linux.cmake" \
    "$source_dir/bundle/Dependencies.cmake" "$bundle_dir/"

if [[ ! -x "$source_dir/.venv/bin/python" ]]; then
    python3 -m venv "$source_dir/.venv"
fi
export VIRTUAL_ENV="$source_dir/.venv"
export PATH="$VIRTUAL_ENV/bin:$PATH"

cmake -S "$bundle_dir" -B "$build_dir" -G Ninja \
    -DDASI_SOURCE_DIR="$source_dir" \
    -DCMAKE_BUILD_TYPE="${DASI_BUILD_TYPE:-Debug}" \
    -DCMAKE_INSTALL_PREFIX="${DASI_INSTALL_PREFIX:-/workspace/install}" \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DENABLE_TESTS=ON -DBUILD_TESTING=ON -DBUILD_PYTHON=ON