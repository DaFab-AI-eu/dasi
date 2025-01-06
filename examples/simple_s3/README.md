# DASI Usage: Archival and Retrieval (via S3)

This example demonstrates DASI usage of data archival and retrieval to external object stores (MinIO) that support S3 API.
The `Archive.cc` shows putting data into a DASI database, and the `Retrieve.cc` shows getting data from a DASI database.

## Setup

### MinIO Server

For testing purposes, the `docker-compose.yml` file can be used to run a local MinIO server (`minio:9000`) as:

```bash
docker compose up -d
```

### Configuration

The following file configures DASI to use `./database` (POSIX filesystem) for indexing purposes
and an external server `minio:9000` for storing the bulk data.

```yaml
---
type: local
engine: toc
schema: ./schema
store: s3
spaces:
  - roots:
    - path: ./database
s3:
  configuration: ./S3Config.yaml
  credentials: ./S3Credentials.yaml
  roots:
  - endpoint: minio:9000
    bucket: dasi-example-bucket
```

### Schema

It is an example 3-level rule.

```yaml
[ institute, user [ type [ level, parameter ] ] ]
```

## Building

This example project uses CMake to build the source code.

> [!IMPORTANT]
> DASI library must be built with S3 support before.

```bash
# configure
cmake -DCMAKE_PREFIX_PATH="/path/to/dasi_bundle" -S . -B ./build
# build
cmake --build ./build
```

## archive app

The `archive` is a CLI app that puts random mock data into the DASI database (POSIX + Object Store).

The options below can be set:

```bash
--user <value> --institute <value> --type <value> --parameter <value> --level <value>
```

Example usage:

```bash
./build/archive -t test
```

## retrieve app

The `retrieve` is a CLI app that gets data from the DASI database (POSIX + Object Store via S3 API)
and saves it into file given by user.

The options below can be set:

```bash
--user <value> --institute <value> --type <value> --parameter <value> --level <value> --output_file <value>
```

Example usage:

```bash
./build/retrieve -t test -p temperature -l 3 -o out.data
```
