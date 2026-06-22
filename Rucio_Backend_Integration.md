# Rucio Backend to PyDASI

## Introduction

The integration enables scientific data archiving and retrieval through Rucio's distributed storage infrastructure while maintaining API compatibility with the existing DASI backend. This deliverable describes the architectural design, implementation approach, and operational characteristics of the Rucio backend integration.

## 1. Architecture

Dafab's DASI layer implements a **pluggable backend architecture** through factory interfaces, enabling seamless integration of multiple storage systems. The Rucio backend follows this design pattern alongside the existing DASI backend:

```
PyDASI Application Layer (Python API)
    ├─→ DASI Core (C++ via FFI) → DASI/FDB Storage
    └─→ Rucio Backend (Pure Python) → Rucio Clients
```

### 1.1 Backend Abstraction

Both backends expose identical high-level operations (`archive()`, `list()`, `retrieve()`, `wipe()`), allowing consumers to swap implementations through configuration changes. This architecture enables portability across scientific computing environments and data federation scenarios.

### 1.2 Rucio Backend Components

The Rucio backend is implemented as a pure Python module (`pydasi.rucio`) comprising:

- **`Rucio` class**: Main factory interface accepting YAML configuration
- **`RucioList/RucioListItem`**: Metadata catalogue listing with lazy iteration
- **`RucioRetrieve/RucioRetrieveItem`**: File download with in-memory data access
- **Archive operations**: Multi-step registration and metadata attachment

### 1.3 S3 Protocol Module (`rucio_s3boto3.py`)

The integration also relies on `pydasi/src/rucio_s3boto3.py`, a custom Rucio RSE protocol implementation (`rucio_s3boto3.Default`) for S3-compatible storage such as MinIO. This module is intentionally kept outside the `pydasi` package so it can be loaded by `rucio-server` for deterministic PFN construction (`lfns2pfns`) without requiring the full PyDASI client stack. In practice, this component provides the transfer primitives (`connect`, `get`, `put`, `exists`, `stat`, `rename`, `delete`) used when the Rucio backend is configured with `protocol: s3`.

---

## 2. Operational Workflow

### 2.1 Archive Operation

Data archiving follows a four-step process:

1. **File generation**: Key-value pairs (e.g., `{"experiment": "exp1", "run": "001"}`) are converted to a sanitized filename (`experiment-exp1_run-001.data`)
2. **Temporary storage**: Data is written to a temporary file
3. **Rucio upload**: The `UploadClient` transfers data to the designated Replica Storage Element (RSE) and registers a Distributed Identifier (DID)
4. **Metadata indexing**: Each key-value pair is attached as a searchable attribute via `client.set_metadata()`

### 2.2 Retrieve Operation

Retrieval implements lazy-evaluated iteration:

1. **Catalogue query**: The Rucio client searches for DIDs matching metadata filters
2. **Filtered download**: For each matched DID, `DownloadClient` retrieves data from the RSE
3. **Data streaming**: Files are read into memory and yielded as `RucioRetrieveItem` objects containing data, metadata, and URIs

### 2.3 List Operation

Lightweight listing returns metadata without data transfer:

1. **Catalogue search**: Query metadata attributes in Rucio's catalogue
2. **Metadata resolution**: Fetch attributes for each matching DID
3. **Result streaming**: Yield DID information with URI and metadata (zero data transfer)

---

## 3. Configuration and Integration

The Rucio backend is configured via YAML specification within the PyDASI configuration file:

```yaml
rucio:
  config_path: rucio.cfg          # Rucio client credentials
  scope: user.root                # Namespace for DIDs
  rse: MINIORSE                   # Target Replica Storage Element
  protocol: s3                    # Transfer protocol preference
```

Integration with PyDASI is transparent:

```python
from pydasi import Dasi, Rucio

# Swap backends via configuration
dasi_rucio = Rucio("config.yml")  # Rucio backend
dasi_fdb = Dasi("config.yml")     # DASI backend (unchanged)

# Both provide identical operations
dasi_rucio.archive({"key": "val"}, data)
for item in dasi_rucio.retrieve([{"key": "val"}]):
    process(item.data)
```

---

## 4. Key Features and Differentiators

| Aspect | DASI Backend | Rucio Backend |
|--------|-------------|---------------|
| **Metadata Model** | Schema-driven hierarchical | Arbitrary key-value attributes |
| **Data Federation** | Single installation | Multi-institutional RSEs |
| **Performance Profile** | High-throughput local I/O | Network-bound distributed access |
| **Scalability** | Cluster-based (DASI native) | Federated (Rucio native) |
| **Use Cases** | Scientific archives, HPC | Data sharing, institutional federation |

---

## 5. Validation and Testing

The implementation has been validated through:

- **Example workflows** in `examples/rucio/`: Archive, list, and retrieve operations demonstrate all functionality
- **Integration testing**: PyDASI test suite extended with Rucio backend tests
- **Environment validation**: Docker-based devcontainer with embedded Rucio server, MinIO RSE, and PostgreSQL catalogue

The Rucio devcontainer environment automatically configures credentials, scopes, and storage elements, enabling rapid validation in isolated environments.

---

## 6. Conclusion

The Rucio backend integration extends DASI's storage capabilities to federated data infrastructures while preserving API compatibility and operational simplicity. The pluggable architecture enables scientific institutions to leverage Rucio's distributed catalogue and transfer capabilities through a unified PyDASI interface, supporting seamless integration with institutional data management workflows and multi-site data federation scenarios.

**Status:** Implementation complete and validated
**Availability:** Feature branch `feature/rucio-backend`

---

*This deliverable fulfills D3.2 objectives for extending DASI storage backends with federated catalogue support through Rucio integration.*
