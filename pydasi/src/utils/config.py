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

import yaml

from pathlib import Path
from typing import Any, TypeVar, List, Dict


PathLike = TypeVar("PathLike", str, Path)

__template__ = """
schema: {schema_}
catalogue: toc
store: file
spaces:
- roots:
  - path: {path_}
"""

__default_config__ = __template__.format(schema_="./schema", path_="./root1")

__default_path__ = "./dasi.yaml"


class Config(object):
    def __init__(self) -> None:
        from utils.log import getLogger

        self._log = getLogger(__name__)

        self._log.debug("Config init...")

        self._yaml = yaml.safe_load("schema:\nspaces:\n  - roots:")

    @property
    def yaml(self):
        return self._yaml

    @property
    def dump(self) -> str:
        return yaml.dump(self._yaml)

    def __repr__(self) -> str:
        return self.dump

    def __str__(self) -> str:
        return "config:\n%s" % self.dump

    def load(self, config: str = __default_config__):
        from backend import DASIException

        self._yaml = yaml.safe_load(config)

        if self._yaml is None:
            raise DASIException("Failed to load config!")

        return self

    def load_file(self, path: PathLike = __default_path__):
        import os

        self._log.debug("- load config: %s", path)

        cfg_ = ""

        if os.path.exists(path):
            with open(path, "r") as file:
                cfg_ = file.read()

        return self.load(cfg_)

    def dump_file(self, path: PathLike = __default_path__):
        self._log.debug("- dump config: %s", path)

        with open(path, "w") as f:
            yaml.dump(self._yaml, f)

    def default(
        self,
        schema: PathLike = "./schema",
        path: PathLike = "./root1",
    ):
        return self.load(__template__.format(schema_=schema, path_=path))

    def schema(self, value: PathLike):
        self._yaml["schema"] = value if isinstance(value, str) else str(value)
        return self

    def roots(self, value: List[Dict[str, Any]]):
        self._yaml["spaces"][0] = {"roots": value}
        return self
