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

import sys
from pathlib import Path
from typing import Any, Dict, List
from helper import setup_logging
import logging as _logging

logger = _logging.getLogger(__name__)

HERE = Path(__file__).parent
CONFIG_PATH = str(HERE / "dasi.yml")
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

    setup_logging()

    if not SOURCE_FILE.exists():
        logger.error(f"source file not found: {SOURCE_FILE}")
        return 1

    dasi = Rucio(CONFIG_PATH)

    data = SOURCE_FILE.read_bytes()

    for item in DATASET:
        did = dasi.archive(
            key=item["key"],
            data=data,
            filename=item["filename"],
        )
        logger.info(f"archived -> {did}")
        logger.info(f"  metadata : {item['key']}")
        logger.info(f"  bytes    : {len(data)}")

    logger.info(f"\n{len(DATASET)} file(s) archived")

    return 0


if __name__ == "__main__":
    sys.exit(main())
