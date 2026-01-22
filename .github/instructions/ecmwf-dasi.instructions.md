---
applyTo: '**'
---
# DASI Project Conventions

## Architecture Overview

**DASI** (Data Access and Storage Interface) is a metadata-driven data store built as a semantic layer on top of **FDB** (Fields DataBase). The architecture has three key layers:

1. **Frontends**: Python (`pydasi`), C++, and C APIs for data access
2. **DASI Core**: Translates domain-specific metadata operations into FDB requests
3. **FDB Backend**: Handles actual storage (POSIX, S3, Ceph, NVRAM)

### Core Dependencies

DASI is built on a stack of ECMWF libraries managed via **ecbuild** bundle (see [bundle/CMakeLists.txt](../../bundle/CMakeLists.txt)):

- **eckit** (>= 1.28): Core utilities - configuration parsing (`YAMLConfiguration`), runtime initialization (`Main`), logging, and I/O
- **metkit**: MARS request translation - converts DASI `Query` objects into `MarsRequest` format for FDB
- **fdb5** (>= 5.13): Database engine - handles data indexing, schema rules, and storage backends
- **AWS SDK C++**: Optional dependency for S3 backend support (enabled via `ENABLE_AWSSDK_S3`)
- **libaec**: Optional compression library for AEC support (enabled via `ENABLE_AEC`)

Dependencies are built in order: `eckit` → `metkit` → `fdb` → `dasi` (all currently on `project/dafab` branch)

### Key Components

- **Schema**: Hierarchical 3-level metadata taxonomy defining how data is indexed (e.g., `[User, Project [Date [Type]]]`)
- **Configuration**: YAML files specify schema path, storage backend (`file`, `s3`), and data roots
- **Key/Query**: Metadata dictionaries for archiving and retrieving data
- **Generators**: Iterator-based APIs for list/retrieve/wipe operations (see [src/api/Dasi.cc](../../src/api/Dasi.cc))

Data flow: `archive(Key, data)` → FDB indexing → backend storage. Retrieval uses `Query` objects that match against indexed metadata.

## Build System

This is a **CMake + ecbuild** project with a **bundle structure** for managing dependencies.

### Bundle Build (Recommended for Development)

The `bundle/` directory is a **superbuild** that clones and builds all dependencies in the correct order:

```bash
# In devcontainer or with GCC toolset 14
source /opt/rh/gcc-toolset-14/enable  # if on Rocky Linux
cmake -S bundle -B /tmp/build/dasi-bundle -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/opt/ecmwf
cmake --build /tmp/build/dasi-bundle --target test -- --output-on-failure
cmake --build /tmp/build/dasi-bundle --target install pydasi_package
```

**Build process**: ecbuild automatically clones dependencies from GitHub, builds them in dependency order, and installs to a common prefix.

**Platform-specific settings** in [bundle/Linux.cmake](../../bundle/Linux.cmake):
- `ENABLE_AWSSDK_S3=ON` / `ENABLE_S3FDB=ON`: S3 backend support
- `ENABLE_AEC=ON`: AEC compression
- `ENABLE_POINTDB=ON`: Point data indexing
- `BUILD_PYTHON=ON` / `BUILD_EXAMPLES=ON` / `BUILD_TESTING=ON`: DASI features

### Standalone Build

If dependencies are already installed:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/deps
cmake --build build
ctest --test-dir build --output-on-failure
```

### CMake Options

- `BUILD_PYTHON=ON`: Build pydasi Python package (requires CFFI)
- `BUILD_EXAMPLES=ON`: Build examples in `examples/`
- `BUILD_TESTING=ON`: Build C++ tests

### Available Tasks

Use VS Code CMake tasks (see [.devcontainer/devcontainer.json](../../.devcontainer/devcontainer.json)):
- `cmake: CMake: configure` - Configure with active preset
- `cmake: CMake: build` - Build active target (uses Ninja)
- `cmake: CMake: clean rebuild` - Clean and rebuild

## Testing

### C++ Tests
```bash
# Run all tests
ctest --test-dir /tmp/build/dasi-bundle --output-on-failure

