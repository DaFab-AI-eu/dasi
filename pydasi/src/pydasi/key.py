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

from backend import FFI, ffi, lib, ffi_decode, ffi_encode, new_key, check_type


class Key:
    """
    Container for keyword:value.
    """

    def __init__(self, key=None):
        import logging

        lib.load()

        self._log = logging.getLogger(__name__)

        self._log.debug("init key: %s", key)

        if isinstance(key, Key):
            self._cdata = key._cdata
        elif isinstance(key, FFI.CData):
            check_type(key, "dasi_key_t *")
            self._cdata = key
        else:
            self._cdata = new_key(key)
            if isinstance(key, dict):
                self.insert(key)

    @property
    def cdata(self):
        return self._cdata

    def __setitem__(self, keyword, value):
        lib.dasi_key_set(self._cdata, ffi_encode(keyword), ffi_encode(value))

    def __getitem__(self, keyword):
        value = ffi.new("const char **")
        lib.dasi_key_get(self._cdata, ffi_encode(keyword), value)
        return ffi_decode(value[0])

    def __delitem__(self, keyword):
        lib.dasi_key_erase(self._cdata, ffi_encode(keyword))

    def __len__(self) -> int:
        count = ffi.new("long*", 0)
        lib.dasi_key_count(self._cdata, count)
        return count[0]

    def __str__(self) -> str:
        return "Key: {}".format(repr(self))

    def __repr__(self) -> str:
        keyword = ffi.new("const char **")
        value = ffi.new("const char **")
        out = ""
        for i in range(len(self)):
            lib.dasi_key_get_index(self._cdata, i, keyword, value)
            out += "<{}:{}>".format(
                ffi_decode(keyword[0]), ffi_decode(value[0])
            )
        return out

    def __ne__(self, other) -> bool:
        return not self == other

    def __eq__(self, other) -> bool:
        return self._compare(other) == 0

    def _compare(self, other) -> int:
        if isinstance(other, Key):
            value = ffi.new("int *", 0)
            lib.dasi_key_compare(self._cdata, other._cdata, value)
            return value[0]
        return -1

    def insert(self, keys: dict):
        for keyword, value in keys.items():
            self[keyword] = value

    def has(self, keyword: str) -> bool:
        has = ffi.new("dasi_bool_t *", 1)
        lib.dasi_key_has(self._cdata, ffi_encode(keyword), has)
        return has[0] != 0

    def clear(self):
        lib.dasi_key_clear(self._cdata)
