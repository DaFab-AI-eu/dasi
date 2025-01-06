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

import pytest


__config1__ = """schema: ./schema1
spaces:
- roots:
  - path: ./root1
  - path: ./root2
    wipe: true
"""

__config2__ = """catalogue: toc
schema: ./schema2
spaces:
- roots:
  - path: ./root3
store: file
"""

__config3__ = """catalogue: toc
schema: ./schema
spaces:
- roots:
  - path: ./root1
store: file
"""

__config_bad__ = """
"""


def test_dasi_import():
    """
    Test importing DASI package
    """

    import pydasi  # noqa: F401

    with pytest.raises(ImportError) as excinfo:
        import dasi  # noqa: F401

    assert str(excinfo.value) == "No module named 'dasi'"


def test_dasi_config(tmp_path_factory: pytest.TempPathFactory):
    """
    Test DASI configuration
    """

    from pydasi import Config, DASIException

    cfg1 = (
        Config()
        .schema("./schema1")
        .roots([{"path": "./root1"}, {"path": "./root2", "wipe": True}])
        .dump
    )

    assert cfg1 == __config1__

    # ==========================================================================

    cfg2 = Config().default("./schema2", "./root3").dump

    assert cfg2 == __config2__

    # ==========================================================================

    cfg3 = Config().load().dump

    assert cfg3 == __config3__

    # ==========================================================================

    cfg4 = Config().load(__config3__).dump

    assert cfg4 == __config3__

    # ==========================================================================

    cfg_path = tmp_path_factory.getbasetemp() / "test-config.yml"

    Config().load(__config1__).dump_file(cfg_path)

    cfg5 = Config().load_file(cfg_path).dump

    assert cfg5 == __config1__

    # ==========================================================================

    with pytest.raises(DASIException):
        Config().load(__config_bad__)

    with open(cfg_path, "w") as f:
        f.write(__config_bad__)

    with pytest.raises(DASIException):
        Config().load_file(cfg_path)
