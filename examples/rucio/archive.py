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
"""Archive several files to a Rucio catalogue using the pydasi Rucio backend.

This example uploads multiple objects, each tagged with a different set of
metadata, so that ``list.py`` and ``retrieve.py`` can demonstrate querying by
varied filters (experiment, parameter, step, ...).

All settings are read from ``pydasi.yml`` in this directory, which in turn
references ``rucio.cfg`` for the connection credentials.  Both ship with
working defaults for the devcontainer stack; adjust them before running
against any other deployment.

Run inside the devcontainer after the Rucio + MinIO stack is up:

    cd /workspace/dasi/examples/rucio
    python archive.py

Run list.py and retrieve.py afterwards to query and download the data.
"""

import logging
import sys
from pathlib import Path
from typing import Any, Dict, List

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s  %(levelname)-8s  %(name)s  %(message)s",
)

HERE = Path(__file__).parent
CONFIG_PATH = str(HERE / "pydasi.yml")
SOURCE_FILE = HERE / "file.nc"

# Each entry pairs a distinct metadata key with a target DID name.  The same
# source payload is reused for every item; only the metadata differs, which is
# what list.py / retrieve.py query against.
DATASET: List[Dict[str, Any]] = [
    {
        "filename": "temperature_run001_step0.nc",
        "key": {
            "experiment": "dasi-dev",
            "run": "001",
            "parameter": "temperature",
            "step": "0",
        },
    },
    {
        "filename": "temperature_run001_step6.nc",
        "key": {
            "experiment": "dasi-dev",
            "run": "001",
            "parameter": "temperature",
            "step": "6",
        },
    },
    {
        "filename": "pressure_run001_step0.nc",
        "key": {
            "experiment": "dasi-dev",
            "run": "001",
            "parameter": "pressure",
            "step": "0",
        },
    },
    {
        "filename": "humidity_run002_step0.nc",
        "key": {
            "experiment": "dasi-dev",
            "run": "002",
            "parameter": "humidity",
            "step": "0",
        },
    },
]


def main() -> int:
    from pydasi import Rucio

    if not SOURCE_FILE.exists():
        print(f"source file not found: {SOURCE_FILE}", file=sys.stderr)
        return 1

    dasi = Rucio(CONFIG_PATH)

    data = SOURCE_FILE.read_bytes()

    for item in DATASET:
        did = dasi.archive(
            key=item["key"],
            data=data,
            filename=item["filename"],
        )
        print(f"archived → {did}")
        print(f"  metadata : {item['key']}")
        print(f"  bytes    : {len(data)}")

    print(f"\n{len(DATASET)} file(s) archived")
    return 0


if __name__ == "__main__":
    sys.exit(main())
