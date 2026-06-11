#!/usr/bin/env python3
# Copyright 2023 European Centre for Medium-Range Weather Forecasts (ECMWF)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Retrieve file data from a Rucio catalogue using the pydasi Rucio backend.

All connection settings are read from ``pydasi.yml`` in this directory, which
in turn references ``rucio.cfg`` for the credentials.  Both ship with working
defaults for the devcontainer stack; adjust them before running against any
other deployment.

Run inside the devcontainer after the Rucio + MinIO stack is up:

    cd /workspace/dasi/examples/rucio
    python retrieve.py

Files are downloaded into the directory specified by OUT_DIR (default:
``./retrieved/``).  Existing files with the same name are overwritten.

Environment variables (optional):

    OUT_DIR       Output directory       [./retrieved]
"""

import logging
import os
import sys
from pathlib import Path

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s  %(levelname)-8s  %(name)s  %(message)s",
)

# ---------------------------------------------------------------------------
# Settings — only example-specific knobs; connection settings live in
# pydasi.yml (which references rucio.cfg).
# ---------------------------------------------------------------------------
HERE = Path(__file__).parent
CONFIG_PATH = str(HERE / "pydasi.yml")
OUT_DIR = Path(os.environ.get("OUT_DIR", HERE / "retrieved"))


def main() -> int:
    from pydasi import Rucio

    dasi = Rucio(CONFIG_PATH)

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    print(f"=== Retrieve output directory: {OUT_DIR}")

    # -----------------------------------------------------------------------
    # 1. Retrieve every file tagged with the temperature parameter
    # -----------------------------------------------------------------------
    query = {"experiment": "dasi-dev", "parameter": "temperature"}
    print(f"\n--- query: {query}")

    retrieved = 0
    for item in dasi.retrieve(query):
        dest = OUT_DIR / item.name
        dest.write_bytes(item.data)
        print(f"  uri={item.uri}  bytes={item.length}  → {dest.name}")
        retrieved += 1

    if retrieved == 0:
        print("  no files matched — run archive.py first")
    else:
        print(f"  {retrieved} file(s) written")

    # -----------------------------------------------------------------------
    # 2. Retrieve a single run/step with a narrower filter
    # -----------------------------------------------------------------------
    query_narrow = {
        "experiment": "dasi-dev",
        "run": "002",
        "parameter": "humidity",
    }
    print(f"\n--- query: {query_narrow}")

    retrieved = 0
    for item in dasi.retrieve(query_narrow):
        dest = OUT_DIR / item.name
        dest.write_bytes(item.data)
        print(f"  uri={item.uri}  bytes={item.length}  → {dest.name}")
        retrieved += 1

    if retrieved == 0:
        print("  no files matched — check the query filters and run archive.py first")
    else:
        print(f"  {retrieved} file(s) written")

    return 0


if __name__ == "__main__":
    sys.exit(main())
