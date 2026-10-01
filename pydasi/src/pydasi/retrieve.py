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

from pydasi.backend import FFI, ffi, lib, new_retrieve
from .key import Key
from .query import Query
from logging import getLogger as _getLogger

logger = _getLogger(__name__)


class RetrieveItem(NamedTuple):
    """A single retrieved object, independent of the iterator that produced it."""

    key: Key
    data: bytearray
    timestamp: int
    offset: int
    length: int

    def __str__(self) -> str:
        return "{}, time: {}, offset: {}, length: {}".format(self.key, self.timestamp, self.offset, self.length)


class Retrieve:
    def __init__(self, dasi: FFI.CData, query):

        logger.debug("Initialize Retrieve...")

        self._cdata = new_retrieve(dasi, Query(query).cdata)

    def __iter__(self):
        return self

    def __next__(self) -> RetrieveItem:
        if lib.dasi_retrieve_next(self._cdata) == lib.DASI_ITERATION_COMPLETE:
            logger.debug("Iteration complete.")
            raise StopIteration
        return self.__read()

    def __len__(self) -> int:
        count = ffi.new("long *", 0)
        lib.dasi_retrieve_count(self._cdata, count)
        return count[0]

    def __read(self) -> RetrieveItem:
        ckey = ffi.new("dasi_key_t **", ffi.NULL)
        time = ffi.new("dasi_time_t *", 0)
        offset = ffi.new("long *", 0)
        length = ffi.new("long *", 0)

        lib.dasi_retrieve_attrs(self._cdata, ckey, time, offset, length)

        data = bytearray(length[0])
        lib.dasi_retrieve_read(self._cdata, ffi.from_buffer(data), length)

        return RetrieveItem(
            key=Key(ffi.gc(ckey[0], lib.dasi_free_key)),
            data=data,
            timestamp=time[0],
            offset=offset[0],
            length=length[0],
        )
