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

from typing import NamedTuple

from pydasi.backend import FFI, ffi, lib, ffi_decode, new_list
from .key import Key
from .query import Query
from logging import getLogger as _getLogger

logger = _getLogger(__name__)


class ListItem(NamedTuple):
    """A single entry of a listing, independent of the iterator that produced it."""

    key: Key
    uri: str
    timestamp: int
    offset: int
    length: int

    def __str__(self) -> str:
        return "{}, uri: {}, time: {}, offset: {}, length: {}".format(
            self.key, self.uri, self.timestamp, self.offset, self.length
        )


class List:
    def __init__(self, dasi: FFI.CData, query):
        logger.debug("Initialize List...")

        self._cdata = new_list(dasi, Query(query).cdata)

    def __iter__(self):
        return self

    def __next__(self) -> ListItem:
        if lib.dasi_list_next(self._cdata) == lib.DASI_ITERATION_COMPLETE:
            raise StopIteration
        return self.__read()

    def __len__(self) -> int:
        logger.debug("not implemented in Dasi C lib!")
        return 0

    def __read(self) -> ListItem:
        ckey = ffi.new("dasi_key_t **", ffi.NULL)
        uri = ffi.new("const char **", ffi.NULL)
        time = ffi.new("dasi_time_t *", 0)
        offset = ffi.new("long *", 0)
        length = ffi.new("long *", 0)

        lib.dasi_list_attrs(self._cdata, ckey, time, uri, offset, length)

        return ListItem(
            key=Key(ffi.gc(ckey[0], lib.dasi_free_key)),
            uri=ffi_decode(uri[0]) if uri[0] != ffi.NULL else "unknown",
            timestamp=time[0],
            offset=offset[0],
            length=length[0],
        )