# Run specific test
/tmp/build/dasi-bundle/bin/test_c_api_simple_archive
```

Tests use eckit's test framework. See [tests/c_api/](../../tests/c_api/) for patterns.

### Python Tests
```bash
cd pydasi
python -m pytest tests/
```

Python tests use pytest with fixtures for temporary DASI configurations (see [pydasi/tests/test_dasi_archive_retrieve.py](../../pydasi/tests/test_dasi_archive_retrieve.py)).

## Development Workflows

### Docker/Devcontainer

The project uses a multi-stage Dockerfile:
- `build-dependencies`: Base with compilers, ecbuild, AWS SDK, libaec
- `dev-env`: Development tools (gdb, valgrind, clang-tools, pytest)
- `dasi-builder`: Builds DASI bundle and pydasi wheel
- `dasi-runtime`: Minimal runtime image

Devcontainer workspace is `/workspace/dasi/bundle` with build directory at `/tmp/build/dasi-bundle`.

### Configuration Files

All examples require a schema file and YAML config. Template structure:

```yaml
type: local
engine: toc
schema: ./schema
store: file  # or s3
spaces:
  - roots:
    - path: ./database
```

For S3 backends, add S3 configuration (see [examples/simple_s3/dasi.yaml](../../examples/simple_s3/dasi.yaml)).

### CLI Tools

Located in `bin/` after build:
- `dasi-list`: Query and list archived data
- `dasi-get`/`dasi-put`: Retrieve/archive data via CLI
- `dasi-wipe`: Remove data matching query
- `dasi-schema`: Dump or validate schema
- `dasi-info`: Show version and configuration

## Project-Specific Patterns

### API Conventions

**C++ API** ([include/dasi/api/Dasi.h](../../include/dasi/api/Dasi.h)):
```cpp
dasi::Dasi dasi("config.yaml");
dasi::Key key{{"User", "alice"}, {"Date", "20240101"}};
dasi.archive(key, data_ptr, data_len);
dasi.flush();  // Ensure persistence

auto results = dasi.retrieve(dasi::Query{{"User", "alice"}});
```

**Python API** ([pydasi/src/pydasi/dasi.py](../../pydasi/src/pydasi/dasi.py)):
```python
from dasi import Dasi
dasi = Dasi("config.yaml")
dasi.archive({"User": "alice", "Date": "20240101"}, data)
results = dasi.retrieve({"User": ["alice", "bob"]})  # Multi-value query
```

### Schema Syntax

- 3 levels: `[directory_keys [file_keys [index_keys]]]`
- Optional keys: append `?` (e.g., `Laboratory?`)
- Example: `[User, Project [DateTime, Processing [Type, Object]]]`

### CFFI Integration (pydasi)

Python wraps C API via CFFI ([pydasi/src/backend/](../../pydasi/src/backend/)):
- `_dasi_cffi.py`: C declarations and wrapper functions
- Error handling: Raises `DASIException` on C library errors
- Memory management: Uses FFI context managers for C pointers

### Dependency Integration

Key imports in [src/api/Dasi.cc](../../src/api/Dasi.cc) show how dependencies are used:

```cpp
#include "eckit/config/YAMLConfiguration.h"   // Config parsing
#include "eckit/runtime/Main.h"              // Application init
#include "fdb5/api/FDB.h"                    // Storage backend
#include "fdb5/rules/Schema.h"               // Metadata schema
#include "metkit/mars/MarsRequest.h"         // Query translation
```

**DasiImpl wraps fdb5::FDB** - all archive/retrieve operations delegate to FDB:

```cpp
class DasiImpl {
    fdb5::FDB fdb_;  // The real storage engine
    
    void archive(const Key& key, const void* data, size_t length) {
        fdb5::Key fdb_key;
        for (const auto& kv : key) { fdb_key.set(kv.first, kv.second); }
        fdb_.archive(fdb_key, data, length);  // Delegate to FDB
    }
};
```

**Version requirements**: eckit >= 1.28, fdb5 >= 5.13 (see [CMakeLists.txt](../../CMakeLists.txt))

## Common Issues

1. **Schema not found**: Ensure `schema:` path in config is absolute or relative to CWD
2. **S3 connection errors**: Check `S3Config.yaml` endpoint/credentials and network access
3. **CFFI import failures**: Verify `libdasi.so` is in `LD_LIBRARY_PATH` or system library path
4. **Bundle build failures**: Use matching branches for dependencies (currently `project/dafab`)

## File Organization

- `bundle/`: Superbuild for all dependencies
- `src/api/`: Main C++ API implementation
- `src/tools/`: CLI utilities
- `include/dasi/api/`: Public C++/C headers
- `pydasi/`: Python package with CFFI backend
- `examples/`: Domain-specific examples (weather, cryoem, histogram, S3)
- `tests/`: C++ unit tests; `pydasi/tests/`: Python unit tests
