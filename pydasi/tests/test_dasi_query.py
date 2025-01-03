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

from pydasi import Query, DASIException, check_type


def test_query_typename():
    """
    Construct a query and assert its type name.
    """

    query = Query()

    check_type(query.cdata, "dasi_query_t *")


def test_query_empty():
    """
    Construct empty keys and assert keys are empty by count() and has().
    """

    query = Query()

    assert query.has("key1") is False
    assert query.has("key2") is False
    assert len(query) == 0

    query["key1"] = ["value1"]
    assert query.has("key1") is True
    assert len(query) == 1
    assert query.count_value("key1") == 1

    assert query.has("key2") is False
    query["key2"] = ["value2a", "value2b"]
    assert query.has("key2") is True
    assert len(query) == 2
    assert query.count_value("key2") == 2


def test_query_clear():
    """
    Assert clear() and erase().
    """

    tmp = {
        "key1a": ["value1a"],
        "key2a": ["value2a"],
        "key3a": ["value3a"],
        "key1b": ["value1b"],
        "key2b": ["value2b"],
        "key3b": ["value3ba", "value3bb"],
    }
    query = Query(tmp)

    assert query.has("key1a") is True
    assert query.has("key2a") is True
    assert query.has("key3a") is True
    assert query.has("key1b") is True
    assert query.has("key2b") is True
    assert query.has("key3b") is True
    assert query.has("key4") is False

    assert len(query) == 6

    assert query.get_value("key1a", 0) == "value1a"
    assert query.get_value("key2a", 0) == "value2a"
    assert query.get_value("key3a", 0) == "value3a"
    assert query.get_value("key1b", 0) == "value1b"
    assert query.get_value("key2b", 0) == "value2b"
    assert query.get_value("key3b", 0) == "value3ba"
    assert query.get_value("key3b", 1) == "value3bb"

    with pytest.raises(DASIException):
        query.get_value("key1a", -1)
    with pytest.raises(DASIException):
        query.get_value("key1a", 1)
    with pytest.raises(DASIException):
        query.get_value("key3b", 2)

    assert query.count_value("key1a") == 1
    assert query.count_value("key2a") == 1
    assert query.count_value("key3a") == 1
    assert query.count_value("key1b") == 1
    assert query.count_value("key2b") == 1
    assert query.count_value("key3b") == 2

    # delete pairs
    del query["key1a"]
    assert len(query) == 5
    del query["key2a"]
    assert len(query) == 4
    # add more pairs
    query["key4"] = ["value4a", "value4b"]
    assert query.has("key4") is True
    assert len(query) == 5
    # delete all pairs
    query.clear()
    assert len(query) == 0
    assert query.count_value("key1a") == 0
    assert query.count_value("key2a") == 0
    assert query.count_value("key3a") == 0
    assert query.count_value("key1b") == 0
    assert query.count_value("key2b") == 0
    assert query.count_value("key3b") == 0
    assert query.count_value("key4") == 0


def test_query_from_string():
    """
    Construct keys from string.
    """

    pairs = "key3=value3a/value3b/value3c,key1=value1,key2=value2"
    query = Query(pairs)

    assert len(query) == 3
    assert query.has("key1") is True
    assert query.has("key2") is True
    assert query.has("key3") is True
    assert query.count_value("key1") == 1
    assert query.count_value("key2") == 1
    assert query.count_value("key3") == 3

    assert query.get_value("key1", 0) == "value1"
    assert query.get_value("key2", 0) == "value2"
    assert query.get_value("key3", 0) == "value3a"
    assert query.get_value("key3", 1) == "value3b"
    assert query.get_value("key3", 2) == "value3c"


def test_query_invalid():
    """
    Assert invalid query.
    """

    with pytest.raises(DASIException):
        pairs = "key3=value3=value3b,key1=value1,key2=value2"
        Query(pairs)

    with pytest.raises(DASIException):
        pairs = "key3=value3/value3b,key1=value1;key2=value2"
        Query(pairs)
