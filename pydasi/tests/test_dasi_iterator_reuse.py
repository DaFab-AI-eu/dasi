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

"""
List and Retrieve yield the iterator itself from __next__, so every item
collected by list(dasi.list(q)) is the same object showing the final record.
"""

import pytest

from pydasi import Dasi

__data_jp2__ = b"JP2"
__data_tiff__ = b"TIFF"

__base_key__ = {
    "key1": "value1",
    "key2": "123",
    "key3": "value1",
    "key1a": "value1",
    "key2a": "value1",
    "key3a": "321",
    "key1b": "value1",
    "key2b": "value1",
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

    path = tmp_path_factory.mktemp("iterator_reuse")

    schema_ = path / "schema"
    schema_.write_text(__dasi_schema__)

    root_ = path / "root"
    root_.mkdir()

    return Config().default(schema_, root_).dump


@pytest.fixture(scope="session")
def query(dasi_cfg: str) -> dict:
    dasi = Dasi(dasi_cfg)
    dasi.archive({**__base_key__, "key3b": "image_jp2"}, __data_jp2__)
    dasi.archive({**__base_key__, "key3b": "image_tiff"}, __data_tiff__)
    dasi.flush()

    query = {keyword: [value] for keyword, value in __base_key__.items()}

    # read the key3b values from the archived items
    query["key3b"] = [item.key["key3b"] for item in dasi.list(query)]

    return query


def test_list_items_are_distinct(dasi_cfg: str, query: dict):
    items = list(Dasi(dasi_cfg).list(query))

    assert [item.key["key3b"] for item in items] == ["image_jp2", "image_tiff"]


def test_retrieve_items_are_distinct(dasi_cfg: str, query: dict):
    items = list(Dasi(dasi_cfg).retrieve(query))

    assert [(item.key["key3b"], bytes(item.data)) for item in items] == [
        ("image_jp2", __data_jp2__),
        ("image_tiff", __data_tiff__),
    ]


def test_values_read_during_iteration_are_correct(dasi_cfg: str, query: dict):
    """The behaviour that already works, and must keep working."""

    during = [item.key["key3b"] for item in Dasi(dasi_cfg).list(query)]

    assert during == ["image_jp2", "image_tiff"]
