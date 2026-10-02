#!/usr/bin/env bash
set -euo pipefail

source /opt/rh/gcc-toolset-14/enable
source_dir="${BUNDLE_DIR:-/workspace/bundle}/dasi"
# Separate from the Debug dev tree so the two never reconfigure each other.
export DASI_BUILD_DIR="${DASI_BUILD_DIR:-/tmp/build/dasi-release}"
export DASI_BUILD_TYPE="${DASI_BUILD_TYPE:-Release}"
export DASI_INSTALL_PREFIX="${DASI_INSTALL_PREFIX:-/workspace/install}"
build_dir="$DASI_BUILD_DIR"
bash "$source_dir/scripts/configure.sh"

artifact_dir="$source_dir/.artifacts"
mkdir -p "$artifact_dir"
cmake --build "$build_dir" --parallel "${BUILD_JOBS:-2}" --target all pydasi_develop
# test_fdb5_s3_store: upstream FDB dry-run wipe bug (test_store.cc "VIA FDB API"); re-enable once fixed.
ctest --test-dir "$build_dir" --output-on-failure --no-tests=error \
    --parallel "${TEST_JOBS:-2}" -E 'fdb_move_auxiliary\.sh|test_fdb5_s3_store' \
    --output-junit "$artifact_dir/ctest.xml"
export PYTEST_ADDOPTS="--junitxml=$artifact_dir/pytest.xml"
cmake --build "$build_dir" --target pydasi_test

cmake -E remove_directory "$source_dir/pydasi/dist"
cmake --build "$build_dir" --target install pydasi_package
cmake -E remove_directory "$artifact_dir/install"
cmake -E remove_directory "$artifact_dir/wheels"
mkdir -p "$artifact_dir/install/lib64" "$artifact_dir/wheels"
cp -a "$DASI_INSTALL_PREFIX/." "$artifact_dir/install/"
cp "$source_dir/pydasi/dist/"pydasi-*.whl "$artifact_dir/wheels/"
shopt -s nullglob
for library_dir in /usr/local/lib /usr/local/lib64; do
    for library in "$library_dir/"*.so*; do
        cp -a "$library" "$artifact_dir/install/lib64/"
    done
done
cp -L "$(g++ -print-file-name=libstdc++.so.6)" "$artifact_dir/install/lib64/libstdc++.so.6"
ccache --show-stats
