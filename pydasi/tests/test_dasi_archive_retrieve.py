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

from pydasi import Dasi, DASIException

__simple_data_1__ = b"TESTING SIMPLE ARCHIVE 11111111111"
__simple_data_2__ = b"TESTING SIMPLE ARCHIVE 22222222222"
__simple_data_3__ = b"TESTING SIMPLE ARCHIVE 33333333333"

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

    tmp_path = tmp_path_factory.getbasetemp()

    schema_path = tmp_path / "schema"
    with open(schema_path, "w") as f:
        f.write(__dasi_schema__)

    return Config().default(schema_path, tmp_path).dump


def test_simple_archive(dasi_cfg: str):
    """
    Test Dasi archiving
    """

    dasi = Dasi(dasi_cfg)

    key = {
        "key2": "123",
        "key3": "value1",
        "key1a": "value1",
        "key2a": "value1",
        "key3a": "321",
        "key1b": "value1",
        "key2b": "value1",
        "key3b": "value1",
    }

    try:
        key["key1"] = "value1"
        dasi.archive(key, __simple_data_1__)
        key["key1"] = "value2"
        dasi.archive(key, __simple_data_2__)
        key["key1"] = "value3"
        dasi.archive(key, __simple_data_3__)
    except DASIException:
        pytest.fail("Cannot archive!")


def test_archive_fail(dasi_cfg: str):
    """
    Test Dasi archiving failing
    """

    dasi = Dasi(dasi_cfg)

    key = {
        "key1": "value1",
        "key2": "123",
        "key3": "value1",
    }

    with pytest.raises(DASIException):
        dasi.archive(key, __simple_data_1__)


def test_simple_retrieve(dasi_cfg: str):
    """
    Test Dasi retrieve
    """

    dasi = Dasi(dasi_cfg)

    query = {
        "key1": ["value1", "value2", "value3"],
        "key2": ["123"],
        "key3": ["value1"],
        "key1a": ["value1"],
        "key2a": ["value1"],
        "key3a": ["321"],
        "key1b": ["value1"],
        "key2b": ["value1"],
        "key3b": ["value1"],
    }

    retrieved = dasi.retrieve(query)
    assert len(retrieved) == 3

    results = {}
    for r in retrieved:
        object = r.key["key1"]
        results[object] = r.data

    assert results["value1"] == __simple_data_1__
    assert results["value2"] == __simple_data_2__
    assert results["value3"] == __simple_data_3__


def test_empty_retrieve(dasi_cfg: str):
    """
    Test Dasi retrieve
    """

    dasi = Dasi(dasi_cfg)

    query = {
        "key2": ["1234"],
        "key3": ["value1"],
        "key1a": ["value1"],
        "key2a": ["value1"],
        "key3a": ["321"],
        "key1b": ["value1"],
        "key2b": ["value1"],
        "key3b": ["value1"],
    }

    retrieved = dasi.retrieve(query)

    assert len(retrieved) == 0
