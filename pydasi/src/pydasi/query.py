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

from backend import (
    FFI,
    DASIException,
    ffi,
    lib,
    ffi_encode,
    ffi_decode,
    check_type,
    new_query,
)


class Query:
    """
    Container for the keyword:value for retrieving data.
    """

    def __init__(self, query=None):
        from utils.log import getLogger

        lib.load()

        self._log = getLogger(__name__)

        self._log.debug("init query: %s", query)

        if isinstance(query, Query):
            self._cdata = query._cdata
        elif isinstance(query, FFI.CData):
            check_type(query, "dasi_query_t *")
            self._cdata = query
        else:
            self._cdata = new_query(query)
            if isinstance(query, dict):
                self.insert(query)

    @property
    def cdata(self):
        return self._cdata

    def __setitem__(self, keyword, value):
        length = len(value)
        buf = [ffi.from_buffer(ffi_encode(item)) for item in value]
        lib.dasi_query_set(self._cdata, ffi_encode(keyword), buf, length)

    def __getitem__(self, keyword):
        value = ffi.new("const char **")
        lib.dasi_query_get(self._cdata, ffi_encode(keyword), value)
        return ffi_decode(value[0])

    def __delitem__(self, keyword):
        lib.dasi_query_erase(self._cdata, ffi_encode(keyword))

    def __len__(self) -> int:
        count = ffi.new("long*", 0)
        lib.dasi_query_keyword_count(self._cdata, count)
        return count[0]

    def insert(self, query: dict):
        for keyword, value in query.items():
            self[keyword] = value

    def append(self, keyword, value):
        lib.dasi_query_append(
            self._cdata, ffi_encode(keyword), ffi_encode(value)
        )

    def get_value(self, keyword, number) -> str:
        value = ffi.new("const char **")
        lib.dasi_query_get(self._cdata, ffi_encode(keyword), number, value)
        return ffi_decode(value[0])

    def has(self, keyword) -> bool:
        has = ffi.new("dasi_bool_t*", 1)
        lib.dasi_query_has(self._cdata, ffi_encode(keyword), has)
        return has[0] != 0

    def count_value(self, keyword) -> int:
        try:
            count = ffi.new("long*", 0)
            lib.dasi_query_value_count(self._cdata, ffi_encode(keyword), count)
            return count[0]
        except DASIException as e:
            self._log.debug(e.error)
            return 0

    def clear(self):
        lib.dasi_query_clear(self._cdata)
