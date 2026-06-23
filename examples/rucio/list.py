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
from typing import Any, Dict

from helper import setup_logging

logger = _logging.getLogger(__name__)

HERE = Path(__file__).parent
CONFIG_PATH = str(HERE / "dasi.yml")

# A few queries of increasing specificity to show how metadata filters narrow
# the result set produced by archive.py.
QUERIES = [
    {"experiment": "dasi-dev"},
    {"experiment": "dasi-dev", "run": "001"},
    {"experiment": "dasi-dev", "parameter": "temperature"},
    {"experiment": "dasi-dev", "run": "002", "parameter": "humidity"},
]


def run_query(dasi, query: Dict[str, Any]):
    logger.info(f"=== QUERY: {query} ===")
    count = 0
    for item in dasi.list(query):
        count += 1
        logger.info(f"#{count} name={item.name} bytes={item.length} uri={item.uri}")
    if count == 0:
        logger.info("No matches found")


def main() -> int:
    from pydasi import Rucio

    setup_logging()

    dasi = Rucio(CONFIG_PATH)

    for query in QUERIES:
        run_query(dasi, query)
        logger.info("")

    return 0


if __name__ == "__main__":
    sys.exit(main())
