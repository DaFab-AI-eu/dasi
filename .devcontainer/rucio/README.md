# Rucio client: example config and runtime instructions

Files

- Example config: [rucio.cfg.example](rucio.cfg.example)

Quick build

```sh
docker build -t mytools:rucio -f .devcontainer/Dockerfile .
```

Run with mounted X.509 certificates

```sh
# Mount your grid certs and (optionally) a local rucio config directory
docker run --rm -it \
  -v $HOME/.globus:/etc/grid-security:ro \
  -v $HOME/.rucio:/root/.rucio:ro \
  mytools:rucio \
  rucio whoami
```

Run with environment variables (userpass example)

```sh
docker run --rm -it \
  -e RUCIO_ACCOUNT=example_account \
  -e RUCIO_AUTH_TYPE=userpass \
  -e RUCIO_USERNAME=myuser \
  -e RUCIO_PASSWORD=mypassword \
  mytools:rucio \
  rucio list-rules
```

Where to place the config

- Put the file `rucio.cfg` in `$HOME/.rucio/rucio.cfg` on the host and mount the directory into the container at `/root/.rucio`.
- Alternatively, set environment variables prefixed with `RUCIO_` for immediate use by the client.

Security notes

- Avoid baking secrets into images. Mount credentials at runtime or supply ephemeral secrets via your orchestration tool.

Help and docs

- Rucio docs and authentication methods vary by deployment; check your Rucio instance docs for exact auth endpoints and required fields.

---

## Basic client commands (local devcontainer)

The devcontainer stack starts a Rucio server, MinIO S3 RSE (`MINIORSE`), and auto-configures the client via `rucio-setup.sh`.
Default values come from [.env](../.env):

| Variable | Default |
|---|---|
| `RUCIO_ACCOUNT` | `root` |
| `RUCIO_AUTH_TYPE` | `userpass` (`ddmlab` / `secret`) |
| Scope | `user.root` |
| RSE | `MINIORSE` |

### Check connectivity

```sh
rucio whoami
rucio ping
```

### Upload

Upload one or more local files into a scope on the RSE:

```sh
# Create sample files
dd if=/dev/urandom of=file.nc bs=1M count=1
dd if=/dev/urandom of=file2.nc bs=1M count=1

# Single file
rucio upload --rse MINIORSE --scope user.root file.nc

# Multiple files with a common dataset (--name not supported; use two steps):
# 1. Upload each file
rucio upload --rse MINIORSE --scope user.root file.nc
rucio upload --rse MINIORSE --scope user.root file2.nc
# 2. Create the dataset and attach the files
rucio did add user.root:my_dataset --type dataset
rucio did content add --to-did user.root:my_dataset \
    user.root:file.nc user.root:file2.nc

# Specify a lifetime (seconds) for the replica rule
rucio upload --rse MINIORSE --scope user.root --lifetime 86400 file.nc
```

The resulting Data IDentifier (DID) is `<scope>:<filename>`, e.g. `user.root:file.nc`.

### List

```sh
# List all DIDs in a scope (files, datasets, containers)
rucio did list --filter 'type=all' 'user.root:*'

# List DIDs matching a pattern (files only)
rucio did list --filter 'type=file' 'user.root:file*'

# List files inside a dataset
rucio did content list user.root:my_dataset

# List the RSEs that hold replicas of a DID
rucio replica list file user.root:file.nc

# Show all RSEs registered in the catalogue
rucio rse list
```

### Download

> **Note**: This devcontainer uses an S3/MinIO RSE. Add `--protocol s3` to all
> `rucio download` calls so the client uses the S3 protocol directly (the default
> metalink-based resolver does not include S3 URLs).

```sh
# Download a single file to the current directory
rucio download --protocol s3 user.root:file.nc

# Download to a specific directory
rucio download --protocol s3 --dir /tmp/data user.root:file.nc

# Download an entire dataset
rucio download --protocol s3 user.root:my_dataset

# Download only from a specific RSE  (flag is --rses, not --rse)
rucio download --protocol s3 --rses MINIORSE user.root:file.nc
```

### Query (replication rules and metadata)

```sh
# List all active rules for a DID
rucio rule list --did user.root:file.nc

# Show detailed info (size, checksums, creation time) for a DID
rucio did show user.root:file.nc

# List replicas and their RSE locations
rucio replica list file user.root:file.nc

# Show account info and quota on an RSE
rucio account show root
rucio account limit list root
```

### Search

Rucio supports key-value metadata attached to DIDs. Use `did metadata add`/`did metadata list`
to store arbitrary attributes, then filter with `did list`:

```sh
# Attach metadata to a DID
rucio did metadata add user.root:file.nc --key experiment --value dasi-dev
rucio did metadata add user.root:file.nc --key run --value 42

# Read metadata
rucio did metadata list user.root:file.nc

# Search DIDs by metadata filter (server must support the plugin)
rucio did list --filter 'experiment=dasi-dev' 'user.root:*'

# Combine multiple filters
rucio did list --filter 'experiment=dasi-dev,run=42' 'user.root:*'
```

### Delete

There is no `rucio delete` command. Deletion is a two-concept operation in Rucio:

**1. Expire a DID** — schedules the DID (and its replicas if `purge_replicas=True`) for deletion after a 24-hour grace period:

```sh
# Mark a file or dataset DID for expiry (irreversible after 24 h)
rucio did remove user.root:file.nc
rucio did remove user.root:my_dataset

# Undo within the 24-hour window
rucio did remove --undo user.root:file.nc
```

**2. Remove a replication rule** — removing the last rule releases the replica lock; the reaper daemon then purges the physical copy:

```sh
# Remove a rule by rule ID (get ID from: rucio rule list --did <DID>)
rucio rule remove <RULE_ID>

# Remove and immediately purge the replicas
rucio rule remove --purge-replicas <RULE_ID>

# Remove all rules for a DID on a specific RSE
rucio rule remove user.root:file.nc --rses MINIORSE
```

**3. Tombstone a specific replica** — marks one physical copy for deletion by the reaper (only works when the replica is not locked by a rule):

```sh
rucio replica remove --rse MINIORSE user.root:file.nc
```

> **Note**: `replica remove` will fail with "Replica is locked" as long as an
> active replication rule holds the replica. Remove or expire the rule first.
