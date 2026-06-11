# pydasi Rucio backend

A pure-Python backend that wires pydasi's `list` / `retrieve` / `archive` API
on top of a [Rucio](https://rucio.cern.ch/) data-management server.

## Files

| File | Purpose |
|---|---|
| `archive.py` | End-to-end archive example (upload files + attach metadata) |
| `list.py` | List DIDs in the catalogue with optional metadata filters |
| `retrieve.py` | Download files matching a metadata query |
| `pydasi.yml` | Backend settings loaded via `Rucio("pydasi.yml")` |
| `rucio.cfg` | Annotated client configuration (credentials, server URL) |
| `rucio_ca_bundle.pem` | CA certificate bundle for TLS verification |

## Requirements

```sh
pip install "rucio-clients>=40" boto3
```

## Quick start (devcontainer)

The workspace `.devcontainer/` stack starts a Rucio server (`rucio-server:80`)
and a MinIO S3 RSE (`MINIORSE`).  Once the stack is running:

```sh
cd /workspace/dasi/examples/rucio
python archive.py    # upload sample files
python list.py       # list DIDs in the catalogue
python retrieve.py   # download matching files
```

Default credentials come from `.devcontainer/.env`:

| Variable | Default |
|---|---|
| `RUCIO_HOST` | `http://rucio-server:80` |
| `RUCIO_ACCOUNT` | `root` |
| `RUCIO_AUTH_TYPE` | `userpass` |
| `RUCIO_USERNAME` | `ddmlab` |
| `RUCIO_PASSWORD` | `secret` |
| `RUCIO_RSE` | `MINIORSE` |
| `RUCIO_SCOPE` | `user.root` |

## Upload files with the native Rucio CLI

The Python examples above wrap the same operations exposed by the `rucio`
command-line client.  When you only need to push a file into the catalogue
(without the pydasi metadata wiring) the CLI is the quickest path.

Point the client at the example config and confirm the connection:

```sh
cd /workspace/dasi/examples/rucio
export RUCIO_CONFIG="$PWD/rucio.cfg"

rucio whoami        # verify authentication
rucio ping          # verify server reachability
```

Upload a single file to the MinIO RSE under the `user.root` scope.  Rucio
registers a file DID named after the local file (`user.root:file.nc`):

```sh
rucio upload --rse MINIORSE --scope user.root file.nc
```

Upload multiple files, overriding the DID name and setting a replica
lifetime (in seconds — here ~1 day):

```sh
rucio upload --rse MINIORSE --scope user.root \
    --name sample_run001.bin --lifetime 86400 \
    file.nc
```

Attach DASI-style metadata so the files are discoverable by `list.py` /
`retrieve.py`.  Note that Rucio only accepts arbitrary metadata keys when the
server has the generic (JSON) metadata plugin enabled; otherwise use one of
the built-in attributes:

```sh
rucio set-metadata --did user.root:file.nc --key experiment --value rucio-backend-example
rucio set-metadata --did user.root:file.nc --key run        --value 001
```

Verify and inspect the uploaded DID:

```sh
rucio list-dids       user.root:* --filter type=file
rucio list-file-replicas user.root:file.nc
rucio get-metadata    user.root:file.nc
```

Download it again to a local directory:

```sh
rucio download --dir ./retrieved user.root:file.nc
```

> Tip: the older subcommand spellings (`rucio list-dids`, `rucio set-metadata`)
> are being replaced by the `rucio did`, `rucio replica`, and
> `rucio scope`/`rucio rse` command groups in newer clients
> (`rucio-clients >= 35`).  Run `rucio --help` to see which form your
> installed client uses.

## API reference

### `Rucio`

The backend can be configured either from a single YAML file or by passing
settings explicitly.  The examples in this directory use the YAML form:

```python
from pydasi import Rucio

dasi = Rucio("pydasi.yml")
```

`pydasi.yml` holds the backend settings (relative path-like values are
resolved relative to the file's location):

```yaml
rucio:
  config_path: rucio.cfg   # path to rucio.cfg (credentials + server URL)
  scope: user.root
  rse: MINIORSE
  protocol: s3             # preferred transfer protocol
```

Equivalent explicit form (keyword arguments override any YAML values):

```python
dasi = Rucio(
    config_path="rucio.cfg",   # path to rucio.cfg (credentials + server URL)
    scope="user.root",
    rse="MINIORSE",
    protocol="s3",             # preferred transfer protocol (optional)
)
```

#### `dasi.list(query, fetch_metadata=True)`

Query the Rucio catalogue by DID metadata.  Returns an iterable of
`RucioListItem` objects:

```python
for item in dasi.list({"experiment": "dasi-dev", "run": "42"}):
    print(item.uri)        # "user.root:myfile.nc"
    print(item.key)        # {"experiment": "dasi-dev", "run": "42", ...}
    print(item.length)     # file size in bytes
    print(item.timestamp)  # DID creation time
```

The `query` dict maps directly to Rucio DID metadata filters.  A special
`"scope"` key selects the Rucio scope (default: `scope` from the
constructor).

#### `dasi.retrieve(query)`

Download files matching *query* and expose their raw content:

```python
for item in dasi.retrieve({"experiment": "dasi-dev", "run": "42"}):
    print(item.uri)        # "user.root:myfile.nc"
    print(item.data)       # raw bytes
    print(item.key)        # attached metadata
    print(item.length)     # bytes read
```

Files are downloaded into a temporary directory and read into memory; the
temporary directory is removed after each file.

#### `dasi.archive(key, data, filename=None, lifetime=None)`

Upload *data* and attach *key* as DID metadata.  The target scope and RSE come
from the `Rucio` instance (i.e. `pydasi.yml`):

```python
did = dasi.archive(
    key={"experiment": "dasi-dev", "run": "001", "parameter": "temperature"},
    data=open("result.nc", "rb").read(),
    filename="result.nc",
)
# did == "user.root:result.nc"
```

`archive.py` uploads several files that share an `experiment` but differ in
`run`, `parameter`, and `step`, so `list.py` / `retrieve.py` can demonstrate
filtering by each of those keys.

## Mapping DASI keys to Rucio metadata

DASI keys are flat key → value dicts.  They map to Rucio DID metadata
attributes set via `set_metadata` / queried via `list_dids(filters=...)`.

```
DASI key                     Rucio metadata attribute
────────────────────────────────────────────────────
{"experiment": "dasi-dev"}   experiment = "dasi-dev"
{"run": "42"}                run = "42"
{"scope": "user.alice"}      → selects Rucio scope "user.alice"
```

## Security notes

- Never hard-code credentials.  Store them in `rucio.cfg` and mount the
  file at runtime; never commit real passwords to source control.
- Set `ca_cert` in `rucio.cfg` to the path of a valid CA bundle (e.g.
  `rucio_ca_bundle.pem`) in production.  Disabling TLS verification
  (`ca_cert = False`) is only acceptable on a local development stack
  that does not carry real data.
