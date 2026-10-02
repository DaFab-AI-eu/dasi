#!/usr/bin/env bash
set -euo pipefail

source /opt/rh/gcc-toolset-14/enable
bundle_dir="${BUNDLE_DIR:-/workspace/bundle}"
source_dir="$bundle_dir/dasi"
build_dir="${DASI_BUILD_DIR:-/tmp/build/dasi-bundle}"
mkdir -p "$build_dir" "$CCACHE_DIR"
for file in CMakeLists.txt Linux.cmake Dependencies.cmake; do
    ln -sfn "dasi/bundle/$file" "$bundle_dir/$file"
done

# eckit's Python bindings configure against this venv and need the image's Cython.
venv="$source_dir/.venv"
if ! grep -qs '^include-system-site-packages = true' "$venv/pyvenv.cfg" ||
    ! grep -qsF "$venv" "$venv/pyvenv.cfg"; then
    python3 -m venv --clear --system-site-packages "$venv"
fi
export VIRTUAL_ENV="$venv"
export PATH="$VIRTUAL_ENV/bin:$PATH"

cmake -S "$bundle_dir" -B "$build_dir" -G Ninja \
    -DCMAKE_BUILD_TYPE="${DASI_BUILD_TYPE:-Debug}" \
    -DCMAKE_INSTALL_PREFIX="${DASI_INSTALL_PREFIX:-/workspace/install}" \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DENABLE_TESTS=ON -DBUILD_TESTING=ON -DBUILD_PYTHON=ON
