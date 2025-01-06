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

from pydasi import Dasi, Key, DASIException

__simple_data_0__ = b"TESTING SIMPLE LIST 00000000000"
__simple_data_1__ = b"TESTING SIMPLE LIST 11111111111"
__simple_data_2__ = b"TESTING SIMPLE LIST 22222222222"

__list_0__ = {
    "key1": "value0",
    "key2": "123",
    "key3": "value1",
    "key1a": "value1",
    "key2a": "value1",
    "key3a": "321",
    "key1b": "value1",
    "key2b": "value1",
    "key3b": "value1",
}

__list_1__ = {
    "key1": "value1",
    "key2": "123",
    "key3": "value1",
    "key1a": "value1",
    "key2a": "value1",
    "key3a": "321",
    "key1b": "value1",
    "key2b": "value1",
    "key3b": "value1",
}

__list_2__ = {
    "key1": "value2",
    "key2": "123",
    "key3": "value1",
    "key1a": "value1",
    "key2a": "value1",
    "key3a": "321",
    "key1b": "value1",
    "key2b": "value1",
    "key3b": "value1",
}

__dasi_schema__ = """
key2:  Integer;
key3a: Integer;

[ key1, key2, key3
    [ key1a, key2a, key3a
        [ key1b, key2b, key3b ]]]

"""


@pytest.fixture(scope="session")
def dasi_cfg(tmp_path_factory: pytest.TempPathFactory) -> str:
    from pydasi import Config

    path = tmp_path_factory.getbasetemp()

    schema_ = path / "schema"
    schema_.write_text(__dasi_schema__)

    (path / "root").mkdir()

    return Config().default(schema_, path / "root").dump


def test_empty_list(dasi_cfg: str):
    """
    Test empty list
    """

    dasi = Dasi(dasi_cfg)

    query = {
        "key2": ["123"],
        "key3": ["value1"],
        "key1a": ["value1"],
        "key2a": ["value1"],
        "key3a": ["321"],
        "key1b": ["value1"],
        "key2b": ["value1"],
        "key3b": ["value1"],
    }

    dlist = dasi.list(query)

    assert len(dlist) == 0


def test_simple_list(dasi_cfg: str):
    """
    Test Dasi list
    """

    dasi = Dasi(dasi_cfg)

    try:
        dasi.archive(__list_0__, __simple_data_0__)
        dasi.archive(__list_1__, __simple_data_1__)
        dasi.archive(__list_2__, __simple_data_2__)
    except DASIException:
        pytest.fail("Archive failed!")

    dasi.flush()

    query = {
        "key2": ["123"],
        "key3": ["value1"],
        "key1a": ["value1"],
        "key2a": ["value1"],
        "key3a": ["321"],
        "key1b": ["value1"],
        "key2b": ["value1"],
        "key3b": ["value1"],
    }

    keys = []
    for item in dasi.list(query):
        keys.append(item.key)

    assert keys[0] == Key(__list_0__)
    assert keys[1] == Key(__list_1__)
    assert keys[2] == Key(__list_2__)
