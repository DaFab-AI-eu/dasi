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

from pydasi import Key, DASIException


def test_key_typename():
    """
    Construct a key and assert its type name.
    """
    from pydasi import check_type

    key = Key()

    check_type(key.cdata, "dasi_key_t *")


def test_key_clear():
    """
    Assert clear() and erase() keys.
    """

    key = Key()

    assert len(key) == 0

    # add keyword:value pair
    key["key1"] = "value1"
    assert len(key) == 1

    # insert multiple pairs
    pair = {"key2": "value2", "key3": "value3"}
    key.insert(pair)
    assert len(key) == 3
    assert key.has("key2") is True

    # delete a key
    del key["key1"]
    assert len(key) == 2
    assert key.has("key1") is False

    # delete all pairs
    key.clear()
    assert len(key) == 0


def test_key_empty():
    """
    Construct empty keys and assert keys are empty by count() and has().
    """

    key = Key()

    assert key.has("key1") is False
    assert key.has("key2") is False
    assert len(key) == 0

    # add keyword:value pair
    key["key1"] = "value1"
    assert len(key) == 1
    assert key.has("key1") is True

    # get the value of keyword 'key1'
    value1 = key["key1"]
    assert value1 == "value1"


def test_key_from_string():
    """
    Construct keys from string.
    """

    pairs = "key3=value3,key1=value1,key2=value2"
    key = Key(pairs)

    assert len(key) == 3
    assert key.has("key1") is True
    assert key.has("key2") is True
    assert key.has("key3") is True
    assert key.has("other") is False

    assert key["key1"] == "value1"
    assert key["key2"] == "value2"
    assert key["key3"] == "value3"


def test_key_invalid():
    """
    Assert invalid keys.
    """

    with pytest.raises(DASIException):
        pairs = "key3=value3=value3b,key1=value1,key2=value2"
        Key(pairs)

    with pytest.raises(DASIException):
        pairs = "key3=value3/value3b,key1=value1,key2=value2"
        Key(pairs)


def test_key_dictionary():
    """
    Construct a key from dictionary and assert values.
    """

    # dict version
    pair = {"key1": "value1"}
    key = Key(pair)

    assert key.has("key1") is True

    # get the value of keyword 'key1'
    value1 = key["key1"]
    assert value1 == "value1"

    assert len(key) == 1

    # add keyword:value pair
    key["key2"] = "value2"

    assert key.has("key2") is True

    # get the value of keyword 'key2'
    value2 = key["key2"]
    assert value2 == "value2"

    assert len(key) == 2

    # insert multiple pairs
    pair = {"key3": "value3", "key4": "value4"}
    key.insert(pair)

    assert key.has("key3") is True
    assert key.has("key4") is True

    # get the value of keyword 'key3'
    value3 = key["key3"]
    assert value3 == "value3"
    # get the value of keyword 'key4'
    value4 = key["key4"]
    assert value4 == "value4"

    assert len(key) == 4


def test_key_modify():
    pair = {"key1": "value1"}
    key = Key(pair)

    assert len(key) == 1
    assert key.has("key1") is True

    # get the value of keyword 'key1'
    value1 = key["key1"]
    assert value1 == "value1"

    key["key1"] = "value2"
    value2 = key["key1"]
    assert value2 == "value2"


def test_key_compare():
    pair = {"key1": "value1"}
    key = Key(pair)

    # add another key
    key["key2"] = "value2"

    assert key == Key({"key1": "value1", "key2": "value2"})
