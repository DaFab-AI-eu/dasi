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

import logging as _logging
import sys
from pathlib import Path
from typing import Any, Dict, List

from helper import setup_logging

logger = _logging.getLogger(__name__)

HERE = Path(__file__).parent
CONFIG_PATH = str(HERE / "dasi.yml")

DATASET: List[Dict[str, Any]] = [
    {
        "key": {
            "experiment": "dasi-dev",
            "run": "001",
            "parameter": "temperature",
            "step": "0",
        },
    },
    {
        "key": {
            "experiment": "dasi-dev",
            "run": "001",
            "parameter": "temperature",
            "step": "6",
        },
    },
    {
        "key": {
            "experiment": "dasi-dev",
            "run": "001",
            "parameter": "pressure",
            "step": "0",
        },
    },
    {
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

    dasi = Rucio(CONFIG_PATH)

    for item in DATASET:
        key = item["key"]
        data = (
            "generated sample dataset for rucio archive example:\n"
            + "\n".join(f"{k}={v}" for k, v in key.items())
        ).encode()
        logger.info(f"Archiving data with key={key}, bytes={len(data)}")
        dasi.archive(key=key, data=data)

    logger.info(f"\n{len(DATASET)} file(s) archived")

    return 0


if __name__ == "__main__":
    sys.exit(main())
